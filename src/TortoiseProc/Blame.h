// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
#include "SVN.h"
#include "ProgressDlg.h"

class CBlame : public SVN
{
public:
	CBlame();
	virtual ~CBlame();

	/**
	 * Determine for every line in a versioned file the author, revision of last change, date of last
	 * change. The result is saved to a temporary file.\n
	 * Since this operation takes a long time a progress dialog is shown if \a showprogress is set to TRUE
	 * \param startrev the starting revision of the operation
	 * \param endrev the revision to stop the operation
	 * \param path the path to the file to determine the required information
	 * \return The path to the temporary file or an empty string in case of an error.
	 */
	CString		BlameToTempFile(CString path, LONG startrev, LONG endrev, BOOL showprogress = TRUE);
private:
	BOOL		BlameCallback(LONG linenumber, LONG revision, CString author, CString date, CString line);
	BOOL		Cancel();

private:
	BOOL		m_bCancelled;			///< TRUE if the operation should be cancelled
	LONG		m_nCounter;				///< Counts the number of calls to the Cancel() callback (revisions?)
	LONG		m_nHeadRev;				///< The HEAD revision of the file

	CString		m_sSavePath;			///< Where to save the blame data
	CStdioFile	m_saveFile;				///< The file object to write to
	CProgressDlg m_progressDlg;			///< The progress dialog shown during operation
};