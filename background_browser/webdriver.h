// Copyright (c) 2015, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#ifndef BACKGROUND_BROWSER_WEBDRIVER_H_
#define BACKGROUND_BROWSER_WEBDRIVER_H_

#include <windows.h>
#include <cstdint>

class host_window;

class webdriver final {
 public:
  webdriver() = default;
  ~webdriver();

  bool init();
  void uninit();
  bool navigate(const wchar_t *url, int32_t *errcode_ptr = nullptr) const;
  bool execute_script(const wchar_t *script, bool wait_for_doc = false) const;
  const wchar_t* get_html() const;
  const wchar_t* get_raw_html() const;
  bool set_mobile_mode(bool mobile_mode);

  bool set_document_complete(HRESULT func(IDispatch*, VARIANT*));
  bool set_navigation_complete(HRESULT func(IDispatch*, VARIANT*));
  bool set_navigate_error(HRESULT func(IDispatch *disp_ptr, VARIANT *vt_url_ptr,
    VARIANT *vt_frame_ptr, VARIANT *vt_status_ptr, VARIANT *vt_cancle_ptr));
  bool set_before_navigate2(HRESULT func(IDispatch* disp_ptr, VARIANT *vt_url_ptr,
    VARIANT *vt_flag_ptr, VARIANT *vt_frame_ptr, VARIANT *vt_post_ptr,
    VARIANT *vt_header_ptr, VARIANT_BOOL *vt_cancel_ptr));
  bool set_show_message(HRESULT func(HWND, LPWSTR, LPWSTR, DWORD, LPWSTR, DWORD, LRESULT*));

 private:
  static unsigned _stdcall relay_proc(void* arg_ptr);
  unsigned thread_proc();

  volatile bool init_ = false;
  volatile HWND hwnd_ = NULL;
  HANDLE hthread_ = NULL;
  HANDLE hevent_ = NULL;
  host_window *host_ptr_ = nullptr;
};

#endif  // BACKGROUND_BROWSER_WEBDRIVER_H_
