/** @file main.h
 * 
 * @brief 
 *
 * @par       
 * COPYRIGHT NOTICE: (c) 2021  All rights reserved.
 */ 

#ifndef MAIN_H
#define MAIN_H


#define DS_PIN 23

#define LOGIN_MAX_LEN 4
#define MAX_PAYLOAD_LENGTH 32


typedef struct{

    float temperature;
    float weight;
    float level;

} Measurement;


static void log_error_if_nonzero(const char * message, int error_code);
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void mqtt_app_start(void);
void mainTask(void *pvParameters);
void vReadTemperature(void *pvParameters);
void vPublishDataOverMQTT(void *params);


#endif /* KEYPAD_H */

/*** end of file ***/
