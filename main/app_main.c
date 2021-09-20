#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include  "ds18b20.h"
#include "balance.h"//Include library
const int DS_PIN = 13; //GPIO where you connected ds18b20

void mainTask(void *pvParameters){
  ds18b20_init(DS_PIN);


  while (1) {
    printf("Temperature: %f\n",ds18b20_get_temp());
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
void app_main()
{
    nvs_flash_init();
    //system_init();
    xTaskCreatePinnedToCore(&mainTask, "mainTask", 2048, NULL, 5, NULL, 0);
}
