/*********************	P r	o g	r a	m  -  M	o d	u l	e ***********************
 *
 *		   Name: m22_drv.c
 *		Project: M22 / M24 module driver 
 *
 *		 Author: uf
 *
 *  Description: Low-level driver for M22 and M24 M-Modules.
 *   The init routine detects the device type from the M-Module ID
 *   of the EEPROM. The driver supports 8 digital I/O channels and
 *   interrupts on changing inputs and alarms for the M22. The driver
 *   supports 16 digital input channels and interrupts on changing inputs
 *   for the M24.
 *   An installed signal will be sent on interrupt. The IRQ-causing
 *   channel can be read via GetStat.
 *
 *   For detailed information on the M-Modules' capabilities see the
 *   respective hardware manuals.
 *
 *   Note: If two enabled channels are changing at the same time,
 *         the M-Module detects only one channel as the one causing the IRQ.
 *         To prevent loss of events, read all active channels.
 *
 *
 *	   Required: ---
 *	   Switches: _ONE_NAMESPACE_PER_DRIVER_
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

#define _NO_LL_HANDLE		/* ll_defs.h: don't define LL_HANDLE struct */

#include <MEN/men_typs.h>   /* system dependent definitions   */
#ifndef INT32_OR_64
#define INT32_OR_64		int32  /* for backward compatibility with old system packages */
#endif

#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/dbg.h>        /* debug functions                */
#include <MEN/oss.h>        /* oss functions                  */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/modcom.h>     /* ID PROM functions              */
#include <MEN/mdis_api.h>   /* MDIS global defs               */
#include <MEN/mdis_com.h>   /* MDIS common defs               */
#include <MEN/mdis_err.h>   /* MDIS error codes               */
#include <MEN/ll_defs.h>    /* low-level driver definitions   */

/*-----------------------------------------+
  |  TYPEDEFS								   |
  +------------------------------------------*/
typedef	struct
{
	int32			ownMemSize;
	u_int32			dbgLevel;
	DBG_HANDLE		*dbgHdl;		/* debug handle	*/
	OSS_HANDLE		*osHdl;			/* for complicated os */
	MDIS_IDENT_FUNCT_TBL idFuncTbl;	/* id function table */
	MACCESS			ma;
	OSS_SIG_HANDLE	*sigHdl;
	OSS_IRQ_HANDLE	*irqHdl;
	u_int32			irqCount;

	int32			modId;			/*	M22	| M24 */
	int32			modIdM22_R4;		/*	M22	with new power switches */
	int32			nbrOfChannels;
	int32			irqEnabled;
	u_int16			irqSource;		/*	IRQ-causing channel */
	u_int8			activeCh[16];	    /*	active channels (ch 0..7..15) */
	u_int8			inputEdgeMask[16];	/*	input edge masks (ch 0..7..15) */
	u_int8			alarmEdgeMask[8];	/*	alarm edge masks (ch 0..7) */
	u_int8			stateBuf[16];
	u_int8			alarmStateBuf[8];
} LL_HANDLE;

/* include files which need LL_HANDLE */
#include <MEN/ll_entry.h>   /* low-level driver jump table  */
#include <MEN/m22_drv.h>   /* M22 driver header file */

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*-----------------------------------------+
  |  DEFINES & CONST						   |
  +-----------------------------------------*/
#define	DBG_MYLEVEL		llHdl->dbgLevel
#define	DBH				llHdl->dbgHdl

#define	M22_CH_WIDTH	1	 /*	byte */

#define	MOD_ID_MAGIC		0x5346		/* eeprom identification (magic) */
#define	MOD_ID_SIZE			128			/* eeprom size */
#define	M22_MOD_ID			22			/* module id */
#define	M24_MOD_ID			24			/* module id */
#define	M22_ID_CHECK		1


/*---------------------	M22	register defs/offsets ------------------------*/
#define	IOREG(ch)		(ch<<1)					/* i/o regs	   */
#define	ALARMREG(ch)	((ch<<1)+0x10)			/* alarm regs	(M22 only)   */
#define	INTREG		0xfe						/* interrupt status  */

/* IOREG bits */
#define	IOREG_OUTPUT_SWITCH				0x01	/* output switch */
#define	IOREG_IRQ_ENABLE_RISING_EDGE	0x02	/* rising  edge	irq	enable */
#define	IOREG_IRQ_ENABLE_FALLING_EDGE	0x04	/* falling edge	irq	enable */
#define	IOREG_INPUT_OR_ALARM_VAL		0x08	/* input or alarm value  */
#define	IOREG_RISING_EDGE_OCCURRED		0x10	/* rising  edge	occurred */
#define	IOREG_FALLING_EDGE_OCCURRED		0x20	/* falling edge	occurred */

#define	IRQ_ENABLE_MASK			(IOREG_IRQ_ENABLE_RISING_EDGE | IOREG_IRQ_ENABLE_FALLING_EDGE)
#define	EDGE_OCCURRED_MASK		(IOREG_RISING_EDGE_OCCURRED | IOREG_FALLING_EDGE_OCCURRED)

/* INTREG */
#define	M22_IRQ_CH_NBR		0x0e
#define	M22_IRQ_ALARM		0x10
#define	M24_IRQ_CH_NBR		0x1e

#define	M22_IRQ_DISABLE		0
#define	M22_IRQ_ENABLE		1

#ifdef DBG
#	define errorStartStr	"*** ERROR - "
#	define errorLineStr	" (line "
#	define errorEndStr	")***\n"
#endif
/*-----------------------------------------+
  |  GLOBALS								   |
  +-----------------------------------------*/

/*-----------------------------------------+
  |  STATICS								   |
  +-----------------------------------------*/

/*-----------------------------------------+
  |  PROTOTYPES							   |
  +-----------------------------------------*/
static int32 M22_Init( DESC_SPEC *descSpec,
					   OSS_HANDLE	   *osHdl,
					   MACCESS		   *ma,
					   OSS_SEM_HANDLE  *devSemHdl,
					   OSS_IRQ_HANDLE  *irqHdl,
					   LL_HANDLE	   **llHdlP	);

static int32 M22_Exit(LL_HANDLE **llHdlP );
static int32 M22_Read(LL_HANDLE *llHdl, int32 ch,int32 *value );
static int32 M22_Write(LL_HANDLE *llHdl, int32 ch,int32 value	);
static int32 M22_SetStat(LL_HANDLE *llHdl,int32 ch, int32 code, INT32_OR_64 value32_or_64);
static int32 M22_GetStat(LL_HANDLE *llHdl, int32 ch, int32 code, INT32_OR_64 *value32_or_64P);
static int32 M22_BlockRead(LL_HANDLE *llHdl, int32 ch, void  *buf,int32 size,int32 *nbrRdBytesP );
static int32 M22_BlockWrite(  LL_HANDLE	*llHdl, int32 ch,void  *buf,int32 size,	int32 *nbrWrBytesP);
static int32 M22_Irq(LL_HANDLE	*llHdl );
static int32 M22_Info(int32 infoType, ... );

static int32 getStatBlock
(
 LL_HANDLE		   *llHdl,
 int32			   code,
 M_SETGETSTAT_BLOCK *blockStruct
 );
static int32 setStatBlock
(
 LL_HANDLE		   *llHdl,
 int32			   code,
 M_SETGETSTAT_BLOCK *blockStruct
 );

/*****************************	M22_Ident  **********************************
 *
 *	Description:  Gets the pointer to ident	string.
 *
 *
 *---------------------------------------------------------------------------
 *	Input......:  -
 *
 *	Output.....:  return  pointer to ident string
 *
 *	Globals....:  -
 ****************************************************************************/
