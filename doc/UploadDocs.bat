@echo off
rem run this batch file to upload the docs to our sourceforge server
rem to make this work, you have to create a file "docserverlogin.bat"
rem which sets the variables USERNAME and PASSWORD, and also set the PSCP
rem variable to point to your scp program the PLINK variable to the plink
rem program and the ZIP variable to your zip program

rem example docserverlogin.bat file
rem 
rem @echo off
rem set USERNAME=myname
rem set PASSWORD=mypassword
rem set PsCP="C:\Programme\PuttY\pscp.exe"
rem set PLINK="C:\Programme\Putty\plink.exe"
rem set ZIP="C:\Programme\7-zip\7z.exe"

SETLOCAL ENABLEDELAYEDEXPANSION

call docserverlogin.bat

cd output
del docs.zip
if "%1"=="" (
  FOR /F %%L IN ('DIR /AD /B') DO (
    move /Y %%L\images\backgrounddraft.png %%L\images\background.png
  )
)

%ZIP% a -r -tzip docs.zip *

%PSCP% -r -l %USERNAME% -pw %PASSWORD% docs.zip shell.sourceforge.net:/home/groups/t/to/tortoisesvn/htdocs/docs

if "%1"=="" (
%PLINK% shell.sourceforge.net -l %USERNAME% -pw %PASSWORD% unzip -o /home/groups/t/to/tortoisesvn/htdocs/docs/docs.zip -d /home/groups/t/to/tortoisesvn/htdocs/docs/nightly;chgrp -R tortoisesvn /home/groups/t/to/tortoisesvn/htdocs/docs/nightly/*;chmod -R g+rw /home/groups/t/to/tortoisesvn/htdocs/docs/nightly;rm -f /home/groups/t/to/tortoisesvn/htdocs/docs/docs.zip
) else (
%PLINK% shell.sourceforge.net -l %USERNAME% -pw %PASSWORD% unzip -o /home/groups/t/to/tortoisesvn/htdocs/docs/docs.zip -d /home/groups/t/to/tortoisesvn/htdocs/docs/release;chgrp -R tortoisesvn /home/groups/t/to/tortoisesvn/htdocs/docs/release/*;chmod -R g+rw /home/groups/t/to/tortoisesvn/htdocs/docs/release;rm -f /home/groups/t/to/tortoisesvn/htdocs/docs/docs.zip
)
del docs.zip

cd ..
