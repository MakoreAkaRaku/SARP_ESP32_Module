idf_component_register(SRCS "Module.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_timer esp_adc nvs_flash driver HttpsClient)