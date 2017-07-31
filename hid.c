#include "uf2uno.h"
#include "uf2hid.h"

uint8_t hidBuffer[HID_IO_EPSIZE];

static void hidSendCore(void) {
    Endpoint_SelectEndpoint(HID_IN_EPADDR);
    Endpoint_WaitUntilReady();
    while (!Endpoint_IsINReady())
        ;
    for (uint8_t i = 0; i < sizeof(hidBuffer); ++i)
        Endpoint_Write_8(hidBuffer[i]);
    Endpoint_ClearIN();
    hidBuffer[0] = 0;
}

void hidSendReply(void) {
    hidBuffer[0] |= 0x40;
    hidSendCore();
}

static void checkFlush(void) {
    if (hidBuffer[0] == HID_IO_EPSIZE - 1)
        hidSendCore();
}

void hidWrite(const void *ptr, uint8_t size) {
    while (size--) {
        hidBuffer[++hidBuffer[0]] = *(uint8_t *)ptr++;
        checkFlush();
    }
}

void hidWrite_P(const void *ptr, uint8_t size) {
    while (size--) {
        hidBuffer[++hidBuffer[0]] = pgm_read_byte(ptr++);
        checkFlush();
    }
}

struct HF2_Command {
    uint32_t command_id;
    uint16_t tag;
    uint8_t reserved0;
    uint8_t reserved1;
    uint8_t data[0];
};

extern const char infoUf2File[] PROGMEM;

void HID_Task(void) {
    /* Device must be connected and configured for the task to run */
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    Endpoint_SelectEndpoint(HID_OUT_EPADDR);

    /* Check to see if a packet has been sent from the host */
    if (Endpoint_IsOUTReceived()) {
        uint8_t data[64];

        /* Check to see if the packet contains data */
        if (Endpoint_IsReadWriteAllowed()) {

            for (uint8_t i = 0; i < HID_IO_EPSIZE; ++i)
                data[i] = Endpoint_Read_8();

            Endpoint_ClearOUT();

            if (data[0] & 0x80) {
                data[0] &= 63;
                for (uint8_t i = 0; i < data[0]; ++i) {
                    RingBuffer_Insert(&USBtoUSART_Buffer, data[i + 1]);
                }
            } else {
                struct HF2_Command *cmd = (void *)(data + 1);
                uint32_t tmp = cmd->tag; // implicit zero status
                if (cmd->command_id == HF2_CMD_BININFO) {
                    hidWrite(&tmp, 4);
                    tmp = HF2_MODE_BOOTLOADER;
                    hidWrite(&tmp, 4);
                    tmp = SPM_PAGESIZE;
                    hidWrite(&tmp, 4);
                    tmp = 32 * (1024 / SPM_PAGESIZE);
                    hidWrite(&tmp, 4);
                    tmp = HID_IO_EPSIZE - 1;
                    hidWrite(&tmp, 4);
                } else if (cmd->command_id == HF2_CMD_INFO) {
                    hidWrite(&tmp, 4);
                    hidWrite_P(infoUf2File, strlen_P(infoUf2File));
                } else {
                    hidWrite(&tmp, 2);
                    tmp = 1; // command not understood
                    hidWrite(&tmp, 2);
                }
                hidSendReply();
            }
        }
    }

    Endpoint_SelectEndpoint(HID_IN_EPADDR);

    if (needsFlush && Endpoint_IsINReady()) {
        uint8_t len = RingBuffer_GetCount(&USARTtoUSB_Buffer);
        needsFlush = 0;

        if (len > 0) {
            if (len > HID_IO_EPSIZE - 1) {
                len = HID_IO_EPSIZE - 1;
                needsFlush = 1;
            }

            for (uint8_t i = 0; i < len; ++i)
                hidBuffer[i + 1] = RingBuffer_Remove(&USARTtoUSB_Buffer);

            hidBuffer[0] = 0x80 | len;
            hidSendCore();
        }
    }
}
