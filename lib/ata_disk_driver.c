#include "../include/ata_disk_driver.h"
#include "../include/vga.h"
uint8_t set_master_slave_super_lba(uint32_t lba, uint8_t master_slave); // Sets the master or slave and also finds out the top lba
uint8_t get_low_lba(uint32_t lba);
uint8_t get_mid_lba(uint32_t lba);
uint8_t get_high_lba(uint32_t lba);



void ata_wait_ready() {
    // Keep reading status port until BSY clears
    while ((inb(ATA_STATUS) & 0x80)) {}

}

void ata_read_sector(uint32_t lba, uint8_t sector_count, uint16_t* buffer) {
    ata_wait_ready();
    
    outb(ATA_DRIVE, set_master_slave_super_lba(lba, ATA_MASTER)); // Select master drive, LBA mode
    outb(ATA_SECTOR_COUNT, sector_count);                      // Read 1 sector
    outb(ATA_LBA_LOW,  get_low_lba(lba));
    outb(ATA_LBA_MID,  get_mid_lba(lba));
    outb(ATA_LBA_HIGH, get_high_lba(lba));
    while(inb(0x1F7) & 0x80); // wait BSY clear
    outb(ATA_COMMAND, ATA_CMD_READ);
    
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);


    ata_wait_ready();
    
    // Now read 256 uint16_t words from data port into buffer


    for (int s = 0; s < sector_count; s++) {

        ata_wait_ready();


        while (!(inb(ATA_STATUS) & 0x08)) {} // Checking the DRQ if we are ready to transfer data
        for (int i = 0; i < 256; i++) {
            buffer[s*256+i] = inw(ATA_DATA);
        }
    }

}



void ata_write_sector(uint32_t lba, uint8_t sector_count, uint16_t* buffer) {
    ata_wait_ready();
    
    outb(ATA_DRIVE, set_master_slave_super_lba(lba, ATA_MASTER)); // Select master drive, LBA mode
    outb(ATA_SECTOR_COUNT, sector_count);                      // Read 1 sector
    outb(ATA_LBA_LOW,  get_low_lba(lba));
    outb(ATA_LBA_MID,  get_mid_lba(lba));
    outb(ATA_LBA_HIGH, get_high_lba(lba));
    while(inb(0x1F7) & 0x80); // wait BSY clear
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);
    
    ata_wait_ready();
    
    // Now read 256 uint16_t words from data port into buffer


    for (int s = 0; s < sector_count; s++) {

        ata_wait_ready();

        while (!(inb(ATA_STATUS) & 0x08)) {} // Checking the DRQ if we are ready to transfer data
        for (int i = 0; i < 256; i++) {
            outw(ATA_DATA, buffer[s*256+i]);
        }
    }

    outb(ATA_COMMAND, 0xE7);
    ata_wait_ready();

}








// ======================================= HELPER FUNCTIONS ====================================================================


uint8_t set_master_slave_super_lba(uint32_t lba, uint8_t drive_select) {
   return drive_select | ((lba >> 24) & 0x0F);
}

uint8_t get_low_lba(uint32_t lba) {
    return (lba)      & 0xFF;
}

uint8_t get_mid_lba(uint32_t lba) {
    return (lba >> 8) & 0xFF;
}

uint8_t get_high_lba(uint32_t lba) {
    return (lba >> 16) & 0xFF;
}