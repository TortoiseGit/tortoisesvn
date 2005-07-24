// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2005 - Stefan Kueng

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

// A class to encapsulate some of the details of working with filenames and descriptive names
// for the various files used in TMerge
class CWorkingFile
{ 
public:
	CWorkingFile(void);
	~CWorkingFile(void);

public:
	// Is this file in use?
	bool InUse() const		{ return !m_sFilename.IsEmpty(); }
	bool Exists() const;
	void SetFileName(const CString& newFilename);
	void SetDescriptiveName(const CString& newDescName);
	void CreateEmptyFile();
	CString GetWindowName() const;
	CString GetFilename() const		{ return m_sFilename; }
	void SetOutOfUse()				{ m_sFilename.Empty(); m_sDescriptiveName.Empty(); }

	// Move the details of the specified file to the current one, and then mark the specified file
	// as out of use
	void TransferDetailsFrom(CWorkingFile& rightHandFile);

private:
	CString m_sFilename;
	CString m_sDescriptiveName;

}; 
