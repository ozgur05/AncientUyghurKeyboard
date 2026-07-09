// Register.h — (un)registration of the text-service COM server and TSF profile.
#pragma once

#include <windows.h>

// Register/unregister the in-proc COM server (HKCR\CLSID\...\InprocServer32).
HRESULT RegisterServer();
void    UnregisterServer();

// Register/unregister the TSF language profile + keyboard category.
HRESULT RegisterProfiles();
void    UnregisterProfiles();
HRESULT RegisterCategories();
void    UnregisterCategories();
