/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m22_main.c
 *      Project: MDIS 4.x
 *
 *      Author: ufranke 
 *
 *  Description: test of the m22_drv.c m22_drv.h
 *               signal handling and buffer modes
 *
 *     Required: USE M22_MAX.DSC!!!
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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <MEN/usr_oss.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>

#include <MEN/m22_drv.h>

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

#ifdef VXWORKS
	#include <logLib.h>
#endif

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
static const int32 T_OK = 0;
static const int32 T_ERROR = 1;

/* copy of defines from dbg.h, this shouldn't be here */
#define DBG_LEV2_LOC      0x00000002  /* level 2: more verbose */
#define DBG_INTR_LOC      0x80000000  /* called from interrupt routines */

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
static int  m22SignalCount;
static int  m24SignalCount;

static int m22_openclose( char *devName );
static int m22_m24_test( char *devNameM22, char *devNameM24, int endless );

static void sighdl( u_int32 sigNo );

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
int main( int argc, char *argv[] )
{
	int error = 1;
	int line = __LINE__;
	int	n;
	int	i;

    printf("Automatic Test Program for M22/M24\n");
    if( argc < 2){
        printf("usage: m22_main <device name m22> <device name m24> [<n>]\n");
	    printf("requires: maximal descriptor all channels inactive and enabled edges\n");
    	printf("          external test adapter with loop   m22 ch #d - m24 ch #d ch #d+8 (d=0..7)\n");
    	printf("          n - number of runs - default 1 - 0 is endless\n");
    	printf("The M24 needs the M22 for test stimuli.\n");
        return 1;
    }
	UOS_MikroDelayInit();

	n = 1;
	if( argc == 4 )
	{
		n = atoi( argv[3] );
	}

	for( i=1; (n==0) || i <= n; i++ )
	{
		printf("M22/M24 TEST current loop: %d\n\n", i );
		/* open/close M22 */
	    error = m22_openclose(argv[1]);
	    if( error )
	    {
	    	line = __LINE__;
	    	goto CLEANUP;
		}

	    if( argc > 2 )
	    {
			/* open/close M24 */
		    error = m22_openclose(argv[2]);
		    if( error )
		    {
		    	line = __LINE__;
		    	goto CLEANUP;
			}
		}
	    UOS_Delay( 500 );
	    error = m22_m24_test( argv[1], argv[2], !n );
	    if( error )
	    {
	    	line = __LINE__;
	    	goto CLEANUP;
		}
	}/*for*/
	
CLEANUP:	
	if( error )
	{
		printf("*** %s: error at line %d\n", __FUNCTION__, line );
	}
    return error;
}

/***************************** sighdl ***************************************
 *
 *  Description: Signal handler.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  sigNo  signal number
 *
 *  Output.....:  -
 *
 *  Globals....:  SigNo, SignalCount
 *
 ****************************************************************************/
static void sighdl( u_int32 sigNo )
{
    if( sigNo == UOS_SIG_USR1)
	    m22SignalCount++;
	else if( sigNo == UOS_SIG_USR2)
	    m24SignalCount++;
	else
#ifdef VXWORKS
		logMsg("%s %s: unknown MDIS signal\n",(int)__FILE__, (int)__FUNCTION__ ,0,0,0,0 );
#else
		;
#endif	
}/*sighdl*/


/***************************** errShow ***************************************
 *
 *  Description:  Show MDIS or OS error message.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  msg	message string
 *
 *  Output.....:  -
 *
 *  Globals....:  errno
 *
 ****************************************************************************/
static void errShow( char *msg )
{
   u_int32 error;
   char    buf[2][200];

   error = UOS_ErrnoGet();
   M_errstringTs(	error, buf[0] );
   UOS_ErrStringTs(	error, buf[1] );

   printf("*** %s:\n\t-%s\n\t- %s\n", msg, buf[0], buf[1] );
}/*errShow*/

