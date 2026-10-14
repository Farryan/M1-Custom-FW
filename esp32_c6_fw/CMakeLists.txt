idf_component_register(
    SRCS "main.c" "m1_esp_spi_protocol.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_wifi esp_event nvs_flash spi_flash
)
