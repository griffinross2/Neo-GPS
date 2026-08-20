idf_component_register(SRCS
    "main.cpp"
    "gps_if.cpp"
    "tools.cpp"
    "filters.cpp"
    "track_l1ca.cpp"
    "ephm_l1ca.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_spi esp_driver_gpio)