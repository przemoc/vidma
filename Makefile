OBJS := main.o vdi.o
OUT := vidma

CC_V := $(shell $(CC) -v 2>&1)

ifneq (,$(findstring mingw,$(CC_V)))
	OBJS += common_win.o
else
	OBJS += common_posix.o
endif

VIDMA_VERSION := $(shell \
	git log -0 && git describe --tags --dirty || \
	awk "/\* What's new in version /"'{print "v" $$6 ((NR!=1)?"+":"");nextfile}' NEWS)

CFLAGS += -Wall -O2
override CFLAGS += -std=c99 -Wno-variadic-macros \
                   -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=600 \
                   -DVIDMA_VERSION="\"$(VIDMA_VERSION)\""

PREFIX := $(DESTDIR)/usr/local
VIDMA_DIR := $(PREFIX)
VIDMA_BINDIR := $(VIDMA_DIR)/bin

INSTALL := install -c -o root -g 0
INSTALL_PROGRAM := $(INSTALL) -m 755

all: $(OUT)

main.o: FORCE main.c common.h
vdi.o: vdi.c vdi.h common.h

vidma: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

clean:
	$(RM) $(OUT) $(OBJS)

install:
	$(INSTALL_PROGRAM) $(OUT) $(VIDMA_BINDIR)/$(OUT)

uninstall:
	$(RM) $(VIDMA_BINDIR)/$(OUT)

FORCE:

.PHONY: all clean install uninstall
