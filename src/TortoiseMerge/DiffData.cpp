#include "StdAfx.h"
#include "diff.h"
#include "TempFiles.h"
#include "registry.h"
#include "Resource.h"
#include ".\diffdata.h"

int CDiffData::abort_on_pool_failure (int retcode)
{
	abort ();
	return -1;
}

CDiffData::CDiffData(void)
{
	m_diffYourBase = NULL;
	m_diffTheirBase = NULL;
	m_diffTheirYourBase = NULL;
	m_arBaseFile.SetSize(0, 10);
	m_arTheirFile.SetSize(0, 10);
	m_arYourFile.SetSize(0, 10);
}

CDiffData::~CDiffData(void)
{
}

void CDiffData::GetColors(DiffStates state, COLORREF &crBkgnd, COLORREF &crText)
{
	switch (state)
	{
	case DIFFSTATE_UNKNOWN:
		crBkgnd = ::GetSysColor(COLOR_WINDOW);
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case DIFFSTATE_NORMAL:
		crBkgnd = ::GetSysColor(COLOR_WINDOW);
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case DIFFSTATE_REMOVED:
		crBkgnd = RGB(255,100,100);
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_ADDED:
		crBkgnd = RGB(255,255,0);
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_WHITESPACE:
		crBkgnd = RGB(150,150,255);
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_EMPTY:
		crBkgnd = RGB(200,200,200);
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_CONFLICTEMPTY:
		crBkgnd = RGB(200,200,200);
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_CONFLICTED:
		crBkgnd = RGB(255,0,0);
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_IDENTICAL:
		crBkgnd = RGB(150,255,150);	//green
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_IDENTICALREMOVED:
		crBkgnd = RGB(150,255,150);	//green
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_IDENTICALADDED:
		crBkgnd = RGB(150,255,150);	//green
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_CONFLICTADDED:
		crBkgnd = RGB(255,0,0);
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_THEIRSREMOVED:
		crBkgnd = RGB(0,255,255);	//cyan
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_THEIRSADDED:
		crBkgnd = RGB(0,255,255);	//cyan
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_YOURSREMOVED:
		crBkgnd = RGB(255,0,255);	//magenta
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	case CDiffData::DIFFSTATE_YOURSADDED:
		crBkgnd = RGB(255,0,255);	//magenta
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	default:
		crBkgnd = ::GetSysColor(COLOR_WINDOW);
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
		break;
	}
}

int CDiffData::GetLineCount()
{
	int count = 0;
	count = (int)m_arBaseFile.GetCount();
	count = (int)(count > m_arTheirFile.GetCount() ? count : m_arTheirFile.GetCount());
	count = (int)(count > m_arYourFile.GetCount() ? count : m_arYourFile.GetCount());
	return count;
}

int CDiffData::GetLineActualLength(int index)
{
	int count = 0;
	if (index < m_arBaseFile.GetCount())
		count = (count > m_arBaseFile.GetAt(index).GetLength() ? count : m_arBaseFile.GetAt(index).GetLength());
	if (index < m_arTheirFile.GetCount())
		count = (count > m_arTheirFile.GetAt(index).GetLength() ? count : m_arTheirFile.GetAt(index).GetLength());
	if (index < m_arYourFile.GetCount())
		count = (count > m_arYourFile.GetAt(index).GetLength() ? count : m_arYourFile.GetAt(index).GetLength());
	return count;
}

LPCTSTR CDiffData::GetLineChars(int index)
{
	if (index < m_arBaseFile.GetCount())
		return m_arBaseFile.GetAt(index);
	if (index < m_arTheirFile.GetCount())
		return m_arTheirFile.GetAt(index);
	if (index < m_arYourFile.GetCount())
		return m_arYourFile.GetAt(index);
	return NULL;
}

BOOL CDiffData::Load()
{
	CStringA f1, f2, f3, fc1, fc2, fc3;
	f1 = m_sBaseFile;
	f2 = m_sTheirFile;
	f3 = m_sYourFile;
	CString sLine;
	apr_pool_t * pool;
	svn_error_t * svnerr = NULL;

	apr_initialize();
	apr_pool_create_ex (&pool, NULL, abort_on_pool_failure, NULL);

	m_arBaseFile.RemoveAll();
	m_arYourFile.RemoveAll();
	m_arTheirFile.RemoveAll();
	m_arDiffYourBaseBoth.RemoveAll();
	m_arStateYourBaseBoth.RemoveAll();
	m_arDiffYourBaseLeft.RemoveAll();
	m_arStateYourBaseLeft.RemoveAll();
	m_arDiffYourBaseRight.RemoveAll();
	m_arStateYourBaseRight.RemoveAll();
	m_arDiffTheirBaseBoth.RemoveAll();
	m_arStateTheirBaseBoth.RemoveAll();
	m_arDiffTheirBaseLeft.RemoveAll();
	m_arStateTheirBaseLeft.RemoveAll();
	m_arDiffTheirBaseRight.RemoveAll();
	m_arStateTheirBaseRight.RemoveAll();
	m_arDiff3.RemoveAll();
	m_arStateDiff3.RemoveAll();

	m_diffYourBase = NULL;
	m_diffTheirBase = NULL;
	m_diffTheirYourBase = NULL;

	CFileTextLines converted;
	CTempFiles tempfiles;
	CRegDWORD regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
	CRegDWORD regIgnoreEOL = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreEOL"), TRUE);
	DWORD dwIgnoreWS = regIgnoreWS;
	BOOL bIgnoreEOL = ((DWORD)regIgnoreEOL)!=0;
	if (!m_sBaseFile.IsEmpty())
	{
		if (!m_arBaseFile.Load(m_sBaseFile))
		{
			m_sError = m_arBaseFile.GetErrorString();
			return FALSE;
		} // if (!m_arBaseFile.Load(m_sBaseFile))
		if (!converted.Load(m_sBaseFile))
		{
			m_sError = converted.GetErrorString();
			return FALSE;
		} // if (!converted.Load(m_sBaseFile))
		else
		{
			CString sTemp = tempfiles.GetTempFilePath();
			fc1 = sTemp;
			converted.Save(sTemp, dwIgnoreWS > 0, bIgnoreEOL);
		}
	} // if (!m_sBaseFile.IsEmpty())
	if (!m_sTheirFile.IsEmpty())
	{
		if (!m_arTheirFile.Load(m_sTheirFile))
		{
			m_sError = m_arTheirFile.GetErrorString();
			return FALSE;
		}
		if (!converted.Load(m_sTheirFile))
		{
			m_sError = converted.GetErrorString();
			return FALSE;
		} // if (!converted.Load(m_sBaseFile))
		else
		{
			CString sTemp = tempfiles.GetTempFilePath();
			fc2 = sTemp;
			converted.Save(sTemp, dwIgnoreWS > 0, bIgnoreEOL);
		}
	} // if (!m_sTheirFile.IsEmpty())
	if (!m_sYourFile.IsEmpty())
	{
		if (!m_arYourFile.Load(m_sYourFile))
		{
			m_sError = m_arYourFile.GetErrorString();
			return FALSE;
		}
		if (!converted.Load(m_sYourFile))
		{
			m_sError = converted.GetErrorString();
			return FALSE;
		} // if (!converted.Load(m_sBaseFile))
		else
		{
			CString sTemp = tempfiles.GetTempFilePath();
			fc3 = sTemp;
			converted.Save(sTemp, dwIgnoreWS > 0, bIgnoreEOL);
		}
	} // if (!m_sYourFile.IsEmpty()) 
	//#region if ((!m_sBaseFile.IsEmpty()) && (!m_sYourFile.IsEmpty()) && m_sTheirFile.IsEmpty())
	if ((!m_sBaseFile.IsEmpty()) && (!m_sYourFile.IsEmpty()) && m_sTheirFile.IsEmpty())
	{
		svnerr = svn_diff_file_diff(&m_diffYourBase, fc1, fc3, pool);
		if (svnerr)
		{
			TRACE(_T("diff-error in CDiffData::Load()\n"));
			CString sMsg = CString(svnerr->message);
			while (svnerr->child)
			{
				svnerr = svnerr->child;
				sMsg += _T("\n");
				sMsg += CString(svnerr->message);
			} // while (m_err->child)
			apr_pool_destroy (pool);					// free the allocated memory
			apr_terminate();
			m_sError.Format(IDS_ERR_DIFF_DIFF, sMsg);
			return FALSE;
		} // if (m_svnerr)
		svn_diff_t * tempdiff = m_diffYourBase;
		LONG baseline = 0;
		LONG yourline = 0;
		while (tempdiff)
		{
			for (int i=0; i<tempdiff->original_length; i++)
			{
				m_arDiffYourBaseBoth.Add(m_arBaseFile.GetAt(baseline));
				if (tempdiff->type == svn_diff__type_common)
				{
					m_arStateYourBaseBoth.Add(DIFFSTATE_NORMAL);
					yourline++;		//in both files
				}
				else
				{
					m_arStateYourBaseBoth.Add(DIFFSTATE_REMOVED);
				}
				baseline++;
			} // iff->original_length; i++)
			if (tempdiff->type == svn_diff__type_diff_modified)
			{
				for (int i=0; i<tempdiff->modified_length; i++)
				{
					m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
					m_arStateYourBaseBoth.Add(DIFFSTATE_ADDED);
					yourline++;
				} // iff->modified_length; i++) 
			} // if (tempdiff->type == svn_diff__type_diff_modified) 
			tempdiff = tempdiff->next;
		}
		tempdiff = m_diffYourBase;
		baseline = 0;
		yourline = 0;
		while (tempdiff)
		{
			if (tempdiff->type == svn_diff__type_common)
			{
				for (int i=0; i<tempdiff->original_length; i++)
				{
					m_arDiffYourBaseLeft.Add(m_arBaseFile.GetAt(baseline));
					m_arDiffYourBaseRight.Add(m_arYourFile.GetAt(yourline));
					if (m_arBaseFile.GetAt(baseline).Compare(m_arYourFile.GetAt(yourline))!=0)
					{
						if (dwIgnoreWS == 2)
						{
							CString s1 = m_arBaseFile.GetAt(baseline);
							s1 = s1.TrimLeft(_T(" \t"));
							CString s2 = m_arYourFile.GetAt(yourline);
							s2 = s2.TrimLeft(_T(" \t"));
							if (s1.Compare(s2)!=0)
							{
								m_arStateYourBaseLeft.Add(DIFFSTATE_WHITESPACE);
								m_arStateYourBaseRight.Add(DIFFSTATE_WHITESPACE);
							}
							else
							{
								m_arStateYourBaseLeft.Add(DIFFSTATE_NORMAL);
								m_arStateYourBaseRight.Add(DIFFSTATE_NORMAL);
							}
						}
						else if (dwIgnoreWS == 0)
						{
							m_arStateYourBaseLeft.Add(DIFFSTATE_WHITESPACE);
							m_arStateYourBaseRight.Add(DIFFSTATE_WHITESPACE);
						}
						else
						{
							m_arStateYourBaseLeft.Add(DIFFSTATE_NORMAL);
							m_arStateYourBaseRight.Add(DIFFSTATE_NORMAL);
						}
					}
					else
					{
						m_arStateYourBaseLeft.Add(DIFFSTATE_NORMAL);
						m_arStateYourBaseRight.Add(DIFFSTATE_NORMAL);
					}
					baseline++;
					yourline++;
				} // iff->original_length; i++)
			}
			if (tempdiff->type == svn_diff__type_diff_modified)
			{
				apr_off_t original_length = tempdiff->original_length;
				for (int i=0; i<tempdiff->modified_length; i++)
				{
					m_arDiffYourBaseRight.Add(m_arYourFile.GetAt(yourline++));
					if (original_length-- <= 0)
					{
						m_arDiffYourBaseLeft.Add(_T(""));
						m_arStateYourBaseLeft.Add(DIFFSTATE_EMPTY);
						m_arStateYourBaseRight.Add(DIFFSTATE_ADDED);
					} // if (tempdiff->original_length == 0)
					else
					{
						m_arDiffYourBaseLeft.Add(m_arBaseFile.GetAt(baseline++));
						m_arStateYourBaseLeft.Add(DIFFSTATE_REMOVED);
						m_arStateYourBaseRight.Add(DIFFSTATE_ADDED);
					}
				} // iff->modified_length; i++) 
				apr_off_t modified_length = tempdiff->modified_length;
				for (int i=0; i<tempdiff->original_length; i++)
				{
					if (modified_length-- <= 0)
					{
						m_arDiffYourBaseLeft.Add(m_arBaseFile.GetAt(baseline++));
						m_arStateYourBaseLeft.Add(DIFFSTATE_ADDED);
						m_arDiffYourBaseRight.Add(_T(""));
						m_arStateYourBaseRight.Add(DIFFSTATE_EMPTY);
					} // if (tempdiff->original_length == 0)
				} // iff->modified_length; i++) 
			} // if (tempdiff->type == svn_diff__type_diff_modified) 
			tempdiff = tempdiff->next;
		}
		TRACE(_T("done with diff\n"));
	} // if ((!m_sBaseFile.IsEmpty()) && (!m_sYourFile.IsEmpty())) 
	//#endregion
	//#region if ((!m_sBaseFile.IsEmpty()) && (!m_sTheirFile.IsEmpty()) && m_sYourFile.IsEmpty())
	if ((!m_sBaseFile.IsEmpty()) && (!m_sTheirFile.IsEmpty()) && m_sYourFile.IsEmpty())
	{
		svnerr = svn_diff_file_diff(&m_diffTheirBase, fc1, fc2, pool);
		if (svnerr)
		{
			TRACE(_T("diff-error in CDiffData::Load()\n"));
			CString sMsg = CString(svnerr->message);
			while (svnerr->child)
			{
				svnerr = svnerr->child;
				sMsg += _T("\n");
				sMsg += CString(svnerr->message);
			} // while (m_err->child)
			apr_pool_destroy (pool);					// free the allocated memory
			apr_terminate();
			m_sError.Format(IDS_ERR_DIFF_DIFF, sMsg);
			return FALSE;
		} // if (m_svnerr)
		svn_diff_t * tempdiff = m_diffTheirBase;
		LONG baseline = 0;
		LONG theirline = 0;
		while (tempdiff)
		{
			for (int i=0; i<tempdiff->original_length; i++)
			{
				m_arDiffTheirBaseBoth.Add(m_arBaseFile.GetAt(baseline));
				if (tempdiff->type == svn_diff__type_common)
				{
					m_arStateTheirBaseBoth.Add(DIFFSTATE_NORMAL);
					theirline++;		//in both files
				}
				else
				{
					m_arStateTheirBaseBoth.Add(DIFFSTATE_REMOVED);
				}
				baseline++;
			} // iff->original_length; i++)
			if (tempdiff->type == svn_diff__type_diff_modified)
			{
				for (int i=0; i<tempdiff->modified_length; i++)
				{
					m_arDiffTheirBaseBoth.Add(m_arTheirFile.GetAt(theirline));
					m_arStateTheirBaseBoth.Add(DIFFSTATE_ADDED);
					theirline++;
				} // iff->modified_length; i++) 
			} // if (tempdiff->type == svn_diff__type_diff_modified) 
			tempdiff = tempdiff->next;
		}
		tempdiff = m_diffTheirBase;
		baseline = 0;
		theirline = 0;
		while (tempdiff)
		{
			if (tempdiff->type == svn_diff__type_common)
			{
				for (int i=0; i<tempdiff->original_length; i++)
				{
					m_arDiffTheirBaseLeft.Add(m_arBaseFile.GetAt(baseline));
					m_arDiffTheirBaseRight.Add(m_arTheirFile.GetAt(theirline));
					if (m_arBaseFile.GetAt(baseline).Compare(m_arTheirFile.GetAt(theirline))!=0)
					{
						if (dwIgnoreWS == 2)
						{
							CString s1 = m_arBaseFile.GetAt(baseline);
							s1 = s1.TrimLeft(_T(" \t"));
							CString s2 = m_arTheirFile.GetAt(theirline);
							s2 = s2.TrimLeft(_T(" \t"));
							if (s1.Compare(s2)!=0)
							{
								m_arStateTheirBaseLeft.Add(DIFFSTATE_WHITESPACE);
								m_arStateTheirBaseRight.Add(DIFFSTATE_WHITESPACE);
							}
							else
							{
								m_arStateTheirBaseLeft.Add(DIFFSTATE_NORMAL);
								m_arStateTheirBaseRight.Add(DIFFSTATE_NORMAL);
							}
						}
						else if (dwIgnoreWS == 0)
						{
							m_arStateTheirBaseLeft.Add(DIFFSTATE_WHITESPACE);
							m_arStateTheirBaseRight.Add(DIFFSTATE_WHITESPACE);
						}
						else
						{
							m_arStateTheirBaseLeft.Add(DIFFSTATE_NORMAL);
							m_arStateTheirBaseRight.Add(DIFFSTATE_NORMAL);
						}
					}
					else
					{
						m_arStateTheirBaseLeft.Add(DIFFSTATE_NORMAL);
						m_arStateTheirBaseRight.Add(DIFFSTATE_NORMAL);
					}
					baseline++;
					theirline++;
				} // iff->original_length; i++)
			}
			if (tempdiff->type == svn_diff__type_diff_modified)
			{
				apr_off_t original_length = tempdiff->original_length;
				for (int i=0; i<tempdiff->modified_length; i++)
				{
					m_arDiffTheirBaseRight.Add(m_arTheirFile.GetAt(theirline++));
					if (original_length-- <= 0)
					{
						m_arDiffTheirBaseLeft.Add(_T(""));
						m_arStateTheirBaseLeft.Add(DIFFSTATE_EMPTY);
						m_arStateTheirBaseRight.Add(DIFFSTATE_ADDED);
					} // if (tempdiff->original_length == 0)
					else
					{
						m_arDiffTheirBaseLeft.Add(m_arBaseFile.GetAt(baseline++));
						m_arStateTheirBaseLeft.Add(DIFFSTATE_REMOVED);
						m_arStateTheirBaseRight.Add(DIFFSTATE_ADDED);
					}
				} // iff->modified_length; i++) 
				apr_off_t modified_length = tempdiff->modified_length;
				for (int i=0; i<tempdiff->original_length; i++)
				{
					if (modified_length-- <= 0)
					{
						m_arDiffTheirBaseLeft.Add(m_arBaseFile.GetAt(baseline++));
						m_arStateTheirBaseLeft.Add(DIFFSTATE_ADDED);
						m_arDiffTheirBaseRight.Add(_T(""));
						m_arStateTheirBaseRight.Add(DIFFSTATE_EMPTY);
					} // if (tempdiff->original_length == 0)
				} // iff->modified_length; i++) 
			} // if (tempdiff->type == svn_diff__type_diff_modified) 
			tempdiff = tempdiff->next;
		}
		TRACE(_T("done with diff\n"));
	} // if ((!m_sBaseFile.IsEmpty()) && (!m_sTheirFile.IsEmpty())) 
	//#endregion
	//#region if ((!m_sBaseFile.IsEmpty()) && (!m_sTheirFile.IsEmpty()) && (!m_sYourFile.IsEmpty()))
	if ((!m_sBaseFile.IsEmpty()) && (!m_sTheirFile.IsEmpty()) && (!m_sYourFile.IsEmpty()))
	{
		svnerr = svn_diff_file_diff3(&m_diffTheirYourBase, fc1, fc2, fc3, pool);
		if (svnerr)
		{
			TRACE(_T("diff-error in CDiffData::Load()\n"));
			CString sMsg = CString(svnerr->message);
			while (svnerr->child)
			{
				svnerr = svnerr->child;
				sMsg += _T("\n");
				sMsg += CString(svnerr->message);
			} // while (m_err->child)
			apr_pool_destroy (pool);					// free the allocated memory
			apr_terminate();
			m_sError.Format(IDS_ERR_DIFF_DIFF, sMsg);
			return FALSE;
		} // if (m_svnerr)
		svn_diff_t * tempdiff = m_diffTheirYourBase;
		LONG baseline = 0;
		LONG yourline = 0;
		LONG theirline = 0;
		while (tempdiff)
		{
			if (tempdiff->type == svn_diff__type_common)
			{
				ASSERT((tempdiff->latest_length == tempdiff->modified_length) && (tempdiff->modified_length == tempdiff->original_length));
				for (int i=0; i<tempdiff->original_length; i++)
				{
					m_arDiff3.Add(m_arBaseFile.GetAt(baseline));
					m_arStateDiff3.Add(DIFFSTATE_NORMAL);
					m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
					m_arStateYourBaseBoth.Add(DIFFSTATE_NORMAL);
					m_arDiffTheirBaseBoth.Add(m_arTheirFile.GetAt(theirline));
					m_arStateTheirBaseBoth.Add(DIFFSTATE_NORMAL);
					baseline++;
					yourline++;
					theirline++;
				} // iff->original_length; i++)
			} // if (tempdiff->type == svn_diff__type_common)
			else if (tempdiff->type == svn_diff__type_diff_common)
			{
				if ((tempdiff->modified_length == 0)&&(tempdiff->latest_length == 0))
				{
					//both theirs and yours have lines deleted!
					for (int i=0; i<tempdiff->original_length; i++)
					{
						m_arDiff3.Add(m_arBaseFile.GetAt(baseline));
						m_arStateDiff3.Add(DIFFSTATE_IDENTICALREMOVED);
						m_arDiffYourBaseBoth.Add(m_arBaseFile.GetAt(baseline));
						m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
						m_arDiffTheirBaseBoth.Add(m_arBaseFile.GetAt(baseline));
						m_arStateTheirBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
						baseline++;
					} // iff->original_length; i++)
				} // if ((tempdiff->modified_length == 0)&&(tempdiff->latest_length == 0))
				else if (tempdiff->original_length == 0)
				{
					ASSERT(tempdiff->latest_length == tempdiff->modified_length);
					//both theirs and yours have lines added!
					for (int i=0; i<tempdiff->modified_length; i++)
					{
						m_arDiff3.Add(m_arYourFile.GetAt(yourline));
						m_arStateDiff3.Add(DIFFSTATE_IDENTICALADDED);
						m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
						m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALADDED);
						m_arDiffTheirBaseBoth.Add(m_arTheirFile.GetAt(theirline));
						m_arStateTheirBaseBoth.Add(DIFFSTATE_IDENTICALADDED);
						yourline++;
						theirline++;
					} // iff->original_length; i++)
				}
				else
				{
					ASSERT((tempdiff->latest_length == tempdiff->modified_length)&&(tempdiff->modified_length == tempdiff->original_length));
					//both theirs and yours have lines replaced
					for (int i=0; i<tempdiff->modified_length; i++)
					{
						m_arDiff3.Add(m_arBaseFile.GetAt(baseline));
						m_arStateDiff3.Add(DIFFSTATE_IDENTICALREMOVED);
						m_arDiffYourBaseBoth.Add(m_arBaseFile.GetAt(baseline));
						m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
						m_arDiffTheirBaseBoth.Add(m_arBaseFile.GetAt(baseline));
						m_arStateTheirBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
						baseline++;
					} // iff->original_length; i++)
					for (int i=0; i<tempdiff->modified_length; i++)
					{
						m_arDiff3.Add(m_arYourFile.GetAt(yourline));
						m_arStateDiff3.Add(DIFFSTATE_IDENTICAL);
						m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
						m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALADDED);
						m_arDiffTheirBaseBoth.Add(m_arTheirFile.GetAt(theirline));
						m_arStateTheirBaseBoth.Add(DIFFSTATE_IDENTICALADDED);
						yourline++;
						theirline++;
					} // iff->original_length; i++)
				}
			}
			else if (tempdiff->type == svn_diff__type_conflict)
			{
				apr_off_t length = max(tempdiff->original_length, tempdiff->modified_length);
				length = max(tempdiff->latest_length, length);
				apr_off_t original = tempdiff->original_length;
				apr_off_t modified = tempdiff->modified_length;
				apr_off_t latest = tempdiff->latest_length;

				apr_off_t originalresolved = 0;
				apr_off_t modifiedresolved = 0;
				apr_off_t latestresolved = 0;

				LONG base = baseline;
				LONG your = yourline;
				LONG their = theirline;
				if (tempdiff->resolved_diff)
				{
					originalresolved = tempdiff->resolved_diff->original_length;
					modifiedresolved = tempdiff->resolved_diff->modified_length;
					latestresolved = tempdiff->resolved_diff->latest_length;
				}
				for (int i=0; i<length; i++)
				{
					if (original)
					{
						m_arDiff3.Add(m_arBaseFile.GetAt(baseline));
						m_arStateDiff3.Add(DIFFSTATE_IDENTICALREMOVED);
					} // if (tempdiff->original_length--)
					else if ((originalresolved)||((modifiedresolved)&&(latestresolved)))
					{
						m_arDiff3.Add(_T(""));
						m_arStateDiff3.Add(DIFFSTATE_IDENTICALREMOVED);
					}
					if ((latest)&&(original))
					{
						m_arDiffYourBaseBoth.Add(m_arBaseFile.GetAt(baseline));
						m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
					} // if (tempdiff->modified_length--)
					else
					{
						if (original)
						{
							m_arDiffYourBaseBoth.Add(m_arBaseFile.GetAt(baseline));
							m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
						} // if (original)
						else if ((latestresolved)&&(modifiedresolved))
						{
							m_arDiffYourBaseBoth.Add(_T(""));
							m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
						}
					}
					if ((modified)&&(original))
					{
							m_arDiffTheirBaseBoth.Add(m_arBaseFile.GetAt(baseline));
							m_arStateTheirBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
					} // if (tempdiff->latest_length--)
					else
					{
						if (original)
						{
							m_arDiffTheirBaseBoth.Add(m_arBaseFile.GetAt(baseline));
							m_arStateTheirBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
						} // if (original)
						else if ((modifiedresolved)&&(latestresolved))
						{
							m_arDiffTheirBaseBoth.Add(_T(""));
							m_arStateTheirBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
						}
					}
					if (original)
					{
						original--;
						baseline++;
					} // if (original)
					if (originalresolved)
						originalresolved--;

					if (modified)
					{
						modified--;
						theirline++;
					} // if (modified)
					if (modifiedresolved)
						modifiedresolved--;
					if (latest)
					{
						latest--;
						yourline++;
					} // if (latest)
					if (latestresolved)
						latestresolved--;
				} // for (int i=0; i<length; i++) 
				original = tempdiff->original_length;
				modified = tempdiff->modified_length;
				latest = tempdiff->latest_length;
				baseline = base;
				yourline = your;
				theirline = their;
				if (tempdiff->resolved_diff)
				{
					originalresolved = tempdiff->resolved_diff->original_length;
					modifiedresolved = tempdiff->resolved_diff->modified_length;
					latestresolved = tempdiff->resolved_diff->latest_length;
				}
				for (int i=0; i<length; i++)
				{
					if ((latest)||(modified))
					{
						m_arDiff3.Add(_T("--- Unresolved Conflict!! ---"));
						m_arStateDiff3.Add(DIFFSTATE_CONFLICTED);
					}

					if (latest)
					{
						m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
						m_arStateYourBaseBoth.Add(DIFFSTATE_CONFLICTADDED);
					} // if (tempdiff->modified_length--)
					else if ((latestresolved)||(modified)||(modifiedresolved))
					{
						m_arDiffYourBaseBoth.Add(_T(""));
						m_arStateYourBaseBoth.Add(DIFFSTATE_CONFLICTEMPTY);
					}
					if (modified)
					{
						m_arDiffTheirBaseBoth.Add(m_arTheirFile.GetAt(theirline));
						m_arStateTheirBaseBoth.Add(DIFFSTATE_CONFLICTADDED);
					} // if (tempdiff->latest_length--)
					else if ((modifiedresolved)||(latest)||(latestresolved))
					{
						m_arDiffTheirBaseBoth.Add(_T(""));
						m_arStateTheirBaseBoth.Add(DIFFSTATE_CONFLICTEMPTY);
					}
					if (original)
					{
						original--;
						baseline++;
					}
					if (originalresolved)
						originalresolved--;
					if (modified)
					{
						modified--;
						theirline++;
					}
					if (modifiedresolved)
						modifiedresolved--;
					if (latest)
					{
						latest--;
						yourline++;
					}
					if (latestresolved)
						latestresolved--;
				} // for (int i=0; i<length; i++) 
			} // if (tempdiff->type == svn_diff__type_conflict)  
			else if (tempdiff->type == svn_diff__type_diff_modified)
			{
				//deleted!
				for (int i=0; i<tempdiff->original_length; i++)
				{
					m_arDiff3.Add(m_arBaseFile.GetAt(baseline));
					m_arStateDiff3.Add(DIFFSTATE_THEIRSREMOVED);
					m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
					m_arStateYourBaseBoth.Add(DIFFSTATE_NORMAL);
					m_arDiffTheirBaseBoth.Add(m_arBaseFile.GetAt(baseline));
					m_arStateTheirBaseBoth.Add(DIFFSTATE_THEIRSREMOVED);
					baseline++;
					yourline++;
				} // iff->modified_length; i++) 
				//added
				for (int i=0; i<tempdiff->modified_length; i++)
				{
					m_arDiff3.Add(m_arTheirFile.GetAt(theirline));
					m_arStateDiff3.Add(DIFFSTATE_THEIRSADDED);
					m_arDiffYourBaseBoth.Add(_T(""));
					m_arStateYourBaseBoth.Add(DIFFSTATE_EMPTY);
					m_arDiffTheirBaseBoth.Add(m_arTheirFile.GetAt(theirline));
					m_arStateTheirBaseBoth.Add(DIFFSTATE_THEIRSADDED);
					theirline++;
				} // iff->modified_length; i++) 
			} // if (tempdiff->type == svn_diff__type_diff_modified) 
			else if (tempdiff->type == svn_diff__type_diff_latest)
			{
				//YOURS differs from base

				for (int i=0; i<tempdiff->original_length; i++)
				{
					m_arDiff3.Add(m_arBaseFile.GetAt(baseline));
					m_arStateDiff3.Add(DIFFSTATE_YOURSREMOVED);
					m_arDiffYourBaseBoth.Add(m_arBaseFile.GetAt(baseline));
					m_arStateYourBaseBoth.Add(DIFFSTATE_YOURSREMOVED);
					m_arDiffTheirBaseBoth.Add(m_arTheirFile.GetAt(theirline));
					m_arStateTheirBaseBoth.Add(DIFFSTATE_NORMAL);
					baseline++;
					theirline++;
				}
				for (int i=0; i<tempdiff->latest_length; i++)
				{
					m_arDiff3.Add(m_arYourFile.GetAt(yourline));
					m_arStateDiff3.Add(DIFFSTATE_YOURSADDED);
					m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
					m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALADDED);
					m_arDiffTheirBaseBoth.Add(_T(""));
					m_arStateTheirBaseBoth.Add(DIFFSTATE_YOURSADDED);
					yourline++;
				} // iff->original_length; i++) 
			} // if (tempdiff->type == svn_diff__type_diff_latest) 
			else
			{
				TRACE(_T("something bad happened during diff!\n"));
			}
			tempdiff = tempdiff->next;

		} // iff) 
		ASSERT(m_arDiff3.GetCount() == m_arDiffYourBaseBoth.GetCount());
		ASSERT(m_arDiffTheirBaseBoth.GetCount() == m_arDiffYourBaseBoth.GetCount());

		TRACE(_T("done with diff\n"));
	} // if ((!m_sBaseFile.IsEmpty()) && (!m_sTheirFile.IsEmpty()))  
	//#endregion

	apr_pool_destroy (pool);					// free the allocated memory
	apr_terminate();
	return TRUE;
}

