
#include "esp_http_client.h"


static esp_err_t _http_event_handler(esp_http_client_event_t *evt);
esp_err_t perform_http_request(esp_http_client_method_t method,
                               const char *url,
                               const char *post_data,
                               char *response_buffer,
                               size_t buffer_len);
