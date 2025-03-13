#include <stdio.h>
#include "LedHandler.h"
#include "WiFiHandler.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char TAG[] = "Main_App";

void FlashInit()
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NO MEMORY");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
}

void InitComponents()
{
    FlashInit();
    InitLEDS();
    InitWiFi();
}

void app_main(void)
{
    InitComponents();

    SwitchWiFi();

    if (!BlockUntilHasConnection())
    {
        // Rethink this chunk of code
        esp_restart();
    }
}
