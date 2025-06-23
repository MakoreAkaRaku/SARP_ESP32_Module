#include "esp_log.h"
#include "Module.h"
#include "HttpsClient.h"

//static char *token_api;
static char *module_uuid;

static size_t module_uuid_length = 37; // UUID length is 36 characters + 1 for null terminator

nvs_handle_t https_nvs_handle;
static const char *TAG = "Module";

void HandleModuleInit()
{
  module_uuid = malloc(module_uuid_length * sizeof(char)); // UUID length is 36 characters + null terminator
  esp_err_t ret = nvs_get_str(https_nvs_handle, "module_uuid", module_uuid, &module_uuid_length);
  if (ret == ESP_ERR_NVS_NOT_FOUND)
  {
    // TODO: fetch token_api from BLE and declare it into the static var from this .c, if not found, show error.
    const char *token_api = "eb9ab986-27ae-4a59-a26f-c536d3ba185f";
    module_uuid = RegisterModule(token_api);
    if (module_uuid == NULL)
    {
      ESP_LOGE(TAG, "Failed to register module");
      return;
    }
    ret = nvs_set_str(https_nvs_handle, "module_uuid", module_uuid);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to save module UUID to NVS: %s", esp_err_to_name(ret));
      return;
    }
  }
  for (int i = 0; i < module_uuid_length; i++)
  {
    if (module_uuid[i] == '\0')
    {
      module_uuid_length = i;
      break;
    }
  }

  for (size_t i = 0; i < N_PERIPHERAL_TYPES; i++)
  {
    const char *p_type = peripheral_type[i];
    const int peripheral_id = RegisterPeripheral(module_uuid, p_type);
    if (peripheral_id < 0)
    {
      ESP_LOGE(TAG, "Failed to register peripheral %s", p_type);
    }
    else
    {
      ESP_LOGI(TAG, "Peripheral registered with ID: %d", peripheral_id);

    }
  }
}