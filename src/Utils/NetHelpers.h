// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif


/**
 * \ingroup CommonClasses
 * \return the number of adapters currently active on the system with a valid ip address (other than localhost)
 */
int GetNumberOfIPAdapters();
/**
 * \ingroup CommonClasses
 * returns the ip number of the given adapter. if the param is set to 0 (or ommited) the first found ip address is returned.
 * if the adapternumber is higher than the available ip adapters, an empty string is returned.
 * \remark on most systems only one adapter with a valid ip address is available. So calling this function without parameter
 * will return the local ip address.
 * \return the ip address
 */
CString GetIPNumber(int adapternumber = 0);
/**
 * \ingroup CommonClasses
 * returns the ip netmask of the given adapter.  if the param is set to 0 (or ommited) the first found ip netmask is returned.
 * if the adapternumber is higher than the available ip adapters, an empty string is returned.
 * \remark on most systems only one adapter with a valid ip address is available. So calling this function without parameter
 * will return the local ip netmask.
 * \return the ip netmask
 */
CString GetIPMask(int adapternumber = 0);
/**
 * \ingroup CommonClasses
 * returns the ip gateway of the given adapter.  if the param is set to 0 (or ommited) the first found ip gateway is returned.
 * if the adapternumber is higher than the available ip adapters, an empty string is returned.
 * \remark on most systems only one adapter with a valid ip address is available. So calling this function without parameter
 * will return the local ip gateway.
 * \return the ip gateway
 */
CString GetIPGateway(int adapternumber = 0);
/**
 * \ingroup CommonClasses
 * returns the mac address of the given adapter in the format "xx xx xx xx xx xx".  if the param is set to 0 (or ommited) the first found mac address is returned.
 * if the adapternumber is higher than the available ip adapters, an empty string is returned.
 * \remark on most systems only one adapter with a valid ip address is available. So calling this function without parameter
 * will return the local mac address.
 * \return the mac address
 */
CString GetMACAddress(int adapternumber = 0);

/**
 * \ingroup CommonClasses
 * Converts a string containing an ip address to an S_addr. If the string is already a valid ip then it's simply converted. If the string is a 
 * domain name then it is looked up via DNS and then converted.
 * \param ipstring an ip address in the form "a.b.c.d" or "www.domain.com"
 * \return the converted ip address or NULL if an error occured. Use WSAGetLastError() for error information.
 */   
ULONG GetIPFromString(LPCTSTR ipstring);

/**
 * Returns the login name of the current user. If not available an empty string is returned.
 */	 	 		
static CString GetUserName();

/**
 * Returns the domain name the current user is logged on. If not available an empty string is returned.
 */	 	 		
static CString GetDomainName();
