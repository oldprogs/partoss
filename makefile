OBJS = archives.o attach.o buftomsg.o buftopkt.o \
       buftosqd.o chains.o control.o crc32.o environ.o errors.o handyman.o \
       headers.o killold.o linkareb.o linkbase.o lowlevel.o \
       morfiles.o msgtobuf.o packets.o parscomp.o parsmain.o parsrout.o \
       partoss.o partpost.o partserv.o partset.o partsqd.o \
       pkttobuf.o rebuild.o scanbase.o sqdtobuf.o template.o \
       times.o tossarcs.o _portlib.o mappath.o astring.o areaalias.o
SRCS = archives.cc attach.cc buftomsg.cc \
       buftopkt.cc buftosqd.cc chains.cc control.cc crc32.cc environ.cc \
       errors.cc handyman.cc headers.cc killold.cc linkareb.cc \
       linkbase.cc lowlevel.cc morfiles.cc msgtobuf.cc \
       packets.cc parscomp.cc parsmain.cc parsrout.cc partoss.cc \
       partpost.cc partserv.cc partset.cc partsqd.cc \
       pkttobuf.cc rebuild.cc scanbase.cc sqdtobuf.cc \
       template.cc times.cc tossarcs.cc _portlib.cc mappath.cc astring.cc \
       areaalias.cc

VERSION=1.10.03b-lnx
BUILD=alpha-build
TARFILE="partoss-${VERSION}-${BUILD}.tar.gz"
SRC_TARFILE="partoss-${VERSION}-${BUILD}-src.tar.gz"

BINS = partossl
PREFIX = /fido/partoss

INSTALL = install
INSOPTS = -s -m 755 -o 0 -g 0
CC = gcc
CFLAGS =  -O2 -m386 -pipe -static -g -ansi
CFLAGS += -Wall -Wstrict-prototypes -Wcast-align
CFLAGS += -fpack-struct -funsigned-char -fno-common
CFLAGS += -D_GNU_SOURCE -D_BSD_SOURCE -D__LNX__
CPPFLAGS=$(CFLAGS)

# include dependancies if present
ifeq (.depend,$(wildcard .depend))
include .depend
endif

all: partossl

partossl: $(OBJS)
	$(CC) $(CFLAGS) -o $(BINS) $(OBJS)

test: $(test_obj)
	$(CC) $(CFLAGS) -o test $(test_obj)

clean:
	rm -f $(BINS)
	rm -f $(OBJS)
	rm -f *~

install: all
	for $i in $(BINS) ; do \
	  $(INSTALL) $(INSOPTS) $$i $(PREFIX) ; \
	done
	for $i in $(cat .cfilelist) ; do \
	  $(INSTALL) $(INSOPTS) $$i $(PREFIX) ; \
	done

dep depend:
	$(CC) -M $(CPPFLAGS) $(SRCS) >.depend
	$(CC) -M $(CPPFLAGS) $(test_src) >>.depend


tar: ${TARFILE}
src_tar: ${SRC_TARFILE}

${TARFILE}:
	rm -f ${TARFILE}
	echo partossl > .filelist
	echo NOTES.lnx >>.filelist
	tar cvTf .filelist - | gzip >${TARFILE}

${SRC_TARFILE}: src_filelist
	rm -f ${SRC_TARFILE}
	tar cvTf .sfilelist - | gzip >${SRC_TARFILE}

src_filelist:
	echo Makefile >.sfilelist
	ls *.cpp >>.sfilelist
	ls *.h >>.sfilelist

cfg_filelist:
	ls cfg/* >.cfilelist
