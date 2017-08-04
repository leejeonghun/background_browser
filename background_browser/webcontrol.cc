// Copyright (c) 2015, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#include "background_browser/webcontrol.h"
#include <exdispid.h>
#include <mshtml.h>
#include <mshtmdid.h>

// MSDN references:
// WebBrowser Control- https://msdn.microsoft.com/en-us/library/aa752040(v=vs.85).aspx
// How to handle script errors as a WebBrowser control host- https://support.microsoft.com/en-us/kb/261003


webfeature webcontrol::feature_;

webcontrol::~webcontrol() {
  uninit();
}

bool webcontrol::init(HWND hwnd) {
  bool init = false;

  if (browser_ptr_ == nullptr && IsWindow(hwnd)) {
    OleInitialize(NULL);
    HRESULT hr = CoCreateInstance(CLSID_WebBrowser, nullptr,
      CLSCTX_INPROC_SERVER, IID_IWebBrowser2,
      reinterpret_cast<void**>(&browser_ptr_));

    if (SUCCEEDED(hr) && browser_ptr_ != nullptr) {
      hwnd_ = hwnd;
      browser_ptr_->put_Silent(VARIANT_TRUE);
      init = set_client_site(true, hwnd_) && set_event_sink(true);
    }
  }

  return init;
}

