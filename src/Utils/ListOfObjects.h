// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2012 - TortoiseSVN

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
#pragma once
//#include <algorithm>

#include "EOL.h"

/// Object array - Iterator based solution
/**
    A template class to make an array which looks like a CStringArray or CDWORDArray but
    is faster at large sizes
    Only used methods are implemented
*/
template <typename T> class CListOfObjectsI
{
public:
    CListOfObjectsI() {
        Init();
    }
    ~CListOfObjectsI() {
        RemoveAll();
        delete [] m_tArray;
    }

    CListOfObjectsI(const CListOfObjectsI & src) {
        Init();
        Copy(src); 
    }
    CListOfObjectsI & operator =(const CListOfObjectsI & src) {
        Init();
        Copy(src);
        return *this;
    }

    int GetCount() const { 
        return (m_tLast-m_tArray); 
    }
    const T& GetAt(int index) const {
        T ** tElement = m_tArray+index;

        ASSERT(tElement>=m_tArray && tElement<m_tLast);

        return **(tElement); 
    }
    void RemoveAt(int index)    {
        ASSERT(0); // not implemented/verified

        T ** tElement = m_tArray+index;

        ASSERT(tElement>=m_tArray && tElement<m_tLast);

        delete *tElement;
        memmove(tElement, tElement+1, (m_tLast-tElement)*sizeof(T*));
    }
    void InsertAt(int index, const T& /*strVal*/)   {
        ASSERT(0); // not implemented/verified

        T ** tElement = m_tArray+index;

        ASSERT(tElement>=m_tArray && tElement<m_tLast);
/*        ASSERT(index>=0 && index<m_nUsed);

        if (m_nAlocated<=m_nUsed)
        {
            // todo expand here to remove one copy
            calculate new length
            create new array
            copy 0-index-1
            add new on index
            copy index+1-count-1
            delete onl array
        }
        else
        {
            memmove(tElement+1, tElement, (m_nUsed-index)*sizeof(T *));
            m_array[index] = new T(strVal);
            ++m_tTop;
        }//*/
    }
    //void InsertAt(int index, const T& strVal, int nCopies)  { m_vec.insert(m_vec.begin()+index, nCopies, strVal); }
    //void SetAt(int index, const T& strVal)  { m_vec[index] = strVal; }
    void Add(const T& strVal)
    {
        if (m_tLast>=m_tTop)
            Expand();
        *m_tLast++ = new T(strVal);
    }
    void RemoveAll()
    {
        for (T ** p = m_tLast; p!=m_tArray; )
            delete *--p;
        m_tLast = m_tArray;
    }
    void Reserve(int lengthHint)    {
        int nSizePowerTwo = std::max<int>(lengthHint, GetCount());
        nSizePowerTwo |= nSizePowerTwo>>1;
        nSizePowerTwo |= nSizePowerTwo>>2;
        nSizePowerTwo |= nSizePowerTwo>>4;
        nSizePowerTwo |= nSizePowerTwo>>8;
        nSizePowerTwo |= nSizePowerTwo>>16;
        nSizePowerTwo++;
        ExpandTo(nSizePowerTwo);
    }

private:
    void Copy(const CListOfObjectsI & /*src*/)
    {
//        PROFILE_BLOCK;

/*        RemoveAll();
        Reserve(src.GetCount());
        m_nUsed = src.m_nUsed;
        for (int i=0; i<m_nUsed; i++)
        {
            // Add()
            m_array[i] = new T(*src.m_array[i]);
        }//*/
    }
    void Init()
    {
        m_tArray = NULL;
        m_tLast = NULL;
        m_tTop = NULL;
    }

    void Expand()
    {
//        PROFILE_BLOCK;
        ExpandTo(GetCount() ? GetCount()*2 : 256);
    }

    void ExpandTo(int nNewSize)
    {
        T ** pNewArray = new T *[nNewSize];
//        memcpy(pNewArray, m_tArray, (m_tLast-m_tArray)*sizeof(T*));
        memcpy(pNewArray, m_tArray, ((BYTE *)m_tLast-(BYTE *)m_tArray));
        m_tLast += pNewArray - m_tArray;
        m_tTop = pNewArray + nNewSize;
        delete [] m_tArray;
        m_tArray = pNewArray;
    }

    T ** m_tArray; // first
    T ** m_tLast; // pointer to push
    T ** m_tTop; // pointer to element after array
};

