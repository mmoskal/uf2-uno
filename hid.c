#include "uf2uno.h"
#include "uf2hid.h"

uint8_t hidBuffer[64];

static void hidSendCore(void) {
    while (!Endpoint_IsINReady()) {
        // wait
    }
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
    if (hidBuffer[0] == 63)
        hidSendCore();
}

void hidWrite(const void *ptr0, uint8_t size) {
    const uint8_t *ptr = ptr0;
    while (size--) {
        hidBuffer[++hidBuffer[0]] = *ptr++;
        checkFlush();
    }
}

void hidWrite_P(const void *ptr0, uint8_t size) {
    const uint8_t *ptr = ptr0;
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

            for (uint8_t i = 0; i < sizeof(data); ++i)
                data[i] = Endpoint_Read_8();

            if (data[0] & 0x80) {
                data[0] &= 63;
                for (uint8_t i = 0; i < data[0]; ++i) {
                    RingBuffer_Insert(&USBtoUSART_Buffer, data[i + 1]);
                }
            } else {
                struct HF2_Command *cmd = (void*)(data + 1);
                uint32_t tmp = cmd->tag; // implicit zero status
                if (cmd->command_id == HF2_CMD_BININFO) {
                    hidWrite(&tmp, 4);
                    tmp = HF2_MODE_BOOTLOADER;
                    hidWrite(&tmp, 4);
                    tmp = SPM_PAGESIZE;
                    hidWrite(&tmp, 4);
                    tmp = 32 * (1024 / SPM_PAGESIZE);
                    hidWrite(&tmp, 4);
                    tmp = 63;
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

        /* Finalize the stream transfer to send the last packet */
        Endpoint_ClearOUT();
    }

    Endpoint_SelectEndpoint(HID_IN_EPADDR);

    /* Check to see if the host is ready to accept another packet */
    if (needsFlush && Endpoint_IsINReady()) {
        uint8_t len = RingBuffer_GetCount(&USARTtoUSB_Buffer);
        needsFlush = 0;

        if (len > 0) {
            if (len > 63)
                len = 63;
            hidBuffer[0] = 0x80 | len;
            for (uint8_t i = 0; i < len; ++i)
                hidBuffer[i + 1] = RingBuffer_Remove(&USARTtoUSB_Buffer);
            hidSendCore();
        }
    }
}
