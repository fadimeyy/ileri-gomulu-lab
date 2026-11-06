/*
 * İleri Gömülü Sistemler - Laboratuvar 1 (Raspberry Pi 4)
 * libgpiod v2 API ile GPIO17'ye bağlı LED'in sürülmesi ve
 * kullanıcı alanından en yüksek pin değiştirme frekansının tahmini.
 *
 * Derleme (Pi üzerinde):  gcc -O2 -Wall blink.c -lgpiod -o blink
 */
#include <gpiod.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define CHIP_PATH     "/dev/gpiochip0"   /* Pi 4: başlık pinleri gpiochip0 */
#define LED_OFFSET    17u                /* BCM numarası, fiziksel pin 11  */
#define TOGGLE_COUNT  100000

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(void)
{
    unsigned int offset = LED_OFFSET;
    int ret = 1;

    struct gpiod_chip *chip = gpiod_chip_open(CHIP_PATH);
    if (!chip) { perror("gpiod_chip_open"); return 1; }

    struct gpiod_line_settings *ls = gpiod_line_settings_new();
    struct gpiod_line_config   *lc = gpiod_line_config_new();
    struct gpiod_request_config *rc = gpiod_request_config_new();
    struct gpiod_line_request  *req = NULL;
    if (!ls || !lc || !rc) { fprintf(stderr, "bellek hatası\n"); goto out; }

    gpiod_line_settings_set_direction(ls, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(ls, GPIOD_LINE_VALUE_INACTIVE);
    gpiod_line_config_add_line_settings(lc, &offset, 1, ls);
    gpiod_request_config_set_consumer(rc, "lab1");

    req = gpiod_chip_request_lines(chip, rc, lc);
    if (!req) { perror("gpiod_chip_request_lines"); goto out; }

    /* Bölüm A: 1 Hz yanıp sönme */
    printf("Bölüm A: 1 Hz yanıp sönme (5 kez)\n");
    for (int i = 0; i < 5; i++) {
        gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_ACTIVE);
        usleep(500000);
        gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_INACTIVE);
        usleep(500000);
    }

    /* Bölüm B: en yüksek hızda değiştirme */
    printf("Bölüm B: en yüksek hızda değiştirme (Ctrl+C ile çıkış)\n");
    for (;;) {
        double t0 = now_s();
        for (int i = 0; i < TOGGLE_COUNT; i++) {
            gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_ACTIVE);
            gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_INACTIVE);
        }
        double dt = now_s() - t0;
        printf("%d periyot, %.3f s, ortalama f = %.1f kHz\n",
               TOGGLE_COUNT, dt, TOGGLE_COUNT / dt / 1000.0);
        usleep(100000);
    }
    ret = 0;

out:
    if (req) gpiod_line_request_release(req);
    if (rc)  gpiod_request_config_free(rc);
    if (lc)  gpiod_line_config_free(lc);
    if (ls)  gpiod_line_settings_free(ls);
    gpiod_chip_close(chip);
    return ret;
}
