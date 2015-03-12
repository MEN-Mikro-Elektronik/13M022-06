#***************************  M a k e f i l e  *******************************
#
#         Author: uf
#          $Date: 2004/04/29 11:22:47 $
#      $Revision: 1.2 $
#        $Header: /dd2/CVSR/COM/DRIVERS/MDIS_LL/M022/DRIVER/COM/driver.mak,v 1.2 2004/04/29 11:22:47 cs Exp $
#
#    Description: makefile descriptor file for common
#                 modules MDIS 4.x   e.g. low level driver
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: driver.mak,v $
#   Revision 1.2  2004/04/29 11:22:47  cs
#   Minor changes for MDIS4/2004 conformity
#     added MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED
#     added include dbg.h
#
#   Revision 1.1  2000/07/06 16:49:11  Franke
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=m22

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED \

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/desc$(LIB_SUFFIX)     \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/id$(LIB_SUFFIX)       \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/oss$(LIB_SUFFIX)      \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/dbg$(LIB_SUFFIX) 


MAK_INCL=\
         $(MEN_INC_DIR)/m22_drv.h     \
         $(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/oss.h         \
         $(MEN_INC_DIR)/dbg.h         \
         $(MEN_INC_DIR)/mdis_err.h    \
         $(MEN_INC_DIR)/maccess.h     \
         $(MEN_INC_DIR)/desc.h        \
         $(MEN_INC_DIR)/mdis_api.h    \
         $(MEN_INC_DIR)/mdis_com.h    \
         $(MEN_INC_DIR)/modcom.h      \
         $(MEN_INC_DIR)/ll_defs.h     \
         $(MEN_INC_DIR)/ll_entry.h    \


MAK_INP1=m22_drv$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)

