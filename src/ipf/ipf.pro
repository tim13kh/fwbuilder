#-*- mode: makefile; tab-width: 4; -*-
#
include(../../qmake.inc)
#
SOURCES	 =  ipf.cpp

HEADERS	 = ../../config.h \
		../pflib/OSData.h \
		../pflib/NATCompiler_ipf.h \
		../pflib/NATCompiler_pf.h \
		../pflib/OSConfigurator_freebsd.h \
		../pflib/OSConfigurator_solaris.h \
		../pflib/PolicyCompiler_ipf.h \
		../pflib/PolicyCompiler_pf.h

!win32 {
	QMAKE_COPY    = ../../install.sh -m 0755 -s
}

win32:CONFIG += console

unix { !macx: CONFIG -= qt }

INCLUDEPATH += "../pflib"
DEPENDPATH   = "../pflib"

win32:LIBS  += ../pflib/release/fwbpf.lib
!win32:LIBS += ../pflib/libfwbpf.a

LIBS  += $$LIBS_FWCOMPILER

TARGET      = fwb_ipf
