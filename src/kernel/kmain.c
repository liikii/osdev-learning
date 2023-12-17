#include <multiboot.h>
#include <system.h>
#include <printf.h>
#include <gdt.h>
#include <idt.h>
#include <tss.h>
#include <vga.h>
#include <timer.h>
#include <asciiart.h>
#include <pmm.h>
#include <paging.h>
#include <kheap.h>
#include <pci.h>
#include <ata.h>
#include <vfs.h>
#include <string.h>
#include <ext2.h>
#include <process.h>
#include <usermode.h>
#include <syscall.h>
#include <elf_loader.h>
#include <vesa.h>
#include <bitmap.h>
#include <compositor.h>
#include <mouse.h>
#include <keyboard.h>
#include <font.h>
#include <rtc.h>
#include <rtl8139.h>
#include <ethernet.h>
#include <ip.h>
#include <udp.h>
#include <dhcp.h>
#include <serial.h>
#include <blend.h>
#include <spinlock.h>


extern uint8_t * bitmap;
extern ata_dev_t primary_master;
extern datetime_t current_datetime;

#define MSIZE 48 * M
#define GUI_MODE 0
#define NETWORK_MODE 0

void user_process2() {
    uint32_t lock = 0;
    while(1) {
        for(int i = 0; i < 10000; i++) {
            for(int j= 0; j < 2000; j++) {

            }
        }
        spinlock_lock(&lock);
        qemu_printf("hi there2\n");
        spinlock_unlock(&lock);
    }
}

void user_process() {
    uint32_t lock = 0;
    for(int i = 0; i < 20; i++) {
        create_process_from_routine(user_process2, "user process2");
    }
    while(1) {
        for(int i = 0; i < 10000; i++) {
            for(int j= 0; j < 2000; j++) {

            }
        }
        spinlock_lock(&lock);
        qemu_printf("hi there\n");
        spinlock_unlock(&lock);
    }
}


int kmain(multiboot_info_t * mb_info) {
    completed_init();

    process_init();
    syscall_init();

    // Set TSS stack so that when process return from usermode to kernel mode, the kernel have a ready-to-use stack
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    tss_set_stack(0x10, esp);

    // Start the first process
    create_process_from_routine(user_process, "user process");

    qemu_printf("\nDone!\n");
    for(;;);
    return 0;
}


/*
 * Move all the init code to here, so the kmain function is more clean and easier to read
 * */
