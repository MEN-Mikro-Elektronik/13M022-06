#***************************  M a k e f i l e  *******************************
#
#        $Author: cs $
#          $Date: 2004/04/29 11:23:15 $
#      $Revision: 1.2 $
#        $Header: /mnt/swserver-disc/CVSR/COM/DRIVERS/MDIS_LL/M022/TEST/M22_MAIN/COM/program.mak,v 1.2 2004/04/29 11:23:15 cs Exp $
#
#    Description: makefile descriptor file for common
#                 modules MDIS 4.x   e.g. low level driver
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.2  2004/04/29 11:23:15  cs
#   removed needless switch MAK_OPTIM=$(OPT_1)
#
#   Revision 1.1  2000/07/06 16:49:43  Franke
#   Initial Revision
#
#   Revision 1.1  1998/02/19 16:13:46  franke
#   Added by mcvs
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=m22_main

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)    \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)     \


MAK_INCL=$(MEN_INC_DIR)/m22_drv.h     \
         $(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/mdis_api.h    \
         $(MEN_INC_DIR)/mdis_err.h    \
         $(MEN_INC_DIR)/usr_oss.h     \


MAK_INP1=m22_main$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
