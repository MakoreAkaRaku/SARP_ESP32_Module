#include <stdio.h>
#include "LedHandler.h"
#include "WiFiHandler.h"
#include "HttpsClient.h"
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
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  InitLEDS();
  InitWiFi();
}

void app_main(void)
{
  InitComponents();

  SwitchWiFi();

  if (BlockUntilHasConnection())
  {
    // Rethink this chunk of code
    // esp_restart();
    const char *token_api = "37a21070-30d0-44c9-8c10-85f3ec3aa830";
    const char *module_token = RegisterModule(token_api);
    if (module_token != NULL)
    {
      ESP_LOGI(TAG, "Module registered successfully with token: %s", module_token);
      free((void *)module_token); // Free the allocated memory for the module token
    }
    else
    {
      ESP_LOGE(TAG, "Failed to register module");
    }
  }
}
