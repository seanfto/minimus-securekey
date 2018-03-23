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
 *  Main source file for the SecureKey demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "SecureKey.h"

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

/** Circular buffer to hold data from the host before it is REPL. */
static RingBuffer_t USBtoREPL_Buffer;

/** Underlying data buffer for \ref USBtoREPL_Buffer, where the stored bytes are located. */
static uint8_t      USBtoREPL_Buffer_Data[128];

/** Circular buffer to hold data from the REPL before it is sent to the host. */
static RingBuffer_t REPLtoUSB_Buffer;

/** Underlying data buffer for \ref REPLtoUSB_Buffer, where the stored bytes are located. */
static uint8_t      REPLtoUSB_Buffer_Data[128];

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

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber   = INTERFACE_ID_CDC_CCI,
				.DataINEndpoint           =
					{
						.Address          = CDC_TX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint =
					{
						.Address          = CDC_RX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.NotificationEndpoint =
					{
						.Address          = CDC_NOTIFICATION_EPADDR,
						.Size             = CDC_NOTIFICATION_EPSIZE,
						.Banks            = 1,
					},
			},
	};

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	RingBuffer_InitBuffer(&USBtoREPL_Buffer, USBtoREPL_Buffer_Data, sizeof(USBtoREPL_Buffer_Data));
	RingBuffer_InitBuffer(&REPLtoUSB_Buffer, REPLtoUSB_Buffer_Data, sizeof(REPLtoUSB_Buffer_Data));

	RingBuffer_InitBuffer(&Secret2USB_Buffer, Secret2USB_Buffer_Data, sizeof(Secret2USB_Buffer_Data));
	uint16_t SecLen = sizeof(secret);
	uint16_t SecCnt = SecLen;

	while (SecCnt--)
		RingBuffer_Insert(&Secret2USB_Buffer, secret[SecLen-SecCnt]);

	GlobalInterruptEnable();

	for (;;)
	{
		/* Echo all received data on the second CDC interface */
		int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
		if (!(ReceivedByte < 0))
			CDC_Device_SendByte(&VirtualSerial_CDC_Interface, (uint8_t)ReceivedByte);

		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
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
	ALL_OFF;
	led_red(1);

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);
	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

	USB_Device_EnableSOFEvents();

	if (ConfigSuccess )
	{
		led_red_toggle();
		led_blue(1);
	}
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
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
	char* ReportString = NULL;
	static bool ActionSent = false;

	if (hwb_is_pressed())
		ReportString = "HWB Pressed\r\n";
	else
		ActionSent = false;

	if ((ReportString != NULL) && (ActionSent == false))
	{
		ActionSent = true;
		/* Alternatively, without the stream: */
		CDC_Device_SendString(&VirtualSerial_CDC_Interface, ReportString);
	}

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

/** CDC class driver callback function the processing of changes to the virtual
 *  control lines sent from the host..
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo)
{
	/* You can get changes to the virtual CDC lines in this callback; a common
	   use-case is to use the Data Terminal Ready (DTR) flag to enable and
	   disable CDC communications in your application when set to avoid the
	   application blocking while waiting for a host to become ready and read
	   in the pending data from the USB endpoints.
	*/
	bool HostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR) != 0;
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
