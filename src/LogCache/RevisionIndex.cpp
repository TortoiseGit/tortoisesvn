// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "StdAfx.h"
#include ".\revisionindex.h"

#include "DiffIntegerInStream.h"
#include "DiffIntegerOutStream.h"

// begin namespace LogCache

namespace LogCache
{

// construction / destruction

CRevisionIndex::CRevisionIndex(void)
	: firstRevision(0)
{
}

CRevisionIndex::~CRevisionIndex(void)
{
}

// range of actually available revisions:
// GetFirstCachedRevision() .. GetLastCachedRevision()-1

revision_t CRevisionIndex::GetFirstCachedRevision() const
{
    for (size_t i = 0, count = indices.size(); i < count; ++i)
        if (indices[i] != NO_INDEX)
            return firstRevision + static_cast<revision_t>(i);

    return (revision_t)NO_REVISION;
}

revision_t CRevisionIndex::GetLastCachedRevision() const
{
    for (size_t i = indices.size(); i > 0; --i)
        if (indices[i-1] != NO_INDEX)
            return firstRevision + static_cast<revision_t>(i);

    return (revision_t)NO_REVISION;
}

// insert info (must be NO_INDEX before)

void CRevisionIndex::SetRevisionIndex (revision_t revision, index_t index)
{
	// parameter check

	assert (operator[](revision) == NO_INDEX);
	assert (index != NO_INDEX);

	if (revision == NO_REVISION)
		throw std::exception ("Invalid revision");

	// special cases

	if (indices.empty())
	{
		indices.push_back (index);
		firstRevision = revision;
		return;
	}

	if (revision == GetLastRevision())
	{
		indices.push_back (index);
		return;
	}

	// make sure, there is an entry in indices for that revision

	if (revision < firstRevision)
	{
		// efficiently grow on the lower end

		revision_t indexSize = (revision_t)indices.size();
		revision_t newFirstRevision = firstRevision < indexSize
			? 0
			: min (firstRevision - indexSize, revision);

		indices.insert (indices.begin(), firstRevision - newFirstRevision, (index_t)NO_INDEX);
		firstRevision = newFirstRevision;
	}
	else
	{
		revision_t size = (revision_t)indices.size();
		if (revision - firstRevision >= size)
		{
			// efficiently grow on the upper end

			size_t toAdd = max (size, revision + 1 - firstRevision - size);
			indices.insert (indices.end(), toAdd, (index_t)NO_INDEX);
		}
	}

	// bucket exists -> just write the value

	indices [revision - firstRevision] = index;
}

// reset content

void CRevisionIndex::Clear()
{
	indices.clear();

	firstRevision = 0;
}

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CRevisionIndex& container)
{
	// read the string offsets

	CDiffIntegerInStream* indexStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionIndex::INDEX_STREAM_ID));

	container.firstRevision 
		= static_cast<revision_t>(indexStream->GetSizeValue());
	*indexStream >> container.indices;

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CRevisionIndex& container)
{
	// the string positions

	CDiffIntegerOutStream* indexStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionIndex::INDEX_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));

	indexStream->AddSizeValue (container.firstRevision);
	*indexStream << container.indices;

	// ready

	return stream;
}

// end namespace LogCache

}

