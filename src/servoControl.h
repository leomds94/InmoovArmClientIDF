#ifndef _SERVO_CONTROL_H_
#define _SERVO_CONTROL_H_

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>

class servoControl
{
  protected:
    const int _freqHz = 50;

    ledc_channel_t _ledcChannel;
    unsigned int _min;
    unsigned int _max;

    double getDutyByPercentage(double percentage);
    double getDutyByuS(double uS);

  public:
    void attach(gpio_num_t pin, unsigned int minuS = 400, unsigned int maxuS = 2600, ledc_channel_t ledcChannel = LEDC_CHANNEL_0, ledc_timer_t ledcTimer = LEDC_TIMER_0);
    void writeMicroSeconds(unsigned int uS);
    void write(unsigned int value);
    void detach();
};
#endif