void completed_init() {
    // 清 vga内存区
    video_init();
    qemu_printf("%s\n", simpleos_logo);

    // Initialize everything (green)
    set_curr_color(LIGHT_GREEN);
    qemu_printf("Initializing video(text mode 80 * 25)...\n");

    qemu_printf("Initializing gdt, idt and tss...\n");
    // kernel code, kernel data, user code, user data, tss seg
    gdt_init();

    // init idt
    idt_init();

    // tss 
    // Segment Selector
    // Segment Selector
    // 15    3 2   1   0
    // Index   TI  RPL
    // Index: Bits 3-15 of the Index of the GDT or LDT entry referenced by the selector. Since Segment Descriptors are 8 bytes in length, the value of Index is never unaligned and contains all zeros in the lowest 3 bits.
    // TI: Specifies which descriptor table to use. If clear (0) then the GDT is used, if set (1) then the current LDT is used.
    // RPL: The requested Privilege Level of the selector, determines if the selector is valid during permission checks and may set execution or memory access privilege.
    // , the low three bits are zero
    tss_init(5, 0x10, 0);

    qemu_printf("Initializing physical memory manager...\n");
    // 
    // bitmap init
    pmm_init(1096 * M);

    qemu_printf("Initializing paging...\n");
    paging_init();

    // 在提供的代码库中，kheap_init函数的目的是初始化操作系统的堆内存管理器。它接受三个参数：start、end和max。

    // start参数表示堆内存的起始地址，即可用内存块的开始位置。
    // end参数表示堆内存的结束地址，即可用内存块的结束位置。
    // max参数表示堆内存的最大地址，即堆可以扩展的最大位置。

    // 通过这些参数的设置，kheap_init函数将为堆内存管理器设置初始值，以确保堆管理器可以正确地分配和释放内存。这样，操作系统就能够使用堆内存来动态分配和管理内存，以满足程序的运行需求。
    // #define KHEAP_START         (void*)0xC0400000
    // #define KHEAP_INITIAL_SIZE  48 * M
    // #define KHEAP_MAX_ADDRESS   (void*)0xCFFFFFFF
    // #define HEAP_MIN_SIZE       4 * M
    qemu_printf("Initializing kernel heap...\n");
    kheap_init(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, KHEAP_MAX_ADDRESS);

    // 时钟唤醒
    qemu_printf("Initializing timer...\n");
    timer_init();

    // pci初始化，主要设置pci config space 各域大小
    qemu_printf("Initializing pci...\n");
    pci_init();

    // 注册keyboard 事件
    qemu_printf("Initializing keyboard...\n");
    keyboard_init();

    qemu_printf("Initializing vfs, ext2 and ata/dma...\n");

    vfs_init();
    
    ata_init();
    ext2_init("/dev/hda", "/");


    qemu_printf("Initializing real time clock...\n");
    rtc_init();
    qemu_printf("Current date and time: %s\n", datetime_to_str(&current_datetime));

#if GUI_MODE
    vesa_init();
    compositor_init();
    mouse_init();

    // Terminal
    window_t * red_w = window_create(get_super_window(), 20, 300, 750, 450, WINDOW_NORMAL, "window_black");
    window_add_title_bar(red_w);
    window_add_close_button(red_w);
    window_add_minimize_button(red_w);
    window_add_maximize_button(red_w);
    canvas_t canvas_red = canvas_create(red_w->width, red_w->height, red_w->frame_buffer);
    set_font_color(VESA_COLOR_BLACK + 1);
    draw_text(&canvas_red, "Terminal", 1, 42);
    window_add_round_corner(red_w);
    blend_windows(red_w);

    // File Browser
    window_t * green_w = window_create(get_super_window(), 100, 100, 400, 400, WINDOW_NORMAL, "window_classic");
    window_add_title_bar(green_w);
    qemu_printf("Adding close buttons\n");
    window_add_close_button(green_w);
    qemu_printf("Adding minimize buttons\n");
    window_add_minimize_button(green_w);
    qemu_printf("Adding maximize buttons\n");
    window_add_maximize_button(green_w);
    canvas_t canvas_green = canvas_create(green_w->width, green_w->height, green_w->frame_buffer);
    set_font_color(VESA_COLOR_BLACK + 1);
    draw_text(&canvas_green, "File browser", 1, 19);
    window_add_round_corner(green_w);
    blend_windows(green_w);

    // Top desktop bar
    window_t * bar_w = window_create(get_super_window(), 0, 0, 1024, 25, WINDOW_DESKTOP_BAR, "desktop_bar");
    canvas_t canvas_bar = canvas_create(bar_w->width, bar_w->height, bar_w->frame_buffer);
    set_font_color(VESA_COLOR_BLACK+1);
    draw_text(&canvas_bar, get_current_datetime_str(), 1, 115);
    blend_windows(bar_w);
    display_all_window();
    video_memory_update(NULL, 0);
    print_windows_depth();
#endif

#if NETWORK_MODE
    qemu_printf("Initializing network driver...\n");
    rtl8139_init();
    arp_init();

    uint8_t mac_addr[6];
    mac_addr[0] = 0xAA;
    mac_addr[1] = 0xBB;
    mac_addr[2] = 0xCC;
    mac_addr[3] = 0xDD;
    mac_addr[4] = 0xEE;
    mac_addr[5] = 0xFF;
    get_mac_addr(mac_addr);

    uint8_t ip_addr[6];
    ip_addr[0] = 10;
    ip_addr[1] = 0;
    ip_addr[2] = 2;
    ip_addr[3] = 15;
    char * str = "This is a message sent from simpleos to an Ubuntu host";
    //ethernet_send_packet(mac_addr, str, strlen(str), 0x0021);
    //ip_send_packet(ip_addr, str, strlen(str));

    // Ask QEMU's dhcp server for an IP address
    dhcp_discover();
    // If IP is ready, send a UDP message from simpleos to a host machine running Ubuntu
    while(gethostaddr(mac_addr) == 0);
    udp_send_packet(ip_addr, 1234, 1153, str, strlen(str));
    for(;;);
#endif

}