/**************************** CheckInteger **********************************
 *
 *  Description:  Compare integers.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  soll    cmp value
 *                ist     current value
 *                cmpType '=' | '<' | '>'
 *                quit    -
 *
 *  Output.....:  return  T_OK or T_ERROR
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
static int32 CheckInteger( int32 soll, int32 ist, char cmpType, int32 quit )
{
    int32 ret = T_OK;

    switch( cmpType )
    {
        case '<':
           if( soll < ist )
           {
               printf("    => OK\n");
           }
           else
           {
               printf(" '=' failed %d %d\n", (int)soll, (int)ist );
               ret = T_ERROR;
           }/*if*/
           break;

        case '>':
           if( soll > ist )
           {
               printf("    => OK\n");
           }
           else
           {
               printf(" '=' failed %d %d\n", (int)soll, (int)ist );
               ret = T_ERROR;
           }/*if*/
           break;

        case '=':
        default:
           if( soll == ist )
           {
               if( !quit )
                   printf("    => OK\n");
           }
           else
           {
               printf(" '==' failed %d %d\n", (int)soll, (int)ist );
               ret = T_ERROR;
           }/*if*/

    }/*switch*/

    return( ret );
}/*CheckInteger*/


/**************************** m22_openclose *********************************
 *
 *  Description:  Open and close the device. Read the SW-Revision ID's
 *                and the M-Modul ID.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  devName device name in the system e.g. "/m22/0"
 *
 *  Output.....:  return  T_OK or T_ERROR
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
static int m22_openclose( char *devName )
{
    int    ret = T_OK;
    int32  fd;
    int32  size;
    char   *buf;
    M_SETGETSTAT_BLOCK blkStruct;
    u_int16 *dataP;
    u_int32 i, dev;
    u_int32 maxWords;
    
    printf("%s\n", IdentString);
    printf("=========================\n");
    printf("%s - open close \n", devName);
    printf("    open\n");
    fd = M_open( devName );
    if( fd >= 0 )
    {
    	printf("    => OK\n");

       	size = 4096;
       	buf = (char*) malloc( size );
       	if( buf )
       	{
			/* check M22/24 */
	   		printf("    M_LL_CH_NUMBER\n");
           	if( M_getstat( fd, M_LL_CH_NUMBER, (int32*)&dev ) )
           	{
               	errShow("M_getstat M_LL_CH_NUMBER");
               	printf("    => Error\n");
               	ret = T_ERROR;
           	}
           	else
           	{
           		if( dev == M22_MAX_CH )
           		{
           			printf("       %s - M22 detected\n", devName);
	               	printf("    => OK\n");
           		}
           		else if( dev == M24_MAX_CH )
           		{
           			printf("       %s - M24 detected\n", devName);
	               	printf("    => OK\n");
	            }
	            else
	            {
	               	printf("    => Error\n");
    	           	ret = T_ERROR;
           		}/*if*/
          	}/*if*/


			/* get REVISION */
	       	printf("    revision id's\n");
           	*buf = 0;
           	blkStruct.size = size;
           	blkStruct.data = buf;

           	if( M_getstat( fd, M_MK_BLK_REV_ID, (int32*) &blkStruct ) )
           	{
               	errShow("M_getstat M_MK_BLK_REV_ID");
               	printf("    => Error\n");
               	ret = T_ERROR;
           	}
           	else
           	{
               	printf("    => OK\n");
           	}/*if*/

           	printf( buf );
			fflush(stdout);


			/* get M-Module ID */
           	printf("    module id\n");
           	maxWords = 128;
           	blkStruct.size = maxWords*2;
           	if( M_getstat( fd, M_LL_BLK_ID_DATA, (int32*) &blkStruct ) )
           	{
               	errShow("M_getstat M_LL_BLK_ID_DATA");
               	printf("    => Error\n");
               	ret = T_ERROR;
           	}
           	else
           	{
               	printf("    => OK\n");
               	dataP = (u_int16*) blkStruct.data;
               	for( i=1; i<=maxWords; i++ )
               	{
                   	printf( " 0x%04x", (int)*dataP );
                   	if( !(i%8) )
                       	printf("\n");
                   	dataP++;
               	}/*for*/
           	}/*if*/
			fflush(stdout);


           	free(buf);
       	}
       	else
           	printf(" can't allocate user buffer\n");

       	printf("    close\n");
       	if( M_close( fd ) == 0 )
       	{
           	printf("    => OK\n");

       	}
       	else
       	{
           	errShow("M_close");
           	printf("    => Error\n");
           	ret = T_ERROR;
       	}/*if*/
	}
   	else
   	{
    	errShow("M_open");
       	printf("    => Error\n");
       	ret = T_ERROR;
    }/*if*/

   	return( ret );
}/*m22_openclose*/

