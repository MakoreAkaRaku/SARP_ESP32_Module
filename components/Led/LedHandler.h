#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum LED_MODE
{
    SWITCH_MODE,
    WIFI_CONNECTING,
    BLE_SCANNING,
    WIFI_CONNECTED,
    BLE_CONFIG_SETTED,
    MEM_ALLOC_FAILURE,
    ERROR,
    OK,
};

void InitLEDS();

/**
 * @brief The LEDEvent function controls the blinking behavior
 * of an LED based on the specified LED_MODE state. It uses predefined
 * timing and repetition patterns for different states, such as error,
 * Wi-Fi connection status, BLE scanning, and mode switching, by setting
 * GPIO pin levels and introducing delays.
 * 
 */
void LEDEvent();