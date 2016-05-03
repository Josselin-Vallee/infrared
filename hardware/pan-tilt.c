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

#define SPI_CHANNEL           (0)
#define SPI_BAUD              (1000000)
#define SPI_FLAG_MM           (0b0000000000000000000000)
#define SPI_FLAG_PX           (0b0000000000000000000000)
#define SPI_FLAG_UX           (0b0000000000000000000000)
#define SPI_FLAG_A            (0b0000000000000000000000)
#define SPI_FLAG_W            (0b0000000000000000000000)
#define SPI_FLAG_NNNN         (0b0000000000000000000000)
#define SPI_FLAG_T            (0b0000000000000000000000)
#define SPI_FLAG_R            (0b0000000000000000000000)
#define SPI_FLAG_BBBBBB       (0b0000000000000000000000)
#define SPI_FLAGS             (SPI_FLAG_MM | SPI_FLAG_PX | SPI_FLAG_UX | SPI_FLAG_A | SPI_FLAG_W | SPI_FLAG_NNNN | SPI_FLAG_T | SPI_FLAG_R | SPI_FLAG_BBBBBB)

#define JOYSTICK_MIN          (0)
#define JOYSTICK_MAX          (4095)
#define JOYSTICK_THRES        (5) /* modify if needed */

#define PWM_GPIO_PIN_X        (2)
#define PWM_GPIO_PIN_Y        (3)
/* servo motors typically expect to be updated every 20 ms with a pulse
   between 1 ms and 2 ms */
#define PWM_GPIO_FREQ_HZ      (50)
#define PWM_GPIO_RANGE_US     (1000000 / PWM_GPIO_FREQ_HZ)
#define PWM_PULSEWIDTH_MIN_US (1000)
#define PWM_PULSEWIDTH_MAX_US (2000)
#define PWM_PULSE_WIDTH(x)    ((((PWM_PULSEWIDTH_MAX_US - PWM_PULSEWIDTH_MIN_US) * ((x) - JOYSTICK_MIN)) / (JOYSTICK_MAX - JOYSTICK_MIN)) + PWM_PULSEWIDTH_MIN_US)

#define USLEEP_DELAY          (100000)

/*
 * struct joystick_t
 *
 * (x,y) tuple representing horizontal and vertical axis of joystick
 */
struct joystick_t {
    uint32_t x;
    uint32_t y;
};

/* global variables */
int fd_spi = 0;

/*
 * read_joystick_x
 *
 * Returns the horizontal value of the joystick in the range [0, JOYSTICK_MAX]
 */
uint32_t read_joystick_x() {
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

    spiXfer(fd_spi, txbuf, rxbuf, 3);

    return ((rxbuf[1] & 0xf) << 8) + rxbuf[2];
}

/*
 * read_joystick_y
 *
 * Returns the vertical value of the joystick in the range [0, JOYSTICK_MAX]
 */
uint32_t read_joystick_y() {
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

    spiXfer(fd_spi, txbuf, rxbuf, 3);

    return ((rxbuf[1] & 0xf) << 8) + rxbuf[2];
}

/*
 * read_joystick
 *
 * Returns an (x,y) tuple containing the current value of the joystick. The
 * returned value is only modified if the joystick has moved by more than
 * JOYSTICK_THRES in each axis.
 */
struct joystick_t read_joystick() {
    static struct joystick_t saved = {JOYSTICK_MAX / 2, JOYSTICK_MAX / 2};

    uint32_t x_new = read_joystick_x();
    uint32_t y_new = read_joystick_y();

    saved.x = (abs(x_new - saved.x) > JOYSTICK_THRES) ? x_new : saved.x;
    saved.y = (abs(x_new - saved.y) > JOYSTICK_THRES) ? y_new : saved.y;

    return saved;
}

void move_pan_tilt(struct joystick_t joystick) {
    uint32_t pulsewidth_x = PWM_PULSE_WIDTH(joystick.x);
    uint32_t pulsewidth_y = PWM_PULSE_WIDTH(joystick.y);

    if (gpioServo(PWM_GPIO_PIN_X, pulsewidth_x) != 0) {
        printf("Error: gpioServo() failed for PWM_GPIO_PIN_X\n");
        exit(EXIT_FAILURE);
    }

    if (gpioServo(PWM_GPIO_PIN_Y, pulsewidth_y) != 0) {
        printf("Error: gpioServo() failed for PWM_GPIO_PIN_Y\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * initialize
 */
void initialize() {
    if (gpioInitialise() < 0) {
        printf("Error: gpioInitialise() failed\n");
        exit(EXIT_FAILURE);
    }

    fd_spi = spiOpen(SPI_CHANNEL, SPI_BAUD, SPI_FLAGS);
    if (fd_spi < 0) {
        printf("Error: spiOpen() failed\n");
        exit(EXIT_FAILURE);
    }

    if (gpioSetPWMfrequency(PWM_GPIO_PIN_X, PWM_GPIO_FREQ_HZ) == PI_BAD_USER_GPIO) {
        printf("Error: gpioSetPWMfrequency() failed for PWM_GPIO_PIN_X\n");
        exit(EXIT_FAILURE);
    }

    if (gpioSetPWMfrequency(PWM_GPIO_PIN_Y, PWM_GPIO_FREQ_HZ) == PI_BAD_USER_GPIO) {
        printf("Error: gpioSetPWMfrequency() failed for PWM_GPIO_PIN_Y\n");
        exit(EXIT_FAILURE);
    }

    int retval = 0;
    retval = gpioSetPWMrange(PWM_GPIO_PIN_X, PWM_GPIO_RANGE_US);
    if ((retval == PI_BAD_USER_GPIO) || (retval == PI_BAD_DUTYRANGE)) {
        printf("Error: gpioSetPWMrange() failed for PWM_GPIO_PIN_X\n");
        exit(EXIT_FAILURE);
    }

    retval = gpioSetPWMrange(PWM_GPIO_PIN_Y, PWM_GPIO_RANGE_US);
    if ((retval == PI_BAD_USER_GPIO) || (retval == PI_BAD_DUTYRANGE)) {
        printf("Error: gpioSetPWMrange() failed for PWM_GPIO_PIN_Y\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * finalize
 */
void finalize() {
    if (spiClose(fd_spi) != 0) {
        printf("Error: spiClose() failed\n");
        exit(EXIT_FAILURE);
    }

    gpioTerminate();
}

int main(int argc, char **argv) {
    initialize();

    while (true) {
        move_pan_tilt(read_joystick());
        usleep(USLEEP_DELAY);
    }

    finalize();

    return EXIT_SUCCESS;
}
