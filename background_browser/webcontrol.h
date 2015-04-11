// Copyright (c) 2015, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#ifndef BACKGROUND_BROWSER_WEBCONTROL_H_
#define BACKGROUND_BROWSER_WEBCONTROL_H_

#include <windows.h>
#include <atlcomcli.h>
#include <exdisp.h>
#include <cstdint>
#include <string>
#include "background_browser/useragent.h"

enum {
  WM_NAVIGATE_COMPLETE = WM_APP + 100,
  WM_REQ_NAVIGATE,
  WM_REQ_EXECUTE_SCRIPT,
  WM_REQ_MOBILE_MODE,
  WM_REQ_HTML_RAW_SOURCE,
  WM_REQ_HTML_SOURCE,
  WM_REQ_NAVIGATE_ERRCODE,
};

class webcontrol
    : public IOleInPlaceSite
    , public IOleClientSite
    , public IDispatch {
 public:
  webcontrol() = default;
  virtual ~webcontrol();

  bool init(HWND hwnd);
  void uninit();
  bool set_mobile_mode(bool mobile_mode);
  bool navigate(const wchar_t *url) const;
  bool execute_script(const wchar_t *script) const;
  bool eval_script(const wchar_t *script) const;
  std::wstring get_raw_html_source() const;
  std::wstring get_html_source() const;

 private:
  inline bool set_client_site(bool setup, HWND hwnd);
  inline bool set_event_sink(bool setup);

  HRESULT on_document_complete(IDispatch *disp_ptr, VARIANT *vt_url_ptr);
  HRESULT on_navigate_error(IDispatch *disp_ptr, VARIANT *vt_url_ptr,
    VARIANT *vt_frame_ptr, VARIANT *vt_status_ptr, VARIANT *vt_cancle_ptr);

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

  static useragent useragent_;
  HWND hwnd_ = NULL;
  bool mobile_mode_ = false;
  DWORD advise_cookie_ = 0;
  CComPtr<IWebBrowser2> browser_ptr_;
};

#endif  // BACKGROUND_BROWSER_WEBCONTROL_H_
