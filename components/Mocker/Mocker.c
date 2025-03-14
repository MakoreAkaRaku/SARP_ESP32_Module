#include <stdlib.h>
#include "Mocker.h"

float GetTemperature() {
    float matress = (rand()%4000)/4000.0f;
    float value = rand()/40.f;
    return value+matress;
}

float GetHumidity() {
    float matress = (rand()%4000)/4000.0f;
    float value = rand()/100.f;
    return value+matress;
}