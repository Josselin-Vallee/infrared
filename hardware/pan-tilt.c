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

#include <fcntl.h>
#include <inttypes.h>
#include <pigpio.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SPI_CHANNEL              (0)
#define SPI_BAUD                 (1000000)
#define SPI_FLAG_MM              (0b0000000000000000000000)
#define SPI_FLAG_PX              (0b0000000000000000000000)
#define SPI_FLAG_UX              (0b0000000000000000000000)
#define SPI_FLAG_A               (0b0000000000000000000000)
#define SPI_FLAG_W               (0b0000000000000000000000)
#define SPI_FLAG_NNNN            (0b0000000000000000000000)
#define SPI_FLAG_T               (0b0000000000000000000000)
#define SPI_FLAG_R               (0b0000000000000000000000)
#define SPI_FLAG_BBBBBB          (0b0000000000000000000000)
#define SPI_FLAGS                (SPI_FLAG_MM | SPI_FLAG_PX | SPI_FLAG_UX | SPI_FLAG_A | SPI_FLAG_W | SPI_FLAG_NNNN | SPI_FLAG_T | SPI_FLAG_R | SPI_FLAG_BBBBBB)

#define JOYSTICK_MIN             (0)
#define JOYSTICK_MAX             (4095)
#define JOYSTICK_MIDDLE          ((JOYSTICK_MIN + JOYSTICK_MAX) / 2)
#define JOYSTICK_THRES           (5)
#define JOYSTICK_DEC_THRES       ((3 * JOYSTICK_MAX) / 8)
#define JOYSTICK_INC_THRES       ((5 * JOYSTICK_MAX) / 8)
#define JOYSTICK_INIT_X          (JOYSTICK_MIDDLE)
#define JOYSTICK_INIT_Y          (JOYSTICK_MIDDLE)
#define JOYSTICK_BUTTON_GPIO_PIN (25)

#define PWM_GPIO_PIN_X           (3)
#define PWM_GPIO_PIN_Y           (2)
/* servo motors typically expect to be updated every 20 ms (50 Hz) with a pulse
   between 1 ms and 2 ms */
#define PWM_GPIO_FREQ_HZ         (50)
#define PWM_GPIO_RANGE_US        (1000000 / PWM_GPIO_FREQ_HZ)
#define PWM_PULSEWIDTH_MIN_US    (1250) /* hardware minimum is 1000 us */
#define PWM_PULSEWIDTH_MAX_US    (1750) /* hardware maximum is 2000 us */
#define PWM_PULSEWIDTH_MIDDLE_US ((PWM_PULSEWIDTH_MIN_US + PWM_PULSEWIDTH_MAX_US) / 2)
#define PWM_PULSEWIDTH_INIT_US   (PWM_PULSEWIDTH_MIDDLE_US)
#define PWM_PULSEWIDTH_STEP_US   (10)

#define USLEEP_DELAY             (1 * PWM_GPIO_RANGE_US) /* MUST be a multiple of PWM_GPIO_RANGE_US to avoid modifying the servo when it isn't expecting it */

#define FIFO_NAME                "/tmp/nir_project_fifo"

/* 1 byte of data is sent over the fifo*/
#define OP_SKIN_SMOOTHING        ((uint8_t) 0)
#define OP_SHADOW_DETECTION      ((uint8_t) 1)

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
int fd_fifo = 0;
volatile bool joystick_button_pressed = false;
volatile bool joystick_button_pressed_handling = false;

uint32_t read_joystick_x();
uint32_t read_joystick_y();
struct joystick_t read_joystick();
bool is_joystick_left(struct joystick_t joystick);
bool is_joystick_right(struct joystick_t joystick);
bool is_joystick_up(struct joystick_t joystick);
bool is_joystick_down(struct joystick_t joystick);
bool is_joystick_centered(struct joystick_t joystick);
bool is_joystick_full_left(struct joystick_t joystick);
bool is_joystick_full_right(struct joystick_t joystick);
bool is_joystick_full_up(struct joystick_t joystick);
bool is_joystick_full_down(struct joystick_t joystick);
uint32_t move_pan_tilt_left(uint32_t pulsewidth_x_us);
uint32_t move_pan_tilt_right(uint32_t pulsewidth_x_us);
uint32_t move_pan_tilt_up(uint32_t pulsewidth_y_us);
uint32_t move_pan_tilt_down(uint32_t pulsewidth_y_us);
void move_pan_tilt();
void setup_joystick();
void setup_pwm();
void setup_fifo();
void initialize_pigpio();
void int_handler(int signum);
void joystick_button_isr(int gpio, int level, uint32_t tick);
uint32_t button_press_operation();
void handle_button_press();

