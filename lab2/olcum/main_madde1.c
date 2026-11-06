/*
 * Lab 2 - Madde 1: GPIO4'u registerlarla ve gpio_config() ile yapilandirma
 * USE_DRIVER 0 -> elle register, USE_DRIVER 1 -> gpio_config()
 */
#include <stdint.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define USE_DRIVER              1   /* 0 = elle register, 1 = gpio_config() */

#define DR_GPIO_BASE            0x60004000u
#define DR_IO_MUX_BASE          0x60009000u

#define GPIO_OUT_REG            (DR_GPIO_BASE + 0x0004u)
#define GPIO_OUT_W1TS_REG       (DR_GPIO_BASE + 0x0008u)
#define GPIO_OUT_W1TC_REG       (DR_GPIO_BASE + 0x000Cu)
#define GPIO_ENABLE_REG         (DR_GPIO_BASE + 0x0020u)
#define GPIO_ENABLE_W1TS_REG    (DR_GPIO_BASE + 0x0024u)
#define GPIO_ENABLE_W1TC_REG    (DR_GPIO_BASE + 0x0028u)
#define GPIO_IN_REG             (DR_GPIO_BASE + 0x003Cu)
#define GPIO_PIN_REG(n)         (DR_GPIO_BASE + 0x0074u + 4u * (n))
#define GPIO_FUNC_OUT_SEL_REG(n) (DR_GPIO_BASE + 0x0554u + 4u * (n))
#define IO_MUX_GPIO_REG(n)      (DR_IO_MUX_BASE + 0x0004u + 4u * (n))

#define FUN_PD                  (1u << 7)
#define FUN_PU                  (1u << 8)
#define FUN_IE                  (1u << 9)
#define FUN_DRV_S               10
#define FUN_DRV_M               (0x3u << FUN_DRV_S)
#define MCU_SEL_S               12
#define MCU_SEL_M               (0x7u << MCU_SEL_S)
#define PIN_FUNC_GPIO           1u
#define PIN_PAD_DRIVER          (1u << 2)
#define PIN_INT_TYPE_M          (0x7u << 7)
#define PIN_INT_ENA_M           (0x1Fu << 13)
#define SIG_GPIO_OUT_IDX        256u
#define FUNC_OEN_SEL            (1u << 10)

#define REG(addr)               (*(volatile uint32_t *)(addr))
#define LED_GPIO                4u
#define LED_MASK                (1u << LED_GPIO)

static const char *TAG = "lab2";

#if !USE_DRIVER
static void led_init_regs(void)
{
    REG(GPIO_ENABLE_W1TC_REG) = LED_MASK;
    REG(GPIO_OUT_W1TC_REG)    = LED_MASK;

    uint32_t mux = REG(IO_MUX_GPIO_REG(LED_GPIO));
    mux &= ~(MCU_SEL_M | FUN_DRV_M | FUN_IE | FUN_PU | FUN_PD);
    mux |= (PIN_FUNC_GPIO << MCU_SEL_S) | (2u << FUN_DRV_S) | FUN_IE;
    REG(IO_MUX_GPIO_REG(LED_GPIO)) = mux;

    uint32_t pin = REG(GPIO_PIN_REG(LED_GPIO));
    pin &= ~(PIN_PAD_DRIVER | PIN_INT_TYPE_M | PIN_INT_ENA_M);
    REG(GPIO_PIN_REG(LED_GPIO)) = pin;

    REG(GPIO_FUNC_OUT_SEL_REG(LED_GPIO)) = SIG_GPIO_OUT_IDX | FUNC_OEN_SEL;
    REG(GPIO_ENABLE_W1TS_REG) = LED_MASK;
}
#endif

static void reg_dump(void)
{
    ESP_LOGI(TAG, "IO_MUX_GPIO4  = 0x%08" PRIx32, REG(IO_MUX_GPIO_REG(LED_GPIO)));
    ESP_LOGI(TAG, "GPIO_PIN4     = 0x%08" PRIx32, REG(GPIO_PIN_REG(LED_GPIO)));
    ESP_LOGI(TAG, "FUNC4_OUT_SEL = 0x%08" PRIx32, REG(GPIO_FUNC_OUT_SEL_REG(LED_GPIO)));
    ESP_LOGI(TAG, "GPIO_ENABLE   = 0x%08" PRIx32, REG(GPIO_ENABLE_REG));
    ESP_LOGI(TAG, "GPIO_OUT      = 0x%08" PRIx32, REG(GPIO_OUT_REG));
    ESP_LOGI(TAG, "GPIO_IN       = 0x%08" PRIx32, REG(GPIO_IN_REG));
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

void app_main(void)
{
#if USE_DRIVER
    ESP_LOGI(TAG, "MOD: gpio_config() (driver)");
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
#else
    ESP_LOGI(TAG, "MOD: elle register");
    led_init_regs();
#endif

    while (1) {
        reg_dump();
        blink(3, 500);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}