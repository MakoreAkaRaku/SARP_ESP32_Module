#include "HttpsClient.h"
#include "esp_log.h"
static const char TAG[] = "HTTPSClient";
/* Root cert for the web, taken from firefox navigator by asking for the certificate
   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/

#define SERVER_URL_API "https://sarp01.westeurope.cloudapp.azure.com/api"
extern const char server_root_cert_pem_start[] asm("_binary_server_root_pem_start");
extern const char server_root_cert_pem_end[] asm("_binary_server_root_pem_end");

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  switch (evt->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    if (!esp_http_client_is_chunked_response(evt->client))
    {
      // Write data received to stdout
      // If you're building a real application, you'd store this in a buffer
      // or process it directly.
      printf("%.*s", evt->data_len, (char *)evt->data);
    }
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  case HTTP_EVENT_REDIRECT:
    ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
    break;
  }
  return ESP_OK;
}

/**
 * @brief Performs an HTTP request with a given method and URL.
 *
 * @param method The HTTP method (e.g., HTTP_METHOD_GET, HTTP_METHOD_POST).
 * @param url The URL for the request.
 * @param post_data Optional: Data to send in the request body for POST/PUT.
 * @param response_buffer Buffer to store the response data; must be large enough to hold the response.
 * @param buffer_len Length of the response buffer.
 * @return esp_err_t ESP_OK on success, otherwise an error code.
 */
esp_err_t perform_http_request(esp_http_client_method_t method,
                               const char *url,
                               const char *post_data,
                               char *response_buffer,
                               size_t buffer_len)
{
  // Init config struct
  esp_http_client_config_t config = {
      .cert_pem = server_root_cert_pem_start,
      .url = url,
      .method = method,
      .event_handler = _http_event_handler, // Always good to have an event handler
      .timeout_ms = 10000,                  // Set a timeout for the request
  };

  // Init HTTP client
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == NULL)
  {
    ESP_LOGE(TAG, "Failed to initialize HTTP client");
    return ESP_FAIL;
  }

  // 3. If request method is sends data, use of update POST data if provided
  if (method == HTTP_METHOD_POST || method == HTTP_METHOD_PUT || method == HTTP_METHOD_PATCH)
  {
    if (post_data != NULL)
    {
      esp_err_t err = esp_http_client_set_post_field(client, post_data, strlen(post_data));
      if (err != ESP_OK)
      {
        ESP_LOGE(TAG, "Failed to set POST field: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
      }
      esp_http_client_set_header(client, "Content-Type", "application/json"); // Example for JSON
    }
    else
    {
      ESP_LOGW(TAG, "POST/PUT/PATCH method used but no post_data provided.");
    }
  }

  // Perform the HTTP request
  esp_err_t err = esp_http_client_perform(client);

  // Check results and log any status/errors
  if (err == ESP_OK)
  {
    const long long int content_length = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "HTTP %s Status = %d, content_length = %lld",
             (method == HTTP_METHOD_GET) ? "GET" : ((method == HTTP_METHOD_POST) ? "POST" : "OTHER"),
             esp_http_client_get_status_code(client),
             content_length);
    long long int total_read = 0;
    if (content_length > 0)
    { // Only read if there's content to read
      int read_len = 0;
      while (total_read < buffer_len - 1)
      { // Leave space for null terminator
        read_len = esp_http_client_read(client, response_buffer + total_read, buffer_len - 1 - total_read);
        if (read_len == 0)
        { // No more data
          ESP_LOGD(TAG, "No more data to read.");
          break;
        }
        else if (read_len < 0)
        { // Error
          ESP_LOGE(TAG, "esp_http_client_read failed:");
          err = ESP_FAIL; // Indicate an error occurred during read
          break;
        }
        total_read += read_len;
        ESP_LOGD(TAG, "Read %d bytes, total %lld", read_len, total_read);
      }
    }
    response_buffer[total_read] = '\0'; // Null-terminate the response buffer
  }
  else
  {
    ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
  }

  // Clean up client
  esp_http_client_cleanup(client);

  return err;
}