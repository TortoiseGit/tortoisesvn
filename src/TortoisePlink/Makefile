##########################################################################
# Variables

# Taget name
TARGET=TortoisePlink

# Binary file
BIN=TortoisePlink.exe

# Source files
SOURCE=\
	be_all.c \
	cmdline.c \
	console.c \
	cproxy.c \
	ldisc.c \
	logging.c \
	misc.c \
	noise.c \
	pageantc.c \
	plink.c \
	portfwd.c \
	pproxy.c \
	proxy.c \
	raw.c \
	rlogin.c \
	settings.c \
	ssh.c \
	sshaes.c \
	sshblowf.c \
	sshbn.c \
	sshcrc.c \
	sshcrcda.c \
	sshdes.c \
	sshdh.c \
	sshdss.c \
	sshmd5.c \
	sshpubk.c \
	sshrand.c \
	sshrsa.c \
	sshsh512.c \
	sshsha.c \
	sshzlib.c \
	telnet.c \
	tree234.c \
	version.c \
	wildcard.c \
	windefs.c \
	winmisc.c \
	winnet.c \
	winstore.c \
	x11fwd.c \
	LoginDialog.cpp
	
RESOURCE=plink.rc

# Compiler flags
CXXFLAGS=$(STLP_CFLAGS) -Wall -fno-rtti -DWIN32 -D_MBCS -O2 -DNOTRANSLATE 
CCFLAGS=-Wall -O2 -D_WINDOWS -DWIN32S_COMPAT -D_NO_OLDNAMES -DNO_MULTIMON -I.
ifeq ($(DEBUG),1)
CXXFLAGS+=-D_DEBUG 
CCFLAGS+=-DDEBUG
else
CXXFLAGS+=-DNDEBUG -DRELEASE="0.56"
CCFLAGS+=-DDEBUG -DRELEASE="0.56"
endif

# Linker flags
LDFLAGS=$(STLP_LDFLAGS) -lws2_32 -lkernel32 -luser32 -lgdi32 -lshell32 -lole32 -loleaut32 -luuid -lshlwapi \
	-Wl,--subsystem,console -mwindows
ifeq ($(DEBUG),1)
else
LDFLAGS+=-s
endif

# Include Make environment
include ../Makeenv

# Include Make program
include ../Makeprog



	


	


