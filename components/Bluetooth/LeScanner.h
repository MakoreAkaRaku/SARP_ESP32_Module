#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

void EnableBLE();

void DisableBLE();
void StartScan();

/**
 * @brief Checks whether a scan result matches with a given short UUID.
 * This function checks if the short UUID corresponds to the one
 * advertised in the scan result.
 * @param scanResult The scan result to check.
 * @param short_uuid The short UUID to compare against.
 * @return true if the UUID matches, false otherwise.
 */
static bool UUIDCorresponds(const struct ble_scan_result_evt_param *scanResult,const uint16_t short_uuid);

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

/**
 * @brief Fetches Token API from a scanResult adv that matches with the UUID service established.
 *
 * @param scanResult
 * @param token
 */
static void FetchTokenAPI(const struct ble_scan_result_evt_param *scanResult, uint8_t *token);