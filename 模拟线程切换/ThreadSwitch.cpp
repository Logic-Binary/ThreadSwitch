#include "ThreadSwitch.h"


//定义线程栈的大小
#define GMTHREADSTACKSIZE 0x80000

//当前线程的索引
int CurrentThreadIndex = 0;

//线程的列表
GMThread_t GMThreadList[MAXGMTHREAD] = { NULL, 0 };

//线程状态的标志
enum FLAGS
{
	GMTHREAD_CREATE = 0x1,
	GMTHREAD_READY = 0x2,
	GMTHREAD_SLEEP = 0x4,
	GMTHREAD_EXIT = 0x8,
};

//启动线程的函数
void GMThreadStartup(GMThread_t* GMThreadp)
{
	GMThreadp->func(GMThreadp->lpParameter);
	GMThreadp->Flags = GMTHREAD_EXIT;
	Scheduling();

	return;
}

//空闲线程的函数
void IdleGMThread(void* lpParameter)
{
	printf("IdleGMThread---------------\n");
	Scheduling();
	return;
}

//向栈中压入一个uint值
void PushStack(unsigned int** Stackpp, unsigned int v)
{
	*Stackpp -= 1;//esp - 4
	**Stackpp = v;//

	return;
}

//初始化线程的信息
void initGMThread(GMThread_t* GMThreadp, char* name, void(*func)(void* lpParameter), void* lpParameter)
{
	unsigned char* StackPages;
	unsigned int* StackDWordParam;

	//结构初始化赋值
	GMThreadp->Flags = GMTHREAD_CREATE;
	GMThreadp->name = name;
	GMThreadp->func = func;
	GMThreadp->lpParameter = lpParameter;

	//申请空间
	StackPages = (unsigned char*)VirtualAlloc(NULL, GMTHREADSTACKSIZE, MEM_COMMIT, PAGE_READWRITE);
	//初始化
	ZeroMemory(StackPages, GMTHREADSTACKSIZE);
	//初始化地址(栈底)
	GMThreadp->initialStack = StackPages + GMTHREADSTACKSIZE;
	//堆栈限制
	GMThreadp->StackLimit = StackPages;
	//堆栈地址
	StackDWordParam = (unsigned int*)GMThreadp->initialStack;

	//入栈
	PushStack(&StackDWordParam, (unsigned int)GMThreadp);		//通过这个指针来找到线程函数，线程参数
	PushStack(&StackDWordParam, (unsigned int)0);				//平衡堆栈的(不用管)
	PushStack(&StackDWordParam, (unsigned int)GMThreadStartup);	//线程入口函数 这个函数负责调用线程函数
	PushStack(&StackDWordParam, (unsigned int)5);				//push ebp
	PushStack(&StackDWordParam, (unsigned int)7);				//push edi
	PushStack(&StackDWordParam, (unsigned int)6);				//push esi
	PushStack(&StackDWordParam, (unsigned int)3);				//push ebx
	PushStack(&StackDWordParam, (unsigned int)2);				//push ecx
	PushStack(&StackDWordParam, (unsigned int)1);				//push edx
	PushStack(&StackDWordParam, (unsigned int)0);				//push eax

	//当前线程的栈顶
	GMThreadp->KernelStack = StackDWordParam;

	//当前线程状态
	GMThreadp->Flags = GMTHREAD_READY;

	return;
}

//将一个函数注册为单独线程执行
int RegisterGMThread(char* name, void(*func)(void* lpParameter), void* lpParameter)
{
	int i;
	for (i = 1; GMThreadList[i].name; i++)
	{
		if (0 == _stricmp(GMThreadList[i].name, name))
		{
			break;
		}
	}
	initGMThread(&GMThreadList[i], name, func, lpParameter);

	return (i & 0x55AA0000);
}

//切换线程	1：当前线程结构体指针 2：要切换的线程结构体指针
__declspec(naked) void SwitchContext(GMThread_t* SrcGMThreadp, GMThread_t* DstGMThreadp)
{
	__asm
	{
		//提升堆栈
		push ebp
		mov ebp, esp

		//保存当前线程寄存器
		push edi
		push esi
		push ebx
		push ecx
		push edx
		push eax

		mov esi, SrcGMThreadp
		mov edi, DstGMThreadp

		//保存当前的栈顶到线程结构体里
		mov[esi + GMThread_t.KernelStack], esp

		//经典线程切换，另外一个线程复活
		mov esp, [edi + GMThread_t.KernelStack]

		pop eax
		pop edx
		pop ecx
		pop ebx
		pop esi
		pop edi

		pop ebp
		ret	//第一次把startup(线程函数入口)弹到eip 执行的就是线程函数了
	}
}

//这个函数会让出cpu，从队列里重新选择一个线程执行
void Scheduling()
{
	int TickCount = GetTickCount();

	GMThread_t* SrcGMThreadp = &GMThreadList[CurrentThreadIndex];
	GMThread_t* DstGMThreadp = &GMThreadList[0];


	for (int i = 1; GMThreadList[i].name; i++)
	{
		//如果线程flag是sleep
		if (GMThreadList[i].Flags & GMTHREAD_SLEEP)
		{
			if (TickCount > GMThreadList[i].SleepMillsecondDot)
			{
				GMThreadList[i].Flags = GMTHREAD_READY;
			}
		}
		if (GMThreadList[i].Flags & GMTHREAD_READY)
		{
			DstGMThreadp = &GMThreadList[i];
			break;
		}
	}

	CurrentThreadIndex = DstGMThreadp - GMThreadList;

	SwitchContext(SrcGMThreadp, DstGMThreadp);

	return;
}

void GMSleep(int MilliSeconds)
{
	GMThread_t* GMThreadp;
	GMThreadp = &GMThreadList[CurrentThreadIndex];

	if (GMThreadp->Flags != 0)
	{
		GMThreadp->Flags = GMTHREAD_SLEEP;
		GMThreadp->SleepMillsecondDot = GetTickCount() + MilliSeconds;
	}

	Scheduling();
	return;
}
