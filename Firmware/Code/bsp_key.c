#include "bsp_key.h"
#include "simple_gpio.h"


#define KEY1 PAin(0)
#define KEY2 PCin(13)



static Key_Typedef mykeys = {0};

__weak void KeyProcessHandler(const Key_Typedef keys) {
   /* NOTE : This function should not be modified, when the callback is needed,
            the KeyProcessHandler could be implemented in the user file
   */
}

void KeyHandlerCore(void) {
//  KEY1 = KEY2 = 0;//先把输入端口拉低，检测高电平
  switch (mykeys.k1) {
    case NONE: {
      if (KEY1 == 1) {
        mykeys.k1 = PRESSING;
        mykeys.k1_last_tick = HAL_GetTick();//当确认键按下，需要记录此时的时刻
      }
      break;
    }
    case PRESSING: {
      if (KEY1 == 0) {//按键松开！
        //getTick()获取系统心跳的时刻，若超前于刚被按下时刻800ms
        if (HAL_GetTick() - mykeys.k1_last_tick > 800){
          mykeys.k1 = LONG_PRESSED;//触发长按
        }
        else{
          mykeys.k1 = CLICKED;//若按下时间小于800ms则触发短按
        }
      }
      break;
    }
  }
  switch (mykeys.k2) {
    case NONE: {
      if (KEY2 == 1) {
        mykeys.k2 = PRESSING;
        mykeys.k2_last_tick = HAL_GetTick();//当确认键按下，需要记录此时的时刻
      }
      break;
    }
    case PRESSING: {
      if (KEY2 == 0) {//按键松开！
				//getTick()获取系统心跳的时刻，若超前于刚被按下时刻800ms
        if (HAL_GetTick() - mykeys.k2_last_tick > 800){
					mykeys.k2 = LONG_PRESSED;//触发长按
				}
        else{
          mykeys.k2 = CLICKED;//若按下时间小于800ms则触发短按
        }
      }
      break;
    }
  }
  KeyProcessHandler(mykeys);
  //以下是按键状态捕获与该函数衔接的关键，处理完按键事务后除了pressing状态的其他状态下按键都要复位
  if(mykeys.k2!=PRESSING)mykeys.k2 = NONE;
  if(mykeys.k1!=PRESSING)mykeys.k1 = NONE;
}


