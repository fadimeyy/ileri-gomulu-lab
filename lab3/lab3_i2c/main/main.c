/*
 * Ileri Gomulu Sistemler - Laboratuvar 3, Gorev 2: I2C ve MPU-6050 (GY-521)
 * Baglanti: VCC -> 3V3, GND -> GND, SDA -> GPIO8, SCL -> GPIO9 (AD0 ve INT bos)
 *
 * 1) veri yolu taramasi (ACK / NACK)
 * 2) WHO_AM_I okuma suresi 100 kHz ve 400 kHz'te, kuramsal sureyle karsilastirma
 * 3) sensoru uyandirma ve ivmeyi saniyede 10 kez yazdirma
 *
 * Lojik analizor yok: ACK/NACK sonucu ve islem suresi konsoldan okunur.
 */
#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"

#define SDA_PIN         8
#define SCL_PIN         9
#define MPU_ADDR        0x68
#define REG_WHO_AM_I    0x75
#define REG_PWR_MGMT_1  0x6B
#define REG_ACCEL_XOUT  0x3B

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t mpu;

static void mpu_ekle(uint32_t hiz)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = MPU_ADDR,
        .scl_speed_hz    = hiz,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &mpu));
}

static esp_err_t reg_oku(uint8_t reg, uint8_t *buf, size_t n)
{
    /* yazma (register adresi) + tekrarli START + okuma */
    return i2c_master_transmit_receive(mpu, &reg, 1, buf, n, 100);
}

static void tarama(void)
{
    printf("1) I2C tarama (0x08-0x77), 100 kHz:\n");
    int bulunan = 0;
    for (uint16_t a = 0x08; a < 0x78; a++) {
        if (i2c_master_probe(bus, a, 50) == ESP_OK) {
            printf("   ACK geldi: 0x%02X\n", a);
            bulunan++;
        }
    }
    printf("   toplam cihaz: %d\n", bulunan);
    printf("   0x69 adresi: %s\n",
           i2c_master_probe(bus, 0x69, 50) == ESP_OK ? "ACK" : "NACK (cihaz yok)");
}

static void sure_olc(uint32_t hiz)
{
    mpu_ekle(hiz);
    uint8_t who = 0;
    const int N = 200;
    int64_t t0 = esp_timer_get_time();
    for (int i = 0; i < N; i++) reg_oku(REG_WHO_AM_I, &who, 1);
    int64_t dt = esp_timer_get_time() - t0;
    /* adres+W (9) + register (9) + adres+R (9) + veri (9) = 36 bit; START/STOP haric */
    long long kuram_us = 36LL * 1000000 / hiz;
    printf("   %u Hz: WHO_AM_I=0x%02X, olculen=%lld us/islem, kuramsal (36 bit)=%lld us\n",
           (unsigned)hiz, who, (long long)(dt / N), kuram_us);
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(mpu));
}

static void ivme_oku(int adet)
{
    mpu_ekle(100000);
    uint8_t uyan[2] = {REG_PWR_MGMT_1, 0x00};              /* uyku modundan cik */
    ESP_ERROR_CHECK(i2c_master_transmit(mpu, uyan, 2, 100));
    vTaskDelay(pdMS_TO_TICKS(100));

    printf("3) ivme (+-2 g, 16384 LSB/g), 10 Hz:\n");
    TickType_t last = xTaskGetTickCount();
    for (int i = 0; i < adet; i++) {
        uint8_t b[6];
        if (reg_oku(REG_ACCEL_XOUT, b, 6) != ESP_OK) {
            printf("   okuma hatasi\n");
            break;
        }
        int16_t ax = (int16_t)((b[0] << 8) | b[1]);
        int16_t ay = (int16_t)((b[2] << 8) | b[3]);
        int16_t az = (int16_t)((b[4] << 8) | b[5]);
        printf("   %3d: ham=(%6d,%6d,%6d)  mg=(%+6d,%+6d,%+6d)\n", i,
               ax, ay, az, ax * 1000 / 16384, ay * 1000 / 16384, az * 1000 / 16384);
        xTaskDelayUntil(&last, pdMS_TO_TICKS(100));
    }
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(mpu));
}

void app_main(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = -1,                       /* otomatik sec */
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, /* modulde 4.7k pull-up zaten var */
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));
    esp_log_level_set("i2c.master", ESP_LOG_NONE);   /* tarama sirasindaki NACK hata satirlarini gizler */
    vTaskDelay(pdMS_TO_TICKS(500));

    for (int tur = 1; ; tur++) {
        printf("\n===== I2C testi, tur %d =====\n", tur);
        tarama();
        printf("2) WHO_AM_I okuma suresi:\n");
        sure_olc(100000);
        sure_olc(400000);
        ivme_oku(30);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
