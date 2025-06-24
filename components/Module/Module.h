#include "nvs_flash.h"

void ModuleInit();

void InitPollingTask();
static void UpdateModuleState();
void InitializePeripherals();
float GetHygrometerValue();
float GetThermometerValue();
int GetValveState();
esp_err_t SetValveState(int state);