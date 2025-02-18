#include "LeScanner.h"

static esp_ble_scan_params_t scanParams = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x3E8, //1 sec
    .scan_window            = 0x3E8, //1 sec
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};



void InitScan() {
    ESP_LOGI(TAG,"InitScan");
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);

    
    esp_err_t status;

    if ((status = esp_ble_gap_register_callback(&ScanResultCallback)) != ESP_OK){
        ESP_LOGE(TAG, "Could not register Scan Result Callback: %s",esp_err_to_name(status));
        return;
    }
}

void StartScan() {

}

static void ScanResultCallback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param){
    switch (event)
    {
    // Scan stops
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        ESP_LOGI(TAG,"Scan Stopped");
        return;
        // Scan starts
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: // When we scanned a result event
        break;
    default: // Other cases, just ignore
        break;
    }
}