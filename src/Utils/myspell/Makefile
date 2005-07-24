# Myspell Makefile for Unix Platforms
# Currently Supports Linux, Darwin(MacOSX), FreeBSD, NetBSD

PREFIX?=/usr/local

VERMAJOR=3
VERMINOR=2
VERSION=$(VERMAJOR).$(VERMINOR)

PLATFORM := $(shell uname -s)

ifeq "$(PLATFORM)" "Linux"
  CXX ?= g++
  CXXFLAGS ?= -O2 -Wall -ansi -pedantic -I.
  CC ?= gcc
  CFLAGS ?= -O2 -Wall -ansi -pedantic -I.
  PICFLAGS = -fPIC
  SHARED = -shared -fPIC
  SOSUFFIX = so
  SOPREFIX = lib
  UNIXVERSIONING=YES
  SONAME = -Wl,-h
  LIBPATH = LD_LIBRARY_PATH
  STATICLIB=libmyspell-$(VERSION)_pic.a
  AR=ar rc
  RANLIB=ranlib
endif

ifeq "$(PLATFORM)" "NetBSD"
  CXX ?= g++
  CXXFLAGS ?= -O2 -Wall -ansi -pedantic -I.
  CC ?= gcc
  CFLAGS ?= -O2 -Wall -ansi -pedantic -I.
  PICFLAGS = -fPIC
  SHARED = -shared -fPIC
  SOSUFFIX = so
  SOPREFIX = lib
  UNIXVERSIONING=YES
  SONAME = -Wl,-h
  LIBPATH = LD_LIBRARY_PATH
  STATICLIB=libmyspell-$(VERSION)_pic.a
  AR=ar rc
  RANLIB=ranlib
endif

ifeq "$(PLATFORM)" "FreeBSD"
  CXX ?= g++
  CXXFLAGS ?= -O2 -Wall -ansi -pedantic -I.
  CC ?= gcc
  CFLAGS ?= -O2 -Wall -ansi -pedantic -I.
  PICFLAGS = -fPIC
  SHARED = -shared -fPIC
  SOSUFFIX = so
  SOPREFIX = lib
  UNIXVERSIONING=YES
  SONAME = -Wl,-h
  LIBPATH = LD_LIBRARY_PATH
  STATICLIB=libmyspell-$(VERSION)_pic.a
  AR=ar rc
  RANLIB=ranlib
endif

ifeq "$(PLATFORM)" "Darwin"
  CXX ?= g++
  CXXFLAGS ?= -O2 -Wall -ansi -pedantic -I.
  CC ?= gcc
  CFLAGS ?= -O2 -Wall -ansi -pedantic -I.
  PICFLAGS = -fPIC -fno-common
  SHARED = -dynamiclib -fno-common -fPIC
  SOSUFFIX = dylib
  SOPREFIX = lib
  UNIXVERSIONING=NO
  SETVERSION = -compatibility_version $(VERMAJOR).0 -current_version $(VERSION)
  LIBPATH = DYLD_LIBRARY_PATH
  STATICLIB=libmyspell-$(VERSION)_pic.a
  AR=ar rc
  RANLIB=ranlib
endif



ifeq "$(UNIXVERSIONING)" "YES"
  SETVNAME = $(SONAME)$(SOPREFIX)myspell.$(SOSUFFIX).$(VERMAJOR)
  SOLIBFULL=$(SOPREFIX)myspell.$(SOSUFFIX).$(VERSION)
  SOLIBMAJOR=$(SOPREFIX)myspell.$(SOSUFFIX).$(VERMAJOR)
  SOLIB=$(SOPREFIX)myspell.$(SOSUFFIX)
else
  SOLIB=$(SOPREFIX)myspell.$(SOSUFFIX)
  SOLIBFULL=$(SOLIB)
endif


LDFLAGS=-L. -lmyspell


OBJS = affentry.o affixmgr.o hashmgr.o suggestmgr.o csutil.o myspell.o dictmgr.o

# targets

all: example munch unmunch  $(STATICLIB)


check: example
	$(LIBPATH)=. ./example en_US.aff en_US.dic checkme.lst


clean:
	rm -f *.o *~ example munch unmunch $(SOLIB)* $(STATICLIB)


distclean:	clean


