// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_PREINSTALLED_WEB_APPS_GOOGLE_CHAT_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_PREINSTALLED_WEB_APPS_GOOGLE_CHAT_H_

#include "chrome/browser/web_applications/external_install_options.h"

namespace web_app {

ExternalInstallOptions GetConfigForGoogleChat(bool is_standalone,
                                              bool only_for_new_users);

}  // namespace web_app

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_PREINSTALLED_WEB_APPS_GOOGLE_CHAT_H_
