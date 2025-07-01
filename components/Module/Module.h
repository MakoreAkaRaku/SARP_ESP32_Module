#include "nvs_flash.h"

#define TOKEN_SIZE 36 // Token size in bytes (UUID length)

bool ModuleIsConfigured();
void ModuleInit();

static void InitPollingTask();
static void UpdateModuleState();
static void InitializePeripheralsPinSets();
double GetHygrometerValue();
double GetThermometerValue();
int GetValveState();

void RegisterTokenAPI(const char *token_api);

esp_err_t SetValveState(int state);