#include <stdlib.h>
#include "Mocker.h"

float GetTemperature()
{
    float matress = (rand() % 4000) / 4000.0f;
    int value = rand() % 40;
    return value + matress;
}

float GetHumidity()
{
    float matress = (rand() % 4000) / 4000.0f;
    int value = rand() % 100;
    return value + matress;
}