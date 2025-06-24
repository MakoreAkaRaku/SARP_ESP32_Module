#include "esp_log.h"
#include "Module.h"
#include "HttpsClient.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"

#define N_PERIPHERAL_TYPES 3 // 4 (remove "other" peripheral type if not needed)
#define MINUTES_TO_MICROSECONDS(x) ((x) * 60 * 1000000)
#define HYGROMETER_ADC_CHANNEL ADC_CHANNEL_7      // GPIO35 = ADC_CHANNEL_7
#define THERMOMETER_ADC_CHANNEL ADC_CHANNEL_6     // GPIO34 = ADC_CHANNEL_6
#define VALVE_GPIO_PIN GPIO_NUM_26                // GPIO23 for valve control

static adc_oneshot_unit_handle_t adc1_handle;
struct peripheral
{
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
  ret = nvs_get_str(https_nvs_handle, "module_uuid", module_uuid, &module_uuid_length);
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
    ESP_LOGI(TAG, "Peripheral %s with ID: %ld", p_type, peripheral_id);
    peripherals[i].id = peripheral_id;
    peripherals[i].p_type = p_type;
  }
  ESP_LOGI(TAG, "Module initialized with UUID: %s", module_uuid);
  nvs_commit(https_nvs_handle); // Commit changes to NVS
  nvs_close(https_nvs_handle);
}

/**
 * @brief Setups the polling task for the module, main functionality to update periodically the state of the module.
 *  This function is intended to be called during the module initialization phase.
 *  It will set up a task that periodically checks the module state and sends updates to the server.
 *
 */
void InitPollingTask()
{
  ESP_LOGI(TAG, "Setting up polling task...");
  const esp_timer_create_args_t periodicTimerArgs = {
      .callback = &UpdateModuleState,
      .name = "PeriodicUpdateTimer"};
  esp_timer_handle_t timerHandler;
  ESP_ERROR_CHECK(esp_timer_create(&periodicTimerArgs, &timerHandler));
  ESP_ERROR_CHECK(esp_timer_start_periodic(timerHandler, MINUTES_TO_MICROSECONDS(0.05))); // Update every minute
  ESP_LOGI(TAG, "Started timers, time since boot: %lld us", esp_timer_get_time());
}
/**
 * @brief This function is intended to update the module state.
 * Used to send periodic updates or status checks to the server regarding the module.
 * Implementation details will depend on the specific requirements of the module.
 *
 */
static void UpdateModuleState()
{
  ESP_LOGI(TAG, "Updating module state...");

  // Implementation for updating module state goes here.
}

void InitializePeripherals()
{

  ESP_LOGI(TAG, "Initializing peripherals...");
  // ADC1 INIT//
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT_1, // Use ADC1
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
  /// END INIT ADC1 //
  /// ADC1 CHANNEL CONFIGURATION ///
  adc_oneshot_chan_cfg_t adc1_config = {
      .atten = ADC_ATTEN_DB_12,    // Set attenuation to 12dB for 0-3.3V range
      .bitwidth = ADC_BITWIDTH_12, // Set bitwidth to 12 bits
  };

  for (size_t i = 0; i < N_PERIPHERAL_TYPES; i++)
  {
    // tmp
    peripherals[i].id = i;
    peripherals[i].p_type = peripheral_type[i]; // Initialize peripheral type
    // tmp
    switch (i)
    {
    case 0: // Hygrometer
      ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, HYGROMETER_ADC_CHANNEL, &adc1_config));
      break;
    case 1: // Thermometer
      ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, THERMOMETER_ADC_CHANNEL, &adc1_config));
      break;
    case 2:                                                                        // Valve
      ESP_ERROR_CHECK(gpio_set_direction(VALVE_GPIO_PIN, GPIO_MODE_INPUT_OUTPUT)); // Set GPIO13 as output for valve control
      break;
    case 3: // Other
      // NOT IMPLEMENTED
      ESP_LOGW(TAG, "Peripheral type 'other' is not implemented yet.");
    }
    ESP_LOGI(TAG, "Peripheral %s initialized with ID: %ld", peripherals[i].p_type, peripherals[i].id);
  }
}

/**
 * @brief Reads the hygrometer value from the ADC and converts it to a humidity percentage.
 *
 * @return float The humidity percentage, or -1.0f on error.
 */
float GetHygrometerValue()
{
  int raw_adc_reading = -1;
  ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, HYGROMETER_ADC_CHANNEL, &raw_adc_reading));
  if (raw_adc_reading < 0)
  {
    ESP_LOGE(TAG, "Failed to read ADC value for hygrometer");
    return -1.0f; // Return an error value
  }
  float humidity = (1.0f - (raw_adc_reading / 4095.0f)); // Convert ADC reading to voltage
  ESP_LOGI(TAG, "Hygrometer Humidity: %.2f", humidity);
  return humidity;
}

/**
 * @brief Reads the thermometer value from the ADC and converts it to a temperature in Celsius.
 *
 * @return float The temperature in Celsius, or -1.0f on error.
 */
float GetThermometerValue()
{
  int raw_adc_reading = -1;
  ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, THERMOMETER_ADC_CHANNEL, &raw_adc_reading));
  if (raw_adc_reading < 0)
  {
    ESP_LOGE(TAG, "Failed to read ADC value for thermometer");
    return -1.0f; // Return an error value
  }

  // Convert ADC value to voltage (assuming 3.3V reference and 12-bit ADC)
  float voltage = (raw_adc_reading / 4095.0f) * 3.3f;

  // For a BC547 used as a temperature sensor, you typically use Vbe drop:
  // Vbe decreases by about -2mV/°C, assuming that Vbe at 25°C is about 0.660V.
  // T(°C) = 25 - ((Vbe - 0.660) / 0.002)

  float vbe = voltage; // If direct, else adjust for divider
  float temperature_c = 25.0f - ((vbe - 0.660f) / 0.002f);

  ESP_LOGI(TAG, "Temperature: %.2f °C", temperature_c);
  return temperature_c;
}

int GetValveState()
{
  int valve_state = gpio_get_level(VALVE_GPIO_PIN);
  ESP_LOGI(TAG, "Valve state: %d", valve_state);
  return valve_state;
}

esp_err_t SetValveState(int state)
{
  if (state != 0 && state != 1)
  {
    ESP_LOGE(TAG, "Invalid valve state: %d. Must be 0 or 1.", state);
    return ESP_ERR_INVALID_ARG;
  }
  ESP_ERROR_CHECK(gpio_set_level(VALVE_GPIO_PIN, state));
  ESP_LOGI(TAG, "Valve state set to: %d", state);
  return ESP_OK;
}