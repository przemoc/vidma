OBJS = main.o vdi.o
OUT = vidma

CC_V := $(shell $(CC) -v 2>&1)

ifneq (,$(findstring mingw,$(CC_V)))
	OBJS += common_win.o
else
	OBJS += common_posix.o
endif

CFLAGS += -Wall
override CFLAGS += -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=600

PREFIX = $(DESTDIR)/usr/local
VIDMA_DIR = $(PREFIX)
VIDMA_BINDIR = $(VIDMA_DIR)/bin

INSTALL = install -c -o root -g 0
INSTALL_PROGRAM = $(INSTALL) -m 755

all: $(OUT)

main.o: main.c common.h
vdi.o: vdi.c vdi.h common.h

vidma: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

clean:
	$(RM) -rf $(OUT) $(OBJS)

install:
	$(INSTALL_PROGRAM) $(OUT) $(VIDMA_BINDIR)/$(OUT)

uninstall:
	$(RM) $(VIDMA_BINDIR)/$(OUT)

.PHONY: all clean install uninstall
