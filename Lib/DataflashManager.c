#define INCLUDE_FROM_DATAFLASHMANAGER_C
#include "DataflashManager.h"

const uint8_t uf2magic[] PROGMEM = "UF2\nWQ]\x9E";

// 32k flash, 256b payload, +20% overhead
#define MAX_BLOCKS ((32 * (1024 / 256)) * 5 / 4)

#if MAX_BLOCKS >= 255
#error too high max blocks?
#endif

#define PAGE_SHIFT 7

#if SPM_PAGESIZE != 128
#error unsupported page size
#endif

#if (SPM_PAGESIZE >> PAGE_SHIFT) != 1
#error unsupported page size
#endif

#define BLOCKS_PAR_PAGE (SPM_PAGESIZE / MASS_STORAGE_IO_EPSIZE)

#define CRC_EOP 0x20          // 'SPACE'
#define STK_PROG_PAGE 0x64    // 'd'
#define STK_LOAD_ADDRESS 0x55 // 'U'

static uint16_t numBlocksWritten;
static uint8_t numBlocks;
static uint8_t writtenMask[MAX_BLOCKS / 8];

static void targetReset(void) {
    AVR_RESET_LINE_PORT &= ~AVR_RESET_LINE_MASK;
    _delay_ms(10);
    AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
}

extern volatile uint8_t recv_STK_OK;

static void finishPacket(void) {
    recv_STK_OK = 0;
    Serial_SendByte(CRC_EOP);

    logChar('P');

    wdt_enable(WDTO_250MS);

    while (!recv_STK_OK)
        ;

    // we've sent some packets
    // if we see no action in next 1S, the optiboot is going to stop anyway, so we may as well reset
    // ourselves
    wdt_enable(WDTO_1S);
}

/** Writes blocks (OS blocks, not Dataflash pages) to the storage medium, the board Dataflash IC(s),
 * from
 *  the pre-selected data OUT endpoint. This routine reads in OS sized blocks from the endpoint and
 * writes
 *  them to the Dataflash in Dataflash page sized blocks.
 *
 *  \param[in] MSInterfaceInfo  Pointer to a structure containing a Mass Storage Class configuration
 * and state
 *  \param[in] BlockAddress  Data block starting address for the write sequence
 *  \param[in] TotalBlocks   Number of blocks of data to write
 */
void DataflashManager_WriteBlocks(USB_ClassInfo_MS_Device_t *const MSInterfaceInfo,
                                  const uint32_t BlockAddress, uint16_t TotalBlocks) {
    uint8_t buf[MASS_STORAGE_IO_EPSIZE];
    uint8_t isUF2 = 0;
    uint16_t addr = 0;
    uint8_t i;
    uint8_t numPages = 0;

    /* Wait until endpoint is ready before continuing */
    if (Endpoint_WaitUntilReady())
        return;

    while (TotalBlocks) {
        logChar('w');
        
        for (uint8_t bufno = 0; bufno < 512 / MASS_STORAGE_IO_EPSIZE; ++bufno) {
            /* Check if the endpoint is currently empty */
            if (!(Endpoint_IsReadWriteAllowed())) {
                /* Clear the current endpoint bank */
                Endpoint_ClearOUT();

                /* Wait until the host has sent another packet */
                if (Endpoint_WaitUntilReady())
                    return;
            }

            for (i = 0; i < MASS_STORAGE_IO_EPSIZE; ++i)
                buf[i] = Endpoint_Read_8();

            /* Check if the current command is being aborted by the host */
            if (MSInterfaceInfo->State.IsMassStoreReset)
                return;

            if (bufno == 0) {
                isUF2 = 1;
                for (i = 0; i < 8; ++i) {
                    if (buf[i] != pgm_read_byte(uf2magic + i)) {
                        isUF2 = 0;
                        break;
                    }
                }
                if (buf[8] & 1)
                    isUF2 = 0; // UF2 do not flash flag

                addr = *(uint16_t *)(buf + 12);
                // STK500 address is in words, not bytes
                addr >>= 1;
            }

            if (!isUF2)
                continue;

            if (bufno == 1) {
                numPages = *(uint16_t *)(buf + 0) >> PAGE_SHIFT;

                uint32_t tmp = *(uint32_t *)(buf + 24 - 16);
                if (tmp != numBlocks) {
                    if (tmp > MAX_BLOCKS || numBlocks)
                        numBlocks = 0xff;
                    else
                        numBlocks = tmp;
                }
                if (!numBlocks)
                    numBlocks = 0xff;
                tmp = *(uint32_t *)(buf + 20 - 16);
                if (tmp < MAX_BLOCKS) {
                    uint8_t x = tmp;
                    if ((writtenMask[x >> 3] & (1 << (x & 7))) == 0) {
                        writtenMask[x >> 3] |= (1 << (x & 7));
                        numBlocksWritten++;
                    }
                }
                continue;
            }

            if (numBlocks == 0) {
                logChar('R');
                targetReset();
                _delay_ms(600); // so it stops blinking
            }

            if (!numPages)
                continue;

            if (((bufno - 2) & (BLOCKS_PAR_PAGE - 1)) == 0) {
                Serial_SendByte(STK_LOAD_ADDRESS);
                Serial_SendByte(addr & 0xff); // little endian
                Serial_SendByte(addr >> 8);
                addr += SPM_PAGESIZE >> 1;
                finishPacket();

                Serial_SendByte(STK_PROG_PAGE);
                Serial_SendByte(SPM_PAGESIZE >> 8); // and big endian here, go figure
                Serial_SendByte(SPM_PAGESIZE & 0xff);
                Serial_SendByte('F');
            }

            for (i = 0; i < MASS_STORAGE_IO_EPSIZE; ++i)
                Serial_SendByte(buf[i]);

            if (((bufno - 2) & (BLOCKS_PAR_PAGE - 1)) == BLOCKS_PAR_PAGE - 1) {
                finishPacket();
                numPages--;
            }
        }

        /* Decrement the blocks remaining counter */
        TotalBlocks--;
    }

    /* If the endpoint is empty, clear it ready for the next packet from the host */
    if (!(Endpoint_IsReadWriteAllowed()))
        Endpoint_ClearOUT();
    
    if (numBlocks && numBlocks != 0xff && numBlocksWritten >= numBlocks) {
        numBlocksWritten = 0;
        logChar('0'); 
        // reboot target
        //Serial_SendByte('Q');
        //Serial_SendByte(CRC_EOP);
        targetReset(); // we do a slow reboot of target, so we manage to reset ourselves before it wakes up
        // don't wait for response
        // and reboot ourselves soon
        wdt_enable(WDTO_60MS);
    }
}