static char* M22_Ident(	void ) /*nodoc*/
{
	return(	(char*)IdentString );
}/*M22_Ident*/

/**************************** M22_MemCleanup *********************************
 *
 *	Description:  Deallocates low-level	driver structure and
 *                installed signals.
 *
 *---------------------------------------------------------------------------
 *	Input......:  m22Hdl   m22 low-level driver	handle
 *
 *	Output.....:  return   0 | error code
 *
 *	Globals....:  ---
 *
 ****************************************************************************/
static int32 M22_MemCleanup( LL_HANDLE	*llHdl	) /*nodoc*/
{
	int32		retCode;

	/*--------------------------+
	  | remove installed signal	|
	  +--------------------------*/
	/* deinit lldrv	memory */
	if(	llHdl->sigHdl != NULL )
		OSS_SigRemove( llHdl->osHdl, &llHdl->sigHdl );

	/*-------------------------------------+
	  | free low-level handle				   |
	  +-------------------------------------*/
	/* dealloc lldrv memory	*/
	retCode	= OSS_MemFree( llHdl->osHdl, (int8*) llHdl, llHdl->ownMemSize );

	/* cleanup debug */
	DBGEXIT((&DBH));

	return(	retCode	);
}/*M22_MemCleanup*/


/*****************************	configureIrqForChannel  *********************
 *
 *	Description:  Set up the irq enable on edge bits for
 *                ch, value, active channel and edge mask.
 *
 *---------------------------------------------------------------------------
 *	Input......:  llHdl	  pointer to low-level driver data structure
 *				  ch	  current channel 0..7..15
 *				  value	  clear edge mask
 *                        1 - write edge mask, if active
 *
 *	Output.....:  -
 *
 *	Globals....:  -
 ****************************************************************************/
static void configureIrqForChannel /*nodoc*/
(
 LL_HANDLE *llHdl,
 int32  ch,
 int32  value
 )
{
	/* disable IRQ first */
	MCLRMASK_D16( llHdl->ma, IOREG(ch), IRQ_ENABLE_MASK );
	if( llHdl->modId == M22_MOD_ID )
		MCLRMASK_D16( llHdl->ma, ALARMREG(ch), IRQ_ENABLE_MASK );

	if( value && llHdl->activeCh[ch] )
		{
			/* enable active channels */
			MSETMASK_D16( llHdl->ma, IOREG(ch), llHdl->inputEdgeMask[ch] );
			if( llHdl->modId == M22_MOD_ID )
				MSETMASK_D16( llHdl->ma, ALARMREG(ch), llHdl->alarmEdgeMask[ch]  );
		}/*if*/
}/*configureIrqForChannel*/


/**************************** M22_GetEntry *********************************
 *
 *	Description:  Gets the entry points	of the low-level driver	functions.
 *
 *		   Note:  Is used by MDIS kernel only.
 *
 *---------------------------------------------------------------------------
 *	Input......:  ---
 *
 *	Output.....:  drvP	pointer	to the initialized structure
 *
 *	Globals....:  ---
 *
 ****************************************************************************/
void M22_GetEntry( LL_ENTRY* drvP )  /*nodoc*/
{

	drvP->init		  =	M22_Init;
	drvP->exit		  =	M22_Exit;
	drvP->read		  =	M22_Read;
	drvP->write		  =	M22_Write;
	drvP->blockRead	  =	M22_BlockRead;
	drvP->blockWrite  =	M22_BlockWrite;
	drvP->setStat	  =	M22_SetStat;
	drvP->getStat	  =	M22_GetStat;
	drvP->irq		  =	M22_Irq;
	drvP->info		  =	M22_Info;

}/*M22_GetEntry*/

/******************************** M22_Init ***********************************
 *
 *  Description:  Allocate and return low-level handle, initialize hardware.
 *				  Reads	and	checks the ID, detects hardware type of M22 or M24.
 *				  Clears and disables the M-Module interrupts.
 *				  Switches off all outputs of the M22.
 *
 *	Descriptor Key                Default            Range/Unit
 *  --------------                -------            ----------
 *	DEBUG_LEVEL_DESC              OSS_DBG_DEFAULT    see oss_os.h
 *	DEBUG_LEVEL                   OSS_DBG_DEFAULT    see oss_os.h
 *
 *	CHANNEL_%d/INACTIVE           0                  0..1 0 - active
 *                                                        1 - not active
 *                                                    %d	0..7..15
 *
 *	CHANNEL_%d/INPUT_EDGE_MASK    0                  0..3 0 - disabled
 *                                                        1 - rising edge
 *                                                        2	- falling edge
 *                                                        3	- any edge
 *                                                    %d	0..7..15
 *
 *	CHANNEL_%d/ALARM_EDGE_MASK    0                  0..3 0 - disabled (M22 only)
 *                                                        1	- rising edge
 *                                                        2	- falling edge
 *                                                        3	- any edge
 *                                                    %d	0..7
 *
 *  Note:  Is called by MDIS kernel only.
 *---------------------------------------------------------------------------
 *	Input......:  descSpec descriptor specifier
 *				  osHdl	   pointer to the os specific structure
 *				  ma	   access handle (in simplest case M22 base	address)
 *				  devSem   device semaphore	for	unblocking in wait
 *				  irqHdl   IRQ handle for mask and unmask interrupts
 *				  llHdlP   pointer to the variable where low-level driver
 *						   handle will be stored
 *
 *	Output.....:  *llHdlP  low-level driver	handle
 *				  return   0 | error code
 *
 *	Globals....:  -
 *
 ****************************************************************************/
