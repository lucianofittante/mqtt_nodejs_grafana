idf_component_register(SRCS "main.c" "C:/Users/lucho/esp/esp-idf-v5.1.2/components/esp32-DHT11/dht11.c"
                    INCLUDE_DIRS "C:/Users/lucho/esp/esp-idf-v5.1.2/components/esp32-DHT11/include")

spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)
