#include <linux/module.h>
#include <linux/pwm.h>
#include <linux/printk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include "servo_ctl.h"


MODULE_LICENSE("GPL");

uint32_t currentAngle = 0;

int32_t servo_set_angle_abs(uint32_t angle)
{
	uint64_t converted;

	if(angle > MAX_ANGLE) {
		pr_warn("servo-ctl: angle more than 170 degrees. servomotor will rotate to 170 degrees\n");
		angle = MAX_ANGLE;
	}
	pwm_disable(servo);

	converted = (((MAX_DUTY_CYCLE - MIN_DUTY_CYCLE) / 170) * angle) + MIN_DUTY_CYCLE;
	pwm_config(servo, converted, FREQ_FOR_SERVO);
	pwm_enable(servo);
	if (currentAngle > angle)
		msleep(DELAY_FOR_DEG * (currentAngle - angle));
	else
		msleep(DELAY_FOR_DEG * (angle - currentAngle));
	
	currentAngle = angle;
	pr_info("servo-ctl: servomotor rotaded to  %d degrees\n", currentAngle);

	return 0;
}

int32_t servo_set_angle_rel(int32_t angle)
{
	uint64_t converted;
	if(angle > (int32_t)(MAX_ANGLE - currentAngle)) {
		pr_warn("servo-ctl: angle more than 170 degrees. servomotot will rotate to 170 degrees\n");
		angle = MAX_ANGLE;
	} else if ((int32_t)(angle + currentAngle) < 0) {
		pr_warn("servo-ctl: angle more than 0 degrees. servomotot will rotate to 0 degrees\n");
		angle = MIN_ANGLE;
	} else 
		angle = angle + currentAngle;
	pwm_disable(servo);

	converted = (((MAX_DUTY_CYCLE - MIN_DUTY_CYCLE) / 170) * angle) + MIN_DUTY_CYCLE;
	pwm_config(servo, converted, FREQ_FOR_SERVO);
	pwm_enable(servo);
	if (currentAngle > angle)
		msleep(DELAY_FOR_DEG * (currentAngle - angle));
	else
		msleep(DELAY_FOR_DEG * (angle - currentAngle));

	currentAngle = angle;
	pr_info("servo-ctl: servomotor rotaded %d degrees(%d)\n", MAX_ANGLE - currentAngle, currentAngle);
	
	return 0;
}
