/*
 * Ileri Gomulu Sistemler - Laboratuvar 3, Gorev 3: SPI geri donus
 * ESP32-S3, SPI2 (FSPI): CS = GPIO10, MOSI = GPIO11, SCLK = GPIO12, MISO = GPIO13
 * GPIO11 (MOSI) ile GPIO13 (MISO) bir jumperla birlestirilir.
 *
 * 1) mod 0-3: 4 bayt gonder/al, esitlik ve SCLK bosta duzeyi (CPOL)
 * 2) mod 0, 100 kHz / 1 MHz / 10 MHz: islem suresi, kuramsal sureyle karsilastirma
 *
 * Lojik analizor yok: sinyaller yerine rx/tx karsilastirmasi ve sure olculur.
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_err.h"

#define PIN_CS    10
#define PIN_MOSI  11
#define PIN_SCLK  12
#define PIN_MISO  13

static const uint8_t TX[4] = {0xA5, 0x3C, 0x0F, 0xF0};

static spi_device_handle_t cihaz_ekle(int mod, int hiz)
{
    spi_device_interface_config_t dev = {
        .clock_speed_hz = hiz,
        .mode = mod,
        .spics_io_num = PIN_CS,
        .queue_size = 1,
    };
    spi_device_handle_t h;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &h));
    return h;
}

static int gonder(spi_device_handle_t h, uint8_t rx[4])
{
    spi_transaction_t t = {
        .length = 32,                 /* bit sayisi, bayt degil */
        .tx_buffer = TX,
        .rx_buffer = rx,
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(h, &t));
    return memcmp(TX, rx, 4) == 0;
}

static void mod_testi(void)
{
    printf("1) mod 0-3, 1 MHz, tx = A5 3C 0F F0\n");
    for (int mod = 0; mod < 4; mod++) {
        spi_device_handle_t h = cihaz_ekle(mod, 1000000);
        uint8_t rx[4] = {0};
        int ok = gonder(h, rx);
        int sclk_bosta = gpio_get_level(PIN_SCLK);
        printf("   mod %d: rx = %02X %02X %02X %02X  %s   SCLK bosta = %d\n",
               mod, rx[0], rx[1], rx[2], rx[3], ok ? "ESIT" : "FARKLI", sclk_bosta);
        ESP_ERROR_CHECK(spi_bus_remove_device(h));
    }
}

static void hiz_testi(void)
{
    static const int hizlar[] = {100000, 1000000, 10000000};
    printf("2) mod 0, islem suresi (4 bayt = 32 bit):\n");
    for (unsigned i = 0; i < sizeof(hizlar) / sizeof(hizlar[0]); i++) {
        spi_device_handle_t h = cihaz_ekle(0, hizlar[i]);
        uint8_t rx[4];
        const int N = 200;
        int fark = 0;
        int64_t t0 = esp_timer_get_time();
        for (int k = 0; k < N; k++) fark += !gonder(h, rx);
        int64_t dt = esp_timer_get_time() - t0;
        long long kuram_ns = 32LL * 1000000000LL / hizlar[i];
        printf("   %8d Hz: olculen=%lld us/islem, kuramsal (32 bit)=%lld ns, farkli=%d/%d\n",
               hizlar[i], (long long)(dt / N), kuram_ns, fark, N);
        ESP_ERROR_CHECK(spi_bus_remove_device(h));
    }
}

void app_main(void)
{
    spi_bus_config_t bus = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    vTaskDelay(pdMS_TO_TICKS(500));

    for (int tur = 1; ; tur++) {
        printf("\n===== SPI testi, tur %d =====\n", tur);
        mod_testi();
        hiz_testi();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
