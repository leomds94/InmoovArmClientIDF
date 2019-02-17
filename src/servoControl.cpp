#include "servoControl.h"

double servoControl::getDutyByPercentage(double percentage){
	if (percentage <= 0){
		return 0;
	}
	if (percentage > 100){
		percentage = 100;
	}
	return (percentage / 100.0) * ((2<<14)-1); // LEDC_TIMER_15_BIT
}

double servoControl::getDutyByuS(double uS){
	return getDutyByPercentage(((uS * 100.0)/(1000000/_freqHz)));
}

void servoControl::attach(gpio_num_t pin, unsigned int minuS, unsigned int maxuS, ledc_channel_t ledcChannel, ledc_timer_t ledcTimer){
	_min = minuS;
	_max = maxuS;
	_ledcChannel = ledcChannel;

	ledc_timer_config_t timer_conf;
	timer_conf.duty_resolution 	= LEDC_TIMER_15_BIT;
	timer_conf.freq_hz    		= _freqHz;
	timer_conf.speed_mode 		= LEDC_HIGH_SPEED_MODE;
	timer_conf.timer_num  		= ledcTimer;
	ledc_timer_config(&timer_conf);

	ledc_channel_config_t ledc_conf;
	ledc_conf.channel		= _ledcChannel;
	ledc_conf.duty			= 0;
	ledc_conf.gpio_num		= (int)pin;
	ledc_conf.intr_type		= LEDC_INTR_DISABLE;
	ledc_conf.speed_mode	= LEDC_HIGH_SPEED_MODE;
	ledc_conf.timer_sel		= ledcTimer;
	ledc_channel_config(&ledc_conf);
}

void servoControl::detach(){
	ledc_stop(LEDC_HIGH_SPEED_MODE, _ledcChannel, 0);
}

void servoControl::writeMicroSeconds(unsigned int uS){
	ledc_set_duty(LEDC_HIGH_SPEED_MODE, _ledcChannel, getDutyByuS(uS));
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, _ledcChannel);
}

void servoControl::write(unsigned int value) {
	// 0 = MinServoAngle ; 180 = Max ServoAngle;
	int scale = (value - 0) * (_max - _min) / (180 - 0) + _min;
	writeMicroSeconds(scale);
}