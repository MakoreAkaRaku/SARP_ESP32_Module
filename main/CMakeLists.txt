idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES Connection Led HttpsClient Module nvs_flash)