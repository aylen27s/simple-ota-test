#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "string.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

//Para utilizar funciones OTA
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"

// Para configurar la conexión Wifi

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <nvs.h>

// Defino nombre de la red Wifi y su psw
#define WIFI_SSID "MYSSID" 
#define WIFI_PASS "MYPASS" 
#define BIN_URL "https://raw.githubusercontent.com/aylen27s/simple-ota-test/master/main/public-bin/simple-ota-test.bin"
#define HASH_LEN 32 /* SHA-256 digest length */


static esp_netif_t *sta_netif = NULL;
static const char* TAG ="[V2]NODO-ESP32";

static void print_sha256(const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s %s", label, hash_print);
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    case HTTP_EVENT_ON_HEADERS_COMPLETE:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADERS_COMPLETE");
        break;
    case HTTP_EVENT_ON_STATUS_CODE:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_STATUS_CODE");
    break;
    }
    return ESP_OK;
}

void simple_ota_example_task(void)
{
    ESP_LOGI(TAG, "Starting OTA example task");
    esp_http_client_config_t config = {
        .url = BIN_URL, 
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = true,
        .disable_auto_redirect = false, 
    };
    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    ESP_LOGI(TAG, "Attempting to download update from %s", config.url);

    esp_err_t ret = esp_https_ota(&ota_config);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/* ----------------- Conexión WiFi ----------------- */
static void onGotIp(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    const esp_netif_ip_info_t* ip_info = &event->ip_info;
    ESP_LOGI("NETWORK", "IP: " IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI("NETWORK", "Gateway: " IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI("NETWORK", "Netmask: " IPSTR, IP2STR(&ip_info->netmask));
}

static void onWifiDisconnect(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    ESP_LOGW(TAG, "WiFi desconectado. Reintentando...");
    esp_wifi_connect();
}

esp_err_t connectEspToWifi(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &onGotIp, NULL);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &onWifiDisconnect, NULL);

    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            // .threshold.authmode = WIFI_AUTH_WPA2_PSK
        }
    };

    esp_wifi_set_config(WIFI_IF_STA , &wifi_config);

    esp_err_t err = esp_wifi_start();
    if(err != ESP_OK)
        return err;

    err = esp_wifi_connect();
    if(err != ESP_OK)
        return err;
    
    ESP_LOGI(TAG, "WiFi inicializado exitosamente en SSID: %s", WIFI_SSID);
    return ESP_OK;
}

static void tarea_v1(void *pvParameters)
{
    uint8_t counter = 0;
    while(1){
        if(counter == 15){
            counter=0;
            ESP_LOGI(TAG,"V2 cuenta hasta 15 y ejecuta OTA. Vuelve a la v anterior sin ota.");
            simple_ota_example_task();
        }
        ESP_LOGI(TAG,"%d",counter);
        counter++;
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
    
}

void app_main(void)
{
    ESP_LOGI(TAG, "Inicializando. Versión de firmware 2.0");

    // uint8_t sha_256[HASH_LEN] = { 0 };
    // esp_partition_t partition;

    // // get sha256 digest for the partition table
    // partition.address   = ESP_PARTITION_TABLE_OFFSET;
    // partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    // partition.type      = ESP_PARTITION_TYPE_DATA;
    // esp_partition_get_sha256(&partition, sha_256);
    // print_sha256(sha_256, "SHA-256 for the partition table: ");

    // // get sha256 digest for bootloader
    // partition.address   = ESP_BOOTLOADER_OFFSET;
    // partition.size      = ESP_PARTITION_TABLE_OFFSET;
    // partition.type      = ESP_PARTITION_TYPE_APP;
    // esp_partition_get_sha256(&partition, sha_256);
    // print_sha256(sha_256, "SHA-256 for bootloader: ");

    // // get sha256 digest for running partition
    // esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    // print_sha256(sha_256, "SHA-256 for current firmware: ");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* Cnofigurar y conectar WIFI*/
    ESP_LOGI("WIFI","Conectando WiFi en SSID %s ...", WIFI_SSID);
    esp_err_t err_wifi= connectEspToWifi();
    if( err_wifi != ESP_OK){
        ESP_LOGE(TAG,"No se pudo inicializar WIFI. Abortando. Err %d",err_wifi);
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(5000)); 
    xTaskCreate(tarea_v1, "t-v1", 4096, NULL, 5 ,NULL);
}
