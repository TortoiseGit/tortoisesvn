/***************************************************************************
 *   Copyright (C) 2009 by Stefan Fuhrmann                                 *
 *   stefanfuhrmann@alice-dsl.de                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "stdafx.h"
#include "JobBase.h"
#include "JobScheduler.h"

namespace async
{

// nothing special during construction / destuction

CJobBase::CJobBase(void)
    : waiting (TRUE)
{
}

CJobBase::~CJobBase(void)
{
    assert (finished.Test());
}

// call this to put the job into the scheduler

void CJobBase::Schedule (bool transferOwnership, CJobScheduler* scheduler)
{
    if (scheduler == NULL)
        scheduler = CJobScheduler::GetDefault();

    scheduler->Schedule (this, transferOwnership);
}

// will be called by job execution thread

void CJobBase::Execute()
{
    // intended for one-shot execution only

    assert (!finished.Test());
    InterlockedExchange (&waiting, FALSE); 

    InternalExecute();
    finished.Set();
}

// may be called by other (observing) threads

IJob::Status CJobBase::GetStatus() const
{
    if (waiting == TRUE)
        IJob::waiting;

    return finished.Test() ? IJob::done : IJob::running;
}

void CJobBase::WaitUntilDone()
{
    finished.WaitFor();
}

bool CJobBase::WaitUntilDoneOrTimeout(DWORD milliSeconds)
{
	return finished.WaitForEndOrTimeout(milliSeconds);
}

}