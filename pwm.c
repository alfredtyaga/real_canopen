// vi: sw=2 ts=2
// Adapted from:
//  https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library/blob/master/Adafruit_PWMServoDriver.cpp

#include <math.h>
#include "canfestival.h"
#include "twi.h"
#include "pwm.h"
#include "config.h"
#include <util/delay.h>

#ifndef DEBUG_MESSAGES
#define DEBUG_MESSAGES 1
#endif

void pwm_init()
{
  twi_init();
}

/**************************************************************************/
/*!
    @brief  Sets the PWM output of one of the PCA9685 pins
    @param  num One of the PWM output pins, from 0 to 15
    @param  on At what point in the 4096-part cycle to turn the PWM output ON
    @param  off At what point in the 4096-part cycle to turn the PWM output OFF
*/
/**************************************************************************/
void set_pwm_output(uint8_t addr, uint8_t num, uint16_t on, uint16_t off) {
#ifdef DEBUG_MESSAGES
  printf("PWM %d: %d -> %d\n", num, on, off);
#endif
  uint8_t buffer[] = {
    LED0_ON_L+4*num,
    on,
    on>>8,
    off,
    off>>8
  };
  twi_writeTo(addr, (uint8_t*)&buffer, sizeof(buffer), 1, 1);
}

/**************************************************************************/
/*!
    @brief  Sets the PWM frequency for the entire chip, up to ~1.6 KHz
    @param  freq Floating point frequency that we will attempt to match
*/
/**************************************************************************/
void set_pwm_frequency(uint8_t addr, float freq) {
#if DEBUG_MESSAGES
  printf("Attempting to set freq %f", freq);
#endif

  // Correct for overshoot in the frequency setting (see Adafruit-PWM-Servo-Driver-Library/issue #11).
  freq *= 0.9;

  float prescale_val = 25000000;
  prescale_val /= 4096;
  prescale_val /= freq;
  prescale_val -= 1;

#if DEBUG_MESSAGES
  printf("Estimated pre-scale: %.3f\n", prescale_val);
#endif

  uint8_t prescale = floor(prescale_val + 0.5);
#if DEBUG_MESSAGES
  printf("Final pre-scale: %d\n", prescale);
#endif

  uint8_t oldmode = read8(addr, PCA9685_MODE1);
  uint8_t newmode = (oldmode & 0x7F) | 0x10; // sleep

  write8(addr, PCA9685_MODE1, newmode); // go to sleep
  write8(addr, PCA9685_PRESCALE, prescale); // set the prescaler
  write8(addr, PCA9685_MODE1, oldmode);
  _delay_ms(5);
  write8(addr, PCA9685_MODE1, oldmode | 0xa0);  //  This sets the MODE1 register to turn on auto increment.

#if DEBUG_MESSAGES
  printf("Mode now 0x%x\n", read8(addr, PCA9685_MODE1));
#endif
}

uint8_t read8(uint8_t i2c_addr, uint8_t addr) {
  uint8_t buffer;
  twi_readFrom(i2c_addr, &buffer, 1, 1);
  return buffer;
}

void write8(uint8_t i2c_addr, uint8_t addr, uint8_t d) {
  uint8_t buffer[] = {addr, d};
  twi_writeTo(i2c_addr, (uint8_t*)&buffer, sizeof(buffer), 1, 1);
}