typedef struct {
    uint8_t JumpInstruction[3];
    uint8_t OEMInfo[8];
    uint16_t SectorSize;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FATCopies;
    uint16_t RootDirectoryEntries;
    uint16_t TotalSectors16;
    uint8_t MediaDescriptor;
    uint16_t SectorsPerFAT;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t TotalSectors32;
    uint8_t PhysicalDriveNum;
    uint8_t Reserved;
    uint8_t ExtendedBootSig;
    uint32_t VolumeSerialNumber;
    char VolumeLabel[11];
    uint8_t FilesystemIdentifier[8];
} __attribute__((packed)) FAT_BootBlock;

typedef struct {
    char name[8];
    char ext[3];
    uint8_t attrs;
    uint8_t reserved;
    uint8_t createTimeFine;
    uint16_t createTime;
    uint16_t createDate;
    uint16_t lastAccessDate;
    uint16_t highStartCluster;
    uint16_t updateTime;
    uint16_t updateDate;
    uint16_t startCluster;
    uint32_t size;
} __attribute__((packed)) DirEntry;

struct TextFile {
    const char name[11];
};

#define STR0(x) #x
#define STR(x) STR0(x)

const char infoUf2Name[] PROGMEM = "INFO_UF2TXT";
const char infoUf2File[] PROGMEM = //
    "UF2 Bootloader " UF2_VERSION "\r\n"
    "Model: " PRODUCT_NAME "\r\n"
    "Board-ID: " BOARD_ID "\r\n";

const char indexName[] PROGMEM = "INDEX   HTM";
const char indexFile[] PROGMEM = //
    "<!doctype html>\n"
    "<html>"
    "<body>"
    "<script>\n"
    "location.replace(\"" INDEX_URL "\");\n"
    "</script>"
    "</body>"
    "</html>\n";

// yeah... seriously - avr-gcc has trouble with arrays
static const char *getFileData(uint8_t idx, uint8_t tp) {
    switch (idx) {
    case 0:
        return tp ? infoUf2File : infoUf2Name;
    case 1:
        return tp ? indexFile : indexName;
    default:
        return 0;
    }
}

#define NUM_INFO 2

#define NUM_FAT_BLOCKS VIRTUAL_MEMORY_BLOCKS

#define RESERVED_SECTORS 1
#define ROOT_DIR_SECTORS 4
#define SECTORS_PER_FAT ((NUM_FAT_BLOCKS * 2 + 511) / 512)

#define START_FAT0 RESERVED_SECTORS
#define START_FAT1 (START_FAT0 + SECTORS_PER_FAT)
#define START_ROOTDIR (START_FAT1 + SECTORS_PER_FAT)
#define START_CLUSTERS (START_ROOTDIR + ROOT_DIR_SECTORS)

