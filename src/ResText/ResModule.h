#pragma once
#include <vector>
#include <string>
#include <map>
#include "POFile.h"

#define GET_WORD(ptr)        (*(WORD  *)(ptr))
#define GET_DWORD(ptr)       (*(DWORD *)(ptr))
#define ALIGN_DWORD(type, p) ((type)(((DWORD)p + 3) & ~3))

// DIALOG CONTROL INFORMATION
typedef struct tagDlgItemInfo
{
	DWORD   style;
	DWORD   exStyle;
	DWORD   helpId;
	short   x;
	short   y;
	short   cx;
	short   cy;
	WORD    id;
	LPCTSTR className;
	LPCTSTR windowName;
	LPVOID  data;
} DLGITEMINFO, * LPDLGITEMINFO;

// DIALOG TEMPLATE
typedef struct tagDialogInfo
{
	DWORD   style;
	DWORD   exStyle;
	DWORD   helpId;
	WORD    nbItems;
	short   x;
	short   y;
	short   cx;
	short   cy;
	LPCTSTR menuName;
	LPCTSTR className;
	LPCTSTR caption;
	WORD    pointSize;
	WORD    weight;
	BOOL    italic;
	LPCTSTR faceName;
	BOOL    dialogEx;
} DIALOGINFO, * LPDIALOGINFO;

class CResModule
{
public:
	CResModule(void);
	~CResModule(void);

	BOOL	ExtractResources(LPCTSTR lpszSrcLangDllPath, LPCTSTR lpszPOFilePath, BOOL bNoUpdate);
	BOOL	ExtractResources(std::vector<std::wstring> filelist, LPCTSTR lpszPOFilePath, BOOL bNoUpdate);
	BOOL	CreateTranslatedResources(LPCTSTR lpszSrcLangDllPath, LPCTSTR lpszDestLangDllPath, LPCTSTR lpszPOFilePath);
	void	SetQuiet(BOOL bQuiet = TRUE) {m_bQuiet = bQuiet; m_StringEntries.SetQuiet(bQuiet);}
	void	SetLanguage(WORD wLangID) {m_wTargetLang = wLangID;}

private:
	static  BOOL CALLBACK EnumResNameCallback(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG lParam);
	static  BOOL CALLBACK EnumResNameWriteCallback(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG lParam);
	static  BOOL CALLBACK EnumResWriteLangCallback(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, WORD wLanguage, LONG lParam);
	BOOL	ExtractString(UINT nID);
	BOOL	ExtractDialog(UINT nID);
	BOOL	ExtractMenu(UINT nID);
	BOOL	ReplaceString(UINT nID, WORD wLanguage);
	BOOL	ReplaceDialog(UINT nID, WORD wLanguage);
	BOOL	ReplaceMenu(UINT nID, WORD wLanguage);

	const WORD*	ParseMenuResource(const WORD * res);
	const WORD*	CountMemReplaceMenuResource(const WORD * res, int * wordcount, WORD * newMenu);
	const WORD* GetControlInfo(const WORD* p, LPDLGITEMINFO lpDlgItemInfo, BOOL dialogEx, LPBOOL bIsID);
	const WORD*	GetDialogInfo(const WORD * pTemplate, LPDIALOGINFO lpDlgInfo);
	const WORD*	CountMemReplaceDialogResource(const WORD * res, int * wordcount, WORD * newMenu);
	const WORD* ReplaceControlInfo(const WORD * res, int * wordcount, WORD * newDialog, BOOL bEx);

	void	ReplaceStr(LPCWSTR src, WORD * dest, int * count, int * translated, int * def);

	HMODULE			m_hResDll;
	HANDLE			m_hUpdateRes;
	CPOFile			m_StringEntries;
	std::wstring	sDestFile;
	BOOL			m_bQuiet;

	int				m_bTranslatedStrings;
	int				m_bDefaultStrings;
	int				m_bTranslatedDialogStrings;
	int				m_bDefaultDialogStrings;
	int				m_bTranslatedMenuStrings;
	int				m_bDefaultMenuStrings;
	
	int				m_wTargetLang;
};
