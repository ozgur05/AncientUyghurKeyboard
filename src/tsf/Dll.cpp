// Dll.cpp — COM in-proc server entry points for the TSF text service.
#include "Globals.h"
#include "ClassFactory.h"
#include "Register.h"

#include <new>

HINSTANCE g_hInst  = nullptr;
LONG      g_cRefDll = 0;
LONG      g_cLocks  = 0;

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            g_hInst = hInst;
            DisableThreadLibraryCalls(hInst);
            break;
        default:
            break;
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;
    if (!IsEqualCLSID(rclsid, CLSID_AukTextService))
        return CLASS_E_CLASSNOTAVAILABLE;

    CClassFactory* factory = new (std::nothrow) CClassFactory();
    if (!factory) return E_OUTOFMEMORY;
    HRESULT hr = factory->QueryInterface(riid, ppv);
    factory->Release();
    return hr;
}

STDAPI DllCanUnloadNow()
{
    return (g_cRefDll == 0 && g_cLocks == 0) ? S_OK : S_FALSE;
}

STDAPI DllUnregisterServer()
{
    UnregisterCategories();
    UnregisterProfiles();
    UnregisterServer();
    return S_OK;
}

STDAPI DllRegisterServer()
{
    HRESULT hr = RegisterServer();
    if (SUCCEEDED(hr)) hr = RegisterProfiles();
    if (SUCCEEDED(hr)) hr = RegisterCategories();
    if (FAILED(hr)) DllUnregisterServer(); // roll back partial registration
    return hr;
}
