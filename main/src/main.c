#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG ="NODO-ESP32";

static void tarea_v1(void *pvParameters){
    uint8_t counter = 0;
    while(1){
        if(counter == 255)
            counter = 0;

        ESP_LOGI(TAG,"%d",counter);
        counter++;
    }
    
}


void app_main(void)
{
    ESP_LOGI(TAG, "Inicializando. Versión de firmware 1.0");
    xTaskCreate(tarea_v1, "t-v1", 4096, NULL, 5 ,NULL);
}
