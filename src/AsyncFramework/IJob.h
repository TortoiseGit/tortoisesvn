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

// forward declaration

class CJobScheduler;

/**
 * Common interface to all job objects that can be handled by
 * \ref CJobScheduler instances.
 *
 * Jobs will be executed only once making the \ref done state
 * the final state of the internal state machine.
 */

class IJob
{
public:

    /**
     * Possible values returned by \ref GetStatus().
     * The state machine linearly follows the value order.
     */

    enum Status
    {
        waiting = 0,
        running = 1,
        suspended = 2,
        done = 3
    };

    /// destruction

    virtual ~IJob(void) {}

    /// call this to put the job into the scheduler.
    /// If \ref transferOwnership is set, the scheduler must
    /// delete this instance after exection.

    virtual void Schedule ( bool transferOwnership
                          , CJobScheduler* scheduler) = 0;

    /// will be called by job execution thread

    virtual void Execute() = 0;

    /// may be called by other (observing) threads.
    /// Please note that the information returned may already
    /// be outdated as soon as the function returns.

    virtual Status GetStatus() const = 0;

    /// Efficiently wait for instance to reach the 
    /// \ref done \ref Status.

    virtual void WaitUntilDone() = 0;
};
