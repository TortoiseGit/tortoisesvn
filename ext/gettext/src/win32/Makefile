# Makefile - Not for build. Just support.
#
# nmake tags		Generate tags file.
# nmake pkgclean	Clean up for package archive.
# nmake clean		Clean up build directories.
# nmake distclean	All clean up file generated when compile time.

tags:
	ctags *.c ..\intl\*.c ..\lib\*.c ..\src\*.c
pkgclean:
	-del /F /Q Release\*.obj
	-del /F /Q Release\*.res
	-del /F /Q Release\*.idb
	-del /F /Q Release\*.map
	-del /F /Q Release\nlsut.lib
	-del /F /Q Debug\*.*
	-rmdir Debug
	-del /F /Q tags
	-del /F /Q *.plg
	-del /F /Q *.ncb
	-del /F /Q *.opt
	-del /F /Q *.aps
clean:
	-del /F /Q Debug\*.*
	-rmdir Debug
	-del /F /Q Release\*.*
	-rmdir Release
distclean: clean
	-del /F /Q tags
	-del /F /Q *.ncb
	-del /F /Q *.opt
	-del /F /Q *.aps
