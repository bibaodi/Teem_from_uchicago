#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2009--2023  University of Chicago
# Copyright (C) 2005--2008  Gordon Kindlmann
# Copyright (C) 1998--2004  University of Utah
#
# This library is free software; you can redistribute it and/or modify it under the terms
# of the GNU Lesser General Public License (LGPL) as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any later version.
# The terms of redistributing and/or modifying this software also include exceptions to
# the LGPL that facilitate static linking.
#
# This library is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along with
# this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
# Fifth Floor, Boston, MA 02110-1301 USA
#


TEEM_SHEXT = so

OPT_CFLAG ?= -O2
STATIC_CFLAG = -static
SHARED_CFLAG = -shared
SHARED_LDFLAG = -shared
SHARED_RPATH = -Wl,-rpath -Wl,

TEEM_QNANHIBIT = 1
TEEM_DIO = 0
ifeq ($(SUBARCH),ia64)
  ARCH_CFLAG = -fPIC -W -Wall
  ARCH_LDFLAG =
else
  ifeq ($(SUBARCH),amd64)
    ARCH_CFLAG = -fPIC -W -Wall
    ARCH_LDFLAG =
  else
    ifeq ($(SUBARCH),32)
      ARCH_CFLAG = -W -Wall
      ARCH_LDFLAG = 
    else
      $(error linux sub-architecture "$(SUBARCH)" not recognized)
    endif
  endif
endif

TEEM_ZLIB.IPATH ?=
TEEM_ZLIB.LPATH ?=

TEEM_BZIP2.IPATH ?=
TEEM_BZIP2.LPATH ?=