/*
 * read_joystick_x
 *
 * Returns the horizontal value of the joystick in the range [0, JOYSTICK_MAX],
 * as reported by the ADC-converted value obtained from the joystick's VRx pin.
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
 * Returns the vertical value of the joystick in the range [0, JOYSTICK_MAX],
 * as reported by the ADC-converted value obtained from the joystick's VRy pin.
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
 * Returns an (x,y) tuple containing the current value of the joystick as wanted
 * by the user (i.e. rotated by 90° counter-clockwise). The returned value is
 * only modified if the joystick has moved by more than JOYSTICK_THRES in each
 * axis.
 */
struct joystick_t read_joystick() {
    static uint32_t saved_x = JOYSTICK_INIT_X;
    static uint32_t saved_y = JOYSTICK_INIT_Y;

    uint32_t x_new = read_joystick_x();
    uint32_t y_new = read_joystick_y();

    /* stabilize joystick reading */
    saved_x = (abs(x_new - saved_x) > JOYSTICK_THRES) ? x_new : saved_x;
    saved_y = (abs(y_new - saved_y) > JOYSTICK_THRES) ? y_new : saved_y;

    /*
     * joystick is rotated 90° counter-clockwise, so need to remap x and y:
     *
     * |==========|==========|
     * |  ACTUAL  |   WANT   |
     * |==========|==========|
     * |    x-    |    y+    |
     * | y- oo y+ | x- oo x+ |
     * |    x+    |    y-    |
     * |==========|==========|
     */
    struct joystick_t res;
    res.x = saved_y;
    res.y = JOYSTICK_MAX - saved_x;
    return res;
}

/*
 * is_joystick_left
 *
 * Returns true if the joystick is pointing left, and false otherwise. Note that
 * it is possible for the joystick to also be pointing up or down!
 */
bool is_joystick_left(struct joystick_t joystick) {
    if (joystick.x < JOYSTICK_DEC_THRES) {
        return true;
    }
    return false;
}

/*
 * is_joystick_right
 *
 * Returns true if the joystick is pointing right, and false otherwise. Note that
 * it is possible for the joystick to also be pointing up or down!
 */
bool is_joystick_right(struct joystick_t joystick) {
    if (JOYSTICK_INC_THRES < joystick.x) {
        return true;
    }
    return false;
}

/*
 * is_joystick_up
 *
 * Returns true if the joystick is pointing up, and false otherwise. Note that
 * it is possible for the joystick to also be pointing left or right!
 */
bool is_joystick_up(struct joystick_t joystick) {
    if (JOYSTICK_INC_THRES < joystick.y) {
        return true;
    }
    return false;
}

/*
 * is_joystick_down
 *
 * Returns true if the joystick is pointing down, and false otherwise. Note that
 * it is possible for the joystick to also be pointing left or right!
 */
bool is_joystick_down(struct joystick_t joystick) {
    if (joystick.y < JOYSTICK_DEC_THRES) {
        return true;
    }
    return false;
}

/*
 * is_joystick_centered
 *
 * Returns true if the joystick is centered, and false otherwise.
 */
bool is_joystick_centered(struct joystick_t joystick) {
    return !is_joystick_left(joystick) && !is_joystick_right(joystick) &&
           !is_joystick_up(joystick)   && !is_joystick_down(joystick);
}

/*
 * is_joystick_full_left
 *
 * Returns true if the joystick is fully left (i.e not pointing up or down
 * either), and false otherwise.
 */
bool is_joystick_full_left(struct joystick_t joystick) {
    return is_joystick_left(joystick) && !is_joystick_up(joystick) && !is_joystick_down(joystick);
}

/*
 * is_joystick_full_right
 *
 * Returns true if the joystick is fully right (i.e not pointing up or down
 * either), and false otherwise.
 */
bool is_joystick_full_right(struct joystick_t joystick) {
    return is_joystick_right(joystick) && !is_joystick_up(joystick) && !is_joystick_down(joystick);
}

/*
 * is_joystick_full_up
 *
 * Returns true if the joystick is fully up (i.e not pointing left or right
 * either), and false otherwise.
 */
bool is_joystick_full_up(struct joystick_t joystick) {
    return is_joystick_up(joystick) && !is_joystick_left(joystick) && !is_joystick_right(joystick);
}

/*
 * is_joystick_full_down
 *
 * Returns true if the joystick is fully up (i.e not pointing left or right
 * either), and false otherwise.
 */
