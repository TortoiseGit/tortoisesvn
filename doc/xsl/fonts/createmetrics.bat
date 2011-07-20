set LIB=D:\Development\SVN\TortoiseSVN\Tools\fop\lib
set CMD="c:\Program Files (x86)\Java\jre6\bin\java" -cp "D:\Development\SVN\TortoiseSVN\Tools\fop\build\fop.jar;%LIB%\xmlgraphics-commons-1.4.jar;%LIB%\commons-io-1.3.1.jar;%LIB%\commons-logging-1.0.4.jar;%LIB%\avalon-framework-4.2.0.jar;%LIB%\xercesImpl-2.7.1.jar"
set CMD=%CMD% org.apache.fop.fonts.apps.TTFReader
set FONT_DIR=C:\Windows\Fonts
%CMD% %FONT_DIR%\arial.ttf arial.xml
%CMD% %FONT_DIR%\arialdb.ttf arialdb.xml
%CMD% %FONT_DIR%\arialbi.ttf arialbi.xml
%CMD% %FONT_DIR%\ariali.ttf ariali.xml
%CMD% %FONT_DIR%\cour.ttf cour.xml
%CMD% %FONT_DIR%\courbd.ttf courbd.xml
%CMD% %FONT_DIR%\courbi.ttf courbi.xml
%CMD% %FONT_DIR%\couri.ttf couri.xml
%CMD% %FONT_DIR%\iqraa.ttf iqraa.xml
%CMD% %FONT_DIR%\msgothic.ttf msgothic.xml
%CMD% %FONT_DIR%\msmincho.ttf msmincho.xml
%CMD% %FONT_DIR%\msyh.ttf msyh.xml
%CMD% %FONT_DIR%\msyhbd.ttf msyhbd.xml
%CMD% %FONT_DIR%\simhei.ttf simhei.xml
%CMD% %FONT_DIR%\simsun.ttf simsun.xml
%CMD% %FONT_DIR%\times.ttf times.xml
%CMD% %FONT_DIR%\timesbd.ttf timesbd.xml
%CMD% %FONT_DIR%\timesbi.ttf timesbi.xml
%CMD% %FONT_DIR%\timesi.ttf timesi.xml
