// Guids.h — declarations of our COM CLSID and TSF language-profile GUID.
// Definitions live in Guids.cpp (compiled with INITGUID so the symbols and the
// TSF interface GUIDs are instantiated exactly once).
#pragma once

#include <guiddef.h>

EXTERN_C const CLSID CLSID_AukTextService;   // COM class id of the text service
EXTERN_C const GUID  GUID_AukProfile;        // TSF input-method profile id
