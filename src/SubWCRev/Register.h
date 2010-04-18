
#pragma once

//
// Registry.h
//   - Helper functions registering and unregistering a component.
//

// This function will register a component in the Registry.
// The component calls this function from its DllRegisterServer function.
HRESULT RegisterServer(HMODULE hModule,
                       const CLSID& clsid,
                       const TCHAR* szFriendlyName,
                       const TCHAR* szVerIndProgID,
                       const TCHAR* szProgID,
                       const CLSID& libid) ;

// This function will unregister a component.  Components
// call this function from their DllUnregisterServer function.
HRESULT UnregisterServer(const CLSID& clsid,
                         const TCHAR* szVerIndProgID,
                         const TCHAR* szProgID,
                         const CLSID& libid) ;


void RegisterInterface(HMODULE hModule,            // DLL module handle
                       const CLSID& clsid,         // Class ID
                       const TCHAR* szFriendlyName, // Friendly Name
                       const CLSID &libid,
                       const IID &iid);
void UnregisterInterface(const IID &iid);


HRESULT RegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex);
HRESULT UnRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex);

