This directory contains a patched file for apr-iconv which is copied over
the original file by the build script.

The reason for that patch is the way apr-iconv looks for its *.so files. It
looks for them in the path pointed to by the environement variable
APR_ICONV_PATH, but that's a real problem on Windows since there can be
many different builds of apr-iconv which are incompatible to each other.

So the patch makes apr-iconv first look in the directory where the program
using apr-iconv is located and then one folder below too. Example:

c:\program files\myprogram\bin\myprogram.exe
now look first in
c:\program files\myprogram\bin\iconv
then in
c:\program files\myprogram\iconv

and only if the *.so file is still not found then the APR_ICONV_PATH environement
variable is used.

This prevents conflicts on Windows.
