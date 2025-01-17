// VIEWE——lvgl控件例程
// https://www.display-wiki.com/smartdisplay/esp32_5_800_480
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "spi_flash_mmap.h"
#include "nvs_flash.h"

#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include <fcntl.h>
#include <sys/param.h>

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "demos/lv_demos.h"
#include "iot_button.h"

static const char *TAG = "Viewe Display";

static esp_netif_t *wifi_if = NULL;
static SemaphoreHandle_t IP_SEMPH = NULL;
#define SSID "GitHub-2.4G"
#define PASSWORD "fb1e403213"

lv_obj_t *response_label;
lv_obj_t *wifi_label;
lv_obj_t *http_status_label;

void lvgl_hardWare_init() // 外围硬件初始化
{
    ESP_ERROR_CHECK(bsp_i2c_init(I2C_NUM_0, 400000));
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_port_tick_init();
}
void lv_tick_task(void *arg)// LVGL 时钟任务
{
    while (1)
    {
        vTaskDelay((20) / portTICK_PERIOD_MS);
        lv_task_handler();
    }
}


void Touch_IO_RST(void)
{

#if(GT911==1)
	//touch reset pin  低电平复位
//for yysj_5
//#define GPIO_I2C_INT    (GPIO_NUM_18)
//#define GPIO_I2C_RST    (GPIO_NUM_38)
	gpio_reset_pin(18); //INT
	gpio_reset_pin(38); //RST
	gpio_pullup_en(38);
	gpio_pullup_en(18);

	gpio_set_direction(18, GPIO_MODE_OUTPUT);
	gpio_set_direction(38, GPIO_MODE_OUTPUT);
	gpio_set_level(38, 1);
	gpio_set_level(18, 1);
	ESP_LOGI(TAG, "io18 set_high");
	vTaskDelay(pdMS_TO_TICKS(50));
	gpio_pulldown_en(18);
	gpio_pulldown_en(38);
	gpio_set_level(18, 0);
	gpio_set_level(38, 0);
	ESP_LOGI(TAG, "io18 set_low");

	vTaskDelay(pdMS_TO_TICKS(20));
	gpio_set_level(38, 1);
	vTaskDelay(pdMS_TO_TICKS(20));
	gpio_pulldown_en(18);
	gpio_set_level(18, 0);
	//gpio_reset_pin(39);
	//gpio_set_direction(39, GPIO_MODE_INPUT);//中断脚没有用上，这只是用来配置地址



#elif(CST3240==1)

    gpio_reset_pin(39);
	gpio_reset_pin(40);
	
	gpio_set_direction( GPIO_NUM_40, GPIO_MODE_OUTPUT);//RST SET PORT OUTPUT
	gpio_set_level(40, 0);        //RST RESET IO
	vTaskDelay(pdMS_TO_TICKS(50));//DELAY 50ms 
	gpio_set_level(40, 1);        //SET RESET IO
	vTaskDelay(pdMS_TO_TICKS(10));//DELAY 10ms 	
#endif

}

