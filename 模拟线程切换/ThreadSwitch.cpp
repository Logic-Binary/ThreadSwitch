#include "ThreadSwitch.h"


//�����߳�ջ�Ĵ�С
#define GMTHREADSTACKSIZE 0x80000

//��ǰ�̵߳�����
int CurrentThreadIndex = 0;

//�̵߳��б�
GMThread_t GMThreadList[MAXGMTHREAD] = { NULL, 0 };

//�߳�״̬�ı�־
enum FLAGS
{
	GMTHREAD_CREATE = 0x1,
	GMTHREAD_READY = 0x2,
	GMTHREAD_SLEEP = 0x4,
	GMTHREAD_EXIT = 0x8,
};

//�����̵߳ĺ���
void GMThreadStartup(GMThread_t* GMThreadp)
{
	GMThreadp->func(GMThreadp->lpParameter);
	GMThreadp->Flags = GMTHREAD_EXIT;
	Scheduling();

	return;
}

//�����̵߳ĺ���
void IdleGMThread(void* lpParameter)
{
	printf("IdleGMThread---------------\n");
	Scheduling();
	return;
}

//��ջ��ѹ��һ��uintֵ
void PushStack(unsigned int** Stackpp, unsigned int v)
{
	*Stackpp -= 1;//esp - 4
	**Stackpp = v;//

	return;
}

//��ʼ���̵߳���Ϣ
void initGMThread(GMThread_t* GMThreadp, char* name, void(*func)(void* lpParameter), void* lpParameter)
{
	unsigned char* StackPages;
	unsigned int* StackDWordParam;

	//�ṹ��ʼ����ֵ
	GMThreadp->Flags = GMTHREAD_CREATE;
	GMThreadp->name = name;
	GMThreadp->func = func;
	GMThreadp->lpParameter = lpParameter;

	//����ռ�
	StackPages = (unsigned char*)VirtualAlloc(NULL, GMTHREADSTACKSIZE, MEM_COMMIT, PAGE_READWRITE);
	//��ʼ��
	ZeroMemory(StackPages, GMTHREADSTACKSIZE);
	//��ʼ����ַ(ջ��)
	GMThreadp->initialStack = StackPages + GMTHREADSTACKSIZE;
	//��ջ����
	GMThreadp->StackLimit = StackPages;
	//��ջ��ַ
	StackDWordParam = (unsigned int*)GMThreadp->initialStack;

	//��ջ
	PushStack(&StackDWordParam, (unsigned int)GMThreadp);		//ͨ�����ָ�����ҵ��̺߳������̲߳���
	PushStack(&StackDWordParam, (unsigned int)0);				//ƽ���ջ��(���ù�)
	PushStack(&StackDWordParam, (unsigned int)GMThreadStartup);	//�߳���ں��� ���������������̺߳���
	PushStack(&StackDWordParam, (unsigned int)5);				//push ebp
	PushStack(&StackDWordParam, (unsigned int)7);				//push edi
	PushStack(&StackDWordParam, (unsigned int)6);				//push esi
	PushStack(&StackDWordParam, (unsigned int)3);				//push ebx
	PushStack(&StackDWordParam, (unsigned int)2);				//push ecx
	PushStack(&StackDWordParam, (unsigned int)1);				//push edx
	PushStack(&StackDWordParam, (unsigned int)0);				//push eax

	//��ǰ�̵߳�ջ��
	GMThreadp->KernelStack = StackDWordParam;

	//��ǰ�߳�״̬
	GMThreadp->Flags = GMTHREAD_READY;

	return;
}

//��һ������ע��Ϊ�����߳�ִ��
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

//�л��߳�	1����ǰ�߳̽ṹ��ָ�� 2��Ҫ�л����߳̽ṹ��ָ��
__declspec(naked) void SwitchContext(GMThread_t* SrcGMThreadp, GMThread_t* DstGMThreadp)
{
	__asm
	{
		//������ջ
		push ebp
		mov ebp, esp

		//���浱ǰ�̼߳Ĵ���
		push edi
		push esi
		push ebx
		push ecx
		push edx
		push eax

		mov esi, SrcGMThreadp
		mov edi, DstGMThreadp

		//���浱ǰ��ջ�����߳̽ṹ����
		mov[esi + GMThread_t.KernelStack], esp

		//�����߳��л�������һ���̸߳���
		mov esp, [edi + GMThread_t.KernelStack]

		pop eax
		pop edx
		pop ecx
		pop ebx
		pop esi
		pop edi

		pop ebp
		ret	//��һ�ΰ�startup(�̺߳������)����eip ִ�еľ����̺߳�����
	}
}

//����������ó�cpu���Ӷ���������ѡ��һ���߳�ִ��
void Scheduling()
{
	int TickCount = GetTickCount();

	GMThread_t* SrcGMThreadp = &GMThreadList[CurrentThreadIndex];
	GMThread_t* DstGMThreadp = &GMThreadList[0];


	for (int i = 1; GMThreadList[i].name; i++)
	{
		//����߳�flag��sleep
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
