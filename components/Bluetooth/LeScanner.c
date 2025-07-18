#include "LeScanner.h"
#include "WiFiHandler.h"
#include "Module.h"
#include "LedHandler.h"
#define MAX_RETRY 5
#define SCAN_DURATION 10               // Scan duration in seconds.
#define SHORT_WIFI_UUID_TARGET 0xDEAD  // Service target for WiFi credentials
#define SHORT_TOKEN_UUID_TARGET 0xFADE // Service target for token API
#define DATA_START_OFFSET 4            // 4 offset bytes without counting the length byte
static const char TAG[] = "LeScanner";
static int scan_retry = 0;

void EnableBLE()
{
  // ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BTDM));
  ESP_LOGI(TAG, "Enabling BLE");
  esp_err_t status;
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  if ((status = esp_bt_controller_init(&bt_cfg)) != ESP_OK)
  {
    ESP_LOGE(TAG, "BT Controller Init failed: %s", esp_err_to_name(status));
  }

  // Important; setting is only for dual mode (WiFi and BLE coexistence)
  if ((status = esp_bt_controller_enable(ESP_BT_MODE_BTDM)) != ESP_OK)
  {
    ESP_LOGE(TAG, "BT Controller Enable failed: %s", esp_err_to_name(status));
  }
  if (esp_bluedroid_init() != ESP_OK ||
      esp_bluedroid_enable() != ESP_OK)
  {
    ESP_LOGE(TAG, "Error while enabling bluedroid");
  }
  esp_ble_scan_params_t scanParams = {
      .scan_type = BLE_SCAN_TYPE_ACTIVE,
      .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
      .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
      .scan_interval = 0x0500, // 1000 ms
      .scan_window = 0x0100,   // 307  ms
      .scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE};

  if (esp_ble_gap_set_scan_params(&scanParams) != ESP_OK)
  {
    ESP_LOGE(TAG, "Could not set scan params");
  }
  if ((status = esp_ble_gap_register_callback(&ScanResultCallback)) != ESP_OK)
  {
    ESP_LOGE(TAG, "Could not register Scan Result Callback: %s", esp_err_to_name(status));
  }
}

void DisableBLE()
{
  ESP_LOGI(TAG, "Disabling BLE");
  ESP_ERROR_CHECK(esp_bluedroid_disable());
  ESP_ERROR_CHECK(esp_bluedroid_deinit());
  ESP_ERROR_CHECK(esp_bt_controller_disable());
  ESP_ERROR_CHECK(esp_bt_controller_deinit());
}

void StartScan()
{
  ESP_LOGI(TAG, "Start Scan for 10s");
  esp_ble_gap_start_scanning(SCAN_DURATION);
}

void StopScan()
{
  ESP_LOGI(TAG, "Scan Stopped");
  esp_ble_gap_stop_scanning();
}

static bool UUIDCorresponds(const struct ble_scan_result_evt_param *scanResult, const uint16_t short_uuid)
{
  uint8_t *data = scanResult->ble_adv;
  uint16_t uuid = data[2] | (data[3] << 8);
  return uuid == short_uuid;
}

static void SwitchToWiFiCallback()
{
  ESP_LOGI(TAG, "Switching back to WiFi mode");
  LEDEvent(SWITCH_MODE);
  DisableBLE();
  SwitchWiFi();
  vTaskDelete(NULL);
}

