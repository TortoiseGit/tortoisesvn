/*********************************************************************
CReaderWriterLock: A simple and fast reader-writer lock class in C++
                   has characters of .NET ReaderWriterLock class
Copyright (C) 2006 Quynh Nguyen Huu

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

Email questions, comments or suggestions to quynhnguyenhuu@gmail.com
*********************************************************************/

/*********************************************************************
Introduction:
This implementation is inspired by System.Threading.ReaderWriterLock in
.NET framework. Following are some important statements I excerpted
(with some words modified) from .NET document.

http://msdn.microsoft.com/library/default.asp?url=/library/en-us/cpref/html/frlrfSystemThreadingReaderWriterLockClassTopic.asp

"ReaderWriterLock is used to synchronize access to a resource.
At any given time, it allows either concurrent read access for
multiple threads, or write access for a single thread.
In a situation where a resource is changed infrequently, a
ReaderWriterLock provides better throughput than a simple
one-at-a-time lock, such as CriticalSection or Mutex.

This library works best where most accesses are reads, while
writes are infrequent and of short duration.

While a writer is waiting for active reader locks to be released,
threads requesting new reader locks will have to wait in the reader
queue. Their requests are not granted, even though they could share
concurrent access with existing reader-lock holders; this helps
protect writers against indefinite blockage by readers..."
*********************************************************************/

#pragma once

#include <windows.h>
#include <map>

#if (_WIN32_WINNT >= 0x0403)
//////////////////////////////////////////////////////////////////
// On multiprocessor systems, this value define number of times
// that a thread tries to spin before actually performing a wait
// operation (see InitializeCriticalSectionAndSpinCount API)
#ifndef READER_WRITER_SPIN_COUNT
#define READER_WRITER_SPIN_COUNT 400
#endif READER_WRITER_SPIN_COUNT
#endif _WIN32_WINNT


//////////////////////////////////////////////////////////////////
// CReaderWriterLockNonReentrance class
// NOTE: This class doesn't support reentrance & lock escalation.
// May be deadlock in one of following situations:
//  1) Call AcquireReaderLock twice (reentrance)
//     --> Revise execution flow.
//  2) Call AcquireWriterLock twice (reentrance)
//     --> Revise execution flow.
//  3) Call AcquireReaderLock then AcquireWriterLock (lock escalation)
//     --> Use ReleaseReaderAndAcquireWriterLock method
//  4) Call AcquireWriterLock then AcquireReaderLock (lock deescalation)
//     --> Use DowngradeFromWriterLock method
class CReaderWriterLockNonReentrance
{
public:
    CReaderWriterLockNonReentrance() throw();
    ~CReaderWriterLockNonReentrance() throw();
    void AcquireReaderLock() throw();
    void ReleaseReaderLock() throw();
    void AcquireWriterLock() throw();
    void ReleaseWriterLock() throw();
    bool TryAcquireReaderLock() throw();
    bool TryAcquireWriterLock() throw();

    // When a thread calls ReleaseReaderAndAcquireWriterLock the reader lock is released,
    // and the thread goes to the end of the queue for the writer lock.
    // Thus, other threads might write to the resource before the thread
    // that requested the upgrade is granted the writer lock.
    void ReleaseReaderAndAcquireWriterLock() throw();
    void DowngradeFromWriterLock() throw();
protected:
    // A critical section to guard all the other members
    CRITICAL_SECTION m_cs;
    // Auto-reset event, will be dynamically created/destroyed on demand
    HANDLE m_hSafeToWriteEvent;
    // Manual-reset event, will be dynamically created/destroyed on demand
    HANDLE m_hSafeToReadEvent;
    // Total number of writers on this object
    INT m_iNumOfWriter;
    // Total number of readers have already owned this object
    INT m_iNumOfReaderEntered;
    // Total number of readers are waiting to be owners of this object
    INT m_iNumOfReaderWaiting;
    // Internal/Real implementation
    void _AcquireReaderLockAndLeaveCS() throw();
    void _ReleaseReaderLockAndLeaveCS() throw();
    void _AcquireWriterLockAndLeaveCS(BOOL blMustWait) throw();
    void _ReleaseWriterLockAndLeaveCS(BOOL blDowngrade) throw();
};

