#pragma once
#include <windows.h>
#include "stdio.h"

//���֧�ֵ��߳���
#define MAXGMTHREAD 100

//�߳���Ϣ�Ľṹ
typedef struct
{
	char* name;							//�߳��� �൱���߳�ID
	int Flags;							//�߳�״̬
	int SleepMillsecondDot;				//����ʱ��

	void* initialStack;					//�̶߳�ջ��ʼλ��
	void* StackLimit;					//�̶߳�ջ����
	void* KernelStack;					//�̶߳�ջ��ǰλ�ã�Ҳ����ESP

	void* lpParameter;					//�̺߳����Ĳ���
	void(*func)(void* lpParameter);		//�̺߳���
}GMThread_t;

void GMSleep(int MilliSeconds);
int RegisterGMThread(char* name, void(*func)(void* lpParameter), void* lpParameter);
void Scheduling();
