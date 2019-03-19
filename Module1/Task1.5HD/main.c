#include <stdio.h>
#include <string.h>

#include "shell.h"
#include "periph/gpio.h"

gpio_t ledPin = GPIO_PIN(1, 5);     // PB5, Arduino GPIO13
gpio_t motionPin = GPIO_PIN(3, 2);  // PD2, Arduino GPIO2

void ISR_MotionPin(void)
{
    int state = gpio_read(motionPin);
    if (state)
    {
        puts("Begin: Motion detected.");
        puts("Turning LED on...");
        gpio_set(ledPin);
    }
    else
    {
        puts("End: Motion detected.");
        puts("Turning LED off...");
        gpio_clear(ledPin);
    }
}

int main(void)
{
    gpio_init(ledPin, GPIO_OUT);

    gpio_init_int(motionPin, GPIO_IN_PU, GPIO_BOTH, (gpio_cb_t)ISR_MotionPin, NULL);
    gpio_irq_enable(motionPin);

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
