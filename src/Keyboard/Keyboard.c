/*
             LUFA Library
     Copyright (C) Dean Camera, 2018.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2018  Dean Camera (dean [at] fourwalledcubicle [dot] com)

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
 *  Main source file for the Keyboard demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "Keyboard.h"

#define ALL_OFF  (PORTD |= 0b01100000) //turn off leds
#define LED_EN  (DDRD  |= 0b01100000) // enable leds as output
#define HWBIN_EN (DDRD  &= 0b01111111) // make hwb an input

const uint8_t PROGMEM secret[] =
{
  HID_KEYBOARD_SC_CAPS_LOCK,
  HID_KEYBOARD_SC_H, HID_KEYBOARD_SC_E, HID_KEYBOARD_SC_L, HID_KEYBOARD_SC_L,
  HID_KEYBOARD_SC_O,
  HID_KEYBOARD_SC_CAPS_LOCK,
  HID_KEYBOARD_SC_ENTER,
  HID_KEYBOARD_SC_ESCAPE, /* I will use this for separation */
  HID_KEYBOARD_SC_H, HID_KEYBOARD_SC_E, HID_KEYBOARD_SC_L, HID_KEYBOARD_MODIFIER_LEFTSHIFT,
  HID_KEYBOARD_SC_L, HID_KEYBOARD_MODIFIER_LEFTSHIFT,
  HID_KEYBOARD_SC_O,
  HID_KEYBOARD_SC_ENTER
};

/** Circular buffer to hold data from the keystorage before it is sent to the device via the HID. */
static RingBuffer_t Secret2USB_Buffer;

/** Underlying data buffer for \ref Secret2USB_Buffer, where the stored bytes are located. */
static uint8_t      Secret2USB_Buffer_Data[128];

/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Keyboard_HID_Interface =
	{
		.Config =
			{
				.InterfaceNumber              = INTERFACE_ID_Keyboard,
				.ReportINEndpoint             =
					{
						.Address              = KEYBOARD_EPADDR,
						.Size                 = KEYBOARD_EPSIZE,
						.Banks                = 1,
					},
				.PrevReportINBuffer           = PrevKeyboardHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevKeyboardHIDReportBuffer),
			},
	};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	RingBuffer_InitBuffer(&Secret2USB_Buffer, Secret2USB_Buffer_Data, sizeof(Secret2USB_Buffer_Data));
	uint16_t SecLen = sizeof(secret);
	uint16_t SecCnt = SecLen;

	while (SecCnt--)
		RingBuffer_Insert(&Secret2USB_Buffer, secret[SecLen-SecCnt]);

	GlobalInterruptEnable();

	for (;;)
	{
		HID_Device_USBTask(&Keyboard_HID_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware()
{
#if (ARCH == ARCH_AVR8)
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
#endif

	/* Hardware Initialization */
	ALL_OFF;
	LED_EN;
	HWBIN_EN;
	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);

	USB_Device_EnableSOFEvents();
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
	USB_KeyboardReport_Data_t* KeyboardReport = (USB_KeyboardReport_Data_t*)ReportData;
	*ReportSize = sizeof(USB_KeyboardReport_Data_t);
	uint8_t UsedKeyCodes = 0;
	if (RingBuffer_IsEmpty(&Secret2USB_Buffer))
	{
		led_red_heartbeat();
	} else {
		RingBuffer_Remove(&Secret2USB_Buffer); // Consumer
	}
	*ReportSize = sizeof(USB_KeyboardReport_Data_t);
	return false;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
}

void led_blue_heartbeat()
{
	_delay_ms(2500);
	led_blue_toggle();
	_delay_ms(50);
	led_blue_toggle();
}

void led_blue_fast_heartbeat()
{
	_delay_ms(500);
	led_blue_toggle();
	_delay_ms(10);
	led_blue_toggle();
}

void led_red_heartbeat()
{
	_delay_ms(2500);
	led_red_toggle();
	_delay_ms(50);
	led_red_toggle();
}

void led_blue(char on)
{
	if (on) PORTD &= 0b11011111;
	else    PORTD |= 0b00100000;
}

void led_blue_toggle()
{
	PORTD ^= 0b00100000;
}

void led_red(char on)
{
	if (on) PORTD &= 0b10111111;
	else    PORTD |= 0b01000000;
}

void led_red_toggle()
{
	PORTD ^= 0b01000000;
}

char hwb_is_pressed()
{
	return !(PIND & 0b10000000);
}