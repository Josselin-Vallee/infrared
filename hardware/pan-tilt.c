/*
 * pan_tilt.c
 *
 * Controls the pan-tilt module for the NIR imaging project for the
 * computational photography course.
 *
 * Compile with:
 *     gcc -std=gnu11 -o pan-tilt pan-tilt.c -lbcm2835;
 *
 * Be sure to run as root!
 *
 * Author: Sahand Kashani-Akhavan [sahand.kashani-akhavan@epfl.ch]
 */

#include <bcm2835.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

uint32_t read_vertical() {
    /* read channel 0
       ==============
       tbuf[0] = 0b  0,  0,  0,  0,  0,  1,  1,  0
       tbuf[1] = 0b  0,  0,  0,  0,  0,  0,  0,  0
       tbuf[2] = 0b  0,  0,  0,  0,  0,  0,  0,  0
       rbuf[0] = 0b  x,  x,  x,  x,  x,  x,  x,  x
       rbuf[1] = 0b  x,  x,  x,  0,V11,V10, V9, V8
       rbuf[2] = 0b V7, V6, V5, V4, V3, V2, V1, V0 */

    /* transmit and receive buffers */
    char tbuf[3];
    char rbuf[3];

    tbuf[0] = 0b00000110;
    tbuf[1] = 0b00000000;
    tbuf[2] = 0b00000000;

    bcm2835_spi_transfernb(tbuf, rbuf, 3);

    return ((rbuf[1] & 0xf) << 8) + rbuf[2];
}

uint32_t read_horizontal() {
    /* read channel 1
       ==============
       tbuf[0] = 0b  0,  0,  0,  0,  0,  1,  1,  0
       tbuf[1] = 0b  0,  1,  0,  0,  0,  0,  0,  0
       tbuf[2] = 0b  0,  0,  0,  0,  0,  0,  0,  0

       rbuf[0] = 0b  x,  x,  x,  x,  x,  x,  x,  x
       rbuf[1] = 0b  x,  x,  x,  0,H11,H10, H9, H8
       rbuf[2] = 0b H7, H6, H5, H4, H3, H2, H1, H0 */

    /* transmit and receive buffers */
    char tbuf[3];
    char rbuf[3];

    tbuf[0] = 0b00000110;
    tbuf[1] = 0b01000000;
    tbuf[2] = 0b00000000;

    bcm2835_spi_transfernb(tbuf, rbuf, 3);

    return ((rbuf[1] & 0xf) << 8) + rbuf[2];
}

int main(int argc, char **argv) {
    if (!bcm2835_init()) {
        printf("bcm2835_init failed. Be sure to run as root.\n");
        return EXIT_FAILURE;
    }

    /* setup SPI controller */
    if (!bcm2835_spi_begin()) {
        printf("bcm2835_spi_begin failed. Be sure to run as root.\n");
        return EXIT_FAILURE;
    }

    /* 1 MHz SPI communication --> clock divider = 256 = 1.024us = 976.5625kHz */
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    /* perform SPI communication */
    while (true) {
        /* Control bit selections:
           SGL/~DIFF = 1 (Single mode)
           D2, D1, D0 = {0, 0, 0/1} (only 2 channels since 1 joystick) */

        /* command format:
           tbuf[0] = 0b  0, 0, 0, 0, 0, START,SGL,D2
           tbuf[1] = 0b D1,D0, x, x, x,     x,  x, x
           tbuf[2] = 0b  x, x, x, x, x,     x,  x, x */

        uint32_t hx = read_horizontal();
        uint32_t vx = read_vertical();

        printf("hx = %04" PRIu32 ", vx = %04" PRIu32 "\r", hx, vx);
        fflush(stdout);

        usleep(250000);
    }

    bcm2835_spi_end();
    bcm2835_close();

    return EXIT_SUCCESS;
}
