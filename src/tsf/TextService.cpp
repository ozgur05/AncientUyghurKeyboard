#include "TextService.h"
#include "EditSession.h"
#include "../core/VirtualKeys.hpp"

#include <shlobj.h>
#include <string>

namespace {
// The user layouts directory (same location the keyboard app writes/reads).
std::string UserLayoutsDirUtf8()
{
    wchar_t* raw = nullptr;
    std::wstring dir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &raw))) {
        dir = raw;
        CoTaskMemFree(raw);
    }
    dir += L"\\AncientUyghurKeyboard\\layouts";
    int n = WideCharToMultiByte(CP_UTF8, 0, dir.c_str(), (int)dir.size(), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, dir.c_str(), (int)dir.size(), s.data(), n, nullptr, nullptr);
    return s;
}
} // namespace

CTextService::CTextService() : m_cRef(1) { DllAddRef(); }
CTextService::~CTextService() { DllRelease(); }

// ---- IUnknown ---------------------------------------------------------------

STDMETHODIMP CTextService::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextInputProcessor))
        *ppv = static_cast<ITfTextInputProcessor*>(this);
    else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink))
        *ppv = static_cast<ITfThreadMgrEventSink*>(this);
    else if (IsEqualIID(riid, IID_ITfKeyEventSink))
        *ppv = static_cast<ITfKeyEventSink*>(this);
    else if (IsEqualIID(riid, IID_ITfCompositionSink))
        *ppv = static_cast<ITfCompositionSink*>(this);
    else { *ppv = nullptr; return E_NOINTERFACE; }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CTextService::AddRef() { return ++m_cRef; }
STDMETHODIMP_(ULONG) CTextService::Release()
{
    LONG c = --m_cRef;
    if (c == 0) delete this;
    return c;
}

// ---- Activation -------------------------------------------------------------

STDMETHODIMP CTextService::Activate(ITfThreadMgr* ptim, TfClientId tid)
{
    m_pThreadMgr = ptim;
    m_pThreadMgr->AddRef();
    m_clientId = tid;

    LoadLayout();
    if (!InitThreadMgrEventSink()) { Deactivate(); return E_FAIL; }
    if (!InitKeyEventSink())       { Deactivate(); return E_FAIL; }
    return S_OK;
}

STDMETHODIMP CTextService::Deactivate()
{
    UninitKeyEventSink();
    UninitThreadMgrEventSink();
    if (m_pThreadMgr) { m_pThreadMgr->Release(); m_pThreadMgr = nullptr; }
    m_clientId = TF_CLIENTID_NULL;
    return S_OK;
}

void CTextService::LoadLayout()
{
    // Re-point the composer whenever the manager activates a layout.
    m_manager.setOnChange([this](const core::KeyboardLayout* l) {
        m_composer.setLayout(l);
    });
    std::string err;
    m_haveLayout = m_manager.initialize(UserLayoutsDirUtf8(), "old_uyghur", &err);
    // If no layout is available the composer stays empty and every key passes
    // through untouched — exactly like the hook backend with no layout.
}

bool CTextService::InitThreadMgrEventSink()
{
    ITfSource* src = nullptr;
    if (FAILED(m_pThreadMgr->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&src))) || !src)
        return false;
    HRESULT hr = src->AdviseSink(IID_ITfThreadMgrEventSink,
                                 static_cast<ITfThreadMgrEventSink*>(this), &m_threadMgrCookie);
    src->Release();
    return SUCCEEDED(hr);
}

void CTextService::UninitThreadMgrEventSink()
{
    if (m_threadMgrCookie == TF_INVALID_COOKIE || !m_pThreadMgr) return;
    ITfSource* src = nullptr;
    if (SUCCEEDED(m_pThreadMgr->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&src))) && src) {
        src->UnadviseSink(m_threadMgrCookie);
        src->Release();
    }
    m_threadMgrCookie = TF_INVALID_COOKIE;
}

bool CTextService::InitKeyEventSink()
{
    ITfKeystrokeMgr* ksm = nullptr;
    if (FAILED(m_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&ksm))) || !ksm)
        return false;
    HRESULT hr = ksm->AdviseKeyEventSink(m_clientId, static_cast<ITfKeyEventSink*>(this), TRUE);
    ksm->Release();
    return SUCCEEDED(hr);
}

void CTextService::UninitKeyEventSink()
{
    if (!m_pThreadMgr || m_clientId == TF_CLIENTID_NULL) return;
    ITfKeystrokeMgr* ksm = nullptr;
    if (SUCCEEDED(m_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&ksm))) && ksm) {
        ksm->UnadviseKeyEventSink(m_clientId);
        ksm->Release();
    }
}

