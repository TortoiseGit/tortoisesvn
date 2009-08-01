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

#pragma once

#include "IJob.h"
#include "WaitableEvent.h"

namespace async
{

/**
 * Common base implementation of \ref IJob.
 * All your job class deriving from this one
 * has to do is to implement \ref InternalExecute.
 */

class CJobBase : public IJob
{
private:

    // waitable event

    COneShotEvent finished;

    // TRUE until Execute() is called

    volatile LONG waiting;

protected:

    // base class is not intended for creation

    CJobBase(void);

    // implement this in your job class

    virtual void InternalExecute() = 0;

public:

    virtual ~CJobBase(void);

    // call this to put the job into the scheduler

    virtual void Schedule (bool transferOwnership, CJobScheduler* scheduler);

    // will be called by job execution thread

    virtual void Execute();

    // may be called by other (observing) threads

    virtual Status GetStatus() const;
    virtual void WaitUntilDone();
};

}
