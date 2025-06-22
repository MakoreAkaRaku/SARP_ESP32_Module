idf_component_register(SRCS "HttpsClient.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_http_client mbedtls json)