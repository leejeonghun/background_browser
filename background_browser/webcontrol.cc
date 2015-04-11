// Copyright (c) 2015, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#include "background_browser/webcontrol.h"
#include <exdispid.h>
#include <mshtml.h>
#include <mshtmdid.h>

useragent webcontrol::useragent_;

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
  HRESULT hr = E_FAIL;

  mobile_mode_ = mobile_mode;
  if (mobile_mode_) {
    hr = UrlMkSetSessionOption(URLMON_OPTION_USERAGENT,
      const_cast<char*>(useragent_.iphone()), strlen(useragent_.iphone()), 0);
  } else {
    hr = UrlMkSetSessionOption(URLMON_OPTION_USERAGENT_REFRESH, nullptr, 0, 0);
  }

  return SUCCEEDED(hr);
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
  CComPtr<IDispatch> top_doc_ptr;
  HRESULT hr = browser_ptr_->QueryInterface(IID_IDispatch,
    reinterpret_cast<void**>(&top_doc_ptr));

  if (SUCCEEDED(hr) && disp_ptr != nullptr) {
    if (disp_ptr == top_doc_ptr) {
      PostMessage(hwnd_, WM_NAVIGATE_COMPLETE, 0, 0);
    }
  }

  return S_OK;
}

HRESULT webcontrol::on_navigate_error(IDispatch *disp_ptr, VARIANT *vt_url_ptr,
    VARIANT *vt_frame_ptr, VARIANT *vt_status_ptr, VARIANT *vt_cancle_ptr) {
  CComPtr<IDispatch> top_doc_ptr;
  HRESULT hr = browser_ptr_->QueryInterface(IID_IDispatch,
    reinterpret_cast<void**>(&top_doc_ptr));

  if (SUCCEEDED(hr) && disp_ptr != nullptr) {
    if (disp_ptr == top_doc_ptr) {
      PostMessage(hwnd_, WM_NAVIGATE_COMPLETE, vt_status_ptr->lVal, 0);
    }
  }

  return S_OK;
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

// IDispatch
HRESULT _stdcall webcontrol::Invoke(DISPID dispid, REFIID riid, LCID lcid,
    WORD flags, DISPPARAMS *params_ptr, VARIANT *result_ptr,
    EXCEPINFO *ei_ptr, UINT *err_arg_ptr) {
  switch (dispid) {
  case DISPID_DOCUMENTCOMPLETE:
    return this->on_document_complete(params_ptr->rgvarg[1].pdispVal,
      params_ptr->rgvarg[0].pvarVal);
    break;

  case DISPID_NAVIGATEERROR:
    return this->on_navigate_error(params_ptr->rgvarg[4].pdispVal,
      params_ptr->rgvarg[3].pvarVal, params_ptr->rgvarg[2].pvarVal,
      params_ptr->rgvarg[1].pvarVal, params_ptr->rgvarg[0].pvarVal);
    break;

  case DISPID_AMBIENT_USERAGENT:
    if (mobile_mode_) {
      result_ptr->vt = VT_BSTR;
      result_ptr->bstrVal = SysAllocString(CA2W(useragent_.iphone()));
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