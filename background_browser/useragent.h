// Copyright (c) 2015, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#ifndef BACKGROUND_BROWSER_USERAGENT_H_
#define BACKGROUND_BROWSER_USERAGENT_H_

#include <urlmon.h>
#pragma comment(lib, "urlmon")

class useragent final {
 public:
  useragent() { ObtainUserAgentString(0, ie_user_agent_, &length_); }
  ~useragent() = default;

  inline const char* ie() const {
    return (length_) > 0 ? ie_user_agent_ : nullptr; }
  inline const char* iphone() const { return "Mozilla/5.0 (iPhone; "
    "CPU iPhone OS 8_0 like Mac OS X) AppleWebKit/537.51.1 (KHTML, like Gecko) "
    "Version/8.0 Mobile/11A465 Safari/9537.53"; }

 private:
  DWORD length_ = 0;
  char ie_user_agent_[196];
};

#endif  // BACKGROUND_BROWSER_USERAGENT_H_