static const FAT_BootBlock BootBlock PROGMEM = {
    .JumpInstruction = {0xeb, 0x3c, 0x90},
    .OEMInfo = "UF2 UF2 ",
    .SectorSize = 512,
    .SectorsPerCluster = 1,
    .ReservedSectors = RESERVED_SECTORS,
    .FATCopies = 2,
    .RootDirectoryEntries = (ROOT_DIR_SECTORS * 512 / 32),
    .TotalSectors16 = NUM_FAT_BLOCKS - 2,
    .MediaDescriptor = 0xF8,
    .SectorsPerFAT = SECTORS_PER_FAT,
    .SectorsPerTrack = 1,
    .Heads = 1,
    .ExtendedBootSig = 0x29,
    .VolumeSerialNumber = 0x00420042,
    .VolumeLabel = VOLUME_LABEL,
    .FilesystemIdentifier = "FAT16   ",
};

static void write_byte(uint8_t b) { Endpoint_Write_Stream_LE(&b, 1, NULL); }

static void write_from_data(const void *src, uint16_t count) {
    Endpoint_Write_Stream_LE(src, count, NULL);
}

static void write_from_pgm(const void *src, uint16_t count) {
    while (count-- > 0)
        write_byte(pgm_read_byte(src++));
}

static void write_zeros(uint16_t count) { Endpoint_Null_Stream(count, NULL); }

static void padded_memcpy(char *dst, const char *src, int len) {
    for (int i = 0; i < len; ++i) {
        int ch = pgm_read_byte(src);
        if (ch) {
            *dst = ch;
            src++;
        } else {
            *dst = ' ';
        }
        dst++;
    }
}

/** Reads blocks (OS blocks, not Dataflash pages) from the storage medium, the board Dataflash
 * IC(s), into
 *  the pre-selected data IN endpoint. This routine reads in Dataflash page sized blocks from the
 * Dataflash
 *  and writes them in OS sized blocks to the endpoint.
 *
 *  \param[in] MSInterfaceInfo  Pointer to a structure containing a Mass Storage Class configuration
 * and state
 *  \param[in] BlockAddress  Data block starting address for the read sequence
 *  \param[in] TotalBlocks   Number of blocks of data to read
 */
void DataflashManager_ReadBlocks(USB_ClassInfo_MS_Device_t *const MSInterfaceInfo,
                                 uint32_t block_no, uint16_t TotalBlocks) {

    uint16_t i, sectionIdx;

    /* Wait until endpoint is ready before continuing */
    if (Endpoint_WaitUntilReady())
        return;

    logChar('B');

    while (TotalBlocks) {
        /* Check if the endpoint is currently full */
        if (!(Endpoint_IsReadWriteAllowed())) {
            /* Clear the endpoint bank to send its contents to the host */
            Endpoint_ClearIN();

            /* Wait until the endpoint is ready for more data */
            if (Endpoint_WaitUntilReady())
                return;
        }

        sectionIdx = block_no;

        if (block_no == 0) {
            write_from_pgm(&BootBlock, sizeof(BootBlock));
            write_zeros(512 - sizeof(BootBlock) - 2);
            write_byte(0x55);
            write_byte(0xaa);
        } else if (block_no < START_ROOTDIR) {
            sectionIdx -= START_FAT0;
            // logval("sidx", sectionIdx);
            if (sectionIdx >= SECTORS_PER_FAT)
                sectionIdx -= SECTORS_PER_FAT;
            i = 0;
            if (sectionIdx == 0) {
                write_byte(0xf0);
                for (i = 1; i < 4 + NUM_INFO * 2; ++i)
                    write_byte(0xff);
            }
            write_zeros(512 - i);
        } else if (block_no < START_CLUSTERS) {
            sectionIdx -= START_ROOTDIR;
            if (sectionIdx == 0) {
                DirEntry d;
                memset(&d, 0, sizeof(d));
                padded_memcpy(d.name, BootBlock.VolumeLabel, 11);
                d.attrs = 0x28;
                write_from_data(&d, sizeof(d));
                for (i = 0; i < NUM_INFO; ++i) {
                    memset(&d, 0, sizeof(d));
                    d.size = strlen_P(getFileData(i, 1));
                    d.startCluster = i + 2;
                    padded_memcpy(d.name, getFileData(i, 0), 11);
                    write_from_data(&d, sizeof(d));
                }
                write_zeros(512 - (NUM_INFO + 1) * 32);
            } else {
                write_zeros(512);
            }
        } else {
            sectionIdx -= START_CLUSTERS;
            if (sectionIdx < NUM_INFO) {
                i = strlen_P(getFileData(sectionIdx, 1));
                write_from_pgm(getFileData(sectionIdx, 1), i);
                write_zeros(512 - i);
            } else {
                write_zeros(512);
            }
        }

        /* Check if the current command is being aborted by the host */
        if (MSInterfaceInfo->State.IsMassStoreReset)
            return;

        /* Decrement the blocks remaining counter */
        TotalBlocks--;
        block_no++;
    }

    /* If the endpoint is full, send its contents to the host */
    if (!(Endpoint_IsReadWriteAllowed()))
        Endpoint_ClearIN();
}
