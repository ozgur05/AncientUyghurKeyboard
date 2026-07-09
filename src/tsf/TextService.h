// TextService.h — the TSF text input processor.
//
// Implements the standard text-service interface set:
//   ITfTextInputProcessor   (activation),
//   ITfThreadMgrEventSink    (focus / document lifecycle),
//   ITfKeyEventSink          (keystroke interception),
//   ITfCompositionSink       (composition termination).
// Keystrokes are translated into core::KeyInput and run through the shared,
// unit-tested core::Composer; the resulting edits are applied to the focused
// document through a TSF edit session (see EditSession.*).
//
// We implement the base ITfTextInputProcessor (activation via Activate) rather
// than the Ex variant: the base interface is present on every SDK (including
// MinGW's msctf.h) and is fully sufficient — ActivateEx only adds optional
// activation flags we do not need.
#pragma once

#include "Globals.h"
#include "../core/KeyboardLayoutManager.hpp"
#include "../core/Composer.hpp"

class CTextService final : public ITfTextInputProcessor,
                           public ITfThreadMgrEventSink,
                           public ITfKeyEventSink,
                           public ITfCompositionSink {
public:
    CTextService();

    // IUnknown
    STDMETHODIMP          QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG)  AddRef() override;
    STDMETHODIMP_(ULONG)  Release() override;

    // ITfTextInputProcessor
    STDMETHODIMP Activate(ITfThreadMgr* ptim, TfClientId tid) override;
    STDMETHODIMP Deactivate() override;

    // ITfThreadMgrEventSink
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr*) override;
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr*) override;
    STDMETHODIMP OnSetFocus(ITfDocumentMgr* pdimFocus, ITfDocumentMgr* pdimPrev) override;
    STDMETHODIMP OnPushContext(ITfContext*) override;
    STDMETHODIMP OnPopContext(ITfContext*) override;

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL fForeground) override;
    STDMETHODIMP OnTestKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnPreservedKey(ITfContext* pic, REFGUID rguid, BOOL* pfEaten) override;

    // ITfCompositionSink
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition* pComposition) override;

    // Called by the edit session to reach the client id.
    TfClientId ClientId() const { return m_clientId; }

private:
    ~CTextService();

    bool InitKeyEventSink();
    void UninitKeyEventSink();
    bool InitThreadMgrEventSink();
    void UninitThreadMgrEventSink();
    void LoadLayout();

    // Shared handler for real + test key-down. When `apply` is false it only
    // predicts whether the key would be eaten (used by OnTestKeyDown) and does
    // not mutate composer state or the document.
    bool HandleKeyDown(ITfContext* pic, WPARAM wParam, bool apply);
    static bool IsModifierVk(unsigned vk);

    LONG              m_cRef;
    ITfThreadMgr*     m_pThreadMgr = nullptr;
    TfClientId        m_clientId   = TF_CLIENTID_NULL;
    DWORD             m_threadMgrCookie = TF_INVALID_COOKIE;

    core::KeyboardLayoutManager m_manager;
    core::Composer              m_composer;
    bool                        m_haveLayout = false;
};
