#include "esp_log.h"
#include "Module.h"
#include "HttpsClient.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "math.h"

#define N_PERIPHERAL_TYPES 3 // 4 (remove "other" peripheral type if not needed)
#define MINUTES_TO_MICROSECONDS(x) ((x) * 60 * 1000000)
#define HYGROMETER_ADC_CHANNEL ADC_CHANNEL_7  // GPIO35 = ADC_CHANNEL_7
#define THERMOMETER_ADC_CHANNEL ADC_CHANNEL_6 // GPIO34 = ADC_CHANNEL_6
#define VALVE_GPIO_PIN GPIO_NUM_26            // GPIO23 for valve control

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

static char *token_api;
static char *module_uuid;

static const size_t uuid_length = 37; // UUID length is 36 characters + 1 for null terminator

static nvs_handle_t https_nvs_handle;
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
  module_uuid = malloc(uuid_length * sizeof(char)); // UUID length is 36 characters + null terminator
  ret = nvs_get_str(https_nvs_handle, "module_uuid", module_uuid, &uuid_length);
  if (ret == ESP_ERR_NVS_NOT_FOUND)
  {
    token_api = malloc(uuid_length * sizeof(char)); // Allocate memoery for token_api.
    ret = nvs_get_str(https_nvs_handle, "token_api", token_api, &uuid_length);
    if (ret == ESP_ERR_NVS_NOT_FOUND)
    {
      ESP_LOGE(TAG, "Token API not found in NVS, please register it first.");
      free(module_uuid);
      free(token_api);
      nvs_close(https_nvs_handle);
      esp_restart(); // Restart the ESP32 if token_api is not found
      return;
    }
    ESP_LOGI(TAG, "Module UUID not found in NVS, registering module...");
    // TODO: fetch token_api from BLE and declare it into the static var from this .c, if not found, show error.
    const char *token_api = "eb9ab986-27ae-4a59-a26f-c536d3ba185f";
    const char *registered_module_uuid = RegisterModule(token_api);
    ESP_LOGI(TAG, " module uuid content size: %d", strlen(registered_module_uuid));
    if (registered_module_uuid == NULL)
    {
      ESP_LOGE(TAG, "Failed to register module");
      return;
    }
    strncpy(module_uuid, registered_module_uuid, uuid_length);
    free((void *)registered_module_uuid); // Free the memory allocated by RegisterModule
    ret = nvs_set_str(https_nvs_handle, "module_uuid", module_uuid);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to save module UUID to NVS: %s", esp_err_to_name(ret));
      return;
    }
  }
  ESP_LOGI(TAG, "Module initialized with UUID: %s", module_uuid);
  for (size_t i = 0; i < N_PERIPHERAL_TYPES; i++)
  {
    uint32_t peripheral_id = -1;
    const char *p_type = peripheral_type[i];
    ret = nvs_get_u32(https_nvs_handle, p_type, &peripheral_id);
    if (ret == ESP_ERR_NVS_NOT_FOUND || peripheral_id == -1)
    {
      // If the peripheral ID is not found or is -1, we need to register it
      ESP_LOGI(TAG, "Peripheral %s not found in NVS, registering...", p_type);
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
  ESP_ERROR_CHECK(nvs_commit(https_nvs_handle)); // Commit changes to NVS
  nvs_close(https_nvs_handle);
  InitializePeripheralsPinSets(); // Initialize peripherals pinset
  InitPollingTask();              // Set up the polling task
}

/**
 * @brief Setups the polling task for the module, main functionality to update periodically the state of the module.
 *  This function is intended to be called during the module initialization phase.
 *  It will set up a task that periodically checks the module state and sends updates to the server.
 *
 */
static void InitPollingTask()
{
  ESP_LOGI(TAG, "Setting up polling task...");
  const esp_timer_create_args_t periodicTimerArgs = {
      .callback = &UpdateModuleState,
      .name = "PeriodicUpdateTimer"};
  esp_timer_handle_t timerHandler;
  ESP_ERROR_CHECK(esp_timer_create(&periodicTimerArgs, &timerHandler));
  ESP_ERROR_CHECK(esp_timer_start_periodic(timerHandler, MINUTES_TO_MICROSECONDS(1))); // Update every minute
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
  for (size_t i = 0; i < N_PERIPHERAL_TYPES; i++)
  {
    double data = 0.0;
    switch (i)
    {
    case 0: // Hygrometer
      double humidity = GetHygrometerValue();
      if (humidity < 0.0f)
      {
        ESP_LOGE(TAG, "Failed to read hygrometer value.");
        continue; // Skip this peripheral if reading failed
      }
      data = round(humidity * 100.0) / 100.0; // Round to 2 decimal places
      ESP_LOGI(TAG, "Hygrometer Humidity: %f", data);
      break;
    case 1: // Thermometer

      double temperature = GetThermometerValue();
      if (temperature < 0.0f)
      {
        ESP_LOGE(TAG, "Failed to read thermometer value.");
        continue; // Skip this peripheral if reading failed
      }
      data = round(temperature * 100.0) / 100.0; // Round to 2 decimal places
      ESP_LOGI(TAG, "Thermometer Temperature: %f", data);
      break;
    case 2: // Valve

      const char *state = GetPeripheralState(peripherals[i].id); // Get the current state of the valve
      if (state == NULL)
      {
        ESP_LOGE(TAG, "Failed to get valve state.");
        continue; // Skip this peripheral if reading failed
      }
      if (strcmp(state, "off") == 0)
      {
        SetValveState(0);
      }
      else if (strcmp(state, "on") == 0)
      {
        SetValveState(1);
      }
      else
      {
        ESP_LOGE(TAG, "Invalid valve state received: %s", state);
        free((void *)state); // Free the state string if it was dynamically allocated
        continue;            // Skip this peripheral if the state is invalid
      }
      int valve_state = GetValveState();
      free((void *)state);        // Free the state string after use
      data = (double)valve_state; // Convert valve state to double for consistency
      break;
    // Case 3: Other
    // This case is not implemented, but you can add your logic here if needed.
    default:
      continue; // Skip if the peripheral type is not recognized
      break;
    }
    ESP_ERROR_CHECK(PostPeripheralData(peripherals[i].id, data));
  }

  ESP_LOGI(TAG, "Module state updated successfully.");
}

static void InitializePeripheralsPinSets()
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
  }
  ESP_LOGI(TAG, "Peripherals initialized successfully.");
}

