!ifndef FindTSVN_INCLUDED

!define FindTSVN_INCLUDED

Var TSVNPATH

!macro FindTSVNPath

; -------------------------------------------------------------
; Read the 64 bit registry on XP64, if the platform is x64.  --
; taken from http://nsis.sourceforge.net/REG_MULTI_SZ_Reader --
; -------------------------------------------------------------

!define HKEY_CLASSES_ROOT        0x80000000
!define HKEY_CURRENT_USER        0x80000001
!define HKEY_LOCAL_MACHINE       0x80000002
!define HKEY_USERS               0x80000003
!define HKEY_PERFORMANCE_DATA    0x80000004
!define HKEY_PERFORMANCE_TEXT    0x80000050
!define HKEY_PERFORMANCE_NLSTEXT 0x80000060
!define HKEY_CURRENT_CONFIG      0x80000005
!define HKEY_DYN_DATA            0x80000006
 
!define KEY_QUERY_VALUE          0x0001
!define KEY_ENUMERATE_SUB_KEYS   0x0008
 
!define REG_NONE                 0
!define REG_SZ                   1
!define REG_EXPAND_SZ            2
!define REG_BINARY               3
!define REG_DWORD                4
!define REG_DWORD_LITTLE_ENDIAN  4
!define REG_DWORD_BIG_ENDIAN     5
!define REG_LINK                 6
!define REG_MULTI_SZ             7
 
!define KEY_WOW64_64KEY		 0x0100

!define RegOpenKeyEx     "Advapi32::RegOpenKeyExA(i, t, i, i, i) i"
!define RegQueryValueEx  "Advapi32::RegQueryValueExA(i, t, i, i, i, i) i"
!define RegCloseKey      "Advapi32::RegCloseKeyA(i) i"
 
####### Edit this!
 
!define ROOT_KEY         ${HKEY_LOCAL_MACHINE}
!define SUB_KEY          "Software\TortoiseSVN"
!define VALUE            "Directory"
 
####### Stop editing
 
  StrCpy $0 ""
  StrCpy $1 ""
  StrCpy $2 ""
  StrCpy $3 ""
  SetPluginUnload alwaysoff
  System::Call "*(i) i (0) .r0"
  System::Call "*(i) i (0) .r1"
  System::Call "*(i) i (0) .r2"
  System::Call "${RegOpenKeyEx}(${ROOT_KEY}, '${SUB_KEY}', \
    0, ${KEY_QUERY_VALUE}|${KEY_ENUMERATE_SUB_KEYS}|${KEY_WOW64_64KEY}, r0) .r3"
 
  StrCmp $3 0 goon
    MessageBox MB_OK|MB_ICONSTOP "Can't open registry key! ($3)"
    Goto done
goon:
 
  System::Call "*$0(&i4 .r4)"
  System::Call "${RegQueryValueEx}(r4, '${VALUE}', 0, r1, 0, r2) .r3"
 
  StrCmp $3 0 read
    MessageBox MB_OK|MB_ICONSTOP "Can't query registry value! ($3)"
    Goto done
 
read:
 
  System::Call "*$1(&i4 .r3)"
 
  StrCmp $3 ${REG_SZ} multisz
    MessageBox MB_OK|MB_ICONSTOP "Registry value no REG_SZ! ($3)"
    Goto done
 
multisz:
 
  System::Call "*$2(&i4 .r3)"
 
  StrCmp $3 0 0 multiszalloc
    MessageBox MB_OK|MB_ICONSTOP "Registry value empty! ($3)"
    Goto done
 
multiszalloc:
 
  System::Free $1
  System::Alloc $3
  Pop $1
 
  StrCmp $1 0 0 multiszget
    MessageBox MB_OK|MB_ICONSTOP "Can't allocate enough memory! ($3)"
    Goto done
 
multiszget:
 
  System::Call "${RegQueryValueEx}(r4, '${VALUE}', 0, 0, r1, r2) .r3"
 
  StrCmp $3 0 multiszprocess
    MessageBox MB_OK|MB_ICONSTOP "Can't query registry value! ($3)[2]"
    Goto done
 
multiszprocess:
 
  System::Call "*$1(&t${NSIS_MAX_STRLEN} .r3)"
  StrCpy $TSVNPATH $3
 
done:
 
  System::Free $2
  System::Free $1
 
  StrCmp $0 0 noClose
    System::Call "${RegCloseKey}(r0)"
 
noClose:
 
  SetPluginUnload manual
  System::Free $0

  StrCmp $TSVNPATH "" failed ok

failed:
  Abort

ok:

!macroend

!endif
