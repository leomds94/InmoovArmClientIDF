#include "servoControl.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include <esp_http_client.h>

#define SSID "Fortals"
#define PASSPHARSE "pingulinha"

static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "localhost:5002"
#define WEB_PORT 80
#define WEB_URL "https://localhost:5002/getservos"

static const char *TAG = "example";

static const char *REQUEST = "GET " WEB_URL " HTTP/1.0\r\n"
							 "Host: " WEB_SERVER "\r\n"
							 "User-Agent: esp-idf/1.0 esp32\r\n"
							 "\r\n";

servoControl dedao, indicador, medio, anular, dedinho, pulso;
int pos = 0;

int value;
char *payload;

int last_dedao = 0;
int last_indicador = 0;
int last_medio = 0;
int last_anular = 0;
int last_dedinho = 0;
int last_pulso = 0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id)
	{
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		/* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
	default:
		break;
	}
	return ESP_OK;
}

static void initialise_wifi(void)
{
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = SSID,
			.password = PASSPHARSE,
		},
	};
	ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

int servoMove(servoControl servo_i, int begin_angle, int end_angle)
{
	if (begin_angle < end_angle)
	{
		for (pos = begin_angle; pos <= end_angle; pos++)
		{
			servo_i.write(pos);
			vTaskDelay(50 / portTICK_RATE_MS);
		}
	}
	else
	{
		for (pos = begin_angle; pos >= end_angle; pos--)
		{
			// in steps of 1 degree
			servo_i.write(pos);
			vTaskDelay(50 / portTICK_RATE_MS);
		}
	}
	return end_angle;
}

void servosWrite(int dedao_grau, int indicador_grau, int medio_grau, int anular_grau, int dedinho_grau, int pulso_grau)
{
	dedao.write(dedao_grau);
	indicador.write(indicador_grau);
	medio.write(medio_grau);
	anular.write(anular_grau);
	dedinho.write(dedinho_grau);
	pulso.write(pulso_grau);
}

static void https_get(void *pvParameters)
{
	char *buffer;
	esp_http_client_config_t config = {
		.url = "http://192.168.15.7:5002/servovalues",
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	while (1)
	{
		esp_err_t err = esp_http_client_perform(client);

		if (err == ESP_OK)
		{
			ESP_LOGI(TAG, "Status = %d, content_length = %d",
					 esp_http_client_get_status_code(client),
					 esp_http_client_get_content_length(client));
			esp_http_client_read(client, buffer, esp_http_client_get_content_length(client));

			value = atoi(buffer);

			switch (value)
			{
			case 0:
				servosWrite(80, 80, 0, 0, 5, 0);
				break;
			case 1:
				servosWrite(0, 5, 90, 90, 85, 0);
				break;
			case 2:
				servosWrite(0, 80, 90, 90, 5, 0);
				break;
			case 3:
				servosWrite(80, 5, 90, 90, 5, 0);
				break;
			case 4:
				servosWrite(80, 5, 90, 90, 85, 0);
				break;
			case 5:
				servosWrite(80, 80, 0, 0, 5, 90);
				break;
			}
		}
		else if (err == ESP_ERR_HTTP_CONNECT)
		{
			vTaskDelay(5000 / portTICK_RATE_MS);
		}
		vTaskDelay(50 / portTICK_RATE_MS);
		esp_http_client_cleanup(client);
	}
}

extern "C" void app_main()
{
	dedao.attach(GPIO_NUM_2);
	indicador.attach(GPIO_NUM_0);
	medio.attach(GPIO_NUM_4);
	anular.attach(GPIO_NUM_16);
	dedinho.attach(GPIO_NUM_17);
	pulso.attach(GPIO_NUM_5);
	//Defaults: Servo.attach(pin, 400, 2600, LEDC_CHANNEL_0, LEDC_TIMER0);
	servosWrite(-30, 120, 120, 120, 90, 0);

	vTaskDelay(500 / portTICK_RATE_MS);

	initialise_wifi();
	xTaskCreate(&https_get, "http_get", 4096, NULL, 5, NULL);
}