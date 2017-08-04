// Copyright (c) 2015, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#ifndef BACKGROUND_BROWSER_WEBCONTROL_H_
#define BACKGROUND_BROWSER_WEBCONTROL_H_

#include <windows.h>
#include <atlcomcli.h>
#include <exdisp.h>
#include <mshtmhst.h>
#include <cstdint>
#include <string>
#include <functional>
#include "background_browser/webfeature.h"

enum {
  WM_NAVIGATE_COMPLETE = WM_APP + 100,
  WM_REQ_NAVIGATE,
  WM_REQ_EXECUTE_SCRIPT,
  WM_REQ_MOBILE_MODE,
  WM_REQ_HTML_RAW_SOURCE,
  WM_REQ_HTML_SOURCE,
  WM_REQ_NAVIGATE_ERRCODE,
  WM_REQ_CHECK_READY,
  WM_REQ_CURRENT_URL,
};

class webcontrol
    : public IOleInPlaceSite
    , public IOleClientSite
    , public IDocHostShowUI
    , public IDispatch {
  friend class webdriver;

 public:
  webcontrol() = default;
  virtual ~webcontrol();

  bool init(HWND hwnd);
  void uninit();
  bool set_mobile_mode(bool mobile_mode);
  bool navigate(const wchar_t *url) const;
  bool execute_script(const wchar_t *script) const;
  bool eval_script(const wchar_t *script) const;
  bool chk_ready() const;
  std::wstring get_raw_html_source() const;
  std::wstring get_html_source() const;
  std::wstring get_curr_url() const;

 private:
  inline bool set_client_site(bool setup, HWND hwnd);
  inline bool set_event_sink(bool setup);

  inline HRESULT on_document_complete(IDispatch *disp_ptr, VARIANT *vt_url_ptr);
  inline HRESULT on_navigate_complete(IDispatch *disp_ptr, VARIANT *vt_url);
  inline HRESULT on_navigate_error(IDispatch *disp_ptr, VARIANT *vt_url_ptr,
    VARIANT *vt_frame_ptr, VARIANT *vt_status_ptr, VARIANT *vt_cancel_ptr);
  inline HRESULT on_before_navigate2(IDispatch *disp_ptr, VARIANT *vt_url_ptr,
    VARIANT *vt_flag_ptr, VARIANT *vt_frame_ptr, VARIANT *vt_post_ptr,
    VARIANT *vt_header_ptr, VARIANT_BOOL *vt_cancel_ptr);
  inline HRESULT on_new_window3(IDispatch *disp_ptr, VARIANT_BOOL *vt_cancel_ptr,
    DWORD flags, BSTR urlctx, BSTR url);

  // IOleInPlaceSite
  HRESULT _stdcall GetWindowContext(IOleInPlaceFrame**, IOleInPlaceUIWindow**,
    LPRECT, LPRECT, LPOLEINPLACEFRAMEINFO) { return E_UNEXPECTED; }
  HRESULT _stdcall OnPosRectChange(LPCRECT) { return S_OK; }
  HRESULT _stdcall CanInPlaceActivate() { return S_OK; }
  HRESULT _stdcall OnInPlaceActivate() { return S_OK; }
  HRESULT _stdcall OnUIActivate() { return S_OK; }
  HRESULT _stdcall Scroll(SIZE) { return S_OK; }
  HRESULT _stdcall OnUIDeactivate(BOOL) { return S_OK; }
  HRESULT _stdcall OnInPlaceDeactivate() { return S_OK; }
  HRESULT _stdcall DiscardUndoState() { return S_OK; }
  HRESULT _stdcall DeactivateAndUndo() { return S_OK; }

  // IOleWindow (be inherited by IOleInPlaceSite)
  HRESULT _stdcall GetWindow(HWND *phwnd);
  HRESULT _stdcall ContextSensitiveHelp(BOOL) { return E_NOTIMPL; }

  // IOleClientSite
  HRESULT _stdcall SaveObject() { return S_OK; }
  HRESULT _stdcall GetMoniker(DWORD, DWORD, IMoniker**) { return E_NOTIMPL; }
  HRESULT _stdcall GetContainer(IOleContainer**) { return E_NOINTERFACE; }  // NOLINT
  HRESULT _stdcall ShowObject() { return S_OK; }
  HRESULT _stdcall OnShowWindow(BOOL) { return S_OK; }
  HRESULT _stdcall RequestNewObjectLayout() { return E_NOTIMPL; }

  // IDocHostShowUI
  HRESULT _stdcall ShowMessage(HWND hwnd, LPOLESTR text, LPOLESTR caption, DWORD type, LPOLESTR helpfile, DWORD help_ctx, LRESULT *result_ptr);
  HRESULT _stdcall ShowHelp(HWND, LPOLESTR, UINT, DWORD, POINT, IDispatch*) { return E_NOTIMPL; }

  // IDispatch
  HRESULT _stdcall Invoke(DISPID dispid, REFIID riid, LCID lcid,
    WORD flags, DISPPARAMS *params_ptr, VARIANT *result_ptr,
    EXCEPINFO *ei_ptr, UINT *err_arg_ptr);
  HRESULT _stdcall GetTypeInfoCount(UINT*) { return E_NOTIMPL; }  // NOLINT
  HRESULT _stdcall GetTypeInfo(UINT, LCID,
    ITypeInfo**) { return DISP_E_BADINDEX; }
  HRESULT _stdcall GetIDsOfNames(REFIID, LPOLESTR *, UINT,
    LCID, DISPID*) { return DISP_E_UNKNOWNNAME; }

  // IUnknown
  HRESULT _stdcall QueryInterface(REFIID riid, void **obj_pp);
  ULONG _stdcall AddRef() { return 1; }
  ULONG _stdcall Release() { return 1; }

  static webfeature feature_;
  HWND hwnd_ = NULL;
  bool mobile_mode_ = false;
  DWORD advise_cookie_ = 0;
  CComPtr<IWebBrowser2> browser_ptr_;

  std::function<HRESULT(IDispatch*, VARIANT*)> document_complete_handler_;
  std::function<HRESULT(IDispatch*, VARIANT*)> navigate_complete_handler_;
  std::function<HRESULT(IDispatch*, VARIANT*, VARIANT*, VARIANT*, VARIANT*)> navigate_error_handler_;
  std::function<HRESULT(IDispatch*, VARIANT*, VARIANT*, VARIANT*, VARIANT*, VARIANT*, VARIANT_BOOL*)> before_navigate2_handler_;
  std::function<HRESULT(HWND, LPWSTR, LPWSTR, DWORD, LPWSTR, DWORD, LRESULT*)> show_message_handler_;
  std::function<HRESULT(IDispatch*, VARIANT_BOOL*, DWORD, BSTR, BSTR)> new_window3_handler_;
};

#endif  // BACKGROUND_BROWSER_WEBCONTROL_H_
