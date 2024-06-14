#include "WM8960.h"
#include "i2c.h"
#include "i2s.h"
#include "stdio.h"

#define WM8960_ADDRESS  0x1a
#define USE_BOARD_MIC

//resgister value
static uint16_t WM8960_REG_VAL[56] =
{  
  0x0097, 0x0097, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x000A,
  0x01C0, 0x0000, 0x00FF, 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x007B, 0x0100, 0x0032, 0x0000, 0x00C3, 0x00C3, 0x01C0,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0100, 0x0100, 0x0050, 0x0050, 0x0050, 0x0050, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0040, 0x0000, 0x0000, 0x0050, 0x0050, 0x0000,
  0x0000, 0x0037, 0x004D, 0x0080, 0x0008, 0x0031, 0x0026, 0x00ED
};

/**
  * @brief  Write register of WM8960.
  * @param  reg: The number of resigter which to be read.
  * @param  dat: The data which will be writeen to the register.
  * @retval The value of regsiter.
  */
uint8_t WM8960_Write_Reg(uint8_t reg, uint16_t dat)  {
  
  uint8_t res,I2C_Data[2];
  
  I2C_Data[0] = (reg<<1)|((uint8_t)((dat>>8)&0x0001));  //RegAddr
  I2C_Data[1] = (uint8_t)(dat&0x00FF);                  //RegValue
  
  res = HAL_I2C_Master_Transmit(&hi2c1,(WM8960_ADDRESS<<1),I2C_Data,2,10);
  if(res == HAL_OK)
    WM8960_REG_VAL[reg] = dat;
  
  return res;
}

/**
  * @brief  Read register of WM8960.
  * @param  reg: The number of resigter which to be read.
  * @retval The value of regsiter.
  */
uint16_t WM8960_Read_Reg(uint8_t reg) {
  
  return WM8960_REG_VAL[reg];
}


/**
  * @brief  Initialize WM8960 device.
  * @param  mode: only WM8960_MODE_DAC_ENABLE=1 or WM8960_MODE_ADC_ENABLE or their company
  * @retval res :Status
  */
