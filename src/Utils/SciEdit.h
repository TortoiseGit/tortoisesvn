#pragma once
#include "scintilla.h"
#include "myspell\\myspell.hxx"
#include "myspell\\mythes.hxx"

class CSciEdit : public CWnd
{
	DECLARE_DYNAMIC(CSciEdit)
public:
	CSciEdit(void);
	~CSciEdit(void);
	void		Init();
	LRESULT		Call(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
	void		SetText(const CString& sText);
	CString		GetText(void);
	void		SetFont(CString sFontName, int iFontSizeInPoints);
private:
	HMODULE		m_hModule;
	LRESULT		m_DirectFunction;
	LRESULT		m_DirectPointer;
	MySpell *	pChecker;
	MyThes *	pThesaur;

protected:
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	void		CheckSpelling(void);
};
