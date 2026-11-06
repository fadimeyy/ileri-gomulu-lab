#include <gpiod.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define N 1000
#define M 100

static double now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec * 1e-3;
}

int main(void)
{
    unsigned int offset = 17;
    static double d[N];

    struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) { perror("chip"); return 1; }
    struct gpiod_line_settings *ls = gpiod_line_settings_new();
    struct gpiod_line_config *lc = gpiod_line_config_new();
    struct gpiod_request_config *rc = gpiod_request_config_new();
    gpiod_line_settings_set_direction(ls, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_config_add_line_settings(lc, &offset, 1, ls);
    gpiod_request_config_set_consumer(rc, "jitter");
    struct gpiod_line_request *req = gpiod_chip_request_lines(chip, rc, lc);
    if (!req) { perror("request"); return 1; }

    for (int i = 0; i < N; i++) {
        double t0 = now_us();
        for (int k = 0; k < M; k++) {
            gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_ACTIVE);
            gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_INACTIVE);
        }
        d[i] = now_us() - t0;
    }

    double mn = d[0], mx = d[0], s = 0, s2 = 0;
    for (int i = 0; i < N; i++) {
        if (d[i] < mn) mn = d[i];
        if (d[i] > mx) mx = d[i];
        s += d[i];
        s2 += d[i] * d[i];
    }
    double m = s / N, sd = sqrt(s2 / N - m * m);
    printf("blok=%d periyot: min=%.1f maks=%.1f ort=%.1f sd=%.2f us\n", M, mn, mx, m, sd);

    FILE *f = fopen("pi_bloklar.csv", "w");
    for (int i = 0; i < N; i++) fprintf(f, "%.2f\n", d[i]);
    fclose(f);
    return 0;
}
