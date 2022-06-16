#!/bin/sh

# Bash version of CheckIDD.bat
./scripts/chkidd.py source/en/TortoiseSVN ../src/Resources/TortoiseProcENG.rc scripts/TortoiseProcENG.ignore > IDD_Errors.txt
./scripts/chkidd.py source/en/TortoiseMerge ../src/Resources/TortoiseMergeENG.rc scripts/TortoiseMergeENG.ignore >> IDD_Errors.txt