//////////////////////////////////////////////////////////////////
// CReaderWriterLock class
// This class supports reentrance & lock escalation
class CReaderWriterLock : private CReaderWriterLockNonReentrance
{
public:
    CReaderWriterLock();
    ~CReaderWriterLock();
    // If current thread was already a reader
    // it will be upgraded to be writer automatically.
    // BE CAREFUL! Other threads might write to the resource
    // before current thread is successfully upgraded.
    void AcquireWriterLock() throw();
    void ReleaseWriterLock() throw();
    void AcquireReaderLock() throw();
    void ReleaseReaderLock() throw();

    bool TryAcquireReaderLock() throw();
    bool TryAcquireWriterLock() throw();

    // Regardless of how many times current thread acquired reader
    // or writer locks, a call to this method will release all locks.
    // After that, any call to ReleaseWriterLock or ReleaseReaderLock
    // will raise exception in DEBUG mode.
    void ReleaseAllLocks() throw();
    void GetCurrentThreadStatus(DWORD* lpdwReaderLockCounter,
        DWORD* lpdwWriterLockCounter) const throw();
protected:
    typedef std::map<DWORD,DWORD> CMapThreadToState;
    CMapThreadToState m_map;
};

//////////////////////////////////////////////////////////////////
// CAutoReadLockT & CAutoWriteLockT classes
// Couple of template helper classes which would let one acquire a lock
// in a body of code and not have to worry about explicitly releasing
// that lock if an exception is encountered in that piece of code or
// if there are multiple return points out of that piece.

template<typename T>
class CAutoReadLockT
{
public:
    CAutoReadLockT(T& objLock) throw() : m_lock(objLock)
    {
        m_lock.AcquireReaderLock();
    }
    ~CAutoReadLockT() throw()
    {
        m_lock.ReleaseReaderLock();
    }
protected:
    T& m_lock;
};

template<typename T>
class CAutoWriteLockT
{
public :
    CAutoWriteLockT(T& objLock) throw() : m_lock(objLock)
    {
        m_lock.AcquireWriterLock();
    }
    ~CAutoWriteLockT() throw()
    {
        m_lock.ReleaseWriterLock();
    }
protected:
    T& m_lock;
};

template<typename T>
class CAutoReadWeakLockT
{
public:
    CAutoReadWeakLockT(T& objLock) throw() : m_lock(objLock)
    {
        isAcquired = m_lock.TryAcquireReaderLock();
    }
    ~CAutoReadWeakLockT() throw()
    {
        if (isAcquired)
            m_lock.ReleaseReaderLock();
    }
    bool IsAcquired() const
    {
        return isAcquired;
    }
protected:
    T& m_lock;
    bool isAcquired;
};

template<typename T>
class CAutoWriteWeakLockT
{
public :
    CAutoWriteWeakLockT(T& objLock) throw() : m_lock(objLock)
    {
        isAcquired = m_lock.TryAcquireWriterLock();
    }
    ~CAutoWriteWeakLockT() throw()
    {
        release();
    }
    void Release()
    {
        release();
    }
    bool IsAcquired() const
    {
        return isAcquired;
    }
protected:
    T& m_lock;
    bool isAcquired;

    void release()
    {
        if (isAcquired)
        {
            m_lock.ReleaseWriterLock();
            isAcquired = false;
        }
    }
};

//////////////////////////////////////////////////////////////////
// Instances of above template helper classes

typedef CAutoReadLockT<CReaderWriterLock> CAutoReadLock;
typedef CAutoWriteLockT<CReaderWriterLock> CAutoWriteLock;
typedef CAutoReadWeakLockT<CReaderWriterLock> CAutoReadWeakLock;
typedef CAutoWriteWeakLockT<CReaderWriterLock> CAutoWriteWeakLock;
