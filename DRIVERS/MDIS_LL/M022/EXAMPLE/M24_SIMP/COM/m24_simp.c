/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m24_simp.c
 *      Project: MDIS 4.x
 *
 *       Author: uf
 *
 *  Description: simple test of the m22/m24 mdis driver
 *
 *     Required: -
 *     Switches: -
 *
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

#include <MEN/men_typs.h>

#include <stdio.h>
#include <string.h>

#include <MEN/usr_oss.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>

#include <MEN/m22_drv.h>

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
static int _m24_simple( char *DevName );

/******************************* errShow ************************************
 *
 *  Description:  Show MDIS or OS error message.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *
 *  Output.....:  -
 *
 *  Globals....:  errno
 *
 ****************************************************************************/
static void errShow( void )
{
   u_int32 error;

   error = UOS_ErrnoGet();

   printf("*** %s ***\n",M_errstring( error ) );
}


/******************************** main **************************************
 *
 *  Description:  main() - function
 *
 *---------------------------------------------------------------------------
 *  Input......:  argc      number of arguments
 *                *argv     pointer to arguments
 *                argv[1]   device name
 *
 *  Output.....:  return    0   if no error
 *                          1   if error
 *
 *  Globals....:  -
 ****************************************************************************/
int main( int argc, char *argv[ ] )
{
    if( argc < 2){
        printf("usage: m24_simp <device name>\n");
        return 1;
    }
    _m24_simple(argv[1]);
    return 0;
}


/******************************* _m24_simple *********************************
 *
 *  Description:  Opens the device and reads all channels.
 *                Shows the read values.
 *
 *---------------------------------------------------------------------------
 *  Input......:  devName device name in the system e.g. "/m24/0"
 *
 *  Output.....:  return  0 - OK or 1 - ERROR
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
static int _m24_simple( char *devName )
{
    int32  fd;
    int32  ch;
    int32  valL;

    printf("=========================\n");
    printf("%s", IdentString );


    printf("M_open\n");
    if( (fd = M_open(devName)) < 0 ) goto M24_TESTERR;

    /*------------------------------------+
    |  configuration                      |
    +-------------------------------------*/
    if( M_setstat( fd, M_MK_IO_MODE, M_IO_EXEC ) ) goto M24_TESTERR;

	/* M_read */
	for( ch=0; ch < M24_MAX_CH; ch++ )
	{
	    M_setstat(fd, M_MK_CH_CURRENT, ch);    		/* set current channel */
		UOS_Delay( 30 );
    	if( M_read(fd, &valL ) ) goto M24_TESTERR;  /* read value back */
	    printf("M_read  value %x from channel %d\n", (int)valL, (int)ch );
    }/*for*/

    printf("M_close\n");
	UOS_Delay( 300 );
    if( M_close( fd ) ) goto M24_TESTERR;

    printf("    => OK\n");
    return( 0 );


M24_TESTERR:
    errShow();
    printf("    => Error\n");
    if( fd != -1 )
    	M_close( fd );
    return( 1 );
}/*_m24_simple*/

