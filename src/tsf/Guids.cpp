// Guids.cpp — defines *only* this project's own GUIDs.
//
// We deliberately do NOT INITGUID the system/TSF headers here: on MSVC that
// would define the TSF and standard GUIDs which uuid.lib also provides,
// producing duplicate-symbol (LNK2005) errors. Instead we define just our two
// identifiers, and the system GUIDs (IID_IUnknown, CLSID_TF_*,
// GUID_TFCAT_TIP_KEYBOARD, FOLDERID_*) come from uuid (uuid.lib on MSVC,
// -luuid on MinGW).
#include <initguid.h>   // makes the DEFINE_GUID below allocate storage
#include <guiddef.h>
#include "Guids.h"

// CLSID_AukTextService = {9A3B2C1D-4E5F-4A6B-8C7D-0E1F2A3B4C5D}
DEFINE_GUID(CLSID_AukTextService,
    0x9a3b2c1d, 0x4e5f, 0x4a6b, 0x8c, 0x7d, 0x0e, 0x1f, 0x2a, 0x3b, 0x4c, 0x5d);
// GUID_AukProfile      = {1B2C3D4E-5F6A-4B7C-9D8E-1F2A3B4C5D6E}
DEFINE_GUID(GUID_AukProfile,
    0x1b2c3d4e, 0x5f6a, 0x4b7c, 0x9d, 0x8e, 0x1f, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e);
