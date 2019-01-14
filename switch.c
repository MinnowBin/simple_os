#include "tinyOS.h"
#include "stm32f10x.h"

/*
在任务切换中，主要依赖了PendSV进行切换。PendSV其中的一个很重要的作用便是用于支持RTOS的任务切换。
实现方法为：
1、 首先将PendSV的中断优先配置为最低。
    这样只有在其它所有中断完成后，才会触发该中断；
    实现方法为：向NVIC_SYSPRI2写NVIC_PENDSV_PRI
2、 在需要中断切换时，设置挂起位为1，手动触发。这样，当没有其它中断发生时，将会引发PendSV中断。
    实现方法为：向NVIC_INT_CTRL写NVIC_PENDSVSET
3、 在PendSV中，执行任务切换操作。
*/
#define NVIC_INT_CTRL								0xE000ED04//�Ĵ�����ַ
#define NVIC_PENDSVSET							    0x10000000//д��Ĵ�����ֵ
#define NVIC_SYSPRI2								0xE000ED22//�Ĵ�����ַ
#define NVIC_PENDSV_PRI							    0x000000FF//д��Ĵ�����ֵ

#define MEM32(addr)									*(volatile unsigned long *)(addr)//д�Ĵ����ĺ궨��
#define MEM8(addr)									*(volatile unsigned char *)(addr)

uint32_t tTaskEnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void tTaskExitCritical(uint32_t status)
{
    __set_PRIMASK(status);
}

__asm void PendSV_Handler(void)
{
    IMPORT currentTask                              // 使用import导入C文件中声明的全局变量
    IMPORT nextTask                                 // 类似于在C文文件中使用extern int variable

    MRS R0, PSP                                     // 获取当前任务的堆栈指针

    CBZ R0, PendSVHandler_nosave                    // if 这是由tTaskSwitch触发的(此时，PSP肯定不会是0了，0的话必定是tTaskRunFirst)触发
                                                    // 不清楚的话，可以先看tTaskRunFirst和tTaskSwitch的实现

    STMDB R0!, {R4-R11}                             // 需要将除异常自动保存的寄存器之外的其它寄存器自动保存起来{R4, R11}
                                                    // 保存的地址是当前任务的PSP堆栈中，这样就完整的保存了必要的CPU寄存器,便于下次恢复

    LDR R1, =currentTask                            // 保存好后，将最后的堆栈顶位置，保存到currentTask->stack处
    LDR R1, [R1]
    STR R0, [R1]
    //////////////////////
PendSVHandler_nosave
    //����״̬�Ļָ�
    LDR R0, =currentTask
    LDR R1, =nextTask
    LDR R2, [R1]
    STR R2, [R0]

    LDR R0, [R2]
    LDMIA R0!, {R4-R11}

    MSR PSP, R0
    ORR LR, LR, #0x04
    BX LR
}

void tTaskRunFirst()
{
    __set_PSP(0);

    MEM8(NVIC_SYSPRI2) = NVIC_PENDSV_PRI;
    MEM32(NVIC_INT_CTRL) = NVIC_PENDSVSET;
}

void tTaskSwitch()
{
    MEM32(NVIC_INT_CTRL) = NVIC_PENDSVSET;
}

/*pendSVC�쳣�Ĵ�������
void triggerPendSVC(void)
{
		MEM8(NVIC_SYSPRI2) = NVIC_PENDSV_PRI;
		MEM32(NVIC_INT_CTRL) = NVIC_PENDSVSET;
}*/