/* HTTP & WIFI Code */
esp_err_t http_event_handler(esp_http_client_event_t *evt) {
  static char *output_buffer;
  static int output_len;
  switch (evt->event_id) {
  case HTTP_EVENT_ERROR:
    ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
	lv_label_set_text(http_status_label, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
	lv_label_set_text(http_status_label, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
	lv_label_set_text(http_status_label, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
             evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA: {
    ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    int copy_len = 0;
    int content_len = esp_http_client_get_content_length(evt->client);
    if (output_buffer == NULL) {
      // We initialize output_buffer with 0 because it is used by strlen() and
      // similar functions therefore should be null terminated.
      output_buffer = (char *)calloc(content_len + 1, sizeof(char));
      output_len = 0;
      if (output_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
        return ESP_FAIL;
      }
    }
    copy_len = MIN(evt->data_len, (content_len - output_len));
    if (copy_len) {
      memcpy(output_buffer + output_len, evt->data, copy_len);
    }
    output_len += copy_len;
    break;
  }
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
	lv_label_set_text(http_status_label, "HTTP_EVENT_ON_FINISH");
    if (output_buffer != NULL) {
      ESP_LOGI(TAG, "%s", output_buffer);
	  lv_label_set_text_fmt(response_label, "HTTP Response:\n\n%s", output_buffer);

      free(output_buffer);
      output_buffer = NULL;
    }
    output_len = 0;
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
	lv_label_set_text(http_status_label, "HTTP_EVENT_DISCONNECTED");
    if (output_buffer != NULL) {
      free(output_buffer);
      output_buffer = NULL;
    }
    output_len = 0;
    break;
  case HTTP_EVENT_REDIRECT:
    ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
	lv_label_set_text(http_status_label, "HTTP_EVENT_REDIRECT");
    break;
  }
  return ESP_OK;
}

static void got_ip_handler(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data) {
  ESP_LOGI(TAG, "Got IP address");

  lv_label_set_text_fmt(wifi_label, "Got IP address");

  xSemaphoreGive(IP_SEMPH);
}

esp_err_t connect_to_ap() {
	if (wifi_if != NULL) {
		return ESP_OK;
	}
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  cfg.nvs_enable = 0;
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_netif_inherent_config_t esp_netif_config =
      ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
  wifi_if = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
  esp_wifi_set_default_wifi_sta_handlers();

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  IP_SEMPH = xSemaphoreCreateBinary();
  if (IP_SEMPH == NULL) {
    return ESP_ERR_NO_MEM;
  }

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = SSID,
              .password = PASSWORD,
          },
  };
  ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);

  lv_label_set_text_fmt(wifi_label, "Connecting to %s...", wifi_config.sta.ssid);

  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &got_ip_handler, NULL));

  esp_wifi_connect();

  xSemaphoreTake(IP_SEMPH, portMAX_DELAY);

  return ESP_OK;
}

static void stop_wifi() {
  ESP_LOGI(TAG, "Turning off wifi");
  ESP_ERROR_CHECK(esp_wifi_stop());
  ESP_ERROR_CHECK(esp_wifi_deinit());
  ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(wifi_if));
  esp_netif_destroy(wifi_if);
  wifi_if = NULL;
}

static void https_with_url(void *pvParameters) {
  while (true) {
    ESP_ERROR_CHECK(connect_to_ap());

    esp_http_client_config_t config = {
        .url = "https://iot.fbiego.com/api/v1/test.txt",
        .event_handler = http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

	lv_label_set_text_fmt(wifi_label, "URL: %s", config.url);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
      int status_code = esp_http_client_get_status_code(client);
		ESP_LOGI(TAG, "Got %d code", status_code);
		// lv_label_set_text_fmt(response_label, "Got %d code", status_code);
    } else {
        ESP_LOGE(TAG, "Error with https request: %s", esp_err_to_name(err));
		lv_label_set_text_fmt(response_label, "Error with https request: %s", esp_err_to_name(err));
    }

    ESP_ERROR_CHECK(esp_http_client_cleanup(client));

    stop_wifi();
	ESP_LOGI(TAG, "Deleting https_with_url task");
	vTaskDelete(NULL); // delete the task

    vTaskDelay(60000 / portTICK_PERIOD_MS);
  }
}

void app_main(void)
{
	// Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    Touch_IO_RST();
    lvgl_hardWare_init();       // 外围硬件初始化
    ESP_LOGI(TAG, "init ok");

	wifi_label = lv_label_create(lv_screen_active());
	lv_obj_align(wifi_label, LV_ALIGN_TOP_MID, 0, 50);
	lv_label_set_text(wifi_label, "Wifi Test");

	http_status_label = lv_label_create(lv_screen_active());
	lv_obj_align(http_status_label, LV_ALIGN_TOP_MID, 0, 75);
	lv_label_set_text(http_status_label, "HTTP Status");

	response_label = lv_label_create(lv_screen_active());
    lv_obj_align(response_label, LV_ALIGN_TOP_MID, 0, 100);
    lv_label_set_long_mode(response_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(response_label, 400);
    lv_label_set_text(response_label,  "Response");


	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	xTaskCreate(&https_with_url, "https_with_url", 8192, NULL, 5, NULL);


    //lv_demo_music();
    // lv_demo_widgets();
    // lv_demo_benchmark();
    xTaskCreate(lv_tick_task, "lv_tick_task", 1024*10, NULL, 1, NULL);
}
