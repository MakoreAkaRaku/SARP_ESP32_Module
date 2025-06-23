#include "HttpsClient.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "cJson.h"
#define MODULE_REGISTRY_SERVER_RESPONSE_SIZE 128     // Size of the response buffer for module registration
#define PERIPHERAL_REGISTRY_SERVER_RESPONSE_SIZE 128 // Size of the response buffer for peripheral registration
static const char TAG[] = "HTTPSClient";

/**
 * @brief Handles HTTP events for the ESP HTTP client.
 * This function processes various events such as connection, data reception,
 * and disconnection, logging relevant information.
 *
 * @param evt Pointer to the HTTP client event structure.
 * @return esp_err_t ESP_OK on success, otherwise an error code.
 */
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
      memcpy(evt->user_data, evt->data, evt->data_len); // Copy data to user_data buffer
      // Write data received to stdout
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
 * Headers are set as json content type if the method is POST/PUT/PATCH.
 *
 * @param method The HTTP method (e.g., HTTP_METHOD_GET, HTTP_METHOD_POST).
 * @param url The URL for the request.
 * @param post_data Optional: Data to send in the request body for POST/PUT.
 * @param response_buffer Buffer to store the response data; must be large enough to hold the response.
 * @param resp_buff_leng Length of the response buffer.
 * @return esp_err_t ESP_OK on success, otherwise an error code.
 */
esp_err_t PerformHttpRequest(esp_http_client_method_t method,
                             const char *url,
                             const char *post_data,
                             char *response_buffer,
                             size_t resp_buff_leng)
{
  // Init config struct
  esp_http_client_config_t config = {
      .crt_bundle_attach = esp_crt_bundle_attach, // Use the built-in certificate bundle
      .url = url,
      .method = method,
      .event_handler = _http_event_handler, // Always good to have an event handler
      .timeout_ms = 100000,                 // Set a timeout for the request
      .user_data = response_buffer,         // Pass the response buffer to the event handler
      .buffer_size = resp_buff_leng,        // Set the buffer size for the response
  };

  // Init HTTP client
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == NULL)
  {
    ESP_LOGE(TAG, "Failed to initialize HTTP client");
    return ESP_FAIL;
  }

  // If request method is sends data, use of update POST data if provided
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
      esp_http_client_set_header(client, "Content-Type", "application/json");
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
  }
  else
  {
    ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
  }

  // Clean up client
  esp_http_client_cleanup(client);
  return err;
}

/**
 * @brief Returns the module string with the module registration response.
 *
 * @param token_api
 * @return const char* The module token string if successful, NULL on failure.
 */
const char *RegisterModule(const char *token_api)
{
  // Prepare the URL and request body
  char *url = malloc(strlen(SERVER_URL_API) + strlen(MODULE_URL) + 1); // Malloc of 128 bytes for URL plus 1 for null terminator
  if (url == NULL)
  {
    ESP_LOGE(TAG, "Memory allocation failed for URL");
    return NULL;
  }
  strcpy(url, SERVER_URL_API);
  strcat(url, MODULE_URL);

  cJSON *json_token_api = cJSON_CreateObject();
  if (json_token_api == NULL)
  {
    ESP_LOGE(TAG, "Failed to create JSON object");
    free(url);
    return NULL;
  }

  cJSON_AddStringToObject(json_token_api, "token_api", token_api);
  char *post_data = cJSON_PrintUnformatted(json_token_api);
  cJSON_Delete(json_token_api);

  if (post_data == NULL)
  {
    ESP_LOGE(TAG, "Failed to create JSON string");
    free(url);
    return NULL;
  }

  // Prepare response buffer
  char *server_response = malloc(MODULE_REGISTRY_SERVER_RESPONSE_SIZE * sizeof(char)); // Malloc of 128 bytes for response buffer
  if (server_response == NULL)
  {
    ESP_LOGE(TAG, "Memory allocation failed for response buffer");
    free(url);
    free(post_data);
    return NULL;
  }

  // Perform the HTTP request
  esp_err_t err = PerformHttpRequest(HTTP_METHOD_POST, url, post_data, server_response, MODULE_REGISTRY_SERVER_RESPONSE_SIZE);

  // Free allocated resources
  free(url);
  free(post_data);

  if (err != ESP_OK)
  {
    free(server_response);
    return NULL;
  }

  cJSON *json_response = cJSON_Parse(server_response);
  if (json_response->type != cJSON_Object)
  {
    ESP_LOGE(TAG, "Response is not a valid JSON object");
    cJSON_Delete(json_response);
    free(server_response);
    return NULL;
  }
  cJSON *attr_pointer = cJSON_GetObjectItem(json_response, "moduleToken");
  if (attr_pointer == NULL || attr_pointer->type != cJSON_String)
  {
    ESP_LOGE(TAG, "Response does not contain 'moduleToken' or it is not a string");
    cJSON_Delete(json_response);
    free(server_response);
    return NULL;
  }
  const char *response = strdup(attr_pointer->valuestring); // Duplicate the string to return
  if (response == NULL)
  {
    ESP_LOGE(TAG, "Memory allocation failed for response string");
    cJSON_Delete(json_response);
    free(server_response);
    return NULL;
  }
  return response;
}

