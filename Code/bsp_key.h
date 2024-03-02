#ifndef _MY_UI_H_
#define _MY_UI_H_

#include "main.h"

typedef enum{ //����������״̬
    NONE=0,				//�޲���
    CLICKED,			//�̰�
    LONG_PRESSED,	//����
    PRESSING			//���ڰ���
}KeyState;
typedef struct KeyStruct{
    uint8_t k1: 2;//ʹ��λ�򣬽�Լ�ڴ�
    uint8_t k2: 2;
    uint32_t k1_last_tick;
    uint32_t k2_last_tick;
}Key_Typedef;
void KeyHandlerCore();//����״̬�����������ڲ��񰴼�״̬
void KeyProcessHandler(Key_Typedef keys);
#endif