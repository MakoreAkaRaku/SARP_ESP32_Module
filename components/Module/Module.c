#include "esp_log.h"
#include "Module.h"
#include "HttpsClient.h"

struct peripheral {
  uint32_t id;
  const char *p_type;
};

const char *peripheral_type[] = {
    "hygrometer",
    "thermometer",
    "valve",
    "other",
};

static struct peripheral peripherals[N_PERIPHERAL_TYPES];

// static char *token_api;
static char *module_uuid;

static size_t module_uuid_length = 37; // UUID length is 36 characters + 1 for null terminator

nvs_handle_t https_nvs_handle;
static const char *TAG = "Module";

void ModuleInit()
{
  esp_err_t ret;
  ret = nvs_open(TAG, NVS_READWRITE, &https_nvs_handle);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
    return;
  }
  module_uuid = malloc(module_uuid_length * sizeof(char)); // UUID length is 36 characters + null terminator
  esp_err_t ret = nvs_get_str(https_nvs_handle, "module_uuid", module_uuid, &module_uuid_length);
  if (ret == ESP_ERR_NVS_NOT_FOUND)
  {
    // TODO: fetch token_api from BLE and declare it into the static var from this .c, if not found, show error.
    const char *token_api = "eb9ab986-27ae-4a59-a26f-c536d3ba185f";
    const char *registered_module_uuid = RegisterModule(token_api);
    ESP_LOGI(TAG, " module uuid content size: %d", strlen(registered_module_uuid));
    if (registered_module_uuid == NULL)
    {
      ESP_LOGE(TAG, "Failed to register module");
      return;
    }
    strncpy(module_uuid, registered_module_uuid, module_uuid_length);
    free((void *)registered_module_uuid); // Free the memory allocated by RegisterModule
    ret = nvs_set_str(https_nvs_handle, "module_uuid", module_uuid);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to save module UUID to NVS: %s", esp_err_to_name(ret));
      return;
    }
  }

  for (size_t i = 0; i < N_PERIPHERAL_TYPES; i++)
  {
    uint32_t peripheral_id;
    const char *p_type = peripheral_type[i];
    ret = nvs_get_u32(https_nvs_handle, p_type, &peripheral_id);
    if (ret == ESP_ERR_NVS_NOT_FOUND)
    {
      // Peripheral not found, register it
      ESP_LOGI(TAG, "Peripheral %s not found, registering...", p_type);
      peripheral_id = RegisterPeripheral(module_uuid, p_type);
      if (peripheral_id < 0)
      {
        ESP_LOGE(TAG, "Fatal, failed to register peripheral %s", p_type);
        return;
      }
      ret = nvs_set_u32(https_nvs_handle, p_type, peripheral_id);
      if (ret != ESP_OK)
      {
        ESP_LOGE(TAG, "Failed to save peripheral ID for %s: %s", p_type, esp_err_to_name(ret));
        return;
      }
    }
    ESP_LOGI(TAG, "Peripheral %s with ID: %d", p_type, peripheral_id);
    peripherals[i].id = peripheral_id;
    peripherals[i].p_type = p_type;
  }
  ESP_LOGI(TAG, "Module initialized with UUID: %s", module_uuid);
  nvs_commit(https_nvs_handle); // Commit changes to NVS
  nvs_close(https_nvs_handle);
}