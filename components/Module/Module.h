#include "nvs_flash.h"

void ModuleInit();

static void InitPollingTask();
static void UpdateModuleState();
static void InitializePeripheralsPinSets();
double GetHygrometerValue();
double GetThermometerValue();
int GetValveState();
esp_err_t SetValveState(int state);