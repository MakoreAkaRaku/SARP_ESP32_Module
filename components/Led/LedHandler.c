#include "LedHandler.h"

#define LED_GPIO GPIO_NUM_2
void InitLEDS()
{
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
}

void LEDEvent(enum LED_MODE state)
{
    TickType_t timesInterval = 0;                      // Time delay in between secuences.
    int8_t times = 0;                                  // Times to repeat a secuence.
    int8_t blinks = 0;                                 // blinks per secuence.
    TickType_t blinkInterval1 = 0, blinkInterval2 = 0; // Time delay between blinks
    switch (state)
    {
    case ERROR:
        times = 1;
        blinks = 5;
        blinkInterval1 = blinkInterval2 = pdMS_TO_TICKS(500);
        break;
    case WIFI_CONNECTED:
        times = 1;
        blinks = 4;
        blinkInterval1 = blinkInterval2 = pdMS_TO_TICKS(250);
        break;
    case WIFI_CONNECTING:
        times = 1;
        blinks = 3;
        blinkInterval1 = blinkInterval2 = pdMS_TO_TICKS(25);
        break;
    case BLE_SCANNING:
        times = 2;
        blinks = 2;
        blinkInterval1 = blinkInterval2 = pdMS_TO_TICKS(10);
        timesInterval = pdMS_TO_TICKS(50);
        break;
    case SWITCH_MODE:
        times = 1;
        blinks = 2;
        blinkInterval1 = blinkInterval2 = pdMS_TO_TICKS(80);
        break;
    case BLE_CONFIG_SETTED:
        times = 1;
        blinks = 1;
        blinkInterval1 = pdMS_TO_TICKS(1000);
        break;
    default:
        break;
    }

    for (size_t j = 0; j < times; j++)
    {
        for (size_t i = 0; i < blinks; i++)
        {
            gpio_set_level(GPIO_NUM_2, 1);
            vTaskDelay(blinkInterval1);
            gpio_set_level(GPIO_NUM_2, 0);
            vTaskDelay(blinkInterval2);
        }
        vTaskDelay(timesInterval);
    }
}