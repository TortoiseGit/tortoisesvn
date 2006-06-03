// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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
#include "StdAfx.h"
#include "Resource.h"
#include "UnicodeUtils.h"
#include "DirFileEnum.h"
#include "TortoiseMerge.h"
#include "svn_wc.h"
#include "SVNAdminDir.h"
#include "Patch.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPatch::CPatch(void)
{
}

CPatch::~CPatch(void)
{
	FreeMemory();
}

void CPatch::FreeMemory()
{
	for (int i=0; i<m_arFileDiffs.GetCount(); i++)
	{
		Chunks * chunks = m_arFileDiffs.GetAt(i);
		for (int j=0; j<chunks->chunks.GetCount(); j++)
		{
			delete chunks->chunks.GetAt(j);
		}
		chunks->chunks.RemoveAll();
		delete chunks;
	} // iffs.GetCount(); i++) 
	m_arFileDiffs.RemoveAll();
}

BOOL CPatch::OpenUnifiedDiffFile(const CString& filename)
{
	CString sLine;
	INT_PTR nIndex = 0;
	INT_PTR nLineCount = 0;
	g_crasher.AddFile((LPCSTR)(LPCTSTR)filename, (LPCSTR)(LPCTSTR)_T("unified diff file"));

	CFileTextLines PatchLines;
	if (!PatchLines.Load(filename))
	{
		m_sErrorMessage = PatchLines.GetErrorString();
		return FALSE;
	} // if (!PatchLines.Load(filename)) 
	m_UnicodeType = PatchLines.GetUnicodeType();
	FreeMemory();
	nLineCount = PatchLines.GetCount();
	//now we got all the lines of the patch file
	//in our array - parsing can start...

	//first, skip possible garbage at the beginning
	//garbage is finished when a line starts with "Index: "
	//and the next line consists of only "=" characters
	for (; nIndex<PatchLines.GetCount(); nIndex++)
	{
		sLine = PatchLines.GetAt(nIndex);
		if (sLine.Left(7).Compare(_T("Index: "))==0)
		{
			if ((nIndex+1)<PatchLines.GetCount())
			{
				sLine = PatchLines.GetAt(nIndex+1);
				sLine.Replace(_T("="), _T(""));
				if (sLine.IsEmpty())
					break;
			} // if ((nIndex+1)<m_PatchLines.GetCount()) 
		} // if (sLine.Left(7).Compare(_T("Index: "))==0) 
	} // for (nIndex=0; nIndex<m_PatchLines.GetCount(); nIndex++) 
	if ((PatchLines.GetCount()-nIndex) < 2)
	{
		//no file entry found.
		m_sErrorMessage.LoadString(IDS_ERR_PATCH_NOINDEX);
		return FALSE;
	} // if ((m_PatchLines.GetCount()-nIndex) < 2)

	//from this point on we have the real unified diff data
	int state = 0;
	Chunks * chunks = NULL;
	Chunk * chunk = NULL;
	int nAddLineCount = 0;
	int nRemoveLineCount = 0;
	int nContextLineCount = 0;
	for ( ;nIndex<PatchLines.GetCount(); nIndex++)
	{
		sLine = PatchLines.GetAt(nIndex);
		if (sLine.IsEmpty())
			continue;
		switch (state)
		{
		case 0:	//Index: <filepath>
			{
				if (sLine.Left(7).Compare(_T("Index: "))==0)
				{
					if (chunks)
					{
						//this is a new filediff, so add the last one to 
						//our array.
						m_arFileDiffs.Add(chunks);
					} // if (chunks) 
					chunks = new Chunks();
					chunks->sFilePath = sLine.Mid(7).Trim();
					if (chunks->sFilePath.Right(9).Compare(_T("(deleted)"))==0)
						chunks->sFilePath.Left(chunks->sFilePath.GetLength()-9).TrimRight();
					if (chunks->sFilePath.Right(7).Compare(_T("(added)"))==0)
						chunks->sFilePath.Left(chunks->sFilePath.GetLength()-7).TrimRight();
					state++;
				} // if (sLine.Left(7).Compare(_T("Index: "))==0)
				else
				{
					if (nIndex > 0)
					{
						nIndex--;
						state = 4;
						if (chunks == NULL)
						{
							//the line
							//Index: <filepath>
							//was not found at the start of a filediff!
							break;
						}
					}
				}
			} 
		break;
		case 1:	//====================
			{
				sLine.Replace(_T("="), _T(""));
				if (sLine.IsEmpty())
					state++;
				else
				{
					//the line
					//=========================
					//was not found
					m_sErrorMessage.Format(IDS_ERR_PATCH_NOEQUATIONCHARLINE, nIndex);
					goto errorcleanup;
				}
			}
		break;
		case 2:	//--- <filepath>
			{
				if (sLine.Left(3).Compare(_T("---"))!=0)
				{
					//no starting "---" found
					//seems to be either garbage or just
					//a binary file. So start over...
					state = 0;
					nIndex--;
					if (chunks)
					{
						delete chunks;
						chunks = NULL;
					}
					break;
				} // if (sLine.Left(3).Compare(_T("---"))!=0)
				sLine = sLine.Mid(3);	//remove the "---"
				sLine =sLine.Trim();
				//at the end of the filepath there's a revision number...
				int bracket = sLine.ReverseFind('(');
				CString num = sLine.Mid(bracket);		//num = "(revision xxxxx)"
				num = num.Mid(num.Find(' '));
				num = num.Trim(_T(" )"));
				chunks->sRevision = num;
				chunks->sFilePath = sLine.Left(bracket-1).Trim();
				state++;
			}
		break;
		case 3:	//+++ <filepath>
			{
				if (sLine.Left(3).Compare(_T("+++"))!=0)
				{
					//no starting "+++" found
					m_sErrorMessage.Format(IDS_ERR_PATCH_NOADDFILELINE, nIndex);
					goto errorcleanup;
				} // if (sLine.Left(3).Compare(_T("---"))!=0)
				sLine = sLine.Mid(3);	//remove the "---"
				sLine =sLine.Trim();
				//at the end of the filepath there's a revision number...
				int bracket = sLine.ReverseFind('(');
				CString num = sLine.Mid(bracket);		//num = "(revision xxxxx)"
				num = num.Mid(num.Find(' '));
				num = num.Trim(_T(" )"));
				chunks->sRevision2 = num;
				chunks->sFilePath2 = sLine.Left(bracket-1).Trim();
				state++;
			}
		break;
		case 4:	//@@ -xxx,xxx +xxx,xxx @@
			{
				//start of a new chunk
				if (sLine.Left(2).Compare(_T("@@"))!=0)
				{
					//chunk doesn't start with "@@"
					//so there's garbage in between two filediffs
					state = 0;
					if (chunk)
					{
						delete chunk;
						chunk = 0;
						if (chunks)
						{
							for (int i=0; i<chunks->chunks.GetCount(); i++)
							{
								delete chunks->chunks.GetAt(i);
							}
							chunks->chunks.RemoveAll();
							delete chunks;
							chunks = NULL;
						} // if (chunks)
					} // if (chunk) 
					break;		//skip the garbage
					//m_sErrorMessage.Format(IDS_ERR_PATCH_CHUNKSTARTNOTFOUND, nIndex);
					//goto errorcleanup;
				} // if (sLine.Left(2).Compare(_T("@@"))!=0) 
				sLine = sLine.Mid(2);
				sLine = sLine.Trim();
				chunk = new Chunk();
				CString sRemove = sLine.Left(sLine.Find(' '));
				CString sAdd = sLine.Mid(sLine.Find(' '));
				chunk->lRemoveStart = (-_ttol(sRemove));
				if (sRemove.Find(',')>=0)
				{
					sRemove = sRemove.Mid(sRemove.Find(',')+1);
					chunk->lRemoveLength = _ttol(sRemove);
				} // if (sRemove.Find(',')>=0)
				else
				{
					chunk->lRemoveStart = 0;
					chunk->lRemoveLength = (-_ttol(sRemove));
				}
				chunk->lAddStart = _ttol(sAdd);
				if (sAdd.Find(',')>=0)
				{
					sAdd = sAdd.Mid(sAdd.Find(',')+1);
					chunk->lAddLength = _ttol(sAdd);
				} // if (sAdd.Find(',')>=0)
				else
				{
					chunk->lAddStart = 1;
					chunk->lAddLength = _ttol(sAdd);
				}
				state++;
			}
		break;
		case 5: //[ |+|-] <sourceline>
			{
				//this line is either a context line (with a ' ' in front)
				//a line added (with a '+' in front)
				//or a removed line (with a '-' in front)
				TCHAR type;
				if (sLine.IsEmpty())
					type = ' ';
				else
					type = sLine.GetAt(0);
				if (type == ' ')
				{
					//it's a context line - we don't use them here right now
					//but maybe in the future the patch algorithm can be
					//extended to use those in case the file to patch has
					//already changed and no base file is around...
					chunk->arLines.Add(sLine.Mid(1));
					chunk->arLinesStates.Add(PATCHSTATE_CONTEXT);
					nContextLineCount++;
				} // if (type == ' ')
				else if (type == '\\')
				{
					//it's a context line (sort of): 
					//warnings start with a '\' char (e.g. "\ No newline at end of file")
					//so just ignore this...
				}
				else if (type == '-')
				{
					//a removed line
					chunk->arLines.Add(sLine.Mid(1));
					chunk->arLinesStates.Add(PATCHSTATE_REMOVED);
					nRemoveLineCount++;
				}
				else if (type == '+')
				{
					//an added line
					chunk->arLines.Add(sLine.Mid(1));
					chunk->arLinesStates.Add(PATCHSTATE_ADDED);
					nAddLineCount++;
				}
				else
				{
					//none of those lines! what the hell happened here?
					m_sErrorMessage.Format(IDS_ERR_PATCH_UNKOWNLINETYPE, nIndex);
					goto errorcleanup;
				}
				if ((chunk->lAddLength == (nAddLineCount + nContextLineCount)) &&
					chunk->lRemoveLength == (nRemoveLineCount + nContextLineCount))
				{
					//chunk is finished
					if (chunks)
						chunks->chunks.Add(chunk);
					else
						delete chunk;
					chunk = NULL;
					nAddLineCount = 0;
					nContextLineCount = 0;
					nRemoveLineCount = 0;
					state = 0;
				}
			} 
		break;
		default:
			ASSERT(FALSE);
		} // switch (state) 
	} // for ( ;nIndex<m_PatchLines.GetCount(); nIndex++) 
	if (chunk)
	{
		m_sErrorMessage.LoadString(IDS_ERR_PATCH_CHUNKMISMATCH);
		goto errorcleanup;
	} // if (chunk)
	if (chunks)
		m_arFileDiffs.Add(chunks);
	return TRUE;
errorcleanup:
	if (chunk)
		delete chunk;
	if (chunks)
	{
		for (int i=0; i<chunks->chunks.GetCount(); i++)
		{
			delete chunks->chunks.GetAt(i);
		}
		chunks->chunks.RemoveAll();
		delete chunks;
	} // if (chunks)
	FreeMemory();
	return FALSE;
}

