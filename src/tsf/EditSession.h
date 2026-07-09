// EditSession.h — applies a composed edit to the focused document.
//
// TSF requires document changes to happen inside an edit session with an edit
// cookie. This session receives a core::EmitOp (Backspaces + text to insert)
// and applies it: delete the preceding characters, then insert the new text at
// the selection. Inserts are committed directly (no persistent composition),
// which is a valid, simple model for a deterministic mapping IME.
#pragma once

#include "Globals.h"
#include "../core/Composer.hpp"

class CEditSession final : public ITfEditSession {
public:
    CEditSession(ITfContext* pContext, const core::EmitOp& op);

    STDMETHODIMP         QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfEditSession: perform the edit with the granted cookie.
    STDMETHODIMP DoEditSession(TfEditCookie ec) override;

private:
    ~CEditSession();

    LONG          m_cRef;
    ITfContext*   m_pContext;
    core::EmitOp  m_op;
};