/**
 * @brief Registers a peripheral with the given module token and type.
 *
 * @param module_token The token of the module to which the peripheral belongs.
 * @param p_type The type of the peripheral.
 * @return int The ID of the registered peripheral, or -1 on failure.
 */
const int RegisterPeripheral(const char *module_token, const char *p_type)
{
  // Prepare the URL and request body
  char *url = malloc(strlen(SERVER_URL_API) + strlen(PERIPHERAL_URL) + 1); // Malloc of 128 bytes for URL plus 1 for null terminator
  if (url == NULL)
  {
    ESP_LOGE(TAG, "Memory allocation failed for URL");
    return -1;
  }
  strcpy(url, SERVER_URL_API);
  strcat(url, PERIPHERAL_URL);

  cJSON *json_module_token = cJSON_CreateObject();
  if (json_module_token == NULL)
  {
    ESP_LOGE(TAG, "Failed to create JSON object");
    free(url);
    return -1;
  }

  cJSON_AddStringToObject(json_module_token, "parent_module", module_token);
  cJSON_AddStringToObject(json_module_token, "p_type", p_type);

  char *post_data = cJSON_PrintUnformatted(json_module_token);
  ESP_LOGI(TAG, "Post data: %s", post_data);
  cJSON_Delete(json_module_token);

  if (post_data == NULL)
  {
    ESP_LOGE(TAG, "Failed to create JSON string");
    free(url);
    return -1;
  }

  // Prepare response buffer
  char *server_response = malloc(MODULE_REGISTRY_SERVER_RESPONSE_SIZE * sizeof(char)); // Malloc of 128 bytes for response buffer
  if (server_response == NULL)
  {
    ESP_LOGE(TAG, "Memory allocation failed for response buffer");
    free(url);
    free(post_data);
    return -1;
  }

  // Perform the HTTP request
  esp_err_t err = PerformHttpRequest(HTTP_METHOD_POST, url, post_data, server_response, MODULE_REGISTRY_SERVER_RESPONSE_SIZE);

  // Free allocated resources
  free(url);
  free(post_data);

  if (err != ESP_OK)
  {
    free(server_response);
    return -1;
  }

  cJSON *json_response = cJSON_Parse(server_response);

  if (json_response->type != cJSON_Object)
  {
    ESP_LOGE(TAG, "Response is not a valid JSON object");
    cJSON_Delete(json_response);
    free(server_response);
    return -1;
  }

  cJSON *attr_pointer = cJSON_GetObjectItem(json_response, "id");

  if (attr_pointer == NULL || attr_pointer->type != cJSON_Number)
  {
    ESP_LOGE(TAG, "Response does not contain 'id' or it is not an integer");
    return -1;
  }
  int peripheral_id = atoi(attr_pointer->valuestring); // Convert the string to an integer
  cJSON_Delete(json_response);
  free(server_response);
  return peripheral_id; // Return the peripheral ID as an integer
}