bool is_joystick_full_down(struct joystick_t joystick) {
    return is_joystick_down(joystick) && !is_joystick_left(joystick) && !is_joystick_right(joystick);
}

/*
 * move_pan_tilt_left
 *
 * Returns the updated horizontal position of the pan-tilt module.
 */
uint32_t move_pan_tilt_left(uint32_t pulsewidth_x_us) {
    return pulsewidth_x_us += PWM_PULSEWIDTH_STEP_US;
}

/*
 * move_pan_tilt_right
 *
 * Returns the updated horizontal position of the pan-tilt module.
 */
uint32_t move_pan_tilt_right(uint32_t pulsewidth_x_us) {
    return pulsewidth_x_us -= PWM_PULSEWIDTH_STEP_US;
}

/*
 * move_pan_tilt_up
 *
 * Returns the updated vertical position of the pan-tilt module.
 */
uint32_t move_pan_tilt_up(uint32_t pulsewidth_y_us) {
    return pulsewidth_y_us -= PWM_PULSEWIDTH_STEP_US;
}

/*
 * move_pan_tilt_down
 *
 * Returns the updated vertical position of the pan-tilt module.
 */
uint32_t move_pan_tilt_down(uint32_t pulsewidth_y_us) {
    return pulsewidth_y_us += PWM_PULSEWIDTH_STEP_US;
}

/*
 * move_pan_tilt
 *
 * Moving engine left is done by increasing pulsewidth, and moving engine right
 * is done by decreasing pulsewidth
 */