// we are fine with simple solution but it have to be FAST ...
// note this is non validating solution !
// index based solution
template <typename T> class CListOfObjectsN
{
public:
    CListOfObjectsN() {
        Init();
    }
    ~CListOfObjectsN() {
        RemoveAll();
        delete [] m_array;
    }

    CListOfObjectsN(const CListOfObjectsN & src) {
        PROFILE_BLOCK;
        Init();
        Copy(src); 
    }
    CListOfObjectsN & operator =(const CListOfObjectsN & src) {
        PROFILE_BLOCK;
        Init();
        Copy(src);
        return *this;
    }

    int GetCount() const { 
        return m_nUsed; 
    }
    const T& GetAt(int index) const { 
        ASSERT(index>=0 && index<m_nUsed);

        return *(m_array[index]); 
    }
    void RemoveAt(int index)    {
        ASSERT(index>=0 && index<m_nUsed);

        delete m_array[index];
        memmove(&m_array[index], &m_array[index+1], (m_nUsed-index)*sizeof(T *));
        --m_nUsed;
    }
    void InsertAt(int index, const T& strVal)   {
        ASSERT(index>=0 && index<m_nUsed);

        if (m_nAlocated<=m_nUsed)
            // todo expand here to remove one copy
            Expand();
        memmove(&m_array[index+1], &m_array[index], (m_nUsed-index)*sizeof(T *));
        m_array[index] = new T(strVal);
        ++m_nUsed;
    }
    //void InsertAt(int index, const T& strVal, int nCopies)  { m_vec.insert(m_vec.begin()+index, nCopies, strVal); }
    //void SetAt(int index, const T& strVal)  { m_vec[index] = strVal; }
    void Add(const T& strVal)
    {
        if (m_nAlocated<=m_nUsed)
            Expand();
        m_array[m_nUsed++] = new T(strVal);
    }
    void RemoveAll()
    {
        PROFILE_BLOCK;

        for (int i=0; i<m_nUsed; i++)
        {
            delete m_array[i];
        }
        m_nUsed = 0;
    }
    void Reserve(int lengthHint)    {
        int nSizePowerTwo = std::max<int>(lengthHint, GetCount());
        nSizePowerTwo |= nSizePowerTwo>>1;
        nSizePowerTwo |= nSizePowerTwo>>2;
        nSizePowerTwo |= nSizePowerTwo>>4;
        nSizePowerTwo |= nSizePowerTwo>>8;
        nSizePowerTwo |= nSizePowerTwo>>16;
        nSizePowerTwo++;
        Expand(nSizePowerTwo);
    }

private:
    void Copy(const CListOfObjectsN & src)
    {
        RemoveAll();
        Reserve(src.m_nUsed);
        m_nUsed = src.m_nUsed;
        for (int i=0; i<m_nUsed; i++)
        {
            // Add()
            m_array[i] = new T(*src.m_array[i]);
        }
    }
    void Init()
    {
        m_array = NULL;
        m_nAlocated = 0;
        m_nUsed = 0;
    }

    void Expand()
    {
        Expand(m_nAlocated ? m_nAlocated*2 : 256);
    }

    void Expand(int nNewSize)
    {
        m_nAlocated = nNewSize;
        T ** pNewArray = new T *[nNewSize];
        memcpy(pNewArray, m_array, m_nUsed*sizeof(T *));
        delete [] m_array;
        m_array = pNewArray;
    }

    int m_nUsed;
    int m_nAlocated;
    T ** m_array;
};


//typedef CListOfObjectsI CListOfObjects;
#define CListOfObjects CListOfObjectsI