void webcontrol::uninit() {
  if (browser_ptr_ != nullptr) {
    browser_ptr_->Stop();
    browser_ptr_->ExecWB(OLECMDID_CLOSE, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
    browser_ptr_->put_Visible(VARIANT_FALSE);

    set_event_sink(false);
    set_client_site(false, hwnd_);

    browser_ptr_ = nullptr;
    OleUninitialize();
  }
}

bool webcontrol::set_mobile_mode(bool mobile_mode) {
  return feature_.set_mobile_mode(mobile_mode)
    ? mobile_mode_ = mobile_mode, true
    : false;
}

bool webcontrol::navigate(const wchar_t *url) const {
  bool navigate = false;

  if (url != nullptr && browser_ptr_ != nullptr) {
    CComVariant vt_url(url);
    VARIANT vt_empty;
    VariantInit(&vt_empty);
    navigate = SUCCEEDED(browser_ptr_->Navigate2(&vt_url, &vt_empty,
      &vt_empty, &vt_empty, &vt_empty));
  }

  return navigate;
}

bool webcontrol::execute_script(const wchar_t *script) const {
  if (browser_ptr_ != nullptr) {
    CComPtr<IDispatch> doc_disp_ptr;
    HRESULT hr = browser_ptr_->get_Document(&doc_disp_ptr);
    if (SUCCEEDED(hr) && doc_disp_ptr != nullptr) {
      CComPtr<IHTMLDocument2> html_doc_ptr;
      hr = doc_disp_ptr->QueryInterface(__uuidof(IHTMLDocument2),
        reinterpret_cast<void**>(&html_doc_ptr));
      if (FAILED(hr) || html_doc_ptr == nullptr) return false;

      CComPtr<IHTMLWindow2> html_wnd_ptr;
      hr = html_doc_ptr->get_parentWindow(&html_wnd_ptr);
      if (FAILED(hr) || html_wnd_ptr == nullptr) return false;

      CComVariant var;
      hr = html_wnd_ptr->execScript(CComBSTR(script),
        CComBSTR(L"JavaScript"), &var);
      if (FAILED(hr)) return false;

      return true;
    }
  }

  return false;
}

bool webcontrol::eval_script(const wchar_t *script) const {
  if (browser_ptr_ != nullptr) {
    CComPtr<IDispatch> doc_disp_ptr;
    HRESULT hr = browser_ptr_->get_Document(&doc_disp_ptr);
    if (SUCCEEDED(hr) && doc_disp_ptr != nullptr) {
      CComPtr<IHTMLDocument2> html_doc_ptr;
      hr = doc_disp_ptr->QueryInterface(__uuidof(IHTMLDocument2),
        reinterpret_cast<void**>(&html_doc_ptr));
      if (FAILED(hr) || html_doc_ptr == nullptr) return false;

      CComPtr<IHTMLWindow2> html_wnd_ptr;
      hr = html_doc_ptr->get_parentWindow(&html_wnd_ptr);
      if (FAILED(hr) || html_wnd_ptr == nullptr) return false;

      wchar_t *dispname = L"eval";
      DISPID disp_id = 0;
      hr = html_doc_ptr->GetIDsOfNames(IID_NULL, &dispname, 1,
        LOCALE_USER_DEFAULT, &disp_id);
      if (SUCCEEDED(hr)) {
        CComVariant vt_result;
        CComVariant vt_script(script);
        DISPPARAMS params = { &vt_script, nullptr, 1, 0 };
        hr = html_doc_ptr->Invoke(disp_id, IID_NULL, LOCALE_USER_DEFAULT,
          DISPATCH_METHOD, &params, &vt_result, nullptr, nullptr);
        if (FAILED(hr)) return false;

        return true;
      }
    }
  }

  return false;
}

bool webcontrol::chk_ready() const {
  bool ready = false;

  if (browser_ptr_ != nullptr) {
    VARIANT_BOOL busy = VARIANT_FALSE;
    if (SUCCEEDED(browser_ptr_->get_Busy(&busy)) && busy == VARIANT_FALSE) {
      READYSTATE state = READYSTATE_UNINITIALIZED;
      ready = SUCCEEDED(browser_ptr_->get_ReadyState(&state)) &&
        state == READYSTATE_COMPLETE;
    }
  }

  return ready;
}

std::wstring webcontrol::get_raw_html_source() const {
  std::wstring html;

  if (browser_ptr_ != nullptr) {
    CComPtr<IDispatch> disp_ptr;
    HRESULT hr = browser_ptr_->get_Document(&disp_ptr);

    CComPtr<IPersistStreamInit> psi_ptr;
    if (SUCCEEDED(hr) && disp_ptr != nullptr) {
      hr = disp_ptr.QueryInterface(&psi_ptr);
    }

    CComPtr<IStream> stream_ptr;
    if (SUCCEEDED(hr) && psi_ptr != nullptr) {
      hr = CreateStreamOnHGlobal(NULL, true, &stream_ptr);
    }

    if (SUCCEEDED(hr) && stream_ptr != nullptr) {
      hr = psi_ptr->Save(stream_ptr, false);
    }

    if (SUCCEEDED(hr)) {
      hr = stream_ptr->Write("", 1, nullptr);
    }

    if (SUCCEEDED(hr)) {
      HGLOBAL global_handle = NULL;
      if (SUCCEEDED(GetHGlobalFromStream(stream_ptr, &global_handle))) {
        html = CA2W(reinterpret_cast<const char*>(
          GlobalLock(global_handle)), CP_UTF8);
        GlobalUnlock(global_handle);
      }
    }
  }

  return html;
}

std::wstring webcontrol::get_html_source() const {
  std::wstring html;

  if (browser_ptr_ != nullptr) {
    CComPtr<IDispatch> disp_ptr;
    HRESULT hr = browser_ptr_->get_Document(&disp_ptr);

    CComPtr<IHTMLDocument2> doc_ptr;
    if (SUCCEEDED(hr) && disp_ptr != nullptr) {
      hr = disp_ptr.QueryInterface(&doc_ptr);
    }

    CComPtr<IHTMLElement> body_ptr;
    if (SUCCEEDED(hr) && doc_ptr != nullptr) {
      hr = doc_ptr->get_body(&body_ptr);
    }

    CComPtr<IHTMLElement> html_ptr;
    if (SUCCEEDED(hr) && body_ptr != nullptr) {
      hr = body_ptr->get_parentElement(&html_ptr);
    }

    if (SUCCEEDED(hr) && html_ptr) {
      BSTR html_src = nullptr;
      if (SUCCEEDED(html_ptr->get_outerHTML(&html_src))) {
        html = html_src;
      }
    }
  }

  return html;
}

std::wstring webcontrol::get_curr_url() const {
  std::wstring url;

  if (browser_ptr_ != nullptr) {
    BSTR url_ptr = nullptr;
    if (SUCCEEDED(browser_ptr_->get_LocationURL(&url_ptr))) {
      url = url_ptr;
      SysFreeString(url_ptr);
    }
  }

  return url;
}

bool webcontrol::set_client_site(bool setup, HWND hwnd) {
  if (browser_ptr_ != nullptr) {
    CComPtr<IOleObject> ole_obj_ptr;
    HRESULT hr = browser_ptr_->QueryInterface(IID_IOleObject,
      reinterpret_cast<void**>(&ole_obj_ptr));
    if (FAILED(hr)) return false;

    if (setup == true) {
      hr = ole_obj_ptr->SetClientSite(this);
      if (FAILED(hr)) return false;
      hr = ole_obj_ptr->DoVerb(OLEIVERB_SHOW, nullptr, this, 0, hwnd, nullptr);
      if (FAILED(hr)) return false;
    } else {
      hr = ole_obj_ptr->DoVerb(OLEIVERB_HIDE, nullptr, this, 0, hwnd, nullptr);
      if (FAILED(hr)) return false;
      hr = ole_obj_ptr->SetClientSite(nullptr);
      if (FAILED(hr)) return false;
      hr = ole_obj_ptr->Close(OLECLOSE_NOSAVE);
      if (FAILED(hr)) return false;
      hr = CoDisconnectObject(ole_obj_ptr, 0);
      if (FAILED(hr)) return false;
    }
  }

  return true;
}

bool webcontrol::set_event_sink(bool setup) {
  if (browser_ptr_ != nullptr) {
    CComPtr<IConnectionPointContainer> conpnt_ctnr_ptr;
    HRESULT hr = browser_ptr_->QueryInterface(IID_IConnectionPointContainer,
      reinterpret_cast<void **>(&conpnt_ctnr_ptr));
    if (FAILED(hr)) return false;

    CComPtr<IConnectionPoint> conpnt_ptr;
    hr = conpnt_ctnr_ptr->FindConnectionPoint(DIID_DWebBrowserEvents2,
      &conpnt_ptr);
    if (FAILED(hr)) return false;

    if (setup == true) {
      hr = conpnt_ptr->Advise(dynamic_cast<IDispatch*>(this), &advise_cookie_);
      if (FAILED(hr)) return false;
    } else {
      hr = conpnt_ptr->Unadvise(advise_cookie_);
      if (FAILED(hr)) return false;
    }
  }

  return true;
}

HRESULT webcontrol::on_document_complete(IDispatch *disp_ptr,
    VARIANT *vt_url_ptr) {
  HRESULT hr = S_OK;
  if (document_complete_handler_) {
    hr = document_complete_handler_(disp_ptr, vt_url_ptr);
  }

  CComPtr<IDispatch> top_doc_ptr;
  hr = browser_ptr_->QueryInterface(IID_IDispatch,
    reinterpret_cast<void**>(&top_doc_ptr));

  if (SUCCEEDED(hr) && disp_ptr != nullptr) {
    if (disp_ptr == top_doc_ptr) {
      PostMessage(hwnd_, WM_NAVIGATE_COMPLETE, 0, 0);
    }
  }

  return hr;
}

HRESULT webcontrol::on_navigate_complete(IDispatch *disp_ptr, VARIANT *vt_url) {
  HRESULT hr = S_OK;
  if (navigate_complete_handler_) {
    hr = navigate_complete_handler_(disp_ptr, vt_url);
  }

  return hr;
}

HRESULT webcontrol::on_navigate_error(IDispatch *disp_ptr, VARIANT *vt_url_ptr,
    VARIANT *vt_frame_ptr, VARIANT *vt_status_ptr, VARIANT *vt_cancel_ptr) {
  HRESULT hr = S_OK;
  if (navigate_error_handler_) {
    hr = navigate_error_handler_(disp_ptr, vt_url_ptr, vt_frame_ptr,
      vt_status_ptr, vt_cancel_ptr);
  }

  CComPtr<IDispatch> top_doc_ptr;
  hr = browser_ptr_->QueryInterface(IID_IDispatch,
    reinterpret_cast<void**>(&top_doc_ptr));

  if (SUCCEEDED(hr) && disp_ptr != nullptr) {
    if (disp_ptr == top_doc_ptr) {
      PostMessage(hwnd_, WM_NAVIGATE_COMPLETE, vt_status_ptr->lVal, 0);
    }
  }

  return hr;
}

HRESULT webcontrol::on_before_navigate2(IDispatch* disp_ptr, VARIANT *vt_url_ptr,
    VARIANT *vt_flag_ptr, VARIANT *vt_frame_ptr, VARIANT *vt_post_ptr,
    VARIANT *vt_header_ptr, VARIANT_BOOL *vt_cancel_ptr) {
  HRESULT hr = S_OK;
  if (before_navigate2_handler_) {
    hr = before_navigate2_handler_(disp_ptr, vt_url_ptr, vt_flag_ptr,
      vt_frame_ptr, vt_post_ptr, vt_header_ptr, vt_cancel_ptr);
  }

  return hr;
}

HRESULT webcontrol::on_new_window3(IDispatch *disp_ptr, VARIANT_BOOL *vt_cancel_ptr,
    DWORD flags, BSTR urlctx, BSTR url) {
  HRESULT hr = S_OK;
  if (new_window3_handler_) {
    hr = new_window3_handler_(disp_ptr, vt_cancel_ptr, flags, urlctx, url);
  }

  return hr;
}

// IOleWindow (be inherited by IOleInPlaceSite)
HRESULT _stdcall webcontrol::GetWindow(HWND *phwnd) {
  HRESULT hr = E_INVALIDARG;
  if (phwnd != nullptr) {
    *phwnd = hwnd_;
    hr = S_OK;
  }

  return hr;
}

// IDocHostShowUI
HRESULT _stdcall webcontrol::ShowMessage(HWND hwnd, LPOLESTR text,
    LPOLESTR caption, DWORD type, LPOLESTR helpfile, DWORD help_ctx, LRESULT *result_ptr) {
  HRESULT hr = S_OK;
  if (show_message_handler_) {
    hr = show_message_handler_(hwnd, text, caption, type, helpfile, help_ctx, result_ptr);
  }

  return hr;
}

// IDispatch
HRESULT _stdcall webcontrol::Invoke(DISPID dispid, REFIID riid, LCID lcid,
    WORD flags, DISPPARAMS *params_ptr, VARIANT *result_ptr,
    EXCEPINFO *ei_ptr, UINT *err_arg_ptr) {
  switch (dispid) {
  case DISPID_DOCUMENTCOMPLETE:
    return on_document_complete(params_ptr->rgvarg[1].pdispVal,
      params_ptr->rgvarg[0].pvarVal);
    break;

  case DISPID_NAVIGATECOMPLETE2:
      return on_navigate_complete(params_ptr->rgvarg[1].pdispVal,
        params_ptr->rgvarg[0].pvarVal);
     break;

  case DISPID_BEFORENAVIGATE2:
    return on_before_navigate2(params_ptr->rgvarg[6].pdispVal,
      params_ptr->rgvarg[5].pvarVal, params_ptr->rgvarg[4].pvarVal,
      params_ptr->rgvarg[3].pvarVal, params_ptr->rgvarg[2].pvarVal,
      params_ptr->rgvarg[1].pvarVal, params_ptr->rgvarg[0].pboolVal);
    break;

  case DISPID_NAVIGATEERROR:
    return on_navigate_error(params_ptr->rgvarg[4].pdispVal,
      params_ptr->rgvarg[3].pvarVal, params_ptr->rgvarg[2].pvarVal,
      params_ptr->rgvarg[1].pvarVal, params_ptr->rgvarg[0].pvarVal);
    break;

  case DISPID_NEWWINDOW3:
    return this->on_new_window3(params_ptr->rgvarg[4].pdispVal,
      params_ptr->rgvarg[3].pboolVal, params_ptr->rgvarg[2].lVal,
      params_ptr->rgvarg[1].bstrVal, params_ptr->rgvarg[0].bstrVal);
    break;

  case DISPID_AMBIENT_USERAGENT:
    if (mobile_mode_) {
      result_ptr->vt = VT_BSTR;
      result_ptr->bstrVal = SysAllocString(CA2W(feature_.get_useragent().iphone()));
    }
    break;

  default:
    return E_NOTIMPL;
  }

  return S_OK;
}

// IUnknown
HRESULT _stdcall webcontrol::QueryInterface(REFIID riid, void **obj_pp) {
  HRESULT hr = E_INVALIDARG;

  if (obj_pp != nullptr) {
    if (riid == IID_IOleInPlaceSite) {
      *obj_pp = static_cast<IOleInPlaceSite*>(this);
    } else if (riid == IID_IOleClientSite) {
      *obj_pp = static_cast<IOleClientSite*>(this);
    } else if (riid == IID_IDocHostShowUI) {
      *obj_pp = static_cast<IDocHostShowUI*>(this);
    } else if (riid == IID_IDispatch) {
      *obj_pp = static_cast<IDispatch*>(this);
    } else if (riid == IID_IUnknown) {
      *obj_pp = static_cast<IUnknown*>(static_cast<IDispatch*>(this));
    } else {
      hr = E_NOINTERFACE;
    }

    if (hr != E_NOINTERFACE) {
      hr = S_OK;
    }
  }

  return hr;
}