static void ScanResultCallback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
  ESP_LOGI(TAG, "Event number %d", event);
  switch (event)
  {
  // Scan stops
  case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
    ESP_LOGI(TAG, "Scan Stopped");
    return;
    // Scan starts
  case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
    ESP_LOGI(TAG, "Scan Started");
    break;
  case ESP_GAP_BLE_SCAN_RESULT_EVT: // When we scanned a result event
    const struct ble_scan_result_evt_param *scanResult = &(param->scan_rst);
    ESP_LOGI(TAG, "Number of responses: %d", scanResult->num_resps);
    if (UUIDCorresponds(scanResult, SHORT_WIFI_UUID_TARGET) && scanResult->adv_data_len > 0 && scanResult->scan_rsp_len > 0)
    {
      ESP_LOGI(TAG, "Message from WIFI UUID %s", scanResult->ble_adv);
      uint8_t *ssid = malloc(sizeof(uint8_t) * MAX_SSID_SIZE + 1);
      uint8_t *pwd = malloc(sizeof(uint8_t) * MAX_PWD_SIZE + 1);
      // safe check of memory allocation
      if (!pwd || !ssid)
      {
        ESP_LOGE(TAG, "NOT ENOUGH MEMORY TO MAKE ALLOCATION, RESTARTING BOARD");
        LEDEvent(MEM_ALLOC_FAILURE);
        vTaskDelay(500 / portTICK_PERIOD_MS);
      }
      else
      {
        FetchCredentials(scanResult, ssid, pwd);
        SetCredentials(ssid, pwd);
        ESP_LOGI(TAG, "New SSID: [%s] pwd: [%s]", (char *)ssid, (char *)pwd);
        free(ssid);
        free(pwd);
        LEDEvent(BLE_CONFIG_SETTED);
      }
    }
    else if (UUIDCorresponds(scanResult, SHORT_TOKEN_UUID_TARGET) && scanResult->adv_data_len > 0 && scanResult->scan_rsp_len > 0)
    {
      ESP_LOGI(TAG, "Message from Token UUID %s", scanResult->ble_adv);
      uint8_t *token = malloc(sizeof(uint8_t) * TOKEN_SIZE + 1);
      // safe check of memory allocation
      if (!token)
      {
        ESP_LOGE(TAG, "NOT ENOUGH MEMORY TO MAKE ALLOCATION, RESTARTING BOARD");
        LEDEvent(MEM_ALLOC_FAILURE);
        vTaskDelay(500 / portTICK_PERIOD_MS);
      }
      else
      {
        FetchTokenAPI(scanResult, token);
        RegisterTokenAPI((char *)token);
        ESP_LOGI(TAG, "New Token: [%s]", (char *)token);
        free(token);
        LEDEvent(BLE_CONFIG_SETTED);
      }
    }
    else if (scanResult->search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT)
    {
      if (++scan_retry > MAX_RETRY)
      {
        scan_retry = 0;
        xTaskCreate(SwitchToWiFiCallback, "switch_to_WiFi", 2096, NULL, 5, NULL);
      }
      else
      {
        ESP_LOGI(TAG, "Scan %d out of %d", scan_retry, MAX_RETRY);
        StartScan();
      }
    }
    break;
  default: // Other cases, just ignore
    break;
  }
}

static void FetchCredentials(
    const struct ble_scan_result_evt_param *scanResult,
    uint8_t *ssid,
    uint8_t *pwd)
{
  ESP_LOGI(TAG, "Fetching credentials. Adv data length: %d, Scan rsp len: %d",
           scanResult->adv_data_len, scanResult->scan_rsp_len);
  const int size_ssid = (MAX_SSID_SIZE > scanResult->adv_data_len - DATA_START_OFFSET) ? scanResult->adv_data_len - DATA_START_OFFSET : MAX_SSID_SIZE;
  const int size_pwd = (MAX_PWD_SIZE > scanResult->scan_rsp_len - DATA_START_OFFSET) ? scanResult->scan_rsp_len - DATA_START_OFFSET : MAX_PWD_SIZE;
  strncpy((char *)ssid, (char *)(scanResult->ble_adv + DATA_START_OFFSET), size_ssid);
  strncpy((char *)pwd, (char *)(scanResult->ble_adv + scanResult->adv_data_len + DATA_START_OFFSET), size_pwd);
  *(ssid + size_ssid) = '\0';
  *(pwd + size_pwd) = '\0';
}

static void FetchTokenAPI(
    const struct ble_scan_result_evt_param *scanResult,
    uint8_t *token)
{
  ESP_LOGI(TAG, "Fetching Token API. Adv data length: %d, Scan rsp len: %d",
           scanResult->adv_data_len, scanResult->scan_rsp_len);
  strncpy((char *)token, (char *)(scanResult->ble_adv + DATA_START_OFFSET), (int)scanResult->adv_data_len - DATA_START_OFFSET);
  strncpy((char *)(token + scanResult->adv_data_len - DATA_START_OFFSET), (char *)(scanResult->ble_adv + scanResult->adv_data_len + DATA_START_OFFSET), (int)scanResult->scan_rsp_len - DATA_START_OFFSET);
  *(token + TOKEN_SIZE) = '\0';
}