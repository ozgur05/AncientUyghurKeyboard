// ClassFactory.h — IClassFactory that manufactures the text service.
#pragma once

#include "Globals.h"

class CClassFactory final : public IClassFactory {
public:
    CClassFactory() : m_cRef(1) { DllAddRef(); }

    STDMETHODIMP         QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) override;
    STDMETHODIMP LockServer(BOOL fLock) override;

private:
    ~CClassFactory() { DllRelease(); }
    LONG m_cRef;
};
