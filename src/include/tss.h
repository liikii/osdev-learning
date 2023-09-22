#ifndef TSS_H
#define TSS_H
#include <system.h>
#include <string.h>
#include <gdt.h>


/*
 在32位任务状态段（Task State Segment，TSS）中，ss0 字段代表的是内核栈段选择子（Kernel Stack Segment Selector）。这个字段存储了一个16位的值，用于指定内核模式下使用的堆栈段。当处理器从用户模式切换到内核模式时，会使用这个选择子来加载正确的内核堆栈段。

选择子是16位的数值，它包含两个部分：索引和描述符特权级（Descriptor Privilege Level，DPL）。索引指向全局描述符表（Global Descriptor Table，GDT）或局部描述符表（Local Descriptor Table，LDT）中的一个描述符条目，而描述符特权级表示该段的权限级别。

通常情况下，操作系统会在 GDT 中定义一个内核堆栈段描述符，描述内核堆栈段的基地址、段限长以及其他属性。然后，将这个内核堆栈段描述符的选择子存储在 ss0 字段中。

当处理器执行特权级转换（例如，从用户态切换到内核态）时，会自动将 ss0 中的选择子加载到相应的段寄存器中，使其指向内核堆栈段描述符。这样，在内核模式下执行时，处理器就可以使用正确的内核堆栈来保存和恢复相关的上下文信息。

需要注意的是，具体如何设置和使用 ss0 字段取决于操作系统的设计和需求。不同的操作系统可能会有不同的内核堆栈段设置方式，以满足其特定的内存管理和任务切换需求。

希望以上解释能够帮助你更详细地理解 ss0 字段代表的内核栈段选择子在32位任务状态段中的含义。如果还有其他问题，请随时提问！
*/
typedef struct tss_entry {
    uint32_t prevTss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap;
}tss_entry_t;

extern void tss_flush();

void tss_init(uint32_t idx, uint32_t kss, uint32_t kesp);

void tss_set_stack(uint32_t kss, uint32_t kesp);

#endif
