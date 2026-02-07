#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned long long get_rx_bytes() {
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return 0;

    char line[512];
    unsigned long long rx = 0, total = 0;

    /* Skip headers */
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "lo:"))
            continue;

        sscanf(line, "%*[^:]: %llu", &rx);
        total += rx;
    }

    fclose(fp);
    return total;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <minimum_MBps>\n", argv[0]);
        return 1;
    }

    float min_speed = atof(argv[1]);
    if (min_speed <= 0.0f) {
        printf("Minimum speed must be greater than 0\n");
        return 1;
    }

    unsigned long long rx_prev = get_rx_bytes();
    unsigned long long rx_curr;
    float speed_MBps;

    int low_count = 0;
    const int LOW_LIMIT_SECONDS = 60;

    while (1) {
        sleep(1);

        rx_curr = get_rx_bytes();

        if (rx_curr < rx_prev) {
            rx_prev = rx_curr;
            continue;
        }

        speed_MBps = (rx_curr - rx_prev) / (1024.0f * 1024.0f);
        rx_prev = rx_curr;

        printf("\rCurrent download speed: %.2f MB/s | Minimum: %.2f MB/s",speed_MBps, min_speed);
        fflush(stdout);

        if (speed_MBps < min_speed) {
            low_count++;
        } else {
            low_count = 0;
        }

        if (low_count >= LOW_LIMIT_SECONDS) {
            printf("\nDownload finished. Suspending system.\n");
            fflush(stdout);
            sleep(2);
            system("systemctl suspend"); //"systemctl poweroff" to shutdown fully
            break;
        }
    }

    return 0;
}
