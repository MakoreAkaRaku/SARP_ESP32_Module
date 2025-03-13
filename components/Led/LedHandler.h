#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum LED_MODE
{
    SWITCH_TO_BLE_SCAN,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    BLE_CONFIG_SETTED,
    MEM_ALLOC_FAILURE,
    ERROR,
    OK,
};

void InitLEDS();

void LEDEvent();