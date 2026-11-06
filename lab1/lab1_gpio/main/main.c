/*
 * Ileri Gomulu Sistemler - Laboratuvar 1 (ESP32-S3)
 * GPIO4'e bagli LED'in GPIO_OUT_W1TS / W1TC registerlariyla surulmesi
 * ve en yuksek pin degistirme frekansinin tahmini.
 */
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_cpu.h"

#define LED_GPIO              4u

/* ESP32-S3 GPIO registerlari (ESP-IDF soc/gpio_reg.h ile ayni adresler) */
#define REG(addr)             (*(volatile uint32_t *)(addr))
#define GPIO_OUT_W1TS_REG     0x60004008u   /* yazilan 1'ler: pini 1 yap */
#define GPIO_OUT_W1TC_REG     0x6000400Cu   /* yazilan 1'ler: pini 0 yap */
#define GPIO_ENABLE_W1TS_REG  0x60004024u   /* yazilan 1'ler: cikisi ac  */

#define LED_MASK              (1u << LED_GPIO)
#define TOGGLE_COUNT          100000
#define N                     1000
#define M                     100

static const char *TAG = "lab1";

static void led_init(void)
{
    gpio_reset_pin(LED_GPIO);

    REG(GPIO_OUT_W1TC_REG)    = LED_MASK;   /* baslangic duzeyi: 0 */
    REG(GPIO_ENABLE_W1TS_REG) = LED_MASK;   /* cikis surucusunu ac */
}

static void blink(int count, int half_period_ms)
{
    for (int i = 0; i < count; i++) {
        REG(GPIO_OUT_W1TS_REG) = LED_MASK;
        vTaskDelay(pdMS_TO_TICKS(half_period_ms));
        REG(GPIO_OUT_W1TC_REG) = LED_MASK;
        vTaskDelay(pdMS_TO_TICKS(half_period_ms));
    }
}

static void toggle_burst(void)
{
    int64_t t0 = esp_timer_get_time();
    for (int i = 0; i < TOGGLE_COUNT; i++) {
        REG(GPIO_OUT_W1TS_REG) = LED_MASK;
        REG(GPIO_OUT_W1TC_REG) = LED_MASK;
    }
    int64_t dt_us = esp_timer_get_time() - t0;

    int64_t f_hz = (int64_t)TOGGLE_COUNT * 1000000 / dt_us;
    ESP_LOGI(TAG, "%d periyot, %" PRId64 " us, ortalama f = %" PRId64 " Hz",
             TOGGLE_COUNT, dt_us, f_hz);
}

static void jitter(void)
{
    static uint32_t d[N];
    for (int i = 0; i < N; i++) {
        uint32_t c0 = esp_cpu_get_cycle_count();
        for (int k = 0; k < M; k++) {
            REG(GPIO_OUT_W1TS_REG) = LED_MASK;
            REG(GPIO_OUT_W1TC_REG) = LED_MASK;
        }
        d[i] = esp_cpu_get_cycle_count() - c0;
    }
    uint32_t mn = d[0], mx = d[0];
    double s = 0, s2 = 0;
    for (int i = 0; i < N; i++) {
        if (d[i] < mn) mn = d[i];
        if (d[i] > mx) mx = d[i];
        s += d[i];
        s2 += (double)d[i] * d[i];
    }
    double m = s / N, sd = sqrt(s2 / N - m * m);
    ESP_LOGI(TAG, "blok=%d periyot: min=%lu maks=%lu ort=%.1f sd=%.2f cevrim",
             M, (unsigned long)mn, (unsigned long)mx, m, sd);

    printf("CSV_BASLA\n");
    for (int i = 0; i < N; i++) printf("%lu\n", (unsigned long)d[i]);
    printf("CSV_BITIS\n");
}

void app_main(void)
{
    led_init();

    ESP_LOGI(TAG, "Bolum A: 1 Hz yanip sonme (5 kez)");
    blink(5, 500);

    ESP_LOGI(TAG, "Bolum B: jitter olcumu (her 3 s'de bir tekrar)");
    (void)toggle_burst;   /* kullanilmiyor, uyari vermesin */
    while (1) {
        jitter();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}