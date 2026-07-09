#include "Register.h"
#include "Globals.h"

#include <objbase.h>
#include <cstring>
#include <cstdio>
#include <cwchar>

namespace {

// Format a GUID as "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}".
void GuidToString(REFGUID g, wchar_t* buf, size_t cch)
{
    StringFromGUID2(g, buf, static_cast<int>(cch));
}

bool SetRegValue(HKEY root, const wchar_t* subkey, const wchar_t* name, const wchar_t* value)
{
    HKEY key = nullptr;
    if (RegCreateKeyExW(root, subkey, 0, nullptr, REG_OPTION_NON_VOLATILE,
                        KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        return false;
    LONG r = RegSetValueExW(key, name, 0, REG_SZ,
                            reinterpret_cast<const BYTE*>(value),
                            static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    return r == ERROR_SUCCESS;
}

} // namespace

HRESULT RegisterServer()
{
    wchar_t clsid[64];
    GuidToString(CLSID_AukTextService, clsid, 64);

    wchar_t dllPath[MAX_PATH] = {0};
    if (GetModuleFileNameW(g_hInst, dllPath, MAX_PATH) == 0)
        return E_FAIL;

    wchar_t clsidKey[128];
    swprintf_s(clsidKey, L"CLSID\\%s", clsid);
    if (!SetRegValue(HKEY_CLASSES_ROOT, clsidKey, nullptr, kTextServiceDesc))
        return E_FAIL;

    wchar_t inprocKey[160];
    swprintf_s(inprocKey, L"CLSID\\%s\\InprocServer32", clsid);
    if (!SetRegValue(HKEY_CLASSES_ROOT, inprocKey, nullptr, dllPath)) return E_FAIL;
    if (!SetRegValue(HKEY_CLASSES_ROOT, inprocKey, L"ThreadingModel", L"Apartment")) return E_FAIL;
    return S_OK;
}

void UnregisterServer()
{
    wchar_t clsid[64];
    GuidToString(CLSID_AukTextService, clsid, 64);
    wchar_t clsidKey[128];
    swprintf_s(clsidKey, L"CLSID\\%s", clsid);
    RegDeleteTreeW(HKEY_CLASSES_ROOT, clsidKey);
}

HRESULT RegisterProfiles()
{
    ITfInputProcessorProfiles* profiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
        CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles,
        reinterpret_cast<void**>(&profiles));
    if (FAILED(hr) || !profiles) return hr;

    hr = profiles->Register(CLSID_AukTextService);
    if (SUCCEEDED(hr)) {
        hr = profiles->AddLanguageProfile(CLSID_AukTextService, kProfileLangId, GUID_AukProfile,
            kTextServiceDesc, static_cast<ULONG>(wcslen(kTextServiceDesc)),
            nullptr, 0, 0);
    }
    profiles->Release();
    return hr;
}

void UnregisterProfiles()
{
    ITfInputProcessorProfiles* profiles = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
            CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles,
            reinterpret_cast<void**>(&profiles))) && profiles) {
        profiles->Unregister(CLSID_AukTextService);
        profiles->Release();
    }
}

HRESULT RegisterCategories()
{
    ITfCategoryMgr* cat = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
        IID_ITfCategoryMgr, reinterpret_cast<void**>(&cat));
    if (FAILED(hr) || !cat) return hr;
    // Declare this TIP as a keyboard input method.
    hr = cat->RegisterCategory(CLSID_AukTextService, GUID_TFCAT_TIP_KEYBOARD, CLSID_AukTextService);
    cat->Release();
    return hr;
}

void UnregisterCategories()
{
    ITfCategoryMgr* cat = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
            IID_ITfCategoryMgr, reinterpret_cast<void**>(&cat))) && cat) {
        cat->UnregisterCategory(CLSID_AukTextService, GUID_TFCAT_TIP_KEYBOARD, CLSID_AukTextService);
        cat->Release();
    }
}
