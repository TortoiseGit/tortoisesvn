// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011 - TortoiseSVN

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
#include "StdStringIntMapFile.h"

#define STDSTRINGINTMAPFILE_VERSION 1

CStdStringIntMapFile::CStdStringIntMapFile(void)
{
}


CStdStringIntMapFile::~CStdStringIntMapFile(void)
{
}

bool CStdStringIntMapFile::Save( LPCWSTR path )
{
    FILE * stream;
    errno_t err = 0;

    if ((err = _wfopen_s(&stream, path, L"wbS"))==0)
    {
        const int version = STDSTRINGINTMAPFILE_VERSION;
        fwrite(&version, sizeof(int), 1, stream);                                   // version
        int len = static_cast<int>(size());
        fwrite(&len, sizeof(int), 1, stream);                                       // number of entries
        for (auto it = cbegin(); it != cend(); ++it)
        {
            len = static_cast<int>(it->first.size()*sizeof(WCHAR));
            fwrite(&len, sizeof(int), 1, stream);                                   // length of name in bytes
            fwrite(it->first.c_str(), sizeof(WCHAR), it->first.size(), stream);                 // key name
            len = sizeof(int);
            fwrite(&len, sizeof(int), 1, stream);                                   // length of value in bytes
            fwrite(&it->second, sizeof(int), 1, stream);                            // value
        }

        fclose(stream);
        return true;
    }
    return false;
}

bool CStdStringIntMapFile::Load( LPCWSTR path )
{
    FILE * stream;
    _wfopen_s(&stream, path, L"rbS");
    if (stream == NULL)
        return false;
    int version = 0;
    bool bRet = true;
    if (fread(&version, sizeof(int), 1, stream) == 1)
    {
        if (version == STDSTRINGINTMAPFILE_VERSION)
        {
            int entries = 0;
            if (fread(&entries, sizeof(int), 1, stream) == 1)
            {
                while ((entries-- > 0))
                {
                    int nNameBytes = 0;
                    if (fread(&nNameBytes, sizeof(int), 1, stream) == 1)
                    {
                        if ((nNameBytes < 0)||(nNameBytes > 4096))
                        {
                            bRet = false;
                            break;
                        }

                        auto_buffer<WCHAR> pNameBuf(nNameBytes/sizeof(WCHAR));
                        if (fread(pNameBuf, 1, nNameBytes, stream) == (size_t)nNameBytes)
                        {
                            std::wstring sUName = std::wstring (pNameBuf, nNameBytes/sizeof(WCHAR));
                            int nValueBytes = 0;
                            if (fread(&nValueBytes, sizeof(int), 1, stream) == 1)
                            {
                                if (nValueBytes != sizeof(int))
                                {
                                    bRet = false;
                                    break;
                                }

                                int value = 0;
                                if (fread(&value, sizeof(int), 1, stream) == 1)
                                {
                                    (*this)[sUName] = value;
                                }
                                else
                                {
                                    bRet = false;
                                    break;
                                }
                            }
                            else
                            {
                                bRet = false;
                                break;
                            }
                        }
                        else
                        {
                            bRet = false;
                            break;
                        }
                    }
                    else
                    {
                        bRet = false;
                        break;
                    }
                }
                fclose(stream);
            }
        }
    }
    return bRet;
}