install: example $(STATICLIB) $(SOLIB)
	@test -f example || make
	@test -d $(PREFIX)/bin || mkdir $(PREFIX)/bin
	@cp -f munch $(PREFIX)/bin/ || echo "Missing tools"
	@cp -f unmunch $(PREFIX)/bin/ || echo "Missing tools"
	@echo "installed munch and unmunch tools"
	@cp -f $(STATICLIB) $(PREFIX)/lib/ || echo "Missing static library"
	@test -d $(PREFIX)/lib || mkdir $(PREFIX)/lib
	@cp -f $(SOLIBFULL) $(PREFIX)/lib/ || echo "Missing shared library"
	@cp -f $(STATICLIB) $(PREFIX)/lib/ || echo "Missing static library"
	@echo "installed shared and static libraries"
	@test -d $(PREFIX)/include || mkdir $(PREFIX)/include
	@test -d $(PREFIX)/include/myspell || mkdir $(PREFIX)/include/myspell
	@cp -f affentry.hxx $(PREFIX)/include/myspell || echo "Missing header"
	@cp -f affixmgr.hxx $(PREFIX)/include/myspell || echo "Missing header"
	@cp -f atypes.hxx $(PREFIX)/include/myspell || echo "Missing header"
	@cp -f baseaffix.hxx $(PREFIX)/include/myspell || echo "Missing header"
	@cp -f csutil.hxx $(PREFIX)/include/myspell || echo "Missing header"
	@cp -f dictmgr.hxx $(PREFIX)/include/myspell || echo "Missing header"
	@cp -f hashmgr.hxx $(PREFIX)/include/myspell || echo "Missing header"
	@cp -f htypes.hxx $(PREFIX)/include/myspell || echo "Missing header"
	@cp -f munch.h $(PREFIX)/include/myspell || echo "Missing header"
	@cp -f myspell.hxx $(PREFIX)/include/myspell || echo "Missing header"
	@cp -f suggestmgr.hxx $(PREFIX)/include/myspell || echo "Missing header"
	@cp -f unmunch.h $(PREFIX)/include/myspell || echo "Missing header"
	@echo "installed headers"
	@test -d $(PREFIX)/share || mkdir $(PREFIX)/share
	@test -d $(PREFIX)/share/myspell || mkdir $(PREFIX)/share/myspell
	@cp -f en_US.aff $(PREFIX)/share/myspell/ || echo "Missing aff file"
	@cp -f en_US.dic $(PREFIX)/share/myspell/ || echo "Missing dic file"
	@echo "installed dictionaries"
	@echo "installation complete"


# the rules
%.o: %.cxx 
	$(CXX) $(PICFLAGS) $(CXXFLAGS) -c $<


ifeq "$(UNIXVERSIONING)" "YES"
$(SOLIB) : $(SOLIBMAJOR)
	@ln -s $(SOLIBMAJOR) $@

$(SOLIBMAJOR) : $(SOLIBFULL)
	@ln -s $(SOLIBFULL) $@

$(SOLIBFULL) : $(OBJS)
	$(CXX) $(SHARED) $(SETVNAME) -o $@ $(OBJS)
else
$(SOLIB) : $(OBJS)
	$(CXX) $(SHARED) $(SETVERSION) -o $@ $(OBJS)
endif


$(STATICLIB): $(OBJS)
	$(AR) $@ $(OBJS)
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1


example: example.o $(SOLIB)
	$(CXX) $(CXXFLAGS) -o $@ example.o $(LDFLAGS)


munch: munch.c munch.h
	$(CC) $(CFLAGS) -o $@ munch.c


unmunch: unmunch.c unmunch.h
	$(CC) $(CFLAGS) -o $@ unmunch.c



# DO NOT DELETE

affentry.o: license.readme affentry.hxx atypes.hxx baseaffix.hxx affixmgr.hxx
affentry.o: hashmgr.hxx htypes.hxx
affixmgr.o: license.readme affixmgr.hxx atypes.hxx baseaffix.hxx hashmgr.hxx
affixmgr.o: htypes.hxx affentry.hxx
csutil.o: csutil.hxx
dictmgr.o: dictmgr.hxx
example.o: myspell.hxx hashmgr.hxx htypes.hxx affixmgr.hxx atypes.hxx
example.o: baseaffix.hxx suggestmgr.hxx csutil.hxx
hashmgr.o: license.readme hashmgr.hxx htypes.hxx
myspell.o: license.readme myspell.hxx hashmgr.hxx htypes.hxx affixmgr.hxx
myspell.o: atypes.hxx baseaffix.hxx suggestmgr.hxx csutil.hxx
suggestmgr.o: license.readme suggestmgr.hxx atypes.hxx affixmgr.hxx
suggestmgr.o: baseaffix.hxx hashmgr.hxx htypes.hxx
