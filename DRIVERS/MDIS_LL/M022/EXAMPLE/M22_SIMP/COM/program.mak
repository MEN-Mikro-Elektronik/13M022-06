#***************************  M a k e f i l e  *******************************
#
#         Author: uf
#          $Date: 2004/04/29 11:23:09 $
#      $Revision: 1.2 $
#        $Header: /dd2/CVSR/COM/DRIVERS/MDIS_LL/M022/EXAMPLE/M22_SIMP/COM/program.mak,v 1.2 2004/04/29 11:23:09 cs Exp $
#
#    Description: makefile descriptor file for common
#                 modules MDIS 4.x   e.g. low level driver
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.2  2004/04/29 11:23:09  cs
#   removed needless switch MAK_OPTIM
#
#   Revision 1.1  2000/07/06 16:49:34  Franke
#   Initial Revision
#
#   Revision 1.1  1998/02/19 16:13:52  franke
#   Added by mcvs
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=m22_simp

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)    \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)     \


MAK_INCL=$(MEN_INC_DIR)/m22_drv.h     \
         $(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/mdis_api.h    \
         $(MEN_INC_DIR)/mdis_err.h    \
         $(MEN_INC_DIR)/usr_oss.h     \


MAK_INP1=m22_simp$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
