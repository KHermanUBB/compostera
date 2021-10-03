#include <stdio.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "ds18b20.h"
#include "balance.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <lcd.h>
#include "hx711.h"
#include "main.h"

static const char *TAG = "Compostera";
esp_mqtt_client_handle_t client;

/*------------------------------- Queues --------------------------------------*/
xQueueHandle qTemperature;
xQueueHandle qWeight;
xQueueHandle qLevel;
xQueueHandle qLogin;
/*------------------------------- Queues --------------------------------------*/

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("Compostera_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("Compostera_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    qTemperature = xQueueCreate(5, sizeof(float));
    qWeight = xQueueCreate(5, sizeof(int32_t));
    qLevel = xQueueCreate(5, sizeof(int32_t));
    qLogin = xQueueCreate(5, LOGIN_MAX_LEN * sizeof(float));

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();

    //xTaskCreatePinnedToCore(&mainTask, "mainTask", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&vReadTemperature, "DS18B20Task", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&vReadWeight, "BalanceTask", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&vPublishDataOverMQTT, "MQTTPublishTask", 2048, NULL, 5, NULL, 0);
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void mainTask(void *pvParameters)
{

    char payload[24];
    int msg_id;
    static float temp;
    ds18b20_init(DS_PIN);

    while (1)
    {
        temp = ds18b20_get_temp();
        printf("Temperature: %f\n", temp);
        sprintf(payload, "Temp = %f", temp);
        msg_id = esp_mqtt_client_publish(client, "testkh/topic", payload, 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void vReadTemperature(void *pvParameters)
{

    static float temp;
    uint8_t ok;
    ds18b20_init(DS_PIN);
    while (1)
    {
        temp = ds18b20_get_temp();
        ok = xQueueSend(qTemperature, &temp, 5000 / portTICK_PERIOD_MS);
        if (ok)
            ESP_LOGI(TAG, "Temperature  %f send to queue successfully \n", temp);
        else
            ESP_LOGI(TAG, "Temperature  %f failed to send to queue \n", temp);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void vPublishDataOverMQTT(void *params)
{

    float temp;
    int weight;
    int msg_id2;
    char payload[MAX_PAYLOAD_LENGTH];
    
    while (true)
    {
        if (xQueueReceive(qWeight, &weight, 5000 / portTICK_PERIOD_MS))
        {
            sprintf(payload, "%d", weight );
            msg_id2 = esp_mqtt_client_publish(client, "testkh/topic", payload, 0, 1, 0);
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void vReadWeight(void *pvParameters)
{

    uint8_t ok;

    hx711_t dev = {
        .dout   = DOUT_GPIO,
        .pd_sck = PD_SCK_GPIO,
        .gain = HX711_GAIN_A_64
    };

    while (1){
        esp_err_t r = hx711_init(&dev);
        if (r == ESP_OK)
            break;
        printf("Could not initialize HX711: %d (%s)\n", r, esp_err_to_name(r));
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    while (1)
    {
        esp_err_t r = hx711_wait(&dev, 500);
        if (r != ESP_OK)
        {
            printf("Device not found: %d (%s)\n", r, esp_err_to_name(r));
            continue;
        }
        int32_t weight;
        r = hx711_read_data(&dev, &weight);
        if (r != ESP_OK)
        {
            printf("Could not read data: %d (%s)\n", r, esp_err_to_name(r));
            continue;
        }
        
        ok = xQueueSend(qWeight, &weight, 5000 / portTICK_PERIOD_MS);
        
        if (ok)
            ESP_LOGI(TAG, "Weight  %d send to queue successfully \n", weight);
        else
            ESP_LOGI(TAG, "Weight  %d failed to send to queue \n", weight);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
