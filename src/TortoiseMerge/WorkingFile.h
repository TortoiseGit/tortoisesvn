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
