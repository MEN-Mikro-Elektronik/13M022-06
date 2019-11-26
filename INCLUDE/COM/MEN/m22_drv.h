/***********************  I	n c	l u	d e	 -	F i	l e	 ************************
 *
 *		   Name: m22_driver.h
 *
 *		 Author: uf
 *
 *	Description: Header	file for M22 driver	modules
 *				 - M22 specific	status codes
 *				 - M22 function	prototypes
 *
 *	   Switches: _ONE_NAMESPACE_PER_DRIVER_
 *				 _LL_DRV_
 *
 *---------------------------------------------------------------------------
 * Copyright 2000-2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef	_M22_LLDRV_H
#  define _M22_LLDRV_H

#  ifdef __cplusplus
	  extern "C" {
#  endif

/*-----------------------------------------+
|  TYPEDEFS								   |
+------------------------------------------*/

/*-----------------------------------------+
|  DEFINES & CONST						   |
+------------------------------------------*/
#define	M22_MAX_CH		8
#define	M24_MAX_CH	   16

/*---------	M22	specific status	codes  --------*/
										  					/* G,S: S=setstat, G=getstat code		*/
#define	M22_24_IRQ_SOURCE					M_DEV_OF+0x00	/* G  :	get	interrupt causing channel	*/
#define	M22_24_CHANNEL_INACTIVE				M_DEV_OF+0x01	/* G,S:	true if channel inactive 		*/
#define	M22_24_INPUT_EDGE_MASK				M_DEV_OF+0x02	/* G,S:	input edge mask		*/
#define	M22_ALARM_EDGE_MASK					M_DEV_OF+0x03	/* G,S:	alarm edge mask		*/
#define	M22_24_SIG_EDGE_OCCURRED			M_DEV_OF+0x04	/* G,S: install(ed) signal	*/
#define	M22_24_SIG_CLR_EDGE_OCCURRED		M_DEV_OF+0x05	/*   S: deinstall signal 	*/
#define	M22_24_CLEAR_INPUT_EDGE				M_DEV_OF+0x06	/*   S: clears input edge of current channel	*/
#define	M22_GET_ALARM						M_DEV_OF+0x07	/* G  : gets alarm and edges of current channel	*/
#define	M22_CLEAR_ALARM_EDGE				M_DEV_OF+0x08	/*   S: clears alarm edge of current channel	*/

#define	M22_24_SETBLOCK_CLEAR_INPUT_EDGE M_DEV_BLK_OF+0x00	/*   S: clears input edges of active channels	*/
#define	M22_GETBLOCK_ALARM				 M_DEV_BLK_OF+0x01	/* G  : gets alarms and edges of active channels*/
#define	M22_SETBLOCK_CLEAR_ALARM_EDGE	 M_DEV_BLK_OF+0x02	/*   S: clears alarm edges of active channels	*/

/* channel option flags	*/
#define	M22_24_RISING_EDGE_ENABLE	0x1			/* irq on rising edge */
#define	M22_24_FALLING_EDGE_ENABLE	0x2			/* irq on rising edge */

/* channel read status masks	*/
#define	M22_24_READ_INPUT			0x01		/* input value */
#define	M22_24_READ_RISING_EDGE		0x02		/* output rising edge occured */
#define	M22_24_READ_FALLING_EDGE	0x04		/* output falling edge occured */
#define	M22_READ_OUTPUT_SWITCH		0x80		/* output falling edge occured */


/*-----------------------------------------+
|  GLOBALS								   |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES							   |
+------------------------------------------*/
#ifdef _LL_DRV_

#	ifndef _ONE_NAMESPACE_PER_DRIVER_
#		ifdef ID_SW
#			define M22_GetEntry M22_SW_GetEntry
#		endif
		extern void	M22_GetEntry( LL_ENTRY*	drvP );
#	else
#		define M22_GetEntry LL_GetEntry
#	endif /* !_ONE_NAMESPACE_PER_DRIVER_ */

#endif /* _LL_DRV_ */


#  ifdef __cplusplus
	  }
#  endif

#endif/*_M22_LLDRV_H*/

