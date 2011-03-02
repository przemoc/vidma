DOCS := AUTHORS NEWS README.md
OBJS := main.o vdi.o
MAN1 := vidma.1
OUT := vidma

CC_V := $(shell $(CC) -v 2>&1)

ifneq (,$(findstring mingw,$(CC_V)))
	OBJS += common_win.o
else
	OBJS += common_posix.o
endif

VIDMA_VERSION := $(shell \
	git describe --tags --dirty 2>/dev/null || \
	awk "/\* What's new in version /"'{print "v" $$6 ((NR!=1)?"+":"");exit}' NEWS 2>/dev/null || \
	echo unknown)

CFLAGS += -Wall -O2
override CFLAGS += -std=c99 -Wno-variadic-macros \
                   -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=600 \
                   -DVIDMA_VERSION="\"$(VIDMA_VERSION)\""

PREFIX := /usr/local
EPREFIX := $(PREFIX)
BINDIR := $(EPREFIX)/bin
DATADIR := $(PREFIX)/share
DOCDIR := $(DATADIR)/doc/$(OUT)
MANDIR := $(DATADIR)/man

INSTALL := install -o root -g 0
INSTALL_DATA := $(INSTALL) -m 0644
INSTALL_EXEC := $(INSTALL) -m 0755

all: $(OUT)

main.o: FORCE main.c common.h
vdi.o: vdi.c vdi.h common.h

$(OUT): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.1: %.1.ronn
	ronn -r $<

%.1.html: %.1.ronn
	ronn -5 --style=toc $<

clean:
	$(RM) $(OUT) $(OBJS)

install: $(OUT) $(MAN1)
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL_EXEC) $(OUT) $(DESTDIR)$(BINDIR)/$(OUT)
	$(INSTALL) -d $(DESTDIR)$(DOCDIR)
	$(INSTALL_DATA) $(DOCS) $(DESTDIR)$(DOCDIR)
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man1
	$(INSTALL_DATA) $(MAN1) $(DESTDIR)$(MANDIR)/man1/$(MAN1)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(OUT)
	$(RM) -r $(DESTDIR)$(DOCDIR)
	$(RM) $(DESTDIR)$(MANDIR)/man1/$(MAN1)

FORCE:

.PHONY: all clean install uninstall
