#ifndef ATA_DRIVER_H
#define ATA_DRIVER_H
#include <system.h>
#include <paging.h>
#include <vfs.h>

extern page_directory_t * kpage_dir;


/*
prdt_t 结构体：

buffer_phys：一个32位的物理地址，可能代表缓冲区的物理地址。
transfer_size：一个16位的传输大小，表示要传输的数据量。
mark_end：一个16位的标记，可能用于标记传输结束或用作其他用途。


ata_dev_t 结构体：

data、error、sector_count、sector_num、cylinder_low、cylinder_high、drive、command、status、control：这些都是16位的寄存器，用于存储IDE设备的信息和状态。其中，某些寄存器（例如 sector_num）在IDE设备中通常用于存储扇区号。
lba_lo、lba_mid、lba_high：这些是低、中、高3个16位的块地址寄存器，它们一起可以用于存储逻辑块地址（LBA）。
slave：一个整数字段，可能用于标记设备是主设备还是从设备。
bar4：一个32位的寄存器，可能是用于存储某个基址寄存器的值。
BMR_COMMAND、BMR_prdt、BMR_STATUS：这些可能是特定于该IDE设备的命令或状态寄存器。
prdt：一个指向 prdt_t 结构体的指针，可能用于存储传输描述符的信息。
prdt_phys、mem_buffer、mem_buffer_phys：这些是8位的指针，可能用于指向物理内存中的数据缓冲区或其他缓冲区。
mountpoint：一个32字符的字符串，可能用于标记设备的挂载点。
*/
typedef struct prdt {
	uint32_t buffer_phys;
	uint16_t transfer_size;
	uint16_t mark_end;
}__attribute__((packed)) prdt_t;



typedef struct ata_dev {
	// A list of register address
	uint16_t data;
	uint16_t error;
	uint16_t sector_count;

	union {
		uint16_t sector_num;
		uint16_t lba_lo ;
	};
	union {
		uint16_t cylinder_low;
		uint16_t lba_mid ;
	};
	union {
		uint16_t cylinder_high;
		uint16_t lba_high;
	};
	union {
		uint16_t drive;
		uint16_t head;
	};
	union {
		uint16_t command;
		uint16_t status;
	};
	union {
		uint16_t control;
		uint16_t alt_status;
	};

	int slave;
	uint32_t bar4;
	uint32_t BMR_COMMAND;
	uint32_t BMR_prdt;
	uint32_t BMR_STATUS;


	prdt_t * prdt;
	uint8_t * prdt_phys;

	uint8_t * mem_buffer;
	uint8_t * mem_buffer_phys;

	char mountpoint[32];
}__attribute__((packed)) ata_dev_t;


// ATA PCI info
#define ATA_VENDOR_ID 0x8086
#define ATA_DEVICE_ID 0x7010

// Control reg
#define CONTROL_STOP_INTERRUPT 0x2
#define CONTROL_SOFTWARE_RESET 0x4
#define CONTROL_HIGH_ORDER_BYTE 0x80
#define CONTROL_ZERO 0x00


// Command reg
#define COMMAND_IDENTIFY 0xEC
#define COMMAND_DMA_READ 0xC8
#define ATA_CMD_READ_PIO 0x20

// Status reg
#define STATUS_ERR 0x0
#define STATUS_DRQ 0x8
#define STATUS_SRV 0x10
#define STATUS_DF  0x20
#define STATUS_RDY 0x40
#define STATUS_BSY 0x80

// Bus Master Reg Command
#define BMR_COMMAND_DMA_START 0x1
#define BMR_COMMAND_DMA_STOP 0x0
#define BMR_COMMAND_READ 0x8
#define BMR_STATUS_INT 0x4
#define BMR_STATUS_ERR 0x2


// Prdt
#define SECTOR_SIZE 512
#define MARK_END 0x8000

void io_wait(ata_dev_t * dev);

void software_reset(ata_dev_t * dev);

void ata_handler(register_t * reg);

void ata_open(vfs_node_t * node, uint32_t flags);

void ata_close(vfs_node_t * node);

uint32_t ata_read(vfs_node_t * node, uint32_t offset, uint32_t size, char * buf);

uint32_t ata_write(vfs_node_t * node, uint32_t offset, uint32_t size, char * buf);

void ata_write_sector(ata_dev_t * dev, uint32_t lba, char * buf);

char * ata_read_sector(ata_dev_t * dev, uint32_t lba);

vfs_node_t * create_ata_device(ata_dev_t * dev);

void ata_device_init(ata_dev_t * dev, int primary);

void ata_device_detect(ata_dev_t * dev, int primary);

void ata_init();

#endif
