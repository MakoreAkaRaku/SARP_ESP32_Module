idf_component_register(SRCS "HttpsClient.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_http_client)

target_add_binary_data(${COMPONENT_TARGET} "certs/server_root.pem" TEXT)