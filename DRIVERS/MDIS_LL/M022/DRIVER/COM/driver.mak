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
#-----------------------------------------------------------------------------
#   Copyright (c) 1998-2019, MEN Mikro Elektronik GmbH
#*****************************************************************************
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

