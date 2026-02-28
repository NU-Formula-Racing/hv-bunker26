#pragma once

#define SET_VALUE 0x21

#define ENABLE_DTR(val) (val<<0)
#define ENABLE_RTS(val) (val<<1)

#define SET_LINE_CODING 0x20
#define SET_CONTROL_LINE_STATE 0x22

#define USB_CTRL_REQ_CDC_SET_LINE_CODING(ctrl_req_ptr, index, bitrate, cf, parity, bits) ({ \
    (ctrl_req_ptr)->bmRequestType = SET_VALUE; \
    (ctrl_req_ptr)->bRequest = SET_LINE_CODING; \
    (ctrl_req_ptr)->wValue = 0; \
    (ctrl_req_ptr)->wIndex = (index); \
    (ctrl_req_ptr)->wLength = (7); \
})

#define USB_CTRL_REQ_CDC_SET_CONTROL_LINE_STATE(ctrl_req_ptr, index, dtr, rts) ({ \
    (ctrl_req_ptr)->bmRequestType = SET_VALUE; \
    (ctrl_req_ptr)->bRequest = SET_CONTROL_LINE_STATE; \
    (ctrl_req_ptr)->wValue = ENABLE_DTR(dtr) | ENABLE_RTS(rts); \
    (ctrl_req_ptr)->wIndex = (index); \
    (ctrl_req_ptr)->wLength = (0); \
})

typedef struct {
    uint32_t dwDTERate;
    uint8_t bCharFormat;
    uint8_t bParityType;
    uint8_t bDataBits;
} line_coding_t;
