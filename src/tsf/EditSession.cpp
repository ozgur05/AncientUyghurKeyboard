#include "EditSession.h"
#include "../core/Unicode.hpp"

CEditSession::CEditSession(ITfContext* pContext, const core::EmitOp& op)
    : m_cRef(1), m_pContext(pContext), m_op(op)
{
    if (m_pContext) m_pContext->AddRef();
    DllAddRef();
}

CEditSession::~CEditSession()
{
    if (m_pContext) m_pContext->Release();
    DllRelease();
}

STDMETHODIMP CEditSession::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession))
        *ppv = static_cast<ITfEditSession*>(this);
    else { *ppv = nullptr; return E_NOINTERFACE; }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CEditSession::AddRef()  { return ++m_cRef; }
STDMETHODIMP_(ULONG) CEditSession::Release()
{
    LONG c = --m_cRef;
    if (c == 0) delete this;
    return c;
}

STDMETHODIMP CEditSession::DoEditSession(TfEditCookie ec)
{
    if (!m_pContext) return E_FAIL;

    // Current selection (the insertion point / selected range).
    TF_SELECTION sel = {};
    ULONG fetched = 0;
    if (FAILED(m_pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &sel, &fetched)) || fetched == 0)
        return S_OK; // nothing to act on
    ITfRange* range = sel.range; // owned; released below

    // 1. Delete the preceding characters requested by the reconciliation model.
    if (m_op.backspaces > 0) {
        ITfRange* del = nullptr;
        if (SUCCEEDED(range->Clone(&del)) && del) {
            LONG shifted = 0;
            del->ShiftStart(ec, -m_op.backspaces, &shifted, nullptr); // extend backward
            del->SetText(ec, 0, L"", 0);                              // remove them
            del->Release();
        }
    }

    // 2. Insert the new text at the selection.
    if (!m_op.insert.empty()) {
        std::u16string u16 = core::unicode::utf32ToUtf16(m_op.insert);
        ITfInsertAtSelection* insert = nullptr;
        if (SUCCEEDED(m_pContext->QueryInterface(IID_ITfInsertAtSelection,
                                                 reinterpret_cast<void**>(&insert))) && insert) {
            ITfRange* inserted = nullptr;
            insert->InsertTextAtSelection(ec, TF_IAS_NOQUERY,
                reinterpret_cast<const WCHAR*>(u16.c_str()),
                static_cast<LONG>(u16.size()), &inserted);
            if (inserted) inserted->Release();
            insert->Release();
        }
    }

    range->Release();
    return S_OK;
}
