#include "esp_http_client.h"

#define SERVER_URL_API "https://sarp01.westeurope.cloudapp.azure.com/api"
#define MODULE_URL "/module/"

static esp_err_t _http_event_handler(esp_http_client_event_t *evt);
esp_err_t PerformHttpRequest(esp_http_client_method_t method,
                               const char *url,
                               const char *post_data,
                               char *response_buffer,
                               size_t buffer_len);
const char *RegisterModule(const char *token_api);