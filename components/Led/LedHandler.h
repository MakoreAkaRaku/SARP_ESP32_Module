#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum LED_MODE {
    SWAP_TO_BLE_SCAN,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    BLE_CONFIG_RECEIVED,
    ERROR,
    OK,
};

void InitLEDS();

void LEDEvent();