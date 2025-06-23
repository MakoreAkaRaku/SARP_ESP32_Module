idf_component_register(SRCS "Module.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash HttpsClient)