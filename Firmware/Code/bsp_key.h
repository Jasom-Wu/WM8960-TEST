#ifndef _MY_UI_H_
#define _MY_UI_H_

#include "main.h"

typedef enum{ //按键的四种状态
    NONE=0,				//无操作
    CLICKED,			//短按
    LONG_PRESSED,	//长按
    PRESSING			//正在按下
}KeyState;
typedef struct KeyStruct{
    uint8_t k1: 2;//使用位域，节约内存
    uint8_t k2: 2;
    uint32_t k1_last_tick;
    uint32_t k2_last_tick;
}Key_Typedef;
void KeyHandlerCore();//按键状态捕获函数，用于捕获按键状态
void KeyProcessHandler(Key_Typedef keys);
#endif