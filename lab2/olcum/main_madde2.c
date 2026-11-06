/*
 * Lab 2 - Madde 2: GPIO4'u uc yontemle toggle edip en yuksek frekansi olc
 *   toggle_driver : gpio_set_level()
 *   toggle_struct : GPIO.out_w1ts / GPIO.out_w1tc (soc/gpio_struct.h)
 *   toggle_raw    : ham register adresi
 * Her yontem N_TUR kez, her tur N_TOGGLE periyot (set + clear).
 */
#include <stdint.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "esp_timer.h"
#include "esp_log.h"

#define LED_GPIO            4
#define LED_MASK            (1u << LED_GPIO)
#define N_TOGGLE            1000000u
#define N_TUR               5

#define GPIO_OUT_W1TS_ADDR  0x60004008u
#define GPIO_OUT_W1TC_ADDR  0x6000400Cu
#define REG(addr)           (*(volatile uint32_t *)(addr))

static const char *TAG = "lab2";

typedef void (*toggle_fn_t)(uint32_t n);

__attribute__((noinline)) static void toggle_driver(uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        gpio_set_level(LED_GPIO, 1);
        gpio_set_level(LED_GPIO, 0);
    }
}

__attribute__((noinline)) static void toggle_struct(uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        GPIO.out_w1ts = LED_MASK;   /* derleme hatasi olursa: GPIO.out_w1ts.val */
        GPIO.out_w1tc = LED_MASK;   /* derleme hatasi olursa: GPIO.out_w1tc.val */
    }
}

__attribute__((noinline)) static void toggle_raw(uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        REG(GPIO_OUT_W1TS_ADDR) = LED_MASK;
        REG(GPIO_OUT_W1TC_ADDR) = LED_MASK;
    }
}

static void measure(const char *name, toggle_fn_t fn)
{
    uint64_t sum_hz = 0;
    for (int tur = 1; tur <= N_TUR; tur++) {
        int64_t t0 = esp_timer_get_time();
        fn(N_TOGGLE);
        int64_t us = esp_timer_get_time() - t0;
        uint64_t hz = ((uint64_t)N_TOGGLE * 1000000ull) / (uint64_t)us;
        sum_hz += hz;
        ESP_LOGI(TAG, "%s tur %d: %" PRId64 " us, f = %" PRIu64 " Hz",
                 name, tur, us, hz);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    ESP_LOGI(TAG, "%s ortalama: f = %" PRIu64 " Hz", name, sum_hz / N_TUR);
}

void app_main(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    ESP_LOGI(TAG, "Madde 2: N_TOGGLE=%u, N_TUR=%d", (unsigned)N_TOGGLE, N_TUR);
    measure("gpio_set_level", toggle_driver);
    measure("GPIO struct   ", toggle_struct);
    measure("ham register  ", toggle_raw);
    ESP_LOGI(TAG, "Olcum bitti.");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}