static int32 M22_Init
(
 DESC_SPEC		*descSpec,
 OSS_HANDLE		*osHdl,
 MACCESS			*ma,
 OSS_SEM_HANDLE	*devSem,
 OSS_IRQ_HANDLE	*irqHdl,
 LL_HANDLE		**llHdlP
 )
{
	DBGCMD(	static const char functionName[] = "LL - M22_Init:"; )
		LL_HANDLE	*llHdl;
	int32		retCode;
	u_int32		gotsize;
	int32		ch;
	DESC_HANDLE	*descHdl;
	int32			modIdMagic;
	u_int32		mask;
	volatile	u_int16	 intFromCh;
	u_int32		dbgLevelDesc;


	retCode	= DESC_Init( descSpec, osHdl, &descHdl );
	if(	retCode	)
		{
			return(	retCode	);
		}/*if*/

	/*-------------------------------------+
	  |  LL-HANDLE allocate and initialize   |
	  +-------------------------------------*/
	llHdl = (LL_HANDLE*) OSS_MemGet( osHdl, sizeof(LL_HANDLE), &gotsize );
	if(	llHdl == NULL )
		{
			*llHdlP = 0;
			return( ERR_OSS_MEM_ALLOC );
		}/*if*/

	*llHdlP	= (LL_HANDLE*) llHdl;

	/* fill	turkey with	0 */
	OSS_MemFill( osHdl,	gotsize, (char*) llHdl, 0 );
	llHdl->ownMemSize	= gotsize;
	llHdl->osHdl		= osHdl;
	llHdl->ma			= *ma;
	llHdl->irqHdl		= irqHdl;

	/*------------------------------+
	  |  init	id function	table		|
	  +------------------------------*/
	{
		int	i =	0;
		/* drivers ident function */
		llHdl->idFuncTbl.idCall[i++].identCall	=	M22_Ident;
		/* libraries ident functions */
		llHdl->idFuncTbl.idCall[i++].identCall	=	DESC_Ident;
		llHdl->idFuncTbl.idCall[i++].identCall	=	OSS_Ident;
		/* terminator */
		llHdl->idFuncTbl.idCall[i++].identCall	=	NULL;
	}

	DBG_MYLEVEL	= OSS_DBG_DEFAULT;
	DBGINIT((NULL,&DBH));

	/*-------------------------------------+
	  |	get	DEBUG LEVEL					   |
	  +-------------------------------------*/
	/* DEBUG_LEVEL_DESC	*/
	retCode	= DESC_GetUInt32( descHdl,
							  OSS_DBG_DEFAULT,
							  &dbgLevelDesc,
							  "DEBUG_LEVEL_DESC",
							  NULL );
	if(	retCode	!= 0 &&	retCode	!= ERR_DESC_KEY_NOTFOUND ) goto	CLEANUP;
	DESC_DbgLevelSet(descHdl, dbgLevelDesc);
	retCode	= 0;

	/* DEBUG_LEVEL - LL	*/
	retCode	= DESC_GetUInt32( descHdl,
							  OSS_DBG_DEFAULT,
							  &llHdl->dbgLevel,
							  "DEBUG_LEVEL",
							  NULL );
	if(	retCode	!= 0 &&	retCode	!= ERR_DESC_KEY_NOTFOUND ) goto	CLEANUP;
	retCode	= 0;

	DBGWRT_1((DBH, "%s\n", functionName	)  );

	/*---------------------------------+
	  |  detect M-Module type M22 | M24  |
	  +---------------------------------*/
	modIdMagic = m_read((U_INT32_OR_64)llHdl->ma, 0);
	llHdl->modId = m_read((U_INT32_OR_64)llHdl->ma, 1);

	if(	modIdMagic != MOD_ID_MAGIC )
		{
			DBGWRT_ERR( ( DBH, "%s%s: m_read()	  id - illegal magic word %s%d%s",
						  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
			retCode = ERR_LL_ILL_ID;
			goto CLEANUP;
		}/*if*/
	if(	llHdl->modId != M22_MOD_ID	&& llHdl->modId !=	M24_MOD_ID )
		{
			DBGWRT_ERR( ( DBH,	"%s%s:	m_read()   id -	illegal	module id %s%d%s",
						  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
			retCode = ERR_LL_ILL_ID;
			goto CLEANUP;
		}/*if*/

	if(	llHdl->modId == M22_MOD_ID	)
		{
			DBGWRT_2((DBH, "%s M22 detected\n", functionName	)  );
			llHdl->nbrOfChannels =	M22_MAX_CH;
		}
	else
		{
			DBGWRT_2((DBH, "%s M24 detected\n", functionName	)  );
			llHdl->nbrOfChannels =	M24_MAX_CH;
		}/*if*/


	/* dummy access	to clear interrupt */
	intFromCh =	MREAD_D16( llHdl->ma, INTREG );

	/*--------------------------+
	  | config all channels		|
	  +--------------------------*/
	for( ch	= 0; ch	< llHdl->nbrOfChannels; ch++ )
		{
			DBGWRT_2((DBH, "%s reset channel %d\n", functionName, ch )  );
			/* input edge mask */
			retCode	= DESC_GetUInt32( descHdl,
									  0,
									  &mask,
									  "CHANNEL_%d/INPUT_EDGE_MASK",
									  ch );
			if(	retCode	!= 0 &&	retCode	!= ERR_DESC_KEY_NOTFOUND ) goto	CLEANUP;
			retCode	= 0;
			if( mask > 3 )
				{
					retCode = ERR_LL_DESC_PARAM;
					DBGWRT_ERR( ( DBH,	"%s%s: CHANNEL_%d/INPUT_EDGE_MASK out of range %s%d%s",
								  errorStartStr, functionName, ch, errorLineStr, __LINE__, errorEndStr ));
					goto CLEANUP;
				}/*if*/
			llHdl->inputEdgeMask[ch] = (u_int8)(mask<<1);
			/* clear and disable interrupt,	switch output off */
			MWRITE_D16(	llHdl->ma, IOREG(ch), 0);

			/* M22 - alarm edge mask */
			if(	llHdl->modId == M22_MOD_ID	)
				{
					/* input edge mask */
					retCode	= DESC_GetUInt32( descHdl,
											  0,
											  &mask,
											  "CHANNEL_%d/ALARM_EDGE_MASK",
											  ch );
					if(	retCode	!= 0 &&	retCode	!= ERR_DESC_KEY_NOTFOUND ) goto	CLEANUP;
					retCode	= 0;
					if( mask > 3 )
						{
							retCode = ERR_LL_DESC_PARAM;
							DBGWRT_ERR( ( DBH,	"%s%s: CHANNEL_%d/ALARM_EDGE_MASK out of range %s%d%s",
										  errorStartStr, functionName, ch, errorLineStr, __LINE__, errorEndStr ));
							goto CLEANUP;
						}/*if*/
					llHdl->alarmEdgeMask[ch] = (u_int8)(mask<<1);
					/* clear and disable interrupt */
					MWRITE_D16(	llHdl->ma, ALARMREG(ch), 0);
				}/*if*/

			/* inactive channels */
			retCode	= DESC_GetUInt32( descHdl,
									  0,
									  &mask,
									  "CHANNEL_%d/INACTIVE",
									  ch );
			if(	retCode	!= 0 &&	retCode	!= ERR_DESC_KEY_NOTFOUND ) goto	CLEANUP;
			retCode	= 0;
			llHdl->activeCh[ch] = (u_int8) (mask ? 0 : 1);
		}/*for*/

	/* dummy access	to clear interrupt */
	intFromCh =	MREAD_D16( llHdl->ma, INTREG );

	llHdl->irqSource =	0;				/* reset irqSource flag	*/

	DESC_Exit( &descHdl	);
	return(	retCode	);

 CLEANUP:
	DESC_Exit( &descHdl	);
	if(	llHdl->modId == M22_MOD_ID	|| llHdl->modId ==	M24_MOD_ID )
		M22_Exit( llHdlP );
	else
		{
			/* don't touch an unknown module */
			M22_MemCleanup( llHdl );
			*llHdlP = 0;
		}/*if*/
	return(	retCode	);
}/*M22_Init*/

/******************************	M22_Exit *************************************
 *
 *  Description:  De-initialize hardware and clean up memory.
 *
 *  Note:  Is called by MDIS kernel only.
 *---------------------------------------------------------------------------
 *	Input......:  llHdlP  pointer to low-level driver handle
 *
 *	Output.....:  llHdlP  NULL
 *				  return  0	| error	code
 *
 *	Globals....:  -
 *
 ****************************************************************************/
static int32 M22_Exit
(
 LL_HANDLE **llHdlP
 )
{
	DBGCMD(	static const char functionName[] = "LL - M22_Exit:"; )
		int			ch;
	LL_HANDLE	*llHdl	= *llHdlP;
	volatile u_int16	 intFromCh;
	int32		retCode;

	DBGWRT_1((DBH, "%s\n", functionName) );

	/*--------------------------+
	  | output off / disable irq	|
	  +--------------------------*/
	for	(ch=0; ch<llHdl->nbrOfChannels; ch++)
		{
			DBGWRT_2((DBH, "%s reset channel %d\n", functionName, ch )  );
			MWRITE_D16(llHdl->ma, IOREG(ch), 0);
			if(	llHdl->modId == M22_MOD_ID	)
				MWRITE_D16(	llHdl->ma, ALARMREG(ch), 0);
		}/*if*/

	/* dummy access	to clear interrupt */
	intFromCh =	MREAD_D16( llHdl->ma, INTREG );

	retCode	= M22_MemCleanup( llHdl );
	*llHdlP	= 0;

	return(	retCode	);
}/*M22_Exit*/

/******************************	M22_Read *************************************
 *
 *  Description:  Read from the device.
 *                Reads	input and output switch state and edge occurred flags
 *                from current channel.
 *
 *  byte structure of the LSB
 *
 *    bit            7       6       5       4       3       2       1       0
 *               +-------+-------+-------+-------+-------+-------+-------+-------+
 *    meaning    |   OS  |   r   |   r   |   r   |   r   |   FE  |   RE  |   I   |
 *               +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 *               OS:  output switch state
 *               FE:  falling edge occurred
 *               RE:  rising edge occurred
 *               I:   input state
 *               r:   reserved
 *---------------------------------------------------------------------------
 *	Input......:  llHdl	  pointer to low-level driver data structure
 *				  ch	  current channel 0..7..15
 *				  valueP  pointer to variable where	read value is stored
 *
 *	Output.....:  *valueP  read	value
 *				  return   0 always
 *
 *	Globals....:  -
 *
 ****************************************************************************/
static int32 M22_Read
(
 LL_HANDLE	*llHdl,
 int32		ch,
 int32		*valueP
 )
{
	DBGCMD(	static const char functionName[] = "LL - M22_Read:"; )
		u_int8	rdVal;

	DBGWRT_1((DBH, "%s\n",	functionName) );

	rdVal  = (u_int8) MREAD_D16( llHdl->ma,	IOREG(ch) );

	/* update state buffer
	 * - set/reset input bit
	 * - or edge bits
	 */
	if( rdVal & IOREG_INPUT_OR_ALARM_VAL )
		llHdl->stateBuf[ch] |= M22_24_READ_INPUT;
	else
		llHdl->stateBuf[ch] &= ~M22_24_READ_INPUT;

	llHdl->stateBuf[ch] |= (EDGE_OCCURRED_MASK & rdVal) >> 3;

	DBGWRT_2((DBH, "%s   ch=%d reg=%02x val=%02x\n", functionName, ch, rdVal, llHdl->stateBuf[ch]) );

	*valueP	= (	llHdl->stateBuf[ch] );		 /*	read value */
	return(0);
}/*M22_Read*/

/******************************	M22_Write ************************************
 *
 *  Description:  Set/reset the output switch of the current channel.
 *
 *  byte structure of the LSB
 *
 *    bit            7       6       5       4       3       2       1       0
 *               +-------+-------+-------+-------+-------+-------+-------+-------+
 *    meaning    |   r   |   r   |   r   |   r   |   r   |   r   |   r   |   OS  |
 *               +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 *               OS:  output switch state
 *               r:   reserved, should be 0
 *
 *  Note:  This function returns ERROR if M24 or channel is inactive.
 *---------------------------------------------------------------------------
 *	Input......:  llHdl	  pointer to low-level driver data structure
 *				  ch	  current channel 0..7..15
 *				  value	  output switch on/off	  0=off, 1=on
 *
 *	Output.....:  return  0	| error	code
 *
 *	Globals....:  -
 *
 ****************************************************************************/
static int32 M22_Write
(
 LL_HANDLE *llHdl,
 int32  ch,
 int32  value
 )
{
	DBGCMD(	static const char functionName[] = "LL - M22_Write:"; )
		int32   error = 0;

	DBGWRT_1((DBH, "%s ch=%d val=0x%02x\n",	functionName, ch, value) );

	if( !llHdl->activeCh[ch] || llHdl->modId == M24_MOD_ID )
		{
			error = ERR_LL_ILL_CHAN;
			DBGWRT_ERR( ( DBH,	"%s%s channel %d inactive %s%d%s",
						  errorStartStr, functionName, ch, errorLineStr, __LINE__, errorEndStr ));
			goto CLEANUP;
		}/*if*/

	/* update state buffer - set output switch */
	if (value)
		{
			llHdl->stateBuf[ch] |= M22_READ_OUTPUT_SWITCH;      /* set output switch bit */
			MSETMASK_D16(llHdl->ma, IOREG(ch), IOREG_OUTPUT_SWITCH );
		}
	else
		{
			llHdl->stateBuf[ch] &= ~M22_READ_OUTPUT_SWITCH;      /* reset output switch bit */
			MCLRMASK_D16(llHdl->ma, IOREG(ch), IOREG_OUTPUT_SWITCH );
		}/*if*/

 CLEANUP:
	return( error );
}/*M22_Write*/

/******************************	M22_SetStat	**********************************
 *
 *  Description:  Changes the device state.
 *
 *  COMMON CODES
 *  ============
 *
 *  Code                         Values          Meaning
 *  ----                         ------          -------
 *  M_MK_IRQ_ENABLE              0               disable module interrupt
 *								 1				 enable module interrupt
 *                                               by writing input/alarm edge mask
 *                                               of each active channel
 *
 *  M_LL_CH_DIR                  M_CH_INOUT      fix M22
 *                               M_CH_IN         fix M24
 *
 *
 *  M_LL_DEBUG_LEVEL             see oss.h       enable/disable debug output
 *                                               at task or interrupt level
 *
 *  M_LL_IRQ_COUNT               irq count       set the module interrupt counter
 *
 *
 *  SPECIFIC CODES
 *  ==============
 *
 *  Code                         Values          Meaning
 *  ----                         ------          -------
 *  M22_24_INPUT_EDGE_MASK       0..3            bit 0 - rising edge
 *                                               bit 1 - falling edge
 *
 *  M22_ALARM_EDGE_MASK          0..3            bit 0 - rising edge
 *                                               bit 1 - falling edge
 *
 *  M22_24_SIG_EDGE_OCCURRED     signal number   installs signal sent by ISR
 *
 *  M22_24_SIG_CLR_EDGE_OCCURRED signal number   deinstalls signal
 *
 *  M22_24_CLEAR_INPUT_EDGE      -               clears input edges of current ch
 *
 *  M22_24_SETBLOCK_CLEAR_INPUT_EDGE -           clears input edges of all ch
 *
 *  M22_CLEAR_ALARM_EDGE          -              clears alarm edges of current ch
 *                                               M22 only
 *
 *  M22_SETBLOCK_CLEAR_ALARM_EDGE    -           clears alarm edges of all ch
 *                                               M22 only
 *
 *  M22_24_CHANNEL_INACTIVE      0..1            0 - activate channel
 *                                               1 - deactivate channel
 *---------------------------------------------------------------------------
 *	Input......:  llHdl	 pointer to low-level driver data structure
 *				  code	 setstat code
 *				  ch	 current channel (ignored for some codes)
 *				  value	 setstat value or pointer to block setstat data
 *
 *	Output.....:  return 0 | error code
 *
 *	Globals....:  -
 *
 ****************************************************************************/
static int32 M22_SetStat
(
 LL_HANDLE *llHdl,
 int32  code,
 int32  ch,
 INT32_OR_64  value32_or_64
 )
{
	DBGCMD(	static const char functionName[] = "LL - M22_SetStat:"; )
		int32	retCode = 0;
	int32	value;
	INT32_OR_64		valueP;

	value	= (int32)value32_or_64;	/* store 32bit value */
	valueP	= value32_or_64;	/* store pointer     */

	DBGWRT_1((DBH, "%s code=$%04lx data=%ld\n",	functionName, code,	valueP) );

	switch(code)
		{
			/* ------ common setstat codes --------- */
			/*--------------------+
			  |  irq enable		  |
			  +--------------------*/
		case M_MK_IRQ_ENABLE:
			llHdl->irqEnabled = value;
			for( ch=0; ch<llHdl->nbrOfChannels; ch++ )
				{
					configureIrqForChannel( llHdl, ch, llHdl->irqEnabled );
				}/*for*/
			break;

			/*------------------+
			  |  ch direction	    |
			  +------------------*/
		case M_LL_CH_DIR:
			if(    (llHdl->modId == M22_MOD_ID && value != M_CH_INOUT)
				   || (llHdl->modId == M24_MOD_ID && value != M_CH_IN))
				{
					retCode = ERR_LL_ILL_PARAM;
					DBGWRT_ERR(( DBH, "%s%s:  illegal channel mode	%s%d%s",
								 errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
				}/*if*/
			break;

			/*------------------+
			  |  irq count		|
			  +------------------*/
		case M_LL_IRQ_COUNT:
			llHdl->irqCount = value;
			break;

			/*------------------+
			  |  debug level		|
			  +------------------*/
		case M_LL_DEBUG_LEVEL:
			llHdl->dbgLevel = value;
			break;


			/* ------ special setstat codes	--------- */

			/*-----------------------+
			  |  input edge mask of ch |
			  +-----------------------*/
		case M22_24_INPUT_EDGE_MASK:
			llHdl->inputEdgeMask[ch] = (u_int8)( IRQ_ENABLE_MASK & (value << 1 ));
			/* reconfigure irq(s) */
			configureIrqForChannel( llHdl, ch, llHdl->irqEnabled );
			break;

			/*-------------+
			  |  ch inactive |
			  +-------------*/
		case M22_24_CHANNEL_INACTIVE:
			llHdl->activeCh[ch] = (u_int8)(!value);
			/* reconfigure irq(s) */
			configureIrqForChannel( llHdl, ch, llHdl->irqEnabled );
			break;

			/*-----------------------+
			  |  alarm edge mask of ch |
			  +-----------------------*/
		case M22_ALARM_EDGE_MASK:
			if( llHdl->modId != M22_MOD_ID )
				{
					DBGWRT_ERR(	( DBH, "%s%s: M22_ALARM_EDGE_MASK on M22 only %s%d%s",
								  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
					retCode = ERR_LL_ILL_PARAM;
					break;
				}
			llHdl->alarmEdgeMask[ch] = (u_int8)( IRQ_ENABLE_MASK & ( value << 1 ));
			/* reconfigure irq(s) */
			configureIrqForChannel( llHdl, ch, llHdl->irqEnabled );
			break;

			/*--------------------+
			  |  signal conditions  |
			  +--------------------*/
		case M22_24_SIG_EDGE_OCCURRED:
			if( llHdl->sigHdl != NULL )	  /* already defined ? */
				{
					DBGWRT_ERR( ( DBH, "%s%s: signal	already	defined	%s%d%s",
								  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
					return(ERR_OSS_SIG_SET);		   /* can't	set	! */
				}/*if*/
			retCode = OSS_SigCreate(	llHdl->osHdl, value, &llHdl->sigHdl );
			break;

		case M22_24_SIG_CLR_EDGE_OCCURRED:
			retCode = OSS_SigRemove( llHdl->osHdl, &llHdl->sigHdl );
			break;

			/*-----------------------+
			  |  clear occurred edges  |
			  +-----------------------*/
		case M22_24_CLEAR_INPUT_EDGE:
			MCLRMASK_D16(llHdl->ma, IOREG(ch), EDGE_OCCURRED_MASK );
			llHdl->stateBuf[ch] &= ~(EDGE_OCCURRED_MASK >> 3);
			break;

		case M22_CLEAR_ALARM_EDGE:
			if( llHdl->modId != M22_MOD_ID )
				{
					DBGWRT_ERR(	( DBH, "%s%s: M22_CLEAR_ALARM_EDGE on M22 only %s%d%s",
								  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
					retCode = ERR_LL_ILL_PARAM;
					break;
				}
			MCLRMASK_D16(llHdl->ma, ALARMREG(ch), EDGE_OCCURRED_MASK );
			llHdl->alarmStateBuf[ch] &= ~(EDGE_OCCURRED_MASK >> 3);
			break;

			/*----------------+
			  |  default		  |
			  +----------------*/
		default:
			if(	   ( M_LL_BLK_OF <=	code &&	code <=	(M_LL_BLK_OF+0xff) )
				   || ( M_DEV_BLK_OF <= code && code <= (M_DEV_BLK_OF+0xff) )
				   )
				return(	setStatBlock( llHdl, code,	(M_SETGETSTAT_BLOCK*) valueP ) );

			DBGWRT_ERR(	( DBH, "%s%s:  unkown setstat code %s%d%s",
						  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
			retCode = ERR_LL_UNK_CODE;
		}/*switch*/

	return(	retCode	);
}/*M22_SetStat*/


/******************************	M22_GetStat	**********************************
 *
 *  Description:  Gets the device state.
 *
 *  COMMON CODES
 *  ============
 *
 *  Code                         Values          Meaning
 *  ----                         ------          -------
 *  M_LL_CH_NUMBER               8 | 16          number of channels
 *
 *  M_LL_CH_DIR                  M_CH_INOUT      direction of current ch
 *                               M_CH_IN         always in/out | in
 *
 *  M_LL_CH_LEN                  1               length in bits
 *
 *  M_LL_CH_TYP                  M_CH_BINARY     binary
 *
 *  M_LL_IRQ_COUNT               0..x            module irq count
 *
 *  M_LL_ID_CHECK                1               SPROM ID is always checked
 *
 *  M_LL_DEBUG_LEVEL             see oss.h       current debug level
 *
 *  M_LL_ID_SIZE                 128             eeprom size [bytes]
 *
 *  M_LL_BLK_ID_DATA             -               eeprom raw data
 *     blockStruct->size         0..0x80         number of bytes to read
 *     blockStruct->data pointer                 user buffer containing ID data
 *
 *  M_MK_BLK_REV_ID                              pointer to the ident function table
 *
 *
 *  SPECIFIC CODES
 *  ==============
 *
 *  Code                         Values          Meaning
 *  ----                         ------          -------
 *  M22_24_IRQ_SOURCE            irq source      irq-causing channel
 *
 *  M22_24_CHANNEL_INACTIVE      0..1            0 - channel is active
 *                                               1 - channel is inactive
 *
 *  M22_24_INPUT_EDGE_MASK       0..3            bit 0 - rising edge
 *                                               bit 1 - falling edge
 *                                               get the edge mask of the current
 *                                               channel
 *
 *  M22_ALARM_EDGE_MASK          0..3            bit 0 - rising edge
 *                                               bit 1 - falling edge
 *                                               get the edge mask of the current
 *                                               channel
 *
 *  M22_24_SIG_EDGE_OCCURRED     signal number   get the signal number
 *                                               0 if not installed
 *
 *  M22_GET_ALARM				 0..7			 gets the alarm state of
 *                                               the current channel
 *                                               bit 0 - alarm state
 *                                               bit 1 - rising alarm edge
 *                                               bit 2 - falling alarm edge
 *                                               M22 only
 *
 *  M22_GETBLOCK_ALARM                           gets the alarm state of all
 *                                               active channels
 *     blockStruct->size         0..8            number of bytes to read
 *     blockStruct->data pointer                 user buffer containing the
 *                                               alarm data (starting with ch #0)
 *                                               M22 only
 *---------------------------------------------------------------------------
 *	Input......:  llHdl	   pointer to low-level	driver data	structure
 *				  code	   getstat code
 *				  ch	   current channel (ignored	for	some codes)
 *				  valueP   pointer to variable where value is stored or
 *						   pointer to block setstat data
 *
 *	Output.....:  *valueP  getstat value
 *				  return   0 | error code
 *
 *	Globals....:  -
 *
 ****************************************************************************/
static int32 M22_GetStat
(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 *value32_or_64P
 )
{
	int32 retCode = 0;
	int32 processId;
	u_int8 rdVal;
	int32 *valueP = (int32*)value32_or_64P; /* pointer to 32bit value */
	INT32_OR_64 *value64P = value32_or_64P; /* stores 32/64bit pointer */
	DBGCMD( static const char functionName[] = "LL - M22_GetStat:" );

	DBGWRT_1((DBH, "%s code=$%04lx\n", functionName, code) );

	switch(code)
		{
			/* ------ common getstat codes --------- */
			/*------------------+
			  |  get ch count		|
			  +------------------*/
		case M_LL_CH_NUMBER:
			*valueP	= llHdl->nbrOfChannels;
			break;

			/*------------------+
			  |  ch direction		|
			  +------------------*/
		case M_LL_CH_DIR:
			if(	llHdl->modId == M22_MOD_ID	)
				*valueP = M_CH_INOUT;
			else
				*valueP = M_CH_IN;
			break;

			/*--------------------+
			  |  ch length (bit)	  |
			  +--------------------*/
		case M_LL_CH_LEN:
			*valueP = 1;
			break;

			/*------------------+
			  |  ch type			|
			  +------------------*/
		case M_LL_CH_TYP:
			*valueP = M_CH_BINARY;
			break;

			/*------------------+
			  |  irq count		|
			  +------------------*/
		case M_LL_IRQ_COUNT:
			*valueP	= llHdl->irqCount;
			break;

			/*------------------+
			  |  id check	enabled	|
			  +------------------*/
		case M_LL_ID_CHECK:
			*valueP	= 1;    /* always */
			break;

			/*------------------+
			  |  debug level		 |
			  +------------------*/
		case M_LL_DEBUG_LEVEL:
			*valueP	= llHdl->dbgLevel;
			break;

			/*--------------------+
			  |  id prom size		  |
			  +--------------------*/
		case M_LL_ID_SIZE:
			*valueP	= MOD_ID_SIZE;
			break;

			/*--------------------+
			  |  ident table		  |
			  +--------------------*/
		case M_MK_BLK_REV_ID:
			*value64P = (INT32_OR_64)&llHdl->idFuncTbl;
			break;

			/* ------ special getstat codes	--------- */
			/*------------------+
			  |  get irq source	|
			  +------------------*/
		case M22_24_IRQ_SOURCE:
			*valueP	= llHdl->irqSource;
			break;

			/*-------------+
			  |  ch inactive |
			  +-------------*/
		case M22_24_CHANNEL_INACTIVE:
			*valueP	= !llHdl->activeCh[ch];
			break;

			/*--------------------+
			  |  edge	mask of	ch	  |
			  +--------------------*/
		case M22_24_INPUT_EDGE_MASK:
			*valueP	= (llHdl->inputEdgeMask[ch]	& IRQ_ENABLE_MASK) >> 1;
			break;

		case M22_ALARM_EDGE_MASK:
			if( llHdl->modId != M22_MOD_ID )
				{
					DBGWRT_ERR(	( DBH, "%s%s: M22_ALARM_EDGE_MASK on M22 only %s%d%s",
								  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
					retCode = ERR_LL_ILL_PARAM;
					break;
				}
			*valueP	= (llHdl->alarmEdgeMask[ch]	& IRQ_ENABLE_MASK) >> 1;
			break;

			/*--------------------+
			  |  signal conditions  |
			  +--------------------*/
		case M22_24_SIG_EDGE_OCCURRED:
			OSS_SigInfo(	llHdl->osHdl, llHdl->sigHdl, valueP, &processId );
			break;

			/*-------------+
			  |  read alarm  |
			  +-------------*/
		case M22_GET_ALARM:
			if( llHdl->modId != M22_MOD_ID )
				{
					DBGWRT_ERR(	( DBH, "%s%s: M22_GET_ALARM on M22 only %s%d%s",
								  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
					retCode = ERR_LL_ILL_PARAM;
					break;
				}
			rdVal  = (u_int8) MREAD_D16( llHdl->ma,	ALARMREG(ch) );
			/* update state buffer
			 * - set/reset alarm bit
			 * - or edge bits
			 */
			llHdl->alarmStateBuf[ch] = (rdVal & IOREG_INPUT_OR_ALARM_VAL) ? 1 : 0;
			llHdl->alarmStateBuf[ch] |= (EDGE_OCCURRED_MASK & rdVal) >> 3;
			break;

			/*--------------------+
			  |  (unknown)		  |
			  +--------------------*/
		default:
			if(	   ( M_LL_BLK_OF <=	code &&	code <=	(M_LL_BLK_OF+0xff) )
				   || ( M_DEV_BLK_OF <= code && code <= (M_DEV_BLK_OF+0xff) )
				   )
				return(	getStatBlock( llHdl, code,	(M_SETGETSTAT_BLOCK*) valueP ) );

			DBGWRT_ERR(	( DBH, "%s%s:  unkown getstat code %s%d%s",
						  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
			return(ERR_LL_UNK_CODE);

		}/*switch*/

	return( retCode );
}/*M22_GetStat*/

/******************************* M22_BlockRead *******************************
 *
 *  Description:  Reads active channels to buffer.
 *                Stores input state and occurred edges.
 *                Starts with the lowest active channel number.
 *
 *  buffer structure
 *
 *    byte              0            ...            n
 *             +------------------+-     -+-------------------+-
 *    channel  | lowest active ch |  ...  | highest active ch |
 *             +------------------+-     -+-------------------+-
 *
 *  byte structure
 *
 *    bit          7       6       5       4       3       2       1       0
 *             +-------+-------+-------+-------+-------+-------+-------+-------+
 *    meaning  |   OS  |   r   |   r   |   r   |   r   |   FE  |   RE  |   I   |
 *             +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 *             OS:  output switch state
 *             FE:  falling edge occurred
 *             RE:  rising edge occurred
 *             I:   input state
 *             r:   reserved
 *---------------------------------------------------------------------------
 *	Input......:  llHdl		   pointer to low-level	driver data	structure
 *				  ch		   current channel (always ignored)
 *				  buf		   buffer to store read	values
 *				  size		   buffer size
 *				  nbrRdBytesP  pointer to variable where number	of read	bytes
 *							   will be stored
 *
 *	Output.....:  *nbrRdBytesP number of read bytes
 *				  return	   0 | error code
 *
 *	Globals....:  -
 *
 ****************************************************************************/
static int32 M22_BlockRead
(
 LL_HANDLE  *llHdl,
 int32	  ch,
 void	  *buf,
 int32	  size,
 int32	  *nbrRdBytesP
 )
{
	DBGCMD(	static const char functionName[] = "LL - M22_BlockRead:"; )
		int32  retCode = 0;
	int32  nbrRdBytes = 0;
	u_int8 rdVal;
	u_int8 *buffer = (u_int8*) buf;

	DBGWRT_1((DBH, "%s\n", functionName) );

	for( ch	= 0; ch	< llHdl->nbrOfChannels; ch++ )
		{
			if( llHdl->activeCh[ch] && nbrRdBytes < size )
				{
					rdVal  = (u_int8) MREAD_D16( llHdl->ma,	IOREG(ch) );

					/* update state buffer
					 * - set/reset input bit
					 * - or edge bits
					 */
					if( rdVal & IOREG_INPUT_OR_ALARM_VAL )
						llHdl->stateBuf[ch] |= M22_24_READ_INPUT;
					else
						llHdl->stateBuf[ch] &= ~M22_24_READ_INPUT;

					llHdl->stateBuf[ch] |= (EDGE_OCCURRED_MASK & rdVal) >> 3;
					*buffer++ = llHdl->stateBuf[ch];
					nbrRdBytes++;
				}/*if*/
		}/*for*/

	*nbrRdBytesP = nbrRdBytes;
	return(	retCode );
}/*M22_BlockRead*/

/******************************* M22_BlockWrite *******************************
 *
 *  Description:  Writes buffer to active channels.
 *                Starts with the lowest active channel number.
 *
 *  buffer structure
 *
 *    byte              0            ...            n
 *             +------------------+-     -+-------------------+-
 *    channel  | lowest active ch |  ...  | highest active ch |
 *             +------------------+-     -+-------------------+-
 *
 *  byte structure
 *
 *    bit          7       6       5       4       3       2       1       0
 *             +-------+-------+-------+-------+-------+-------+-------+-------+
 *    meaning  |   r   |   r   |   r   |   r   |   r   |   r   |   r   |   OS  |
 *             +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 *             OS:   output switch state
 *             r:    reserved, should be 0
 *---------------------------------------------------------------------------
 *	Input......:  llHdl		   pointer to low-level	driver data	structure
 *				  ch		   current channel (always ignored)
 *				  buf		   buffer
 *				  size		   buffer size
 *				  nbrWrBytesP  pointer to variable where number of written bytes
 *							   will be stored
 *
 *	Output.....:  *nbrWrBytesP number of read bytes
 *				  return	   0 | error code
 *
 *	Globals....:  -
 *
 ****************************************************************************/
static int32 M22_BlockWrite
(
 LL_HANDLE  *llHdl,
 int32	  ch,
 void	  *buf,
 int32	  size,
 int32	  *nbrWrBytesP
 )
{
	DBGCMD(	static const char functionName[] = "LL - M22_BlockWrite:"; )
		int32 retCode = 0;
	int32 nbrWrBytes = 0;
	u_int8 *buffer = (u_int8*) buf;
	u_int8 value;

	DBGWRT_1((DBH, "%s\n", functionName) );

	for( ch	= 0; ch	< llHdl->nbrOfChannels; ch++ )
		{
			if( llHdl->activeCh[ch] && nbrWrBytes < size )
				{
					value = (u_int8)*buffer++;

					/* update state buffer - set output switch */
					if (value)
						{
							llHdl->stateBuf[ch] |= M22_READ_OUTPUT_SWITCH;      /* set output switch bit */
							MSETMASK_D16(llHdl->ma, IOREG(ch), IOREG_OUTPUT_SWITCH );
						}
					else
						{
							llHdl->stateBuf[ch] &= ~M22_READ_OUTPUT_SWITCH;      /* reset output switch bit */
							MCLRMASK_D16(llHdl->ma, IOREG(ch), IOREG_OUTPUT_SWITCH );
						}/*if*/

					nbrWrBytes++;
				}/*if*/
		}/*for*/

	*nbrWrBytesP = nbrWrBytes;
	return(	retCode );
}/*M22_BlockWrite*/

/******************************	M22_Irq	*************************************
 *
 *  Description:  The irq routine reads all	channels.
 *                It stores the input states and the occurred edges to the
 *                read buffer if available.
 *
 *                The function clears the edge flag registers.
 *                If the signal is installed (setstat code M22_SIG_EDGE_OCCURRED)
 *                this is sent.
 *
 *                The function clears the M-Module IRQ and stores the number of
 *                the channel that triggered the interrupt.
 *                (getstat code M22_IRQ_SOURCE)
 *
 *                The function increments the M-Module irq counter if an edge was
 *                detected.
 *               (getstat code M_LL_IRQ_COUNT)
 *---------------------------------------------------------------------------
 *	Input......:  llHdl	 pointer to	low-level driver data structure
 *
 *	Output.....:  return MDIS_IRQ_DEV_NOT |	MDIS_IRQ_DEVICE
 *
 *	Globals....:  -
 *
 ****************************************************************************/
static int32 M22_Irq
(
 LL_HANDLE *llHdl
 )
{
	u_int8	   ch, intreg, alarm;

	IDBGWRT_1( ( DBH, ">>> M22_Irq:\n") );

	/*----------------------+
	  | check/reset irq		|
	  +----------------------*/
	intreg = (u_int8) MREAD_D16( llHdl->ma,	INTREG ); /* get irq source, reset	irq	! */

	if( llHdl->modId == M22_MOD_ID )
		{
			ch 		= (u_int8)((intreg & M22_IRQ_CH_NBR)	>> 1);						  /* channel caused irq */
			alarm 	= (u_int8)((intreg & M22_IRQ_ALARM) >> 4);
			IDBGWRT_2( ( DBH,">>> M22_Irq: ch=%d alarm=%d\n",	ch, alarm ) );
		}
	else
		{
			ch = (u_int8)((intreg & M24_IRQ_CH_NBR)	>> 1);						  /* channel caused irq */
			IDBGWRT_2( ( DBH,">>> M22_Irq: ch=%d\n",	ch ) );
		}

	/*----------------------+
	  | handle signal	cond.	|
	  +----------------------*/
	/* send	signal */
	if(	llHdl->sigHdl != NULL )
		if(	OSS_SigSend( llHdl->osHdl,	llHdl->sigHdl ) )
			IDBGWRT_ERR( ( DBH,	">>>  M22_Irq: OSS_SigSend failed\n") );

	llHdl->irqSource =	ch;				 /*	stores the irq source */
	llHdl->irqCount++;

	return(	LL_IRQ_UNKNOWN ); /* don't really know, if it's a shared interrupt */
}/*M22_Irq*/


/******************************	M22_Info ************************************
 *
 *	Description:  Gets low-level driver	info.
 *
 *				  NOTE:	can	be called before M22_Init().
 *
 *---------------------------------------------------------------------------
 *	Input......:  infoType
 *				  ...		variable argument list
 *
 *	supported info type	codes:		  value			 meaning
 *
 *	LL_INFO_HW_CHARACTER
 *	   arg2	 u_int32 *addrModeCharP	   MDIS_MODE_A08  M-Module characteristic
 *	   arg3	 u_int32 *dataModeCharP	   MDIS_MODE_D08  M-Module characteristic
 *
 *	LL_INFO_ADDRSPACE_COUNT
 *	   arg2	 u_int32 *nbrOfAddrSpaceP  1			  number of	used address spaces
 *
 *	LL_INFO_ADDRSPACE
 *	   arg2	 u_int32  addrSpaceIndex   0			  current address space
 *	   arg3	 u_int32 *addrModeP		   MDIS_MODE_A08  not used
 *	   arg4	 u_int32 *dataModeP		   MDIS_MODE_D16  driver use D16 access
 *	   arg5	 u_int32 *addrSizeP		   0x100		  needed size
 *
 *	LL_INFO_IRQ
 *	   arg2	 u_int32 *useIrqP		   1			  module uses interrupts
 *
 *	LL_INFO_LOCKMODE
 *	   arg2  u_int32 *lockModeP		LL_LOCK_CALL	used lockmode
 *
 *	Output.....:  0	| error	code
 *
 *	Globals....:  -
 *
 ****************************************************************************/
static int32 M22_Info
(
 int32  infoType,
 ...
 )
{
	int32	error;
	va_list	argptr;
	u_int32	*nbrOfAddrSpaceP;
	u_int32	*addrModeP;
	u_int32	*dataModeP;
	u_int32	*addrSizeP;
	u_int32	*useIrqP;
	u_int32	addrSpaceIndex;

	error =	0;
	va_start( argptr, infoType );

	switch(	infoType )
		{
		case LL_INFO_HW_CHARACTER:
			addrModeP	 = va_arg( argptr, u_int32*	);
			dataModeP	 = va_arg( argptr, u_int32*	);
			*addrModeP = MDIS_MA08;
			*dataModeP = MDIS_MD08;
			break;

		case LL_INFO_ADDRSPACE_COUNT:
			nbrOfAddrSpaceP  = va_arg( argptr, u_int32* );
			*nbrOfAddrSpaceP = 1;
			break;

		case LL_INFO_ADDRSPACE:
			addrSpaceIndex = va_arg( argptr, u_int32 );
			addrModeP	 = va_arg( argptr, u_int32*	);
			dataModeP	 = va_arg( argptr, u_int32*	);
			addrSizeP	 = va_arg( argptr, u_int32*	);

			switch( addrSpaceIndex )
				{
				case 0:
					*addrModeP = MDIS_MA08;
					*dataModeP = MDIS_MD16;
					*addrSizeP = 0x100;
					break;

				default:
					error = ERR_LL_ILL_PARAM;
				}/*switch*/
			break;

		case LL_INFO_IRQ:
			useIrqP  = va_arg( argptr, u_int32* );
			*useIrqP = 1;
			break;

		case LL_INFO_LOCKMODE:
			{
				u_int32 *lockModeP = va_arg(argptr, u_int32*);

				*lockModeP = LL_LOCK_CALL;
				break;
			}

		default:
			error	= ERR_LL_ILL_PARAM;
		}/*switch*/

	va_end(	argptr );
	return(	error );
}/*M22_Info*/

/**************************	getStatBlock *************************************
 *
 *	Description:  Decodes the M_SETGETSTAT_BLOCK code and executes them.
 *                Codes are described in the calling function.
 *
 *---------------------------------------------------------------------------
 *	Input......:  llHdl		     m22 handle
 *				  code			 getstat code
 *				  blockStruct	 the struct	with code size and data	buffer
 *
 *	Output.....:  blockStruct->data	 filled	data buffer
 *				  return			 0 | error code
 *
 *	Globals....:  -
 *
 ****************************************************************************/
static int32 getStatBlock     /*nodoc*/
(
 LL_HANDLE		   *llHdl,
 int32			   code,
 M_SETGETSTAT_BLOCK *blockStruct
 )
{
	DBGCMD(	static const char functionName[] = "LL - getStatBlock:";	)
		int32	error, ch;
	u_int8	i, rdVal, *buffer;
	u_int32	maxWords;
	u_int16	*dataP;
	int32   nbrRdBytes=0;

	DBGWRT_1((DBH, "%s\n", functionName) );

	error =	0;
	switch(	code )
		{
		case M_LL_BLK_ID_DATA:
			if (blockStruct->size	< MOD_ID_SIZE)		/* check buf size */
				return(ERR_LL_USERBUF);

			maxWords	= blockStruct->size	/ 2;
			dataP = (u_int16*)(blockStruct->data);
			for(	i=0; i<maxWords; i++ )
				{
					*dataP++ = (int16)m_read((U_INT32_OR_64)llHdl->ma, (int8)i);
				}/*for*/
			break;

		case M22_GETBLOCK_ALARM:
			if( llHdl->modId != M22_MOD_ID )
				{
					DBGWRT_ERR(	( DBH, "%s%s: M22_GETBLOCK_ALARM on M22 only %s%d%s",
								  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
					return( ERR_LL_ILL_PARAM );
				}
			buffer = (u_int8*)(blockStruct->data);
			for( ch	= 0; ch	< llHdl->nbrOfChannels; ch++ )
				{
					if( llHdl->activeCh[ch] && nbrRdBytes < blockStruct->size )
						{
							rdVal  = (u_int8) MREAD_D16( llHdl->ma,	ALARMREG(ch) );

							/* update state buffer
							 * - set/reset alarm bit
							 * - or edge bits
							 */
							llHdl->alarmStateBuf[ch] = (rdVal & IOREG_INPUT_OR_ALARM_VAL) ? 1 : 0;
							llHdl->alarmStateBuf[ch] |= (EDGE_OCCURRED_MASK & rdVal) >> 3;
							*buffer++ = llHdl->alarmStateBuf[ch];
							nbrRdBytes++;
						}/*if*/
				}/*for*/
			break;

		default:
			DBGWRT_ERR( ( DBH, "%s%s:  unkown blockgetstat code %s%d%s",
						  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
			error = ERR_LL_UNK_CODE;
		}/*switch*/

	return(	error );
}/*getStatBlock*/

/**************************	setStatBlock *************************************
 *
 *	Description:  Decodes the M_SETGETSTAT_BLOCK code and executes them.
 *                Codes are described in the calling function.
 *
 *
 *---------------------------------------------------------------------------
 *	Input......:  llHdl		     m22 handle
 *				  code			 getstat code
 *				  blockStruct	 the struct	with code size and data	buffer
 *
 *	Output.....:  blockStruct->data	 filled	data buffer
 *				  return			 0 | error code
 *
 *	Globals....:  -
 *
 ****************************************************************************/
static int32 setStatBlock     /*nodoc*/
(
 LL_HANDLE		   *llHdl,
 int32			   code,
 M_SETGETSTAT_BLOCK *blockStruct
 )
{
	DBGCMD(	static const char functionName[] = "LL - setStatBlock:";	)
		int32	error, ch;

	DBGWRT_1((DBH, "%s\n", functionName) );

	error =	0;
	switch(	code )
		{
		case M22_24_SETBLOCK_CLEAR_INPUT_EDGE:
			for( ch	= 0; ch	< llHdl->nbrOfChannels; ch++ )
				{
					if( llHdl->activeCh[ch] )
						{
							MCLRMASK_D16(llHdl->ma, IOREG(ch), EDGE_OCCURRED_MASK );
							llHdl->stateBuf[ch] &= ~(EDGE_OCCURRED_MASK >> 3);
						}/*if*/
				}/*for*/
			break;

		case M22_SETBLOCK_CLEAR_ALARM_EDGE:
			if( llHdl->modId != M22_MOD_ID )
				{
					DBGWRT_ERR(	( DBH, "%s%s: M22_SETBLOCK_CLEAR_ALARM_EDGE on M22 only %s%d%s",
								  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
					return( ERR_LL_ILL_PARAM );
				}
			for( ch	= 0; ch	< llHdl->nbrOfChannels; ch++ )
				{
					if( llHdl->activeCh[ch] )
						{
							MCLRMASK_D16(llHdl->ma, ALARMREG(ch), EDGE_OCCURRED_MASK );
							llHdl->alarmStateBuf[ch] &= ~(EDGE_OCCURRED_MASK >> 3);
						}/*if*/
				}/*for*/
			break;

		default:
			DBGWRT_ERR( ( DBH, "%s%s:  unkown blockgetstat code %s%d%s",
						  errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
			error = ERR_LL_UNK_CODE;
		}/*switch*/

	return(	error );
}/*setStatBlock*/