void move_pan_tilt() {
    static uint32_t pulsewidth_x_us = PWM_PULSEWIDTH_INIT_US;
    static uint32_t pulsewidth_y_us = PWM_PULSEWIDTH_INIT_US;

    struct joystick_t joystick = read_joystick();

    /* update x & y */
    if (is_joystick_full_left(joystick)) { /* update x */
        pulsewidth_x_us = move_pan_tilt_left(pulsewidth_x_us);
    } else if (is_joystick_full_right(joystick)) {
        pulsewidth_x_us = move_pan_tilt_right(pulsewidth_x_us);
    } else if (is_joystick_full_up(joystick)) { /* update y */
        pulsewidth_y_us = move_pan_tilt_up(pulsewidth_y_us);
    } else if (is_joystick_full_down(joystick)) {
        pulsewidth_y_us = move_pan_tilt_down(pulsewidth_y_us);
    }

    /* bound x */
    if (pulsewidth_x_us <= PWM_PULSEWIDTH_MIN_US) {
        pulsewidth_x_us = PWM_PULSEWIDTH_MIN_US;
    } else if (PWM_PULSEWIDTH_MAX_US <= pulsewidth_x_us) {
        pulsewidth_x_us = PWM_PULSEWIDTH_MAX_US;
    }

    /* bound y */
    if (pulsewidth_y_us <= PWM_PULSEWIDTH_MIN_US) {
        pulsewidth_y_us = PWM_PULSEWIDTH_MIN_US;
    } else if (PWM_PULSEWIDTH_MAX_US <= pulsewidth_y_us) {
        pulsewidth_y_us = PWM_PULSEWIDTH_MAX_US;
    }

    /* set PWM x */
    if (gpioServo(PWM_GPIO_PIN_X, pulsewidth_x_us) != 0) {
        printf("Error: gpioServo() failed for PWM_GPIO_PIN_X\n");
        exit(EXIT_FAILURE);
    }

    /* set PWM y */
    if (gpioServo(PWM_GPIO_PIN_Y, pulsewidth_y_us) != 0) {
        printf("Error: gpioServo() failed for PWM_GPIO_PIN_Y\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * setup_joystick
 *
 * - Enable pull-up resistor for joystick button
 * - Opens an SPI file descriptor
 * - Registers an interrupt for the joystick button press
 */
void setup_joystick() {
    /* create SPI handle */
    fd_spi = spiOpen(SPI_CHANNEL, SPI_BAUD, SPI_FLAGS);
    if (fd_spi < 0) {
        printf("Error: spiOpen() failed\n");
        exit(EXIT_FAILURE);
    }

    /* Joystick is connected to GND when pressed, so we need a pull-up resistor
       to detect changes in the SW signal */
    if (gpioSetPullUpDown(JOYSTICK_BUTTON_GPIO_PIN, PI_PUD_UP) != 0) {
        printf("Error: gpioSetPullUpDown() failed\n");
        exit(EXIT_FAILURE);
    }

    /* register interrupt for joystick button press */
    if (gpioSetISRFunc(JOYSTICK_BUTTON_GPIO_PIN, RISING_EDGE, 0, joystick_button_isr) != 0) {
        printf("Error: gpioSetISRFunc() failed\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * setup_pwm
 *
 * - Sets the PWM frequency
 * - Sets the PWM range
 */
void setup_pwm() {
    /* set PWM frequency */
    if (gpioSetPWMfrequency(PWM_GPIO_PIN_X, PWM_GPIO_FREQ_HZ) == PI_BAD_USER_GPIO) {
        printf("Error: gpioSetPWMfrequency() failed for PWM_GPIO_PIN_X\n");
        exit(EXIT_FAILURE);
    }
    if (gpioSetPWMfrequency(PWM_GPIO_PIN_Y, PWM_GPIO_FREQ_HZ) == PI_BAD_USER_GPIO) {
        printf("Error: gpioSetPWMfrequency() failed for PWM_GPIO_PIN_Y\n");
        exit(EXIT_FAILURE);
    }

    /* set PWM range */
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
 * setup_fifo
 *
 * Sets up a fifo
 */
void setup_fifo() {
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        printf("Error: mkfifo() failed\n");
        exit(EXIT_FAILURE);
    }

    /* opening in O_RDWR instead of in O_WRONLY, because the open() call will
     * block until another process opens the fifo for reading... */
    fd_fifo = open(FIFO_NAME, O_RDWR);
    if (fd_fifo == -1) {
        printf("Error: open() failed\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * initialize_pigpio
 *
 * Initializes the pigpio library
 */
void initialize_pigpio() {
    if (gpioInitialise() < 0) {
        printf("Error: gpioInitialise() failed\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * int_handler
 *
 * Cleans up all open file handles and exits the program.
 */
void int_handler(int signum) {
    if (spiClose(fd_spi) != 0) {
        printf("Error: spiClose() failed\n");
        exit(EXIT_FAILURE);
    }

    if (close(fd_fifo) == -1) {
        printf("Error: close() failed\n");
        exit(EXIT_FAILURE);
    }

    if (unlink(FIFO_NAME) == -1) {
        printf("Error: unlink() failed\n");
        exit(EXIT_FAILURE);
    }

    gpioTerminate();

    exit(EXIT_SUCCESS);
}

/*
 * joystick_button_isr
 *
 * Interrupt service routine for the joystick button.
 */
void joystick_button_isr(int gpio, int level, uint32_t tick) {
    /* only enable button press if previous button press is no longer being
       handled (to avoid duplicate presses) */
    if (!joystick_button_pressed_handling) {
        joystick_button_pressed = true;
    }
}

/*
 * button_press_operation
 *
 * Returns the operation selected by the joystick.
 *   - left  -> OP_SKIN_SMOOTHING
 *   - right -> OP_SHADOW_DETECTION
 */
uint32_t button_press_operation() {
    struct joystick_t joystick = read_joystick();

    while (true) {
        if (is_joystick_full_left(joystick)) {
            return OP_SKIN_SMOOTHING;
        } else if (is_joystick_full_right(joystick)) {
            return OP_SHADOW_DETECTION;
        }

        joystick = read_joystick();
    }
}

/*
 * handle_button_press
 *
 * If a button press has occurred, then send a packet through a fifo to
 * inform the control program of the event.
 */
void handle_button_press() {
    if (joystick_button_pressed) {
        /* prevent multiple presses from being registered */
        joystick_button_pressed_handling = true;

        /* acknowledge button press */
        joystick_button_pressed = false;

        printf("button pressed\n");
        fflush(stdout);

        /* choose operation to send to server */
        uint32_t operation = button_press_operation();

        /* send data to fifo */
        uint8_t buf[1] = {operation};
        write(fd_fifo, (void *) buf, sizeof(buf));

        /* sleep before exiting so the joystick has enough time to come back to its center position */
        sleep(1);

        /* enable future button presses */
        joystick_button_pressed_handling = false;
    }
}

int main(int argc, char **argv) {
    initialize_pigpio();

    /* register signal handler for SIGINT (ctrl+c) */
    if (gpioSetSignalFunc(SIGINT, int_handler) == PI_BAD_SIGNUM) {
        printf("Error: gpioSetSignalFunc() failed\n");
        exit(EXIT_FAILURE);
    }

    setup_joystick();
    setup_pwm();
    setup_fifo();

    while (true) {
        handle_button_press();
        move_pan_tilt();

        /* sleep for some time to avoid the servo from moving too fast */
        usleep(USLEEP_DELAY);
    }

    /* will never get here, but left for main() to correctly compile */
    return EXIT_SUCCESS;
}
