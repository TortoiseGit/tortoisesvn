#include "StdAfx.h"
#include "Resource.h"

#include "ILogReceiver.h"

///////////////////////////////////////////////////////////////
// data structures to accomodate the change list 
// (taken from the SVN class)
///////////////////////////////////////////////////////////////

// convenience method

const CString& LogChangedPath::GetAction() const
{
	if (actionAsString.IsEmpty())
	{
		switch (action)
		{
		case LOGACTIONS_ADDED: 
			actionAsString.LoadString(IDS_SVNACTION_ADD);
			break;

		case LOGACTIONS_DELETED: 
			actionAsString.LoadString(IDS_SVNACTION_DELETE);
			break;

		case LOGACTIONS_REPLACED: 
			actionAsString.LoadString(IDS_SVNACTION_REPLACED);
			break;

		case LOGACTIONS_MODIFIED: 
			actionAsString.LoadString(IDS_SVNACTION_MODIFIED);
			break;

		default:
			// there should always be an action
			assert (0);
		}
	}

	return actionAsString;
}
