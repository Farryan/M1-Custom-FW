idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "." "../../Esp_spi_at"
    REQUIRES driver esp_wifi esp_event nvs_flash bt ieee802154 spi_flash
)
