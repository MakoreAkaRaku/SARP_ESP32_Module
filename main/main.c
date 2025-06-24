#include <stdio.h>
#include "LedHandler.h"
#include "WiFiHandler.h"
#include "HttpsClient.h"
#include "Module.h"
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
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
  }
  ESP_LOGI(TAG, "NVS Flash initialized successfully");
}

void InitComponents()
{
  FlashInit();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  InitLEDS();
  InitWiFi();
  SwitchWiFi();

  BlockUntilHasConnection();

  ModuleInit();
}

void app_main(void)
{
  InitComponents();
}
