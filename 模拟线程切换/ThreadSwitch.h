#pragma once
#include <windows.h>
#include "stdio.h"

//最大支持的线程数
#define MAXGMTHREAD 100

//线程信息的结构
typedef struct
{
	char* name;							//线程名 相当于线程ID
	int Flags;							//线程状态
	int SleepMillsecondDot;				//休眠时间

	void* initialStack;					//线程堆栈起始位置
	void* StackLimit;					//线程堆栈界限
	void* KernelStack;					//线程堆栈当前位置，也就是ESP

	void* lpParameter;					//线程函数的参数
	void(*func)(void* lpParameter);		//线程函数
}GMThread_t;

void GMSleep(int MilliSeconds);
int RegisterGMThread(char* name, void(*func)(void* lpParameter), void* lpParameter);
void Scheduling();
