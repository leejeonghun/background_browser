// Copyright (c) 2016, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#include "background_browser/webfeature.h"
#include <urlmon.h>

#pragma comment(lib, "urlmon")

webfeature::webfeature() {
  disable_navigation_sound();
}

bool webfeature::set_mobile_mode(bool mobile_mode) const {
  bool set_mode = false;

  if (mobile_mode) {
    set_mode = SUCCEEDED(UrlMkSetSessionOption(URLMON_OPTION_USERAGENT,
      const_cast<char*>(useragent_.iphone()), strlen(useragent_.iphone()), 0));
  } else {
    set_mode = SUCCEEDED(UrlMkSetSessionOption(URLMON_OPTION_USERAGENT_REFRESH, nullptr, 0, 0));
  }

  return set_mode;
}

bool webfeature::disable_navigation_sound() const {
  return SUCCEEDED(CoInternetSetFeatureEnabled(
    FEATURE_DISABLE_NAVIGATION_SOUNDS, SET_FEATURE_ON_PROCESS, true));
}
