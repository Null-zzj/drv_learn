#include "led_resource.h"

struct led_resource resource = {
    .led[0] = GROUP_PIN(5,3),
    .led[1] = 0,
};

struct led_resource* get_led_sources(void)
{
    return &resource;
}