CString CPatch::GetFilename(int nIndex)
{
	if (nIndex < 0)
		return _T("");
	if (nIndex < m_arFileDiffs.GetCount())
	{
		Chunks * c = m_arFileDiffs.GetAt(nIndex);
		return c->sFilePath;
	}
	return _T("");
}

CString CPatch::GetRevision(int nIndex)
{
	if (nIndex < 0)
		return 0;
	if (nIndex < m_arFileDiffs.GetCount())
	{
		Chunks * c = m_arFileDiffs.GetAt(nIndex);
		return c->sRevision;
	}
	return 0;
}

CString CPatch::GetFilename2(int nIndex)
{
	if (nIndex < 0)
		return _T("");
	if (nIndex < m_arFileDiffs.GetCount())
	{
		Chunks * c = m_arFileDiffs.GetAt(nIndex);
		return c->sFilePath2;
	}
	return _T("");
}

CString CPatch::GetRevision2(int nIndex)
{
	if (nIndex < 0)
		return 0;
	if (nIndex < m_arFileDiffs.GetCount())
	{
		Chunks * c = m_arFileDiffs.GetAt(nIndex);
		return c->sRevision2;
	} 
	return 0;
}

BOOL CPatch::PatchFile(const CString& sPath, const CString& sSavePath, const CString& sBaseFile)
{
	if (PathIsDirectory(sPath))
	{
		m_sErrorMessage.Format(IDS_ERR_PATCH_INVALIDPATCHFILE, (LPCTSTR)sPath);
		return FALSE;
	}
	int nIndex = -1;
	for (int i=0; i<GetNumberOfFiles(); i++)
	{
		CString temppath = sPath;
		CString temp = GetFilename(i);
		temppath.Replace('/', '\\');
		temp.Replace('/', '\\');
		if (temppath.Mid(temppath.GetLength()-temp.GetLength()-1, 1).CompareNoCase(_T("\\"))==0)
		{
			temppath = temppath.Right(temp.GetLength());
			if ((temp.CompareNoCase(temppath)==0))
			{
				if ((nIndex < 0)&&(! temp.IsEmpty()))
				{
					nIndex = i;
				}
				else
				{
					m_sErrorMessage.Format(IDS_ERR_PATCH_FILEFOUNDTWICE, (LPCTSTR)temppath);
					return FALSE;
				}
			}
		}
		else if (temppath.CompareNoCase(temp)==0)
		{
			if ((nIndex < 0)&&(! temp.IsEmpty()))
			{
				nIndex = i;
			}
		}
	}
	if (nIndex < 0)
	{
		m_sErrorMessage.Format(IDS_ERR_PATCH_FILENOTINPATCH, (LPCTSTR)sPath);
		return FALSE;
	}

	CString sLine;
	CString sPatchFile = sBaseFile.IsEmpty() ? sPath : sBaseFile;
	if (PathFileExists(sPatchFile))
	{
		g_crasher.AddFile((LPCSTR)(LPCTSTR)sPatchFile, (LPCSTR)(LPCTSTR)_T("File to patch"));
	}
	CFileTextLines PatchLines;
	CFileTextLines PatchLinesResult;
	PatchLines.Load(sPatchFile);
	PatchLinesResult = PatchLines;  //.Copy(PatchLines);
	PatchLines.CopySettings(&PatchLinesResult);

	Chunks * chunks = m_arFileDiffs.GetAt(nIndex);

	for (int i=0; i<chunks->chunks.GetCount(); i++)
	{
		Chunk * chunk = chunks->chunks.GetAt(i);
		LONG lRemoveLine = chunk->lRemoveStart;
		LONG lAddLine = chunk->lAddStart;
		for (int j=0; j<chunk->arLines.GetCount(); j++)
		{
			CString sPatchLine = chunk->arLines.GetAt(j);
			if ((m_UnicodeType != CFileTextLines::UTF8)&&(m_UnicodeType != CFileTextLines::UTF8BOM))
			{
				if ((PatchLines.GetUnicodeType()==CFileTextLines::UTF8)||(m_UnicodeType == CFileTextLines::UTF8BOM))
				{
					// convert the UTF-8 contents in CString sPatchLine into a CStringA
					CStringA sPatchLineA;
					char *pszPatchLine = sPatchLineA.GetBuffer(sPatchLine.GetLength());
					for (int k = 0; k < sPatchLine.GetLength(); ++k)
					{
						*pszPatchLine++ = (char)sPatchLine.GetAt(k);
					}
					*pszPatchLine = 0;
					sPatchLineA.ReleaseBuffer();
					sPatchLine = CUnicodeUtils::GetUnicode(sPatchLineA);
				}
			}
			int nPatchState = (int)chunk->arLinesStates.GetAt(j);
			switch (nPatchState)
			{
			case PATCHSTATE_REMOVED:
				{
					if ((lAddLine > PatchLines.GetCount())||(PatchLines.GetCount()==0))
					{
						m_sErrorMessage.Format(IDS_ERR_PATCH_DOESNOTMATCH, _T(""), (LPCTSTR)sPatchLine);
						return FALSE; 
					}
					if (lAddLine == 0)
						lAddLine = 1;
					if ((sPatchLine.Compare(PatchLines.GetAt(lAddLine-1))!=0)&&(!HasExpandedKeyWords(sPatchLine)))
					{
						m_sErrorMessage.Format(IDS_ERR_PATCH_DOESNOTMATCH, (LPCTSTR)sPatchLine, (LPCTSTR)PatchLines.GetAt(lAddLine-1));
						return FALSE; 
					}
					if (lAddLine > PatchLines.GetCount())
					{
						m_sErrorMessage.Format(IDS_ERR_PATCH_DOESNOTMATCH, (LPCTSTR)sPatchLine, _T(""));
						return FALSE; 
					}
					PatchLines.RemoveAt(lAddLine-1);
				} 
				break;
			case PATCHSTATE_ADDED:
				{
					if (lAddLine == 0)
						lAddLine = 1;
					PatchLines.InsertAt(lAddLine-1, sPatchLine);
					lAddLine++;
				}
				break;
			case PATCHSTATE_CONTEXT:
				{
					if (lAddLine > PatchLines.GetCount())
					{
						m_sErrorMessage.Format(IDS_ERR_PATCH_DOESNOTMATCH, _T(""), (LPCTSTR)sPatchLine);
						return FALSE; 
					}
					if (lAddLine == 0)
						lAddLine++;
					if ((sPatchLine.Compare(PatchLines.GetAt(lAddLine-1))!=0) &&
						(!HasExpandedKeyWords(sPatchLine)) &&
						(lRemoveLine <= PatchLines.GetCount()) &&
						(sPatchLine.Compare(PatchLines.GetAt(lRemoveLine-1))!=0))
					{
						m_sErrorMessage.Format(IDS_ERR_PATCH_DOESNOTMATCH, (LPCTSTR)sPatchLine, (LPCTSTR)PatchLines.GetAt(lAddLine-1));
						return FALSE; 
					} 
					lAddLine++;
					lRemoveLine++;
				}
				break;
			default:
				ASSERT(FALSE);
				break;
			} // switch (nPatchState) 
		} // for (j=0; j<chunk->arLines.GetCount(); j++) 
	} // for (int i=0; i<chunks->chunks.GetCount(); i++) 
	if (!sSavePath.IsEmpty())
	{
		PatchLines.Save(sSavePath, false);
	} // if (!sSavePath.IsEmpty())
	return TRUE;
}

