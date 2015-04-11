// Copyright (c) 2015, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#ifndef BACKGROUND_BROWSER_WEBDRIVER_H_
#define BACKGROUND_BROWSER_WEBDRIVER_H_

#include <windows.h>
#include <cstdint>

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

 private:
  static unsigned _stdcall relay_proc(void* arg_ptr);
  unsigned thread_proc();

  volatile bool init_ = false;
  volatile HWND hwnd_ = NULL;
  HANDLE hthread_ = NULL;
  HANDLE hevent_ = NULL;
};

#endif  // BACKGROUND_BROWSER_WEBDRIVER_H_
