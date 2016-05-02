/*
 * pan_tilt.c
 *
 * Controls the pan-tilt module for the NIR imaging project for the
 * computational photography course.
 *
 * Compile with:
 *     gcc -std=gnu11 -Wall pan-tilt.c -o pan-tilt -pthread -lpigpio -lrt
 *
 * Run with:
 *     export LD_LIBRARY_PATH="/home/alarm/PIGPIO"
 *     ./pan-tilt
 *
 * Be sure to run as root!
 *
 * Author: Sahand Kashani-Akhavan [sahand.kashani-akhavan@epfl.ch]
 */

#include <assert.h>
#include <pigpio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define SPI_CHANNEL     (0)
#define SPI_BAUD        (1000000)
#define SPI_FLAG_mm     (0b0000000000000000000000)
#define SPI_FLAG_px     (0b0000000000000000000000)
#define SPI_FLAG_ux     (0b0000000000000000000000)
#define SPI_FLAG_A      (0b0000000000000000000000)
#define SPI_FLAG_W      (0b0000000000000000000000)
#define SPI_FLAG_nnnn   (0b0000000000000000000000)
#define SPI_FLAG_T      (0b0000000000000000000000)
#define SPI_FLAG_R      (0b0000000000000000000000)
#define SPI_FLAG_bbbbbb (0b0000000000000000000000)
#define SPI_FLAGS       (SPI_FLAG_mm | SPI_FLAG_px | SPI_FLAG_ux | SPI_FLAG_A | SPI_FLAG_W | SPI_FLAG_nnnn | SPI_FLAG_T | SPI_FLAG_R | SPI_FLAG_bbbbbb)

uint32_t read_joystick_y(int fd) {
    /* read channel 0
       ==============
       txbuf[0] = 0b  0,  0,  0,  0,  0,  1,  1,  0
       txbuf[1] = 0b  0,  0,  0,  0,  0,  0,  0,  0
       txbuf[2] = 0b  0,  0,  0,  0,  0,  0,  0,  0

       rxbuf[0] = 0b  x,  x,  x,  x,  x,  x,  x,  x
       rxbuf[1] = 0b  x,  x,  x,  0,V11,V10, V9, V8
       rxbuf[2] = 0b V7, V6, V5, V4, V3, V2, V1, V0 */

    /* transmit and receive buffers */
    char txbuf[3];
    char rxbuf[3];

    txbuf[0] = 0b00000110;
    txbuf[1] = 0b00000000;
    txbuf[2] = 0b00000000;

    spiXfer(fd, txbuf, rxbuf, 3);

    return ((rxbuf[1] & 0xf) << 8) + rxbuf[2];
}

uint32_t read_joystick_x(int fd) {
    /* read channel 1
       ==============
       txbuf[0] = 0b  0,  0,  0,  0,  0,  1,  1,  0
       txbuf[1] = 0b  0,  1,  0,  0,  0,  0,  0,  0
       txbuf[2] = 0b  0,  0,  0,  0,  0,  0,  0,  0

       rxbuf[0] = 0b  x,  x,  x,  x,  x,  x,  x,  x
       rxbuf[1] = 0b  x,  x,  x,  0,H11,H10, H9, H8
       rxbuf[2] = 0b H7, H6, H5, H4, H3, H2, H1, H0 */

    /* transmit and receive buffers */
    char txbuf[3];
    char rxbuf[3];

    txbuf[0] = 0b00000110;
    txbuf[1] = 0b01000000;
    txbuf[2] = 0b00000000;

    spiXfer(fd, txbuf, rxbuf, 3);

    return ((rxbuf[1] & 0xf) << 8) + rxbuf[2];
}

int initialize() {
    if (gpioInitialise() < 0) {
        printf("Error: gpioInitialise() failed\n");
        exit(EXIT_FAILURE);
    }

    int fd = spiOpen(SPI_CHANNEL, SPI_BAUD, SPI_FLAGS);
    if (fd < 0) {
        printf("Error: spiOpen() failed\n");
        exit(EXIT_FAILURE);
    }

    return fd;
}

void finalize(int fd) {
    if (spiClose(fd) != 0) {
        printf("Error: spiClose() failed\n");
        exit(EXIT_FAILURE);
    }

    gpioTerminate();
}

int main(int argc, char **argv) {
    int fd_spi = initialize();

    uint32_t hx = 0;
    uint32_t vx = 0;
    uint32_t threshold = 50;

    /* perform SPI communication */
    while (true) {
        /* Control bit selections:
           SGL/~DIFF = 1 (Single mode)
           D2, D1, D0 = {0, 0, 0/1} (only 2 channels since 1 joystick) */

        /* command format:
           tbuf[0] = 0b  0, 0, 0, 0, 0, START,SGL,D2
           tbuf[1] = 0b D1,D0, x, x, x,     x,  x, x
           tbuf[2] = 0b  x, x, x, x, x,     x,  x, x */

        uint32_t hx_new = read_joystick_x(fd_spi);
        uint32_t vx_new = read_joystick_y(fd_spi);

        hx = (abs(hx_new - hx) > threshold) ? hx_new : hx;
        vx = (abs(vx_new - vx) > threshold) ? vx_new : vx;

        printf("hx = %04" PRIu32 ", vx = %04" PRIu32 "\r", hx, vx);
        fflush(stdout);

        usleep(100000);
    }

    finalize(fd_spi);

    return EXIT_SUCCESS;
}