uint8_t WM89060_Init(uint8_t mode)  {

  uint8_t res;
  
  //Reset Device
  res = WM8960_Write_Reg(0x0f, 0x0000);
  if(res != 0)
    return res;
  else
    printf("WM8960 reset completed !!\r\n");
  
  //Set Power Source

  res =  WM8960_Write_Reg(0x19, 1<<7 | 1<<6 | 1<<1 );//VMID为2 x 50kΩ,VREF、MICB
  res += WM8960_Write_Reg(0x2F, 1<<3 | 1<<2);//LOMIX,ROMIX enable
  if(mode & WM8960_MODE_DAC_ENABLE){
    res += WM8960_Write_Reg(0x1A,WM8960_REG_VAL[0x1A] | 1<<8 | 1<<7 | 1<<6 | 1<<5 | 1<<4 | 1<<3 | 1<<1);//DAC、OUT1、SPK、OUT3
  }
  if(mode & WM8960_MODE_ADC_ENABLE){
    res +=  WM8960_Write_Reg(0x19,WM8960_REG_VAL[0x19] | 1<<5|1<<4|1<<3|1<<2);// 0x00E8);ADCL、ADCR、AINL、AINR
    res += WM8960_Write_Reg(0x2F,WM8960_REG_VAL[0x2F] | 1<<5 | 1<<4);//LMIC,RMIC
  }
  if(res != 0)  {
    printf("Source set fail !!\r\n");
    printf("Error code: %d\r\n",res);
    return res;
  }
  
  //Configure clock
  WM8960_Write_Reg(0x04, 0x0000);//divided by 256

  //Configure audio interface
  //I2S format 16 bits word length
  WM8960_Write_Reg(0x07, 0x0002);//FORMAT=10——>I2S Format;WL=00——>16位;MS=0——>slave

  if(mode & WM8960_MODE_DAC_ENABLE) {
    //Configure ADC/DAC
    WM8960_Write_Reg(0x05, WM8960_REG_VAL[0x05] & (~(1<<3)));// Set DAC not Muted

    //Configure HP_L and HP_R OUTPUTS
    WM8960_Write_Reg(0x02, 1<<7 | 0x006F | 0x0100);  //LOUT1 Volume Set +6dB-16dB=-10dB Zero Cross Enable
    WM8960_Write_Reg(0x03, 1<<7 | 0x006F | 0x0100);  //ROUT1 Volume Set +6dB-16dB=-10dB Zero Cross Enable

    //Configure SPK_P and SPK_N
    WM8960_Write_Reg(0x28, 1<<7 | 0x007a | 0x0100); //Left Speaker Volume  Full +6dB Zero Cross Enable
    WM8960_Write_Reg(0x29, 1<<7 | 0x007a | 0x0100); //Right Speaker Volume Full +6dB

    //Enable the OUTPUTS
    WM8960_Write_Reg(0x31, WM8960_REG_VAL[0x31] | 1<<6 | 1<<7); //Enable Class D Speaker Outputs Left and Right

    //Configure DAC volume
    WM8960_Write_Reg(0x0A, 0x00FF | 0x0100);//Left Full 0dB
    WM8960_Write_Reg(0x0B, 0x00FF | 0x0100);//Right Full 0dB

    //3D
//  WM8960_Write_Reg(0x10, 0x001F);
    //Configure MIXER
    WM8960_Write_Reg(0x22, 1<<8 );//| 1<<7);//第7位LI2LO 不使能才对，没用到input3直接输出
    WM8960_Write_Reg(0x25, 1<<8 );//| 1<<7);//第7位RI2RO 不使能才对，没用到input3直接输出
  }


  if(mode & WM8960_MODE_ADC_ENABLE){
    /*********PGA*********/
    //Input PGA
    WM8960_Write_Reg(0x00, 0x003F | 0x0100);//0x003F Left PGA Full
    WM8960_Write_Reg(0x01, 0x003F | 0x0100);//0x003F Right PGA Full

    //Input Signal Path
#ifdef USE_BOARD_MIC
    WM8960_Write_Reg(0x20, 0x0008 | 0x0100);
    WM8960_Write_Reg(0x21, 0x0000);
#elif USE_EARPHONE_MIC
    WM8960_Write_Reg(0x20, 0x0000);
    WM8960_Write_Reg(0x21, 0x0008 | 0x0100);
#endif

    //Input Boost Mixer
    WM8960_Write_Reg(0x2B, 0x0000); //MUTE L2 L3 TO MIXER
    WM8960_Write_Reg(0x2C, 0x0000); //MUTE R2 R3 TO MIXER

/*********ADC*********/

    //ADC Digital Volume Control
    WM8960_Write_Reg(0x15, 0x00EA | 0x0100);//LEFT +30dB - 0.5dB*20 = +20dB
    WM8960_Write_Reg(0x16, 0x00EA | 0x0100);//RIGHT +30dB - 0.5dB*20 = +20dB

#ifdef USE_BOARD_MIC
    WM8960_Write_Reg(0x17, 1<<2);//ADC两个声道采用leftADCdata
#elif USE_EARPHONE_MIC
    WM8960_Write_Reg(0x17, 1<<3);//ADC两个声道采用rightADCdata
#endif

/*********ALC Control*********/
    //Noise Gate Control
    WM8960_Write_Reg(0x14, 0x00F9);//Ignore signal under -30dBfs
  }
  //Jack Detect
  WM8960_Write_Reg(0x18, 1<<6 | 0<<5);//HPSWEN=1；HPSWPOL=0 高电平为耳机
  WM8960_Write_Reg(0x17, WM8960_REG_VAL[0x17] | 0x01C3);//TSDEN=1过热关闭设备，需要开TSENSEN！
	//VSEL=11对3v3节电；TOEN=1、TOCLKSEL=1——>jack detect开启快速响应
  WM8960_Write_Reg(0x30, 0x0009|1<<1);//JD2（LINPUT3） used for jack detect input;
	//MBSEL=1——>MicBias=0.65 * AVDD;TSENSEN=1开温度传感器
  
  return 0;
}