// ---- ITfThreadMgrEventSink (focus/document lifecycle; nothing to track) -----

STDMETHODIMP CTextService::OnInitDocumentMgr(ITfDocumentMgr*)                 { return S_OK; }
STDMETHODIMP CTextService::OnUninitDocumentMgr(ITfDocumentMgr*)               { return S_OK; }
STDMETHODIMP CTextService::OnSetFocus(ITfDocumentMgr*, ITfDocumentMgr*)       { m_composer.reset(); return S_OK; }
STDMETHODIMP CTextService::OnPushContext(ITfContext*)                         { return S_OK; }
STDMETHODIMP CTextService::OnPopContext(ITfContext*)                          { return S_OK; }

// ---- ITfKeyEventSink --------------------------------------------------------

STDMETHODIMP CTextService::OnSetFocus(BOOL /*fForeground*/) { m_composer.reset(); return S_OK; }

bool CTextService::IsModifierVk(unsigned vk)
{
    switch (vk) {
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
        case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
        case VK_MENU: case VK_LMENU: case VK_RMENU:
        case VK_LWIN: case VK_RWIN: case VK_CAPITAL:
            return true;
        default: return false;
    }
}

bool CTextService::HandleKeyDown(ITfContext* pic, WPARAM wParam, bool apply)
{
    if (!m_haveLayout || !m_composer.layout()) return false;
    const unsigned vk = static_cast<unsigned>(wParam);
    if (IsModifierVk(vk)) return false;

    auto down = [](int k) { return (GetKeyState(k) & 0x8000) != 0; };
    const bool ctrl  = down(VK_CONTROL);
    const bool lAlt  = down(VK_LMENU);
    const bool rAlt  = down(VK_RMENU);
    const bool win   = down(VK_LWIN) || down(VK_RWIN);
    const bool altgr = rAlt && ctrl;
    if (win || lAlt || (ctrl && !altgr))
        return false; // let real shortcuts through

    // Prediction path (OnTestKeyDown): decide without mutating state/document.
    if (!apply) {
        if (m_composer.deadPending() || m_composer.composing())
            return true;
        core::KeyInput probe;
        probe.vk = vk; probe.shift = down(VK_SHIFT); probe.altgr = altgr;
        probe.caps = (GetKeyState(VK_CAPITAL) & 1) != 0;
        // Special editing keys the composer handles.
        if (vk == core::vk::Space || vk == core::vk::Back ||
            vk == core::vk::Enter || vk == core::vk::Tab)
            return false; // pass-through unless a dead key is pending (handled above)
        return m_composer.layout()->resolve(vk, probe.shift, probe.altgr, probe.caps) != nullptr;
    }

    core::KeyInput in;
    in.vk    = vk;
    in.shift = down(VK_SHIFT);
    in.altgr = altgr;
    in.caps  = (GetKeyState(VK_CAPITAL) & 1) != 0;

    core::EmitOp op = m_composer.process(in);
    if (!op.suppress)
        return false;
    if (op.empty())
        return true; // consumed (e.g. dead key pending) but no document change yet

    // Apply the edit synchronously through a TSF edit session.
    if (pic) {
        CEditSession* session = new CEditSession(pic, op);
        HRESULT hrSession = S_OK;
        pic->RequestEditSession(m_clientId, session, TF_ES_SYNC | TF_ES_READWRITE, &hrSession);
        session->Release();
    }
    return true;
}

STDMETHODIMP CTextService::OnTestKeyDown(ITfContext* pic, WPARAM wParam, LPARAM, BOOL* pfEaten)
{
    *pfEaten = HandleKeyDown(pic, wParam, /*apply*/ false) ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP CTextService::OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM, BOOL* pfEaten)
{
    *pfEaten = HandleKeyDown(pic, wParam, /*apply*/ true) ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP CTextService::OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* pfEaten) { *pfEaten = FALSE; return S_OK; }
STDMETHODIMP CTextService::OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* pfEaten)     { *pfEaten = FALSE; return S_OK; }
STDMETHODIMP CTextService::OnPreservedKey(ITfContext*, REFGUID, BOOL* pfEaten)     { *pfEaten = FALSE; return S_OK; }

// ---- ITfCompositionSink -----------------------------------------------------
// Inserts are committed directly, so there is no persistent composition to
// track; the sink is provided for interface completeness.
STDMETHODIMP CTextService::OnCompositionTerminated(TfEditCookie, ITfComposition*) { return S_OK; }
