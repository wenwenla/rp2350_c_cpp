#include <stdio.h>
#include "pico/stdlib.h"

bool ds18b20_reset(uint gpio) {
    gpio_set_dir(gpio, true);
    gpio_put(gpio, 0);
    sleep_ms(1);
    gpio_set_dir(gpio, false);
    sleep_us(100);
    bool ret = gpio_get(gpio);
    while (gpio_get(gpio) == 0);
    sleep_us(480);
    return ret == 0;
}

inline static void ds18b20_w0(uint gpio) {
    gpio_set_dir(gpio, true);
    gpio_put(gpio, 0);
    sleep_us(63); // 60us - 120us
    gpio_put(gpio, 1);
    sleep_us(2);
}

inline static void ds18b20_w1(uint gpio) {
    gpio_set_dir(gpio, true);
    gpio_put(gpio, 0);
    sleep_us(2);
    gpio_put(gpio, 1);
    sleep_us(63);
}

void ds18b20_write(uint gpio, uint dat) {
    for (int i = 0; i < 8; ++i) {
        (dat >> i & 1) ? ds18b20_w1(gpio) : ds18b20_w0(gpio);
    }
}

uint ds18b20_read(uint gpio) {
    // 65us
    gpio_set_dir(gpio, true);
    gpio_put(gpio, 0);
    sleep_us(2);
    gpio_set_dir(gpio, false);
    sleep_us(8);
    bool ret = gpio_get(gpio);
    sleep_us(55);
    return ret;
}

int main()
{
    stdio_init_all();

    gpio_init(0);
    gpio_pull_up(0);

    ds18b20_reset(0);
    ds18b20_write(0, 0x33);
    uint64_t data = 0;
    uint temperature = 0;
    for (int i = 0; i < 64; ++i) {
        uint b = ds18b20_read(0);
        data |= ((uint64_t)b << i);
    }
    printf("%llx\n", data);

    while (true) {
        ds18b20_reset(0);
        ds18b20_write(0, 0xcc); // skip rom
        ds18b20_write(0, 0x44); // convert temperature
        // wait for 750ms
        sleep_ms(750);

        ds18b20_reset(0);
        ds18b20_write(0, 0xcc);
        ds18b20_write(0, 0xbe); // read data
        for (int i = 0; i < 2 * 8; ++i) {
            temperature = temperature | ((uint)ds18b20_read(0) << i);
        }
        for (int i = 0; i < 7 * 8; ++i) {
            ds18b20_read(0);
        }
        bool neg = (temperature >> 11 & 1);
        temperature &= 0x7ff;
        int t0 = temperature >> 4;
        float v = (temperature & 0xF) / 16.f;
        printf("T: %u %.4f\n", temperature, (t0 + v) * (neg ? -1 : 1));
        sleep_ms(1000);
    }
}
