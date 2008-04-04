:: The images for our docs should not exceed a certain size and have
:: a default dpi of 96 (this is to prevent problems with the pdf docs)
::
:: to reduce the size of the images, we use NConvert.exe from
:: http://www.xnview.com
::
nconvert.exe -dpi 96 -ratio -resize 500 800 -rflag decr de\*.*
nconvert.exe -dpi 96 -ratio -resize 500 800 -rflag decr en\*.*
nconvert.exe -dpi 96 -ratio -resize 500 800 -rflag decr es\*.*
nconvert.exe -dpi 96 -ratio -resize 500 800 -rflag decr fi\*.*
nconvert.exe -dpi 96 -ratio -resize 500 800 -rflag decr fr\*.*
nconvert.exe -dpi 96 -ratio -resize 500 800 -rflag decr id\*.*
nconvert.exe -dpi 96 -ratio -resize 500 800 -rflag decr ja\*.*
nconvert.exe -dpi 96 -ratio -resize 500 800 -rflag decr ru\*.*

