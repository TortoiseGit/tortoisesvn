// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008, 2011-2014 - TortoiseSVN

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
#include "StringUtils.h"

class Creds
{
public:
    std::unique_ptr<char[]> GetUsername() { return CStringUtils::Decrypt(username); }
    bool                    SetUsername(const char * user) { username = CStringUtils::Encrypt(user); return !username.IsEmpty(); }
    std::unique_ptr<char[]> GetPassword() { return CStringUtils::Decrypt(password); }
    bool                    SetPassword(const char * pass) { password = CStringUtils::Encrypt(pass); return !password.IsEmpty(); }
private:
    CStringA                username;
    CStringA                password;
};


extern std::map<CStringA, Creds> tsvn_creds;

void svn_auth_get_tsvn_simple_provider(svn_auth_provider_object_t **provider,
                                       apr_pool_t *pool);