/******************************* m22_m24_test ***********************************
 *
 *  Description:  Open and close the device.
 *                Check the correct irq causing channel.
 *
 *---------------------------------------------------------------------------
 *  Input......:  devNameM22  - device name of M22 e.g. "/m22/0"
 *				  devNameM24  - device name of M24 e.g. "/m24/0"
 *				  endless	  - don't wait for keys in endless mode	
 *
 *  Output.....:  return  T_OK or T_ERROR
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
static int m22_m24_test( char *devNameM22, char *devNameM24, int endless  )
{
    int32  ch;
    int32  nbrOfIrqs;
    int32  wrVal;
    int32  rdVal;
    int32  oldDebugLevel;
    int32  irqCh;
    int    error;
    int    nbrOfReadBytes;
    int    nbrOfWrBytes;
    int    i;
    M_SETGETSTAT_BLOCK blkStruct;
    char   errMsg[100];
    int	   dualModeM22M24;
    u_int8 m22buf[M22_MAX_CH]; /*usr buffer*/
    u_int8 m24buf[M24_MAX_CH]; /*usr buffer*/
    u_int8 *bufP = 0;
    int32  m22Fd = -1;
    int32  m24Fd = -1;
    
    
    /*-------------------------------+
    |  M22 or M22/24 test            |
    +-------------------------------*/
    printf("=========================\n");
	if( !devNameM24 )
	{
	    printf("M22 test\n");
	    dualModeM22M24 = FALSE;
	}
	else
	{
	    printf("M22/M24 test\n");
	    dualModeM22M24 = TRUE;
	}/*if*/

	/* install signal handler */
    m22SignalCount = 0;
    m24SignalCount = 0;
	if (UOS_SigInit(sighdl)) {
		sprintf( errMsg, "%s", "UOS_SigInit" );
		goto M22_TESTERR;
	}
	/* install signal */
	if (UOS_SigInstall(UOS_SIG_USR1)) {
		sprintf( errMsg, "%s", "UOS_SigInstall" );
		goto M22_TESTERR;
	}
	if (UOS_SigInstall(UOS_SIG_USR2)) {
		sprintf( errMsg, "%s", "UOS_SigInstall" );
		goto M22_TESTERR;
	}

    /*-------------------------------+
    |  M_open                        |
    +-------------------------------*/
    sprintf( errMsg, "%s %s", "M_open", devNameM22 );
    if( (m22Fd = M_open(devNameM22)) < 0 ) goto M22_TESTERR;
    sprintf( errMsg, "%s", "M_setstat M22_24_SIG_EDGE_OCCURRED" );
    if( M_setstat( m22Fd, M22_24_SIG_EDGE_OCCURRED, UOS_SIG_USR1) ) goto M22_TESTERR;

	if( dualModeM22M24 )
	{
	    sprintf( errMsg, "%s %s", "M_open", devNameM24 );
    	if( (m24Fd = M_open(devNameM24)) < 0 ) goto M22_TESTERR;
	    sprintf( errMsg, "%s", "M_setstat M22_24_SIG_EDGE_OCCURRED" );
    	if( M_setstat( m24Fd, M22_24_SIG_EDGE_OCCURRED, UOS_SIG_USR2) ) goto M22_TESTERR;
	}/*if*/
    sprintf( errMsg, "%s", "" ); /* clear error  additional message */

    /*--------------------------------------+
    |  debug level switch on for interrupt  |
    +--------------------------------------*/
    sprintf( errMsg, "%s", "M_getstat M_LL_DEBUG_LEVEL" );
    if( M_getstat(m22Fd, M_LL_DEBUG_LEVEL, &oldDebugLevel ) ) goto M22_TESTERR;
    oldDebugLevel |= DBG_INTR_LOC | DBG_LEV2_LOC;
    if( M_setstat(m22Fd, M_LL_DEBUG_LEVEL, oldDebugLevel ) ) goto M22_TESTERR;

	if( dualModeM22M24 )
	{
	    if( M_getstat(m24Fd, M_LL_DEBUG_LEVEL, &oldDebugLevel ) ) goto M22_TESTERR;
    	oldDebugLevel |= DBG_INTR_LOC | DBG_LEV2_LOC;
    	if( M_setstat(m24Fd, M_LL_DEBUG_LEVEL, oldDebugLevel ) ) goto M22_TESTERR;
	}/*if*/

    /*------------------------------------------+
    |  check all channels inactive              |
    +------------------------------------------*/
    printf("DESC CHANNEL_d/INACTIVE 1 (d 0..7..15)\n");
    error = 0;
	fflush(stdout);
    for( ch=0; ch<M22_MAX_CH; ch++)
    {
        wrVal = ch & 1;
        M_setstat(m22Fd, M_MK_CH_CURRENT, ch);
        error = M_write(m22Fd, wrVal );
        UOS_Delay(1);
        if( !error || UOS_ErrnoGet() != ERR_LL_ILL_CHAN )
        	break;
        error = M_read(m22Fd, &rdVal );
        UOS_Delay(1);
        if( error )
        	break;
	}/*for*/
    if( error )
    {
        printf("    => Error at line %d\n", __LINE__);
		goto M22_TESTERR;
    }
    else
        printf("    => OK\n");

    if( dualModeM22M24 )
    {
	    for( ch=0; ch<M24_MAX_CH; ch++)
    	{
	        wrVal = ch & 1;
    	    M_setstat(m24Fd, M_MK_CH_CURRENT, ch);
	        UOS_Delay(1);
        	error = M_write(m24Fd, wrVal );
	        if( !error || UOS_ErrnoGet() != ERR_LL_ILL_CHAN )
    	    	break;

        	error = M_read(m24Fd, &rdVal );
	        UOS_Delay(1);
        	if( error )
        		break;
		}/*for*/
	    if( error )
	    {
	        printf("    => Error at line %d\n", __LINE__);
			goto M22_TESTERR;
	    }
  	 	else
   	 		printf("    => OK\n");
    }/*if*/

    /*------------------------------------------------+
    |  activate channel 7/15 and check read/write irq |
    +------------------------------------------------*/
    printf("ACTIVATE Channel  7/15\n");
    error = 0;
	fflush(stdout);

	/* M22 */
    M_setstat(m22Fd, M_MK_IRQ_ENABLE, 1);
    ch = 7;
    M_setstat(m22Fd, M_MK_CH_CURRENT, ch);
    M_getstat(m22Fd, M22_24_CHANNEL_INACTIVE, &rdVal);
    if( rdVal != 1 )
    	error = 1;
    
    M_setstat(m22Fd, M22_24_CHANNEL_INACTIVE, 0);
    M_getstat(m22Fd, M22_24_CHANNEL_INACTIVE, &rdVal);
    if( rdVal != 0 )
    	error = 1;

	/* M24 */
    if( dualModeM22M24 )
    {
	    M_setstat(m24Fd, M_MK_IRQ_ENABLE, 1);
	    ch = 15;
	    M_setstat(m24Fd, M_MK_CH_CURRENT, ch); /* ch 15 */
	    M_getstat(m24Fd, M22_24_CHANNEL_INACTIVE, &rdVal);
    	if( rdVal != 1 )
    		error = 1;
    
	    M_setstat(m24Fd, M22_24_CHANNEL_INACTIVE, 0);
    	M_getstat(m24Fd, M22_24_CHANNEL_INACTIVE, &rdVal);
	    if( rdVal != 0 )
    		error = 1;
	}/*if*/
    if( error )
    {
        printf("    => Error at line %d\n", __LINE__);
		goto M22_TESTERR;
    }
    else
        printf("    => OK\n");

    /*-----------------+
    |                  |
    +-----------------*/
    printf("Read Channel  7/15 off, no edges occurred, no irq\n");
    error = 0;
	fflush(stdout);
	/* M22 */
    M_read(m24Fd,&rdVal);
    M_getstat(m22Fd, M_LL_IRQ_COUNT, &nbrOfIrqs );
    if( rdVal != 0  )
	{
		printf("\t*** readVal %2x\n", (int)rdVal );
   		error = 1;
   	}/*if*/
    if( nbrOfIrqs != 0 )
	{
		printf("\t*** nbrOfIrqs %d\n", (int)nbrOfIrqs );
   		error = 1;
   	}/*if*/
	/* M24 */
    if( dualModeM22M24 )
    {
        M_read(m24Fd,&rdVal);
	    M_getstat(m24Fd, M_LL_IRQ_COUNT, &nbrOfIrqs );
	    if( rdVal != 0 || nbrOfIrqs != 0 )
    		error = 1;
	}/*if*/
    if( error )
    {
        printf("    => Error at line %d\n", __LINE__);
		goto M22_TESTERR;
    }
    else
        printf("    => OK\n");



    /*-----------------+
    |                  |
    +-----------------*/
    printf("Rising Edge Channel  7/15\n");
    error = 0;
	fflush(stdout);
    M_write( m22Fd, 1 );
	UOS_Delay( 300 );
	/* M22 */
    M_read(m22Fd,&rdVal);
    M_getstat(m22Fd, M_LL_IRQ_COUNT, &nbrOfIrqs );
    M_getstat(m22Fd, M22_24_IRQ_SOURCE, &irqCh );
    if( rdVal != ( M22_24_READ_INPUT | M22_24_READ_RISING_EDGE | M22_READ_OUTPUT_SWITCH ) 
    	|| irqCh != 7
        || nbrOfIrqs != 1
        || nbrOfIrqs != m22SignalCount )
    {
		printf("\t***  irqCh %d nbrOfIrqs %d m22SignalCount %d\n", 
				(int)irqCh, (int)nbrOfIrqs, m22SignalCount );
   		error = 1;
   	}/*if*/
	/* M24 */
    if( dualModeM22M24 )
    {
        M_read(m24Fd,&rdVal);
	    M_getstat(m24Fd, M_LL_IRQ_COUNT, &nbrOfIrqs );
	    M_getstat(m24Fd, M22_24_IRQ_SOURCE, &irqCh );
	    if( rdVal != ( M22_24_READ_INPUT | M22_24_READ_RISING_EDGE ) 
    		|| irqCh != 15
	        || nbrOfIrqs != 1
    	    || nbrOfIrqs != m24SignalCount )
    	{
			printf("\t***  irqCh %d nbrOfIrqs %d m24SignalCount %d\n",
					(int)irqCh, (int)nbrOfIrqs, m24SignalCount );
    		error = 1;
    	}/*if*/
	}/*if*/
	/* clear edge flags */
    M_setstat(m22Fd, M22_24_CLEAR_INPUT_EDGE, 0);
    if( dualModeM22M24 )
	    M_setstat(m24Fd, M22_24_CLEAR_INPUT_EDGE, 0);
	/* check cleared edge flags */
    M_read(m22Fd,&rdVal);
    if( rdVal != ( M22_24_READ_INPUT | M22_READ_OUTPUT_SWITCH ) )
   		error = 1;
    if( dualModeM22M24 )
    {
        M_read(m24Fd,&rdVal);
    	if( rdVal != ( M22_24_READ_INPUT ) )
    		error = 1;
	}/*if*/
    if( error )
    {
        printf("    => Error at line %d\n", __LINE__);
		goto M22_TESTERR;
    }
    else
        printf("    => OK\n");

    /*-----------------+
    |                  |
    +-----------------*/
    printf("Falling Edge Channel  7/15\n");
    error = 0;
	fflush(stdout);
    M_write( m22Fd, 0 );
	UOS_Delay( 300 );
	/* M22 */
    M_read(m22Fd,&rdVal);
    M_getstat(m22Fd, M_LL_IRQ_COUNT, &nbrOfIrqs );
    M_getstat(m22Fd, M22_24_IRQ_SOURCE, &irqCh );
    if( rdVal != ( M22_24_READ_FALLING_EDGE ) 
    	|| irqCh != 7
        || nbrOfIrqs != 2
        || nbrOfIrqs != m22SignalCount )
    {
		printf("\t***  irqCh %d nbrOfIrqs %d m22SignalCount %d\n",
				(int)irqCh, (int)nbrOfIrqs, m22SignalCount );
   		error = 1;
   	}/*if*/
	/* M24 */
    if( dualModeM22M24 )
    {
        M_read(m24Fd,&rdVal);
	    M_getstat(m24Fd, M_LL_IRQ_COUNT, &nbrOfIrqs );
	    M_getstat(m24Fd, M22_24_IRQ_SOURCE, &irqCh );
	    if( rdVal != ( M22_24_READ_FALLING_EDGE ) 
    		|| irqCh != 15
	        || nbrOfIrqs != 2
    	    || nbrOfIrqs != m24SignalCount )
    	{
			printf("\t***  irqCh %d nbrOfIrqs %d m24SignalCount %d\n",
					(int)irqCh, (int)nbrOfIrqs, m24SignalCount );
    		error = 1;
    	}/*if*/
	    M_setstat(m24Fd, M22_24_CHANNEL_INACTIVE, 1); /* deactivate M24 ch7 */
	}/*if*/
    if( error )
    {
        printf("    => Error at line %d\n", __LINE__);
		goto M22_TESTERR;
    }
    else
        printf("    => OK\n");

	
    /*------------------------------------------+
    |  check read write M_getblock M_setblock   |
    +------------------------------------------*/
    printf("M_write/M_read/M_getblock/M_setblock\n");
    error = 0;
	fflush(stdout);
	UOS_Delay(1);
    M_setstat(m22Fd, M_MK_IRQ_ENABLE, 0);

    for( ch=0; ch<M22_MAX_CH; ch++)
    {
        wrVal = ch & 1;

    	/* set current channel */
        M_setstat(m22Fd, M_MK_CH_CURRENT, ch);
        M_setstat(m22Fd, M22_24_CLEAR_INPUT_EDGE, 0); /* to clear edges on ch 7/15 */
        if( dualModeM22M24 )
        {
	        M_setstat(m24Fd, M_MK_CH_CURRENT, ch+8);
	        M_setstat(m24Fd, M22_24_CLEAR_INPUT_EDGE, 0);
	        M_setstat(m24Fd, M_MK_CH_CURRENT, ch);
	        M_setstat(m24Fd, M22_24_CLEAR_INPUT_EDGE, 0);
	    }/*if*/

		/* M22 write/read/check */
	    M_setstat(m22Fd, M22_24_CHANNEL_INACTIVE, 0); /* allow write to ch */
        M_write(m22Fd, wrVal );
        UOS_Delay(100);

        M_read(m22Fd,&rdVal);
        if( (M22_24_READ_INPUT & rdVal) != wrVal )
        {
           CheckInteger( wrVal, rdVal, '=', 0 );
           error = 1;
        }

		/* M24 read/check */
        if( dualModeM22M24 )
        {
	        M_read(m24Fd,&rdVal);
    	    if( (M22_24_READ_INPUT & rdVal) != wrVal )
        	{
	           CheckInteger( wrVal, rdVal, '=', 0 );
    	       error = 1;
        	}
			/* get upper channels of M24 */
	        M_setstat(m24Fd, M_MK_CH_CURRENT, ch+8);
	        M_read(m24Fd,&rdVal);
    	    if( (M22_24_READ_INPUT& rdVal) != wrVal )
        	{
	           CheckInteger( wrVal, rdVal, '=', 0 );
    	       error = 1;
        	}/*if*/
	    }/*if*/
    }/*for*/

	/* M22 */
	nbrOfReadBytes = M_getblock(m22Fd, m22buf, M22_MAX_CH );
	if( nbrOfReadBytes != M22_MAX_CH )
    	error = 1;
    	
    bufP = m22buf;
    for( ch=0; ch<M22_MAX_CH; ch++)
    {
		if( ch & 1 )
	        rdVal = (ch & 1) | (ch & 1)<<7 | M22_24_READ_RISING_EDGE;
		else
	        rdVal = 0;
        if( *bufP++ != rdVal )
        	error = 1;
    }/*if*/
    
	/* M24 */
    if( dualModeM22M24 )
    {
		nbrOfReadBytes = M_getblock(m24Fd, m24buf, M24_MAX_CH );
		if( nbrOfReadBytes != 0 )
    		error = 1;
	    /* activate all M24 channels */
	    for( ch=0; ch<M24_MAX_CH; ch++)
		{
	        M_setstat(m24Fd, M_MK_CH_CURRENT, ch);
		    M_setstat(m24Fd, M22_24_CHANNEL_INACTIVE, 0); /* allow getblock */
		}/*for*/
		nbrOfReadBytes = M_getblock(m24Fd, m24buf, M24_MAX_CH );
		if( nbrOfReadBytes != M24_MAX_CH )
    		error = 1;
	    bufP = m24buf;
	    for( ch=0; ch<M24_MAX_CH; ch++)
    	{
			if( ch & 1 )
 		       	rdVal = (ch & 1) | M22_24_READ_RISING_EDGE;
			else
				rdVal = 0;
        	if( *bufP++ != rdVal )
        		error = 1;
	    }/*if*/
    }/*if*/

	/* M22 */
	bufP = m22buf;
	for( ch=0; ch<M22_MAX_CH; ch++)
		*bufP++ = 0;
	nbrOfWrBytes = M_setblock(m22Fd, m22buf, M22_MAX_CH );
	if( nbrOfWrBytes != M22_MAX_CH )
		error = 1;

    if( error )
    {
        printf("    => Error at line %d\n", __LINE__);
		goto M22_TESTERR;
    }
    else
        printf("    => OK\n");


    /*-----------------+
    |  check setstat   |
    +-----------------*/
    printf("M_setstat\n");
    error = 0;
	fflush(stdout);
    M_setstat(m22Fd, M_MK_IRQ_ENABLE, 1);
    if( dualModeM22M24 )
	    M_setstat(m24Fd, M_MK_IRQ_ENABLE, 1);
	UOS_Delay(1);

	m22SignalCount = 0;
	m24SignalCount = 0;
	blkStruct.size = 0;
	blkStruct.data = 0;
	M_setstat(m22Fd, M22_24_SETBLOCK_CLEAR_INPUT_EDGE, (int32)(intptr_t)&blkStruct);
	if( dualModeM22M24 )
		M_setstat(m24Fd, M22_24_SETBLOCK_CLEAR_INPUT_EDGE, (int32)(intptr_t)&blkStruct);

    for( ch=0; ch<M22_MAX_CH; ch++)
    {
        M_setstat(m22Fd, M_MK_CH_CURRENT, ch);
	    M_setstat(m22Fd, M22_24_INPUT_EDGE_MASK, M22_24_RISING_EDGE_ENABLE);
	    if( m22SignalCount )
	    	error = 1;
    	if( dualModeM22M24 )
    	{
	        M_setstat(m24Fd, M_MK_CH_CURRENT, ch);
	    	M_setstat(m24Fd, M22_24_INPUT_EDGE_MASK, M22_24_FALLING_EDGE_ENABLE);
			/* set upper channels of M24 */
	        M_setstat(m24Fd, M_MK_CH_CURRENT, ch+8);
	    	M_setstat(m24Fd, M22_24_INPUT_EDGE_MASK, M22_24_FALLING_EDGE_ENABLE);
		    if( m24SignalCount )
		    	error = 1;
	    }/*if*/
    }/*for*/

	/* set/reset channel */
    for( ch=0; ch<M22_MAX_CH; ch++)
    {
        M_setstat(m22Fd, M_MK_CH_CURRENT, ch);
		M_write( m22Fd, 1 );
		UOS_Delay(100);
	}/*if*/
    for( ch=0; ch<M22_MAX_CH; ch++)
    {
        M_setstat(m22Fd, M_MK_CH_CURRENT, ch);
		M_write( m22Fd, 0 );
		UOS_Delay(100);
	}/*if*/

	if( m22SignalCount != 8 || ( dualModeM22M24 && m24SignalCount != 16 ) )
		error = 1;
		
	nbrOfReadBytes = M_getblock(m22Fd, m22buf, M22_MAX_CH );
    bufP = m22buf;
    for( ch=0; ch<M22_MAX_CH; ch++)
    {
        rdVal = M22_24_READ_RISING_EDGE | M22_24_READ_FALLING_EDGE;
        if( *bufP++ != rdVal )
        	error = 1;
    }/*if*/
		
   	if( dualModeM22M24 )
   	{
		nbrOfReadBytes = M_getblock(m24Fd, m24buf, M24_MAX_CH );
    	bufP = m24buf;
	    for( ch=0; ch<M24_MAX_CH; ch++)
    	{
	        rdVal = M22_24_READ_RISING_EDGE | M22_24_READ_FALLING_EDGE;
        	if( *bufP++ != rdVal )
    	    	error = 1;
	    }/*if*/
    }/*if*/

    if( error )
    {
        printf("    => Error at line %d\n", __LINE__);
		goto M22_TESTERR;
    }
    else
        printf("    => OK\n");

    /*-----------------+
    |  check setstat   |
    +-----------------*/
    printf("M22 Alarm - Set OverLoad Switch\n");
    error = 0;
	fflush(stdout);

	UOS_Delay(1);
    m22SignalCount = 0;
	ch = 4;
    M_setstat(m22Fd, M_MK_CH_CURRENT, ch);
	M_write( m22Fd, 1 );
    printf("   set overload switch and press any key!\n");
	fflush(stdout);
	if( !endless )
		UOS_KeyWait();
	
	i=0;
	while( m22SignalCount == 0 )
	{
		printf(".");
		fflush(stdout);
		UOS_Delay(500);
		if( (i++) > 4 )
			break;
	}
	printf("\n");

	if( !m22SignalCount )
		error = 1;

    M_getstat(m22Fd, M22_24_IRQ_SOURCE, &irqCh );
	if( irqCh != 4 )
		error = 1;

	/* getstat alarm + getstat block */
    M_getstat(m22Fd, M22_GET_ALARM, &rdVal );
    if( rdVal != (M22_24_READ_INPUT | M22_24_READ_RISING_EDGE) )
		error = 1;
    
   	blkStruct.size = sizeof(m22buf);
   	blkStruct.data = m22buf;
    M_getstat(m22Fd, M22_GETBLOCK_ALARM, (int32*) &blkStruct );
    bufP = m22buf;
    for( ch=0; ch<M22_MAX_CH; ch++)
    {
    	if( ch == 4 )
        	rdVal = M22_24_READ_INPUT | M22_24_READ_RISING_EDGE;
        else
        	rdVal = 0;
        if( *bufP++ != rdVal )
        	error = 1;
    }/*if*/

    if( error )
        printf("    => Error at line %d - suppressed due to leak of suitable hardware\n", __LINE__ );
    else
        printf("    => OK\n");

    if( M_close( m22Fd ) ) goto M22_TESTERR;
   	if( dualModeM22M24 )
	    if( M_close( m24Fd ) ) goto M22_TESTERR;

    printf("=== finished - OK ===\n");
	UOS_SigRemove(UOS_SIG_USR1);
	UOS_SigRemove(UOS_SIG_USR2);
	UOS_SigExit();
	fflush(stdout);
    return( 0 );



    /*-----------------+
    |  CLEANUP         |
    +-----------------*/
M22_TESTERR:
    errShow( errMsg );
    printf("=== finished - ERROR ===\n");
    if( m22Fd != -1 )
	    M_close( m22Fd );
	UOS_SigRemove(UOS_SIG_USR1);
	if( dualModeM22M24 && m24Fd != -1 )
	{
	    M_close( m24Fd );
		UOS_SigRemove(UOS_SIG_USR2);
	}/*if*/
	UOS_SigExit();
	fflush(stdout);
    return( 1 );
}/*m22_m24_test*/




