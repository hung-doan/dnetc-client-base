#
# Makefile for MorphOS dnetcgui.library
#

TARGET	= ../../../../dnetcgui.library
#TARGET = dnetcgui.library
#
##
#

# If you use another compiler change the following line:
CC	= ppc-morphos-gcc
CDEFS	= -DUSE_INLINE_STDARG
CWARN	= -Wall -Wno-parentheses -Wno-missing-braces
CFLAGS	= -noixemul -O2 $(CDEFS) $(CWARN) -fomit-frame-pointer -finline-functions -mstring -mmultiple
LDFLAGS	= -noixemul -nostartfiles -nostdlib

OBJS	= LibHeader.o LibInit.o AppClass.o MUIGUI.o CreateGUI.o ListClass.o

#
# target
#

.PHONY: all clean

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).db

$(TARGET): $(OBJS)
		$(CC) $(LDFLAGS) -o $@.db $(OBJS) -labox
		ppc-morphos-strip --strip-unneeded --strip-debug --remove-section .comment -o $@ $@.db
		-flushlib dnetcgui.library

GLOBAL		= LibHeader.h CreateGUI.h dnetcgui.h ../dnetcgui.h
LibHeader.o	: LibHeader.c $(GLOBAL) guilib_version.h
LibInit.o	: LibInit.c  $(GLOBAL) AppClass.h ListClass.h
AppClass.o	: AppClass.c $(GLOBAL) AppClass.h ListClass.h
ListClass.o	: ListClass.c $(GLOBAL) ListClass.h
CreateGUI.o	: CreateGUI.c $(GLOBAL) guilib_version.h
MUIGUI.o	: MUIGUI.c $(GLOBAL)


includes: ../fd/dnetcgui_lib.fd ../clib/dnetcgui_protos.h
	mkdir -p clib proto ppcinline
	cvinclude.pl --fd ../fd/dnetcgui_lib.fd --clib ../clib/dnetcgui_protos.h --outclib clib/dnetcgui_protos.h
	cvinclude.pl --fd ../fd/dnetcgui_lib.fd --clib ../clib/dnetcgui_protos.h --proto proto/dnetcgui.h
	cvinclude.pl --fd ../fd/dnetcgui_lib.fd --clib ../clib/dnetcgui_protos.h --inlines ppcinline/dnetcgui.h
