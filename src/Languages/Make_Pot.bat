@echo off
ResText extract TortoiseProcLang.dll TortoiseMergeLang.dll Tmp.pot
msgremove Tmp.pot -i ignore.po -o Tortoise.pot
del Tmp.pot
