OBJS = archives.o attach.o buftomsg.o buftopkt.o \
       buftosqd.o chains.o control.o crc32.o environ.o errors.o handyman.o \
       headers.o killold.o linkareb.o linkbase.o lowlevel.o \
       morfiles.o msgtobuf.o packets.o parscomp.o parsmain.o parsrout.o \
       partoss.o partpost.o partserv.o partset.o partsqd.o \
       pkttobuf.o rebuild.o scanbase.o sqdtobuf.o template.o version.o \
       times.o tossarcs.o _portlib.o mappath.o astring.o arealias.o
SRCS = archives.cc attach.cc buftomsg.cc \
       buftopkt.cc buftosqd.cc chains.cc control.cc crc32.cc environ.cc \
       errors.cc handyman.cc headers.cc killold.cc linkareb.cc \
       linkbase.cc lowlevel.cc morfiles.cc msgtobuf.cc \
       packets.cc parscomp.cc parsmain.cc parsrout.cc partoss.cc \
       partpost.cc partserv.cc partset.cc partsqd.cc \
       pkttobuf.cc rebuild.cc scanbase.cc sqdtobuf.cc version.cc \
       template.cc times.cc tossarcs.cc _portlib.cc mappath.cc astring.cc \
       arealias.cc

BINS = partossl

CC = gcc
CFLAGS =  -O2 -m386 -pipe -static -ansi
CFLAGS += -Wall -Wstrict-prototypes -Wcast-align
CFLAGS += -fpack-struct -funsigned-char -fno-common
CFLAGS += -D_GNU_SOURCE -D_BSD_SOURCE
#CFLAGS += -D__REMAP__ -D__REMAP_LOWER__
CPPFLAGS=$(CFLAGS)

all: partossl

partossl: $(OBJS)
	$(CC) $(CFLAGS) -o $(BINS) $(OBJS)

clean:
	rm -f $(BINS)
	rm -f $(OBJS)
	rm -f *~
		    
