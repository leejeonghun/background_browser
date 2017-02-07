// Copyright (c) 2016, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#ifndef BACKGROUND_BROWSER_WEBFEATURE_H_
#define BACKGROUND_BROWSER_WEBFEATURE_H_

#include "useragent.h"

class webfeature final {
  friend class webcontrol;

 private:
  webfeature();
  ~webfeature() = default;

 public:
  bool set_mobile_mode(bool mobile_mode) const;
  inline const useragent& get_useragent() const { return useragent_; }

 private:
  inline bool disable_navigation_sound() const;

  const useragent useragent_;
};

#endif  // BACKGROUND_BROWSER_WEBFEATURE_H_
