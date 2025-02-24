#include "LedHandler.h"



void InitLEDS() {
    gpio_set_direction(GPIO_NUM_2,GPIO_MODE_OUTPUT);
}

void LEDEvent(enum LED_MODE state ) {
    TickType_t timesInterval = 0;   // Time delay in between secuences.
    int8_t times = 0;               // Times to repeat a secuence.
    int8_t blinks = 0;              // blinks per secuence.
    TickType_t blinkInterval = 0;   // Time delay between blinks
    switch (state)
    {
    case ERROR:
        times = 1;
        blinks = 5;
        timesInterval = 0;
        blinkInterval = 500;
        break;
    case WIFI_CONNECTED:
        times = 1;
        blinks = 2;
        blinkInterval = 500;
        timesInterval = 1000;
        break;
    case BLE_CONFIG_RECEIVED:
        times = 3;
        blinks = 3;
        blinkInterval = 400;
        timesInterval = 800;
        break;
        case WIFI_CONNECTING:
        times = 1;
        blinks = 3;
        blinkInterval = 40;
        timesInterval =0;
        break;
    default:
        break;
    }

    for (size_t j = 0; j < times; j++)
    {
        for (size_t i = 0; i < blinks; i++)
        {
            gpio_set_level(GPIO_NUM_2,1);
            vTaskDelay(blinkInterval);
            gpio_set_level(GPIO_NUM_2,0);
        }
        vTaskDelay(timesInterval);
    }
}