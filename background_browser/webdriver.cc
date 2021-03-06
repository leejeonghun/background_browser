// Copyright (c) 2015, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#include "background_browser/webdriver.h"
#include <process.h>
#include "background_browser/host_window.h"

webdriver::~webdriver() {
  uninit();
}

bool webdriver::init() {
  if (hevent_ == NULL) {
    hevent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  }

  if (hevent_ != NULL && hthread_ == NULL) {
    hthread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, relay_proc, this, 0, nullptr));
    if (hthread_ != NULL) {
      WaitForSingleObject(hevent_, INFINITE);
    }
  }

  return init_;
}

void webdriver::uninit() {
  if (hevent_ != NULL) {
    CloseHandle(hevent_);
    hevent_ = NULL;
  }

  if (hthread_ != NULL) {
    PostMessage(hwnd_, WM_CLOSE, 0, 0);
    WaitForSingleObject(hthread_, INFINITE);
    CloseHandle(hthread_);
    hthread_ = NULL;
  }
}

bool webdriver::navigate(const wchar_t *url, int32_t *errcode_ptr) const {
  bool navigate = false;

  if (hthread_ != NULL) {
    PostMessage(hwnd_, WM_REQ_NAVIGATE,
      reinterpret_cast<WPARAM>(url), 0);
    navigate = WaitForSingleObject(hevent_, INFINITE) == WAIT_OBJECT_0;
    if (navigate && errcode_ptr) {
      *errcode_ptr = SendMessage(hwnd_, WM_REQ_NAVIGATE_ERRCODE, 0, 0);
    }
  }

  return navigate;
}

bool webdriver::execute_script(const wchar_t *script,
    bool wait_for_doc) const {
  bool execute = false;

  if (hthread_ != NULL) {
    execute = SendMessage(hwnd_, WM_REQ_EXECUTE_SCRIPT,
      reinterpret_cast<WPARAM>(script), 0) ? true : false;
    if (wait_for_doc) {
      execute = WaitForSingleObject(hevent_, INFINITE) == WAIT_OBJECT_0;
    }
  }

  return execute;
}

bool webdriver::wait_for_ready() const {
  enum { POLLING_DELAY = 250 };

  bool ready = false;

  if (hthread_ != NULL) {
    while (SendMessage(hwnd_, WM_REQ_CHECK_READY, 0, 0) == FALSE) {
      Sleep(POLLING_DELAY);
    }
    ready = true;
  }

  return ready;
}

const wchar_t* webdriver::get_html() const {
  const wchar_t* html_ptr = nullptr;

  if (hthread_ != NULL) {
    html_ptr = reinterpret_cast<const wchar_t*>(
      SendMessage(hwnd_, WM_REQ_HTML_SOURCE, 0, 0));
  }

  return html_ptr;
}

const wchar_t* webdriver::get_raw_html() const {
  const wchar_t* html_ptr = nullptr;

  if (hthread_ != NULL) {
    html_ptr = reinterpret_cast<const wchar_t*>(
      SendMessage(hwnd_, WM_REQ_HTML_RAW_SOURCE, 0, 0));
  }

  return html_ptr;
}

const wchar_t* webdriver::get_url() const {
  const wchar_t* url_ptr = nullptr;

  if (hthread_ != NULL) {
    url_ptr = reinterpret_cast<const wchar_t*>(
      SendMessage(hwnd_, WM_REQ_CURRENT_URL, 0, 0));
  }

  return url_ptr;
}

bool webdriver::set_mobile_mode(bool mobile_mode) {
  bool set_mode = false;

  if (hthread_ != NULL) {
    set_mode = SendMessage(hwnd_, WM_REQ_MOBILE_MODE,
      mobile_mode, 0) ? true : false;
  }

  return set_mode;
}

bool webdriver::set_document_complete(HRESULT func(IDispatch*, VARIANT*)) {
  return host_ptr_ != nullptr ?
    host_ptr_->get_webctrl().document_complete_handler_ = func, true :
    false;
}

bool webdriver::set_navigation_complete(HRESULT func(IDispatch*, VARIANT*)) {
  return host_ptr_ != nullptr ?
    host_ptr_->get_webctrl().navigate_complete_handler_ = func, true :
    false;
}

bool webdriver::set_navigate_error(HRESULT func(IDispatch *disp_ptr, VARIANT *vt_url_ptr,
    VARIANT *vt_frame_ptr, VARIANT *vt_status_ptr, VARIANT *vt_cancle_ptr)) {
  return host_ptr_ != nullptr ?
    host_ptr_->get_webctrl().navigate_error_handler_ = func, true :
    false;
}

bool webdriver::set_before_navigate2(HRESULT func(IDispatch* disp_ptr, VARIANT *vt_url_ptr,
    VARIANT *vt_flag_ptr, VARIANT *vt_frame_ptr, VARIANT *vt_post_ptr,
    VARIANT *vt_header_ptr, VARIANT_BOOL *vt_cancel_ptr)) {
  return host_ptr_ != nullptr ?
    host_ptr_->get_webctrl().before_navigate2_handler_ = func, true :
    false;
}

bool webdriver::set_show_message(HRESULT func(HWND, LPWSTR, LPWSTR, DWORD, LPWSTR, DWORD, LRESULT*)) {
  return host_ptr_ != nullptr ?
    host_ptr_->get_webctrl().show_message_handler_ = func, true : false;
}

bool webdriver::set_new_window3(HRESULT func(IDispatch*, VARIANT_BOOL*, DWORD, BSTR, BSTR)) {
  return host_ptr_ != nullptr ?
    host_ptr_->get_webctrl().new_window3_handler_ = func, true : false;
}

unsigned _stdcall webdriver::relay_proc(void* arg_ptr) {
  return arg_ptr ? reinterpret_cast<webdriver*>(arg_ptr)->thread_proc() : -1;
}

unsigned webdriver::thread_proc() {
  unsigned retcode = -1;

  host_window browser;
  init_ = browser.create(hevent_);
  if (init_) {
    host_ptr_ = &browser;
    hwnd_ = browser.get_hwnd();
  }
  SetEvent(hevent_);

  if (init_) {
    retcode = browser.run_msg_loop();
    host_ptr_ = nullptr;
  }

  return retcode;
}
