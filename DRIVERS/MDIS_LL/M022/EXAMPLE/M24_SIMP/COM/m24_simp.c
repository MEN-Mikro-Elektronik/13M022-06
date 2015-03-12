/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m24_simp.c
 *      Project: MDIS 4.x
 *
 *       Author: uf
 *        $Date: 2009/03/30 14:02:36 $
 *    $Revision: 1.3 $
 *
 *  Description: simple test of the m22/m24 mdis driver
 *
 *     Required: -
 *     Switches: -
 *
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m24_simp.c,v $
 * Revision 1.3  2009/03/30 14:02:36  ufranke
 * cosmetics
 *
 * Revision 1.2  2006/07/20 15:08:17  ufranke
 * cosmetics
 *
 * Revision 1.1  2000/07/13 11:42:56  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
static char *RCSid="$Id: m24_simp.c,v 1.3 2009/03/30 14:02:36 ufranke Exp $\n";

#include <MEN/men_typs.h>

#include <stdio.h>
#include <string.h>

#include <MEN/usr_oss.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>

#include <MEN/m22_drv.h>

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
    printf("%s", RCSid );


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

