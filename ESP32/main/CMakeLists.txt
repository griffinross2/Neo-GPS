idf_component_register(SRCS "main.cpp" "gps_if.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_spi esp_driver_gpio)