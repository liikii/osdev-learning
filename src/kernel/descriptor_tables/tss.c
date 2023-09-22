#include <tss.h>

tss_entry_t kernel_tss;
/*
 * We don't need tss to assist task switching, but it's required to have one tss for switching back to kernel mode(system call for example)
 *
 * */
void tss_init(uint32_t idx, uint32_t kss, uint32_t kesp) {
    uint32_t base = (uint32_t)&kernel_tss;
    gdt_set_entry(idx, base, base + sizeof(tss_entry_t), 0xE9, 0);
    /* Kernel tss, access(E9 = 1 11 0 1 0 0 1)
        1   present
        11  ring 3
        0   should always be 1, why 0? may be this value doesn't matter at all
        1   code?
        0   can not be executed by ring lower or equal to DPL,
        0   not readable
        1   access bit, always 0, cpu set this to 1 when accessing this sector(why 0 now?)
    */
    memset(&kernel_tss, 0, sizeof(tss_entry_t));
    kernel_tss.ss0 = kss;
    // 上述表述的含义是：

    // 在启动操作系统时，我们通常将任务状态段（Task State Segment，TSS）的 esp 字段设置为0。然而，当切换到用户模式时，我们需要将其设置为真实的堆栈指针（ESP）。这是因为当用户模式应用程序调用内核函数（也称为系统调用）时，CPU 需要知道使用哪个堆栈指针来执行相应的操作。

    // 因此，在代码中存在一个名为 tss_set_stack 的函数，其目的就是设置 TSS 的堆栈指针。通过调用该函数，可以将任务状态段的 esp 字段更新为正确的值，以确保 CPU 在执行用户模式下的系统调用时使用正确的堆栈指针。

    // 具体来说，当操作系统从内核模式切换到用户模式时，它会通过 tss_set_stack 函数将真实的用户堆栈指针传递给 TSS，以便 CPU 知道在用户模式下使用哪个堆栈指针。

    // 这样做的目的是确保在系统调用期间，CPU 可以正确地处理用户模式和内核模式之间的堆栈切换，从而实现安全而有效的系统调用机制。
    // Note that we usually set tss's esp to 0 when booting our os, however, we need to set it to the real esp when we've switched to usermode because
    // the CPU needs to know what esp to use when usermode app is calling a kernel function(aka system call), that's why we have a function below called tss_set_stack
    kernel_tss.esp0 = kesp;
    kernel_tss.cs = 0x0b;
    kernel_tss.ds = 0x13;
    kernel_tss.es = 0x13;
    kernel_tss.fs = 0x13;
    kernel_tss.gs = 0x13;
    kernel_tss.ss = 0x13;
    tss_flush();
}

/*
 * This function is used to set the tss's esp, so that CPU knows what esp the kernel should be using
 * */
void tss_set_stack(uint32_t kss, uint32_t kesp) {
    kernel_tss.ss0 = kss;
    kernel_tss.esp0 = kesp;
}
