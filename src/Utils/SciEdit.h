// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#pragma once
#include "scintilla.h"
#include "SciLexer.h"
#include "myspell\\myspell.hxx"
#include "myspell\\mythes.hxx"
#include "ProjectProperties.h"
#include "PersonalDictionary.h"

//forward declaration
class CSciEdit;

/**
 * \ingroup Utils
 * Helper class which extends the MFC CStringArray class. The only method added
 * to that class is AddSorted() which adds a new element in a sorted order.
 * That way the array is kept sorted.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date JAN-2005
 *
 * \author Stefan Kueng
 *
 */
class CAutoCompletionList : public CStringArray
{
public:
	void		AddSorted(const CString& elem, bool bNoDuplicates = true);
	INT_PTR		Find(const CString& elem);
};

/**
 * \ingroup Utils
 * This class acts as an interface so that CSciEdit can call these methods
 * on other objects which implement this interface.
 * Classes implementing this interface must call RegisterContextMenuHandler()
 * in CSciEdit to register themselves.
 *
 * \par requirements
 * MFC or ATL
 *
 * \version 1.0
 * first version
 *
 * \date MAR-2005
 *
 * \author Stefan Kueng
 *
 */
class CSciEditContextMenuInterface
{
public:
	/**
	 * When the handler is called with this method, it can add entries
	 * to the \a mPopup context menu itself. The \a nCmd param is the command
	 * ID number the handler must use for its commands. For every added command,
	 * the handler is responsible to increment the \a nCmd param by one.
	 */
	virtual void		InsertMenuItems(CMenu& mPopup, int& nCmd);
	
	/**
	 * The handler is called when the user clicks on any context menu entry
	 * which isn't handled by CSciEdit itself. That means the handler might
	 * be called for entries it hasn't added itself! 
	 * \remark the handler should return \a true if it handled the call, otherwis
	 * it should return \a false
	 */
	virtual bool		HandleMenuItemClick(int cmd, CSciEdit * pSciEdit);
};

/**
 * \ingroup Utils
 * Encapsulates the Scintilla edit control. Useable as a replacement for the
 * MFC CEdit control, but not a drop-in replacement!
 * Also provides additional features like spell checking, autocompletion, ...
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 * Scintilla edit control (http://www.scintilla.org)
 *
 * \version 1.0
 * first version
 *
 * \date JAN-2005
 *
 * \author Stefan Kueng
 *
 */
class CSciEdit : public CWnd
{
	DECLARE_DYNAMIC(CSciEdit)
public:
	CSciEdit(void);
	~CSciEdit(void);
	/**
	 * Initialize the scintilla control. Must be called prior to any other
	 * method!
	 */
	void		Init(const ProjectProperties& props);
	void		Init(LONG lLanguage = 0);
	/**
	 * Execute a scintilla command, e.g. SCI_GETLINE.
	 */
	LRESULT		Call(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
	/**
	 * The specified text is written to the scintilla control.
	 */
	void		SetText(const CString& sText);
	/**
	 * The specified text is inserted at the cursor position. If a text is
	 * selected, that text is replaced.
	 * \param bNewLine if set to true, a newline is appended.
	 */
	void		InsertText(const CString& sText, bool bNewLine = false);
	/**
	 * Retreives the text in the scintilla control.
	 */
	CString		GetText(void);
	/**
	 * Sets the font for the control.
	 */
	void		SetFont(CString sFontName, int iFontSizeInPoints);
	/**
	 * Adds a list of words for use in autocompletion.
	 */
	void		SetAutoCompletionList(const CAutoCompletionList& list, const TCHAR separator = ';');
	/**
	 * Returns the word located under the cursor.
	 */
	CString		GetWordUnderCursor(bool bSelectWord = false);
	
	void		RegisterContextMenuHandler(CSciEditContextMenuInterface * object) {m_arContextHandlers.Add(object);}
	
	CStringA	StringForControl(const CString& text);
	CString		StringFromControl(const CStringA& text);
	
private:
	HMODULE		m_hModule;
	LRESULT		m_DirectFunction;
	LRESULT		m_DirectPointer;
	MySpell *	pChecker;
	MyThes *	pThesaur;
	UINT		m_spellcodepage;
	CAutoCompletionList m_autolist;
	TCHAR		m_separator;
	CString		m_sCommand;
	CString		m_sBugID;
	rpattern	m_patCommand;
	rpattern	m_patBugID;
	CArray<CSciEditContextMenuInterface *, CSciEditContextMenuInterface *> m_arContextHandlers;
	CPersonalDictionary m_personalDict;
protected:
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	void		CheckSpelling(void);
	void		SuggestSpellingAlternatives(void);
	void		DoAutoCompletion(void);
	BOOL		LoadDictionaries(LONG lLanguageID);
	BOOL		MarkEnteredBugID(NMHDR* nmhdr);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	DECLARE_MESSAGE_MAP()
};
