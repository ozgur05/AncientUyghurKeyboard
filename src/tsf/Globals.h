// Globals.h — module-wide state for the TSF text-service DLL.
//
// A TSF text input processor is an in-process COM server. This header holds the
// module instance handle and the two lifetime counters COM requires
// (object ref count + LockServer count); the DLL stays loaded while either is
// non-zero. The text service reuses the project's tested core::Composer for all
// composition logic, so this layer is purely COM/TSF plumbing.
#pragma once

#include <windows.h>
#include <msctf.h>

// Our text service CLSID + language profile GUID (defined in Guids.cpp).
#include "Guids.h"

// Some SDK headers (notably older MinGW msctf.h) omit this sentinel; it is
// simply the null client-id value. Define it portably if absent.
#ifndef TF_CLIENTID_NULL
#define TF_CLIENTID_NULL ((TfClientId)0)
#endif

extern HINSTANCE g_hInst;     // set in DllMain
extern LONG      g_cRefDll;   // live COM object count
extern LONG      g_cLocks;    // IClassFactory::LockServer count

inline void DllAddRef()  { InterlockedIncrement(&g_cRefDll); }
inline void DllRelease() { InterlockedDecrement(&g_cRefDll); }
inline void DllLock()    { InterlockedIncrement(&g_cLocks); }
inline void DllUnlock()  { InterlockedDecrement(&g_cLocks); }

// Human-readable description shown in the language bar / settings.
constexpr wchar_t kTextServiceDesc[] = L"Ancient Uyghur (TSF)";

// Language profile id. Old Uyghur is not a standard input language; like most
// custom TIPs we register under en-US and let the user select the profile.
constexpr LANGID kProfileLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
