// Copyright (c) 2015, jeonghun. All rights reserved. Use of this source code
// is governed by a BSD-style license that can be found in the LICENSE file.

#include <cstdint>
#include <iostream>
#include <string>
#include <regex>  // NOLINT
#include "background_browser/webdriver.h"

int wmain() {
  setlocale(LC_ALL, "");

  webdriver webdrv;
  if (webdrv.init()) {
    int32_t errcode = 0;
    if (webdrv.navigate(L"http://www.google.com", &errcode) && errcode == 0) {
      const wchar_t script[] = L"document.getElementById('lst-ib').value="
        L"'github';"
        L"document.f.submit();";
      if (webdrv.execute_script(script, true)) {
        std::wstring search_result = webdrv.get_html();
        size_t find_pos = search_result.find(L"<h3 class=\"r\">");
        if (find_pos != std::string::npos) {
          std::wregex pattern(L"href=['\"]?([^'\"]+)\"");
          auto iter = std::wsregex_iterator(search_result.begin() + find_pos,
            search_result.end(), pattern);
          std::wsregex_iterator end;
          if (iter != end) {
            std::wsmatch match = *iter;
            std::wcout << match.str() << std::endl;
          }
        }
      }
    }
  }

  return 0;
}
