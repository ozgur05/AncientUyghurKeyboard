#include "ClassFactory.h"
#include "TextService.h"

#include <new>

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
        *ppv = static_cast<IClassFactory*>(this);
    else { *ppv = nullptr; return E_NOINTERFACE; }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef() { return ++m_cRef; }
STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    LONG c = --m_cRef;
    if (c == 0) delete this;
    return c;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;
    if (pUnkOuter) return CLASS_E_NOAGGREGATION; // no aggregation support

    CTextService* svc = new (std::nothrow) CTextService();
    if (!svc) return E_OUTOFMEMORY;
    HRESULT hr = svc->QueryInterface(riid, ppv);
    svc->Release(); // QI holds the reference on success
    return hr;
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock) DllLock(); else DllUnlock();
    return S_OK;
}
