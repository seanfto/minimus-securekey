#include "HWif.h"

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
