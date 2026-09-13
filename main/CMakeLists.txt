idf_component_register(
    SRCS
        "main.c"
        "app/traffic_light.c"
        "drivers/button.c"
        "drivers/light_sensor.c"
        "drivers/tick_timer.c"
        "drivers/traffic_leds.c"
    INCLUDE_DIRS "."
)
