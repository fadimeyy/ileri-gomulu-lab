/*
 * Ileri Gomulu Sistemler - Laboratuvar 3, Gorev 1: UART geri donus
 * ESP32-S3: UART1 TX = GPIO17, RX = GPIO18 (GPIO17 ile GPIO18 jumperla birlestirilir)
 *
 * A) ayni baud'da geri donus; gonderme suresi kuramsal sureyle karsilastirilir
 * B) alici UART2'nin baud'u degistirilir (verici hep 115200); hatali alim gozlenir
 *
 * Lojik analizor yok: sure esp_timer ile olculur, cerceveler bayt dokumuyle incelenir.
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_err.h"

#define TX_PIN   17
#define RX_PIN   18
#define BAUD_TX  115200
#define MSG      "Merhaba ESP32-S3, UART geri donus testi 0123456789\n"

static void uart_ac(uart_port_t port, int baud, int tx, int rx)
{
    uart_config_t cfg = {
        .baud_rate  = baud,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(port, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(port, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(port, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void test_a_geri_donus(void)
{
    const char *msg = MSG;
    int n = (int)strlen(msg);
    uint8_t rx[128] = {0};

    uart_flush_input(UART_NUM_1);
    int64_t t0 = esp_timer_get_time();
    uart_write_bytes(UART_NUM_1, msg, n);
    uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(500));
    int64_t t_us = esp_timer_get_time() - t0;
    int got = uart_read_bytes(UART_NUM_1, rx, n, pdMS_TO_TICKS(300));
    if (got < 0) got = 0;

    long long kuram_us = (long long)n * 10 * 1000000 / BAUD_TX;   /* 8N1: 10 bit/bayt */
    printf("A) gonderilen=%d alinan=%d esit=%s\n", n, got,
           (got == n && memcmp(rx, msg, (size_t)n) == 0) ? "EVET" : "HAYIR");
    printf("   gonderme suresi: olculen=%lld us, kuramsal (10 bit/bayt)=%lld us\n",
           (long long)t_us, kuram_us);
    printf("   alinan metin: %.*s", got, (const char *)rx);
}

static void test_b_yanlis_baud(void)
{
    const char *msg = MSG;
    int n = (int)strlen(msg);
    static const int bauds[] = {115200, 57600, 38400, 9600, 230400};
    uint8_t rx[256];

    printf("B) verici UART1 = %d baud; alici UART2 baud'u degisir\n", BAUD_TX);
    for (unsigned i = 0; i < sizeof(bauds) / sizeof(bauds[0]); i++) {
        ESP_ERROR_CHECK(uart_set_baudrate(UART_NUM_2, (uint32_t)bauds[i]));
        uart_flush_input(UART_NUM_1);
        uart_flush_input(UART_NUM_2);
        uart_write_bytes(UART_NUM_1, msg, n);
        uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(500));
        vTaskDelay(pdMS_TO_TICKS(100));
        int got = uart_read_bytes(UART_NUM_2, rx, sizeof(rx), pdMS_TO_TICKS(200));
        if (got < 0) got = 0;
        int dogru = 0;
        for (int k = 0; k < got && k < n; k++) {
            if (rx[k] == (uint8_t)msg[k]) dogru++;
        }
        printf("   alici baud=%6d: alinan=%3d bayt, dogru=%3d/%d, ilk 12 bayt:",
               bauds[i], got, dogru, n);
        for (int k = 0; k < got && k < 12; k++) printf(" %02X", rx[k]);
        printf("\n");
    }
}

void app_main(void)
{
    uart_ac(UART_NUM_1, BAUD_TX, TX_PIN, RX_PIN);
    uart_ac(UART_NUM_2, BAUD_TX, UART_PIN_NO_CHANGE, RX_PIN);   /* yalnizca RX: GPIO18 */
    vTaskDelay(pdMS_TO_TICKS(500));

    for (int tur = 1; ; tur++) {
        printf("\n===== UART testi, tur %d =====\n", tur);
        test_a_geri_donus();
        test_b_yanlis_baud();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
