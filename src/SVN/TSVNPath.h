#pragma once

class CTSVNPath
{
public:
	CTSVNPath(void);
	~CTSVNPath(void);

public:
	void SetFromSVN(const char* pPath);
	void SetFromSVN(const CString& sPath);
	void SetFromWin(LPCTSTR pPath);
	void SetFromWin(const CString& sPath);
	void SetFromWin(const CString& sPath, bool bIsDirectory);
	void SetFromUnknown(const CString& sPath);
	LPCTSTR GetWinPath() const;
	const CString& GetWinPathString() const;
	const CString& GetSVNPathString() const;
	const char* GetSVNPathNarrow() const;
	bool IsDirectory() const;
	CTSVNPath GetDirectory() const;
	CString GetFilename() const;
	CString GetFileExtension() const;
	static bool ArePathStringsEqual(const CString& sP1, const CString& sP2);
	bool IsEmpty() const;
	void Reset();
	bool IsEquivalentTo(const CTSVNPath& rhs);
	bool IsAncestorOf(const CTSVNPath& possibleDescendant) const;
	// Get a string representing the file path, optionally with a base 
	// section stripped off the front
	// Returns a string with fwdslash paths // TODO: Is this really what's wanted?
	CString GetDisplayString(const CTSVNPath* pOptionalBasePath = NULL) const;
	static int Compare(const CTSVNPath& left, const CTSVNPath& right);
		void AppendString(const CString& sAppend);

private:
	// All these functions are const, and all the data
	// is mutable, in order that the hidden caching operations
	// can be carried out on a const CTSVNPath object, which is what's 
	// likely to be passed between functions
	// The public 'SetFromxxx' functions are not const, and so the proper 
	// const-correctness semantics are preserved
	void SetFwdslashPath(const CString& sPath) const;
	void SetBackslashPath(const CString& sPath) const;
	void SetUTF8FwdslashPath(const CString& sPath) const;
	void EnsureBackslashPathSet() const;
	void EnsureFwdslashPathSet() const;

private:
	mutable CString m_sBackslashPath;
	mutable CString m_sFwdslashPath;
	mutable	CStringA m_sUTF8FwdslashPath;
	// Have we yet determined if this is a directory or not?
	mutable bool m_bDirectoryKnown;
	mutable bool m_bIsDirectory;
};

//////////////////////////////////////////////////////////////////////////

// A useful collections of Paths
class CTSVNPathList 
{
public:
	CTSVNPathList();

public:
	void AddPath(const CTSVNPath& newPath);
	bool LoadFromTemporaryFile(const CString& sFilename);
	int GetCount() const;
	const CTSVNPath& operator[](int index) const;
	bool AreAllPathsFiles() const;
	bool AreAllPathsFilesInOneDirectory() const;
	CTSVNPath GetCommonDirectory() const;
//	CString GetDirectory() const;

private:
	typedef std::vector<CTSVNPath> PathVector;
	PathVector m_paths;
	// If the list contains just files in one directory, then
	// this contains the directory name
	mutable CTSVNPath m_commonBaseDirectory;
};


