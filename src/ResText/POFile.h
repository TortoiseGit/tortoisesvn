#pragma once
#include <string>
#include <map>
#include <vector>

typedef struct tagResourceEntry
{
	std::vector<std::wstring>	translatorcomments;
	std::vector<std::wstring>	automaticcomments;
	std::wstring	reference;
	std::wstring	flag;
	std::wstring	msgstr;
} RESOURCEENTRY, * LPRESOURCEENTRY;

class CPOFile : public std::map<std::wstring, RESOURCEENTRY>
{
public:
	CPOFile();
	~CPOFile(void);

	BOOL ParseFile(LPCTSTR szPath, BOOL bUpdateExisting = TRUE);
	BOOL SaveFile(LPCTSTR szPath);
	void SetQuiet(BOOL bQuiet = TRUE) {m_bQuiet = bQuiet;}
private:
	BOOL m_bQuiet;
};
