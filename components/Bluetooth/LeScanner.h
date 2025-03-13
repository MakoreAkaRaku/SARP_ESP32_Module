#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

void EnableBLE();

void DisableBLE();
void StartScan();
/**
 * @brief Checks whether a scan result matches with the UUID service assigned to listen to.
 *
 * @param scanResult
 * @return true
 * @return false
 */
static bool UUIDCorresponds(const struct ble_scan_result_evt_param *scanResult);

void StopScan();

/**
 * @brief Callback function that handles the scanned event results.
 *
 * @param event
 * @param param
 */
static void ScanResultCallback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

static void SwitchToWiFiCallback();

/**
 * @brief Fetches Credentials from a scanResult adv that matches with the UUID service established.
 *
 * @param scanResult
 * @param ssid
 * @param pwd
 */
static void FetchCredentials(const struct ble_scan_result_evt_param *scanResult, uint8_t *ssid, uint8_t *pwd);