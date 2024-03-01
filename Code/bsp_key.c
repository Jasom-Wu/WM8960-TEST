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
  //KEY1 = KEY2 = 1;//�Ȱ�����˿����ߣ����͵�ƽ
  switch (mykeys.k1) {//��switch��乹�ɰ�����״̬��
    case NONE: {
      if (KEY1 == 1)//���ɼ����ߵ�ƽ˵����������
        mykeys.k1 = PRESSING;//��������
      break;
    }
    case PRESSING: {
      if (KEY1 == 0)//��pressing״̬�¼�⵽�����ƽΪ�ͱ������������һ�ζ̰�
        mykeys.k1 = CLICKED;
      break;
    }
  }
  switch (mykeys.k2) {
    case NONE: {
      if (KEY2 == 1) {
        mykeys.k2 = PRESSING;
        mykeys.k2_last_tick = HAL_GetTick();//��ȷ�ϼ����£���Ҫ��¼��ʱ��ʱ��
      }
      break;
    }
    case PRESSING: {
      if (KEY2 == 0) {//�����ɿ���
				//getTick()��ȡϵͳ������ʱ�̣�����ǰ�ڸձ�����ʱ��800ms
        if (HAL_GetTick() - mykeys.k2_last_tick > 800){
					mykeys.k2 = LONG_PRESSED;//��������
				}
        else{
          mykeys.k2 = CLICKED;//������ʱ��С��800ms�򴥷��̰�
        }
      }
      break;
    }
  }
  KeyProcessHandler(mykeys);
  //�����ǰ���״̬������ú����νӵĹؼ��������갴����������pressing״̬������״̬�°�����Ҫ��λ
  if(mykeys.k2!=PRESSING)mykeys.k2 = NONE;
  if(mykeys.k1!=PRESSING)mykeys.k1 = NONE;
}


