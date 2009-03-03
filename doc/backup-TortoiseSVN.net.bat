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


call docserverlogin.bat

mkdir website_backup
:: delete old backup files and run the backup script
%PLINK% www.tortoisesvn.net -l %USERNAME% -pw %PASSWORD% cd /backup;./backup.sh

del website_backup\backup.sh

%PSCP% -r -l %USERNAME% -pw %PASSWORD% www.tortoisesvn.net:/backup/* website_backup

