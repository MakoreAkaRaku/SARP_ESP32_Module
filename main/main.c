#include <stdio.h>
#include "LedHandler.h"
#include "WiFiHandler.h"
#include "LeScanner.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char TAG[] = "Main_App";
void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG,"NO MEMORY");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    InitLEDS();
    InitWiFi();
    SwitchWiFi();
    
    
    printf("Hello World!");
    //InitScan();
}