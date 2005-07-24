# Simple makefile for TortoiseBlame.
# Build using Microsoft Visual C++ Toolkit 2003 and Microsoft Platform SDK
#
# This produces a larger executable than the version compiled using VS.NET
# but no effort has been made to sort out compiler switches, libs, etc.
# It appears to run just as well, but is 50% bigger. This is adequate for
# simple testing.
#
# Use:
#	To build TBlame (debug version, local copy)	nmake
#	To build TBlame (release, local copy)		nmake nodebug=1
#	To delete all object files & exe			nmake clean
#
# NOTES:
# The executable can be copied to the TSVN install directory by uncommenting the
# last line of the exe build rule. The program needs SciLexer.dll and you can
# either copy the program to the install dir, or build the scintilla DLL
# (CD to utils\scintilla\win32 and type nmake -f scintilla_vc6.mak), or you
# can copy the existing DLL to ..\..\bin.
#
# The dependencies are not included in this makefile. More work needed.
#
# Makefile has only been tested on Win2K, and will probably need more work
# to make it more universal.


# Define these prior to including win32.mak
TARGETOS = WINNT
APPVER   = 4.0

# Nmake macros for building Win32 apps
!include <win32.mak>

PROJ = TortoiseBlame
OBJDIR = ..\..\obj
BINDIR = ..\..\bin
UTILS = ..\utils
SCINTILLA = $(UTILS)\scintilla\include
INSTALL_DIR = C:\Program Files\TortoiseSVN\bin

# Define project specific macros
PROJ_OBJS = $(OBJDIR)\TortoiseBlame.obj $(OBJDIR)\Lexer.obj $(OBJDIR)\stdafx.obj $(OBJDIR)\registry.obj
BASE_OBJS  =
# Win32.mak does not seem to include enough libs.
# These are the ones Stefan told me to use.
STD_LIBS   = comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib \
			 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib
EXTRA_LIBS = version.lib
GLOBAL_DEP = generic.h resource.h
RC_DEP     = resource.h

# The /EHsc is required to get stack unwinding to work on cpp.
cflags = $(cflags) /EHsc /I$(UTILS) /I$(SCINTILLA) /I.
# The cflags below are what VS.NET produces, but do not work here.
#cflags = /O1 /GA /MD /GS /W3 /c /Zi /TP /I$(UTILS) /I$(SCINTILLA) /I.

all: $(OBJDIR) $(BINDIR) $(BINDIR)\$(PROJ).exe

#----- If OBJDIR does not exist, then create directory
$(OBJDIR) :
    if not exist "$(OBJDIR)/$(NULL)" mkdir $(OBJDIR)

#----- If BINDIR does not exist, then create directory
$(BINDIR) :
    if not exist "$(BINDIR)/$(NULL)" mkdir $(BINDIR)

# Inference rule for updating the object files
.cpp{$(OBJDIR)}.obj:
    $(cc) $(cdebug) $(cflags) $(cvars) /Fo"$(OBJDIR)\\" /Fd"$(OBJDIR)\\" $**

# Inference rule for updating the object files
{$(UTILS)}.cpp{$(OBJDIR)}.obj:
    $(cc) $(cdebug) $(cflags) $(cvars) /Fo"$(OBJDIR)\\" /Fd"$(OBJDIR)\\" $**

# Build rule for resource file
$(OBJDIR)\$(PROJ).res: $(PROJ).rc $(RC_DEP)
    $(rc) $(rcflags) $(rcvars) /fo $(OBJDIR)\$(PROJ).res $(PROJ).rc

# Build rule for EXE
$(BINDIR)\$(PROJ).EXE: $(BASE_OBJS) $(PROJ_OBJS) $(OBJDIR)\$(PROJ).res
    $(link) $(linkdebug) $(guilflags) \
    $(BASE_OBJS) $(PROJ_OBJS) $(OBJDIR)\$(PROJ).res $(STD_LIBS) $(EXTRA_LIBS) \
    -out:$(BINDIR)\$(PROJ).exe $(MAPFILE)
#	copy /y "$(BINDIR)\$(PROJ).EXE" "$(INSTALL_DIR)"

# Rules for cleaning out those old files
clean:
	if exist $(OBJDIR)/$(NULL) rd /s /q $(OBJDIR)
	if exist $(BINDIR)/$(PROJ).exe del $(BINDIR)\$(PROJ).exe

