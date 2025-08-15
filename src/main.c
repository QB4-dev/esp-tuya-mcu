#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>

#include "tuya-mcu/tuya-mcu.h"

#define SERIAL_PORT "/dev/ttyUSB0" // Change as needed

static tuya_mcu_t mcu;

// Open and configure serial port
int open_serial(const char *port)
{
    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        exit(1);
    }
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag = IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        exit(1);
    }
    return fd;
}

int tuya_mcu_uart_rx(void *ctx, uint8_t *c)
{
    int fd = *(int *)ctx;
    if (fd < 0)
        return -1;
    return read(fd, c, 1);
}
int tuya_mcu_uart_tx(void *ctx, uint8_t c)
{
    int fd = *(int *)ctx;
    if (fd < 0)
        return -1;
    return write(fd, &c, 1);
}
uint32_t tuya_mcu_get_tick(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int on_state_changed(tuya_mcu_t mcu, enum tuya_mcu_state st, void *arg)
{
    if (!mcu) {
        return -1;
    }
    switch (st) {
    case TUYA_MCU_INIT_HEARTBEAT:
        printf("Device initialized and heartbeat sent.\n");
        break;
    case TUYA_MCU_QUERY_INFO:
        printf("Device info queried.\n");
        break;
    case TUYA_MCU_INITIALIZED:
        printf("Device initialized: ID=%s, ver=%s\n", tuya_mcu_get_product_id(mcu),
               tuya_mcu_get_version(mcu));
        break;
    default:
        printf("Unknown state: %d\n", st);
        break;
    }
    return 0;
}

int on_config_request(tuya_mcu_t mcu, void *arg)
{
    if (!mcu) {
        return -1;
    }
    printf("Received config request: %s\n", (char *)arg);
    return 0;
}

int on_dp_received(tuya_mcu_t mcu, tuya_dp_t *dp, void *arg)
{
    if (!mcu || !dp) {
        return -1;
    }
    tuya_dp_print(dp);
    return 0;
}

int main()
{
    int fd = open_serial(SERIAL_PORT);
    if (fd < 0) {
        fprintf(stderr, "Failed to open serial port %s\n", SERIAL_PORT);
        return 1;
    }

    if (tuya_mcu_init(&mcu, &fd) != 0) {
        fprintf(stderr, "Failed to initialize Tuya MCU\n");
        return 1;
    }
    tuya_mcu_set_state_handler(mcu, on_state_changed, NULL);
    tuya_mcu_set_config_handler(mcu, on_config_request, "AP");
    tuya_mcu_set_dp_handler(mcu, on_dp_received, NULL);

    printf("TUYA Serial Sniffer started on %s\n", SERIAL_PORT);

    while (1) {
        if (tuya_mcu_tick(mcu) < 0) {
            fprintf(stderr, "Error in tuya_mcu_tick\n");
            break;
        }
    }

    close(fd);
    return 0;
}