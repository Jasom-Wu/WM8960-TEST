#ifndef __WM8960_H__
#define __WM8960_H__

#include "stm32f1xx_hal.h"


enum {
    WM8960_MODE_DAC_ENABLE=1,
    WM8960_MODE_ADC_ENABLE
};


uint8_t WM8960_Write_Reg(uint8_t reg, uint16_t dat);
uint16_t WM8960_Read_Reg(uint8_t reg);
uint8_t WM89060_Init(uint8_t mode);




#endif
