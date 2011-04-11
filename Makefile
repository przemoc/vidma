DOCS := AUTHORS NEWS README.md
OBJS := main.o vdi.o ui-cli.o
MAN1 := vidma.1
OUT := vidma

CC_V := $(shell $(CC) -v 2>&1)

ifneq (,$(findstring mingw,$(CC_V)))
	OBJS += common_win.o
else
	OBJS += common_posix.o
endif

SRCDIR := .
NOT_IN_SRCDIR := $(shell test ! . -ef $(SRCDIR) && echo 1)
GIT_WORK_TREE := $(SRCDIR)

VIDMA_VERSION := $(shell \
	GIT_WORK_TREE="$(GIT_WORK_TREE)" git describe --tags --dirty 2>/dev/null || \
	awk "/\* What's new in version /"'{print "v" $$6 ((NR!=1)?"+":"");exit}' $(SRCDIR)/NEWS 2>/dev/null || \
	echo unknown )

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

vpath %.c $(SRCDIR)
vpath %.h $(SRCDIR)
vpath %.ronn $(SRCDIR)

all: $(OUT)

main.o: FORCE main.c vdi.h ui.h common.h
vdi.o: vdi.c vdi.h vd.h ui.h common.h
ui-cli.o: ui-cli.c ui.h common.h

$(OUT): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.1: %.1.ronn
	$(if $(shell test -n "$(NOT_IN_SRCDIR)" -a -f $(basename $<) -a ! $(basename $<) -ot $< && echo yes) , \
		cp  $(basename $<) $@ , \
		ronn --pipe -r $< >$@ \
	)

%.1.html: %.1.ronn
	ronn -5 --pipe --style=toc $< >$@

clean:
	$(RM) $(OUT) $(OBJS) $(if $(NOT_IN_SRCDIR),$(MAN1))

install: $(OUT) $(MAN1)
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL_EXEC) $(OUT) $(DESTDIR)$(BINDIR)/$(OUT)
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man1
	$(INSTALL_DATA) $(SRCDIR)/$(MAN1) $(DESTDIR)$(MANDIR)/man1/$(MAN1)
	$(INSTALL) -d $(DESTDIR)$(DOCDIR)
	$(INSTALL_DATA) $(addprefix $(SRCDIR)/,$(DOCS)) $(DESTDIR)$(DOCDIR)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(OUT)
	$(RM) -r $(DESTDIR)$(DOCDIR)
	$(RM) $(DESTDIR)$(MANDIR)/man1/$(MAN1)

FORCE:

.PHONY: all clean install uninstall
