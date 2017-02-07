// Copyright (c) 2015, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#ifndef BACKGROUND_BROWSER_HOST_WINDOW_H_
#define BACKGROUND_BROWSER_HOST_WINDOW_H_

#include <windows.h>
#include <cstdint>
#include "background_browser/webcontrol.h"

class host_window {
 public:
  host_window() = default;
  virtual ~host_window() = default;

  bool create(HANDLE hevent);
  inline HWND get_hwnd() const { return hwnd_; }
  inline webcontrol& get_webctrl() { return web_ctrl_; }
  const wchar_t* get_html();
  const wchar_t* get_raw_html();
  int32_t run_msg_loop() const;

 private:
  static LRESULT CALLBACK precreate_proc(HWND hwnd, UINT msg,
    WPARAM wparam, LPARAM lparam);
  static LRESULT CALLBACK relay_proc(HWND hwnd, UINT msg,
    WPARAM wparam, LPARAM lparam);
  LRESULT wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

  HWND hwnd_ = NULL;
  HANDLE hevent_ = NULL;
  int32_t errcode_ = 0;
  webcontrol web_ctrl_;
  std::wstring html_;
};

#endif  // BACKGROUND_BROWSER_HOST_WINDOW_H_
