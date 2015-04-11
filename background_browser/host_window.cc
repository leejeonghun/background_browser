// Copyright (c) 2015, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#include "background_browser/host_window.h"

bool host_window::create(HANDLE hevent) {
  if (hevent != NULL) {
    WNDCLASSEX wc = { sizeof(wc), 0 };
    wc.lpszClassName = L"web_window_class";
    wc.lpfnWndProc = precreate_proc;
    wc.hInstance = GetModuleHandle(nullptr);

    RegisterClassEx(&wc);
    hwnd_ = CreateWindowEx(WS_EX_NOACTIVATE, wc.lpszClassName, L"",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
      NULL, NULL, wc.hInstance, this);
    hevent_ = hevent;
  }

  return IsWindow(hwnd_) != FALSE && hevent_ != NULL;
}

const wchar_t* host_window::get_html() {
  html_ = web_ctrl_.get_html_source();
  return html_.c_str();
}

const wchar_t* host_window::get_raw_html() {
  html_ = web_ctrl_.get_raw_html_source();
  return html_.c_str();
}

int32_t host_window::run_msg_loop() const {
  MSG msg = { 0 };
  while (::GetMessage(&msg, NULL, 0, 0)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }

  return msg.wParam;
}

LRESULT CALLBACK host_window::precreate_proc(HWND hwnd, UINT msg,
    WPARAM wparam, LPARAM lparam) {
  if (msg == WM_NCCREATE) {
    host_window *this_ptr = reinterpret_cast<host_window*>(
      reinterpret_cast<LPCREATESTRUCT>(lparam)->lpCreateParams);
    if (this_ptr != nullptr) {
      SetWindowLongPtr(hwnd, GWL_USERDATA,
        reinterpret_cast<LONG_PTR>(this_ptr));
      SetWindowLongPtr(hwnd, GWL_WNDPROC,
        reinterpret_cast<LONG_PTR>(relay_proc));
    }
  }

  return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK host_window::relay_proc(HWND hwnd, UINT msg,
    WPARAM wparam, LPARAM lparam) {
  return reinterpret_cast<host_window*>(
    GetWindowLongPtr(hwnd, GWL_USERDATA))->wndproc(hwnd, msg, wparam, lparam);
}

LRESULT host_window::wndproc(HWND hwnd, UINT msg,
    WPARAM wparam, LPARAM lparam) {
  LRESULT result = 0;

  switch (msg) {
  case WM_CREATE:
    web_ctrl_.init(hwnd);
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  case WM_REQ_NAVIGATE:
    web_ctrl_.navigate(reinterpret_cast<const wchar_t*>(wparam));
    break;

  case WM_REQ_EXECUTE_SCRIPT:
    result = web_ctrl_.execute_script(
      reinterpret_cast<const wchar_t*>(wparam));
    break;

  case WM_REQ_MOBILE_MODE:
    result = web_ctrl_.set_mobile_mode(wparam ? true : false);
    break;

  case WM_REQ_HTML_SOURCE:
    html_ = web_ctrl_.get_html_source();
    result = reinterpret_cast<LRESULT>(html_.c_str());
    break;

  case WM_REQ_HTML_RAW_SOURCE:
    html_ = web_ctrl_.get_raw_html_source();
    result = reinterpret_cast<LRESULT>(html_.c_str());
    break;

  case WM_REQ_NAVIGATE_ERRCODE:
    result = errcode_;
    break;

  case WM_NAVIGATE_COMPLETE:
    errcode_ = wparam;
    SetEvent(hevent_);
    break;

  default:
    result = DefWindowProc(hwnd, msg, wparam, lparam);
    break;
  }

  return result;
}