/**
 * @brief Reads the hygrometer value from the ADC and converts it to a humidity percentage.
 *
 * @return double The humidity percentage, or -1.0f on error.
 */
double GetHygrometerValue()
{
  int raw_adc_reading = -1;
  ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, HYGROMETER_ADC_CHANNEL, &raw_adc_reading));
  if (raw_adc_reading < 0)
  {
    ESP_LOGE(TAG, "Failed to read ADC value for hygrometer");
    return -1.0f; // Return an error value
  }
  double humidity = (1.0 - (raw_adc_reading / 4095.0)); // Convert ADC reading to voltage
  ESP_LOGI(TAG, "Hygrometer Humidity: %.2f", humidity);
  return humidity;
}

/**
 * @brief Reads the thermometer value from the ADC and converts it to a temperature in Celsius.
 *
 * @return double The temperature in Celsius, or -1.0 on error.
 */
double GetThermometerValue()
{
  int raw_adc_reading = -1;
  ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, THERMOMETER_ADC_CHANNEL, &raw_adc_reading));
  if (raw_adc_reading < 0)
  {
    ESP_LOGE(TAG, "Failed to read ADC value for thermometer");
    return -1.0f; // Return an error value
  }

  // Convert ADC value to voltage (assuming 3.3V reference and 12-bit ADC)
  double voltage = (raw_adc_reading / 4095.0f) * 3.3f;

  // For a BC547 used as a temperature sensor, you typically use Vbe drop:
  // Vbe decreases by about -2mV/°C, assuming that Vbe at 25°C is about 0.660V.
  // T(°C) = 25 - ((Vbe - 0.660) / 0.002)

  double vbe = voltage; // If direct, else adjust for divider
  double temperature_c = 25.0 - ((vbe - 0.660) / 0.002);

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

void RegisterTokenAPI(const char *token_api)
{
  ESP_LOGI(TAG, "Registering token API: %s", token_api);
  esp_err_t ret = nvs_open(TAG, NVS_READWRITE, &https_nvs_handle);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
    return;
  }
  ret = nvs_set_str(https_nvs_handle, "token_api", token_api);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to save token API to NVS: %s", esp_err_to_name(ret));
    nvs_close(https_nvs_handle);
    return;
  }
  nvs_commit(https_nvs_handle); // Commit changes to NVS
  ESP_LOGI(TAG, "Token API registered successfully.");
  nvs_close(https_nvs_handle);
}