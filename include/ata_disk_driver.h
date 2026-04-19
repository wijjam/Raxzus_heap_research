#ifndef ATA_DISK_DRIVER_H
#define ATA_DISK_DRTIVER_H

#include <stdint.h>

#define ATA_DATA        0x1F0   // This is were the data is being sent through, 16 bit at a time.
#define Error_register  0x1F1 // This is the error register, when a error occurs we read this to se what error happend
#define ATA_SECTOR_COUNT 0x1F2   // This is a 8-bit register that controls how many sectors we want to read or write. If we enter 0 we get back 256 sectors.
#define ATA_LBA_LOW     0x1F3 // LBA The Logic Block Addreessing A 28 bit LBA adress can adress up to 128GB. Holds bits 0 through 7. if 0x00123456 you would write 0x56 here.
#define ATA_LBA_MID     0x1F4 // LBA The Logic Block Addreessing A 28 bit LBA adress can adress up to 128GB. Holds bits 8 through 15. If you want 0x00123456, 0x34 would be this.
#define ATA_LBA_HIGH    0x1F5 // LBA The Logic Block Addreessing A 28 bit LBA adress can adress up to 128GB. Holds bits 16 through 23. if you want 0x00123456, 0x12 would be this.
#define ATA_DRIVE       0x1F6 // Which drive you are talking to master or slave, and it holds the top 4 bits of the LBA address (bits 24 through 27) 1 1 1 D L3 L2 L1 L0
#define ATA_COMMAND     0x1F7  // When you write to 0x1F7 you are sending a command to the controller.
#define ATA_STATUS      0x1F7  // When you read from 0x1F7 you get the status back.

#define ATA_CMD_READ    0x20    // When you write 0x20 to the command port you are telling the controller, to execute a read sectors command using the values loaded in the registers.
#define ATA_CMD_WRITE   0x30    // means write sector

#define ATA_STATUS_BSY  0x80  // Busy
#define ATA_STATUS_DRQ  0x08  // Data ready


#define ATA_MASTER 0xE0
#define ATA_SLAVE  0xF0

void ata_write_sector(uint32_t lba, uint8_t sector_count, uint16_t* buffer);
void ata_read_sector(uint32_t lba, uint8_t sector_count, uint16_t* buffer);

#endif