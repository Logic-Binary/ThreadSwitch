#include "ThreadSwitch.h"

extern int CurrentThreadIndex;

extern GMThread_t GMThreadList[MAXGMTHREAD];

void Thread1(void*)
{
	while (1)
	{
		printf("Thread1\n");
		GMSleep(500);
	}
}

void Thread2(void*)
{
	while (1)
	{
		printf("Thread2\n");
		GMSleep(200);
	}
}

void Thread3(void*)
{
	while (1)
	{
		printf("Thread3\n");
		GMSleep(10);
	}
}

void Thread4(void*)
{
	while (1)
	{
		printf("Thread4\n");
		GMSleep(1000);
	}
}


int main()
{
	RegisterGMThread((char*)"Thread1", Thread1, NULL);
	RegisterGMThread((char*)"Thread2", Thread2, NULL);
	RegisterGMThread((char*)"Thread3", Thread3, NULL);
	RegisterGMThread((char*)"Thread4", Thread4, NULL);

	for (;;)	//模拟系统时钟
	{
		Sleep(20);
		Scheduling();
	}

	return 0;
}
