/** \file
 *
 *  Header file for HWif.c.
 */

#ifndef _HWIF_H_
#define _HWIF_H_

	/* Includes: */
		#include <avr/io.h>

	/* Macros: */


	/* Function Prototypes: */
		void led_blue_heartbeat(void);
		void led_blue_fast_heartbeat(void);

		void led_red_heartbeat(void);

		void led_blue(char);
		void led_blue_toggle(void);

		void led_red(char);
		void led_red_toggle(void);

		char hwb_is_pressed(void);
#endif
