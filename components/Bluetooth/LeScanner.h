#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"


void InitScan();

void StartScan();

void StopScan();

/**
 * @brief Callback function that handles the scanned event results.
 * 
 * @param event 
 * @param param 
 */
static void ScanResultCallback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);