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

#include <pigpio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define SPI_CHANNEL     (0)
#define SPI_BAUD        (1000000)
#define SPI_FLAG_MM     (0b0000000000000000000000)
#define SPI_FLAG_PX     (0b0000000000000000000000)
#define SPI_FLAG_UX     (0b0000000000000000000000)
#define SPI_FLAG_A      (0b0000000000000000000000)
#define SPI_FLAG_W      (0b0000000000000000000000)
#define SPI_FLAG_NNNN   (0b0000000000000000000000)
#define SPI_FLAG_T      (0b0000000000000000000000)
#define SPI_FLAG_R      (0b0000000000000000000000)
#define SPI_FLAG_BBBBBB (0b0000000000000000000000)
#define SPI_FLAGS       (SPI_FLAG_MM | SPI_FLAG_PX | SPI_FLAG_UX | SPI_FLAG_A | SPI_FLAG_W | SPI_FLAG_NNNN | SPI_FLAG_T | SPI_FLAG_R | SPI_FLAG_BBBBBB)

#define PWM_GPIO_PIN_X  (3)
#define PWM_GPIO_PIN_Y  (5)

#define JOYSTICK_MAX    (4095)
#define JOYSTICK_THRES  (50) /* modify if needed */

/*
 * struct joystick_t
 *
 * (x,y) tuple representing horizontal and vertical axis of joystick
 */
struct joystick_t {
    uint32_t x;
    uint32_t y;
};

/*
 * read_joystick_x
 *
 * Returns the horizontal value of the joystick in the range [0, JOYSTICK_MAX]
 */
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

/*
 * read_joystick_y
 *
 * Returns the vertical value of the joystick in the range [0, JOYSTICK_MAX]
 */
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

/*
 * read_joystick
 *
 * Returns an (x,y) tuple containing the current value of the joystick. The
 * returned value is only modified if the joystick has moved by more than
 * JOYSTICK_THRES in each axis.
 */
struct joystick_t read_joystick(int fd) {
    static struct joystick_t saved = {JOYSTICK_MAX / 2, JOYSTICK_MAX / 2};

    uint32_t x_new = read_joystick_x(fd);
    uint32_t y_new = read_joystick_y(fd);

    saved.x = (abs(x_new - saved.x) > JOYSTICK_THRES) ? x_new : saved.x;
    saved.y = (abs(x_new - saved.y) > JOYSTICK_THRES) ? y_new : saved.y;

    return saved;
}

/*
 * initialize
 */
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

/*
 * finalize
 */
void finalize(int fd) {
    if (spiClose(fd) != 0) {
        printf("Error: spiClose() failed\n");
        exit(EXIT_FAILURE);
    }

    gpioTerminate();
}

int main(int argc, char **argv) {
    int fd_spi = initialize();

    int retval = 0;

    // retval = gpioPWM(PWM_GPIO_PIN_X, 32);
    // if (retval != 0) {
    //     printf("Error: gpioPWM() failed\n");
    //     exit(EXIT_FAILURE);
    // }

    // retval = gpioPWM(PWM_GPIO_PIN_Y, 64);
    // if (retval != 0) {
    //     printf("Error: gpioPWM() failed\n");
    //     exit(EXIT_FAILURE);
    // }

    // while (true);

    /* perform SPI communication */
    while (true) {
        struct joystick_t joystick = read_joystick(fd_spi);
        printf("x = %04" PRIu32 ", y = %04" PRIu32 "\r", joystick.x, joystick.y);
        fflush(stdout);

        usleep(100000);
    }

    finalize(fd_spi);

    return EXIT_SUCCESS;
}
