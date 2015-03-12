/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m22_simp.c
 *      Project: MDIS 4.x
 *
 *       Author: uf
 *        $Date: 2010/09/01 12:47:09 $
 *    $Revision: 1.4 $
 *
 *  Description: simple test of the m22 mdis driver
 *               Each channel will be toggle.
 *
 *     Required: -
 *     Switches: -
 *
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m22_simp.c,v $
 * Revision 1.4  2010/09/01 12:47:09  UFranke
 * R: #warning "VxWorks Source Build (VSB) project not specified;
 * M: include order changed
 *
 * Revision 1.3  2008/12/05 10:58:41  ufranke
 * R: error if ch 0..7 was not activated by descriptor
 * M: activate each channel before toggling
 *
 * Revision 1.2  2006/07/20 15:08:10  ufranke
 * cosmetics
 *
 * Revision 1.1  2000/07/13 11:42:45  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000..2010 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
static char *RCSid="$Id: m22_simp.c,v 1.4 2010/09/01 12:47:09 UFranke Exp $\n";

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
static int _m22_simple( char *DevName );

/******************************* errShow ************************************
 *
 *  Description:  Show MDIS or OS error message.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  line
 *
 *  Output.....:  -
 *
 *  Globals....:  errno
 *
 ****************************************************************************/
static void errShow( int line )
{
   u_int32 error;

   error = UOS_ErrnoGet();

   printf("*** %s  from line %d ***\n",M_errstring( error ), line );
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
        printf("usage: m22_simp <device name>\n");
        return 1;
    }
    _m22_simple(argv[1]);
    return 0;
}


/******************************* _m22_simple *********************************
 *
 *  Description:  Opens the device, write and reads all channels.
 *                Shows the read values.
 *
 *---------------------------------------------------------------------------
 *  Input......:  devName device name in the system e.g. "/m22/0"
 *
 *  Output.....:  return  0 - OK or 1 - ERROR
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
static int _m22_simple( char *devName )
{
    int32  fd;
    int32  ch;
    int32  valL, oldVal=0;
    int32  i;
    int    errLine = 0;

    printf("=========================\n");
    printf("%s", RCSid );


    printf("M_open\n");
    if( (fd = M_open(devName)) < 0 ) 
    {
    	errLine = __LINE__;
    	goto M22_TESTERR;
    }

    /*------------------------------------+
    |  configuration                      |
    +-------------------------------------*/
    if( M_setstat( fd, M_MK_IO_MODE, M_IO_EXEC ) )
    {
    	errLine = __LINE__;
    	goto M22_TESTERR;
    }

	
    for( i=0; i<6; i++ )
    {
   		valL = (i%2);

	    printf("M_write value %lx to channel   0..7\n", valL );
		/* M_write */
		for( ch=0; ch < M22_MAX_CH; ch++ )
		{
		    M_setstat(fd, M_MK_CH_CURRENT, ch);    		/* set current channel */
			M_setstat(fd, M22_24_CHANNEL_INACTIVE, 0 ); /* activate each channel */
		    
    		if( M_write(fd, valL ) )                    /* write value to curr ch */
		    {
    			errLine = __LINE__;
		    	goto M22_TESTERR;
    		}
	        UOS_Delay( 30 );
        }/*for*/

		/* M_read */
		for( ch=0; ch < M22_MAX_CH; ch++ )
		{
		    M_setstat(fd, M_MK_CH_CURRENT, ch);    		/* set current channel */
			UOS_Delay( 30 );
    		if( M_read(fd, &valL ) )                    /* read value back */
		    {
    			errLine = __LINE__;
		    	goto M22_TESTERR;
    		}
    		if( ch == 0 )
    			oldVal = valL;
    		else if( oldVal != valL )
    			printf("WARNING channel/value differs 0/%x != %d/%x\n", (int)oldVal, (int)ch, (int)valL );
        }/*for*/
        printf("M_read  value %x from channel 0..7\n\n", (int)valL );
    }/*for*/


    printf("M_close\n");
	UOS_Delay( 300 );
    if( M_close( fd ) )
    {
		errLine = __LINE__;
    	goto M22_TESTERR;
	}

    printf("    => OK\n");
    return( 0 );


M22_TESTERR:
    errShow( errLine );
    printf("    => Error\n");
    if( fd != -1 )
    	M_close( fd );
    return( 1 );
}/*_m22_simple*/