BOOL CPatch::HasExpandedKeyWords(const CString& line)
{
	if (line.Find(_T("$LastChangedDate"))>=0)
		return TRUE;
	if (line.Find(_T("$Date"))>=0)
		return TRUE;
	if (line.Find(_T("$LastChangedRevision"))>=0)
		return TRUE;
	if (line.Find(_T("$Rev"))>=0)
		return TRUE;
	if (line.Find(_T("$LastChangedBy"))>=0)
		return TRUE;
	if (line.Find(_T("$Author"))>=0)
		return TRUE;
	if (line.Find(_T("$HeadURL"))>=0)
		return TRUE;
	if (line.Find(_T("$URL"))>=0)
		return TRUE;
	if (line.Find(_T("$Id"))>=0)
		return TRUE;
	return FALSE;
}

CString	CPatch::CheckPatchPath(const CString& path)
{
	//first check if the path already matches
	if (CountMatches(path) > (GetNumberOfFiles()/3))
		return path;
	//now go up the tree and try again
	CString upperpath = path;
	while (upperpath.ReverseFind('\\')>0)
	{
		upperpath = upperpath.Left(upperpath.ReverseFind('\\'));
		if (CountMatches(upperpath) > (GetNumberOfFiles()/3))
			return upperpath;
	}
	//still no match found. So try subfolders
	bool isDir = false;
	CString subpath;
	CDirFileEnum filefinder(path);
	while (filefinder.NextFile(subpath, &isDir))
	{
		if (!isDir)
			continue;
		if (g_SVNAdminDir.IsAdminDirPath(subpath))
			continue;
		if (CountMatches(subpath) > (GetNumberOfFiles()/3))
			return subpath;
	}
	
	// if a patch file only contains newly added files
	// we can't really find the correct path.
	// But: we can compare paths strings without the filenames
	// and check if at least those match
	upperpath = path;
	while (upperpath.ReverseFind('\\')>0)
	{
		upperpath = upperpath.Left(upperpath.ReverseFind('\\'));
		if (CountDirMatches(upperpath) > (GetNumberOfFiles()/3))
			return upperpath;
	}
	
	return path;
}

int CPatch::CountMatches(const CString& path)
{
	int matches = 0;
	for (int i=0; i<GetNumberOfFiles(); ++i)
	{
		CString temp = GetFilename(i);
		temp.Replace('/', '\\');
		if (PathIsRelative(temp))
			temp = path + _T("\\")+ temp;
		if (PathFileExists(temp))
			matches++;
	}
	return matches;
}

int CPatch::CountDirMatches(const CString& path)
{
	int matches = 0;
	for (int i=0; i<GetNumberOfFiles(); ++i)
	{
		CString temp = GetFilename(i);
		temp.Replace('/', '\\');
		if (PathIsRelative(temp))
			temp = path + _T("\\")+ temp;
		// remove the filename
		temp = temp.Left(temp.ReverseFind('\\'));
		if (PathFileExists(temp))
			matches++;
	}
	return matches;
}
