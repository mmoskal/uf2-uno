/*
             LUFA Library
     Copyright (C) Dean Camera, 2017.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2017  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for VirtualSerial.c.
 */

#ifndef _VIRTUALSERIAL_H_
#define _VIRTUALSERIAL_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/power.h>
		#include <avr/interrupt.h>
		#include <string.h>

		#include "Descriptors.h"

		#include "Lib/SCSI.h"
		#include "Lib/DataflashManager.h"
		#include "Lib/LightweightRingBuff.h"
		#include "Config/AppConfig.h"

		#include <LUFA/Drivers/Board/LEDs.h>
		#include <LUFA/Drivers/Peripheral/Serial.h>
		#include <LUFA/Drivers/USB/USB.h>
		#include <LUFA/Platform/Platform.h>

	/* Macros: */
	/** LED mask for the library LED driver, to indicate TX activity. */
		#define LEDMASK_TX               LEDS_LED1

		/** LED mask for the library LED driver, to indicate RX activity. */
		#define LEDMASK_RX               LEDS_LED2
		
		#define LEDMASK_MSD               LEDMASK_TX
		
		/** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
		#define LEDMASK_ERROR            (LEDS_LED1|LEDS_LED2)
		
		#define LEDMASK_ENUMERATING LEDMASK_ERROR
		#define LEDMASK_NOTREADY LEDMASK_ERROR
		#define LEDMASK_READY 0

	/* Function Prototypes: */
		void SetupHardware(void);

		void EVENT_USB_Device_Connect(void);
		void EVENT_USB_Device_Disconnect(void);
		void EVENT_USB_Device_ConfigurationChanged(void);
		void EVENT_USB_Device_ControlRequest(void);
		void HID_Task(void);
		void logChar(char c);

/** Circular buffer to hold data from the host before it is sent to the device via the serial port. */
extern RingBuff_t USBtoUSART_Buffer;

/** Circular buffer to hold data from the serial port before it is sent to the host. */
extern RingBuff_t USARTtoUSB_Buffer;

extern uint8_t needsFlush;


#define UF2_VERSION "v0.0.0"
#define PRODUCT_NAME "Arduino Uno"
#define BOARD_ID "ATmega328p-Uno-r3"
#define VOLUME_LABEL "UNO BOOT"
#define INDEX_URL "https://pxt.io"

#endif

