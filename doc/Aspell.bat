@echo off
echo --- %3 >> ASpell\spellcheck_%2.log
%1 --mode=sgml --encoding=utf-8 --add-extra-dicts=./ASpell/%2.pws --add-extra-dicts=./ASpell/Temp.pws --lang=%2 list check < %3 >> ASpell\spellcheck_%2.log
