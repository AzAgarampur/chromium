// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_APP_MODE_KIOSK_APP_DATA_DELEGATE_H_
#define CHROME_BROWSER_ASH_APP_MODE_KIOSK_APP_DATA_DELEGATE_H_

#include <string>

#include "base/files/file_path.h"

namespace ash {

class KioskAppDataDelegate {
 public:
  virtual ~KioskAppDataDelegate() = default;

  // Invoked to get the root directory for storing cached icon files.
  virtual base::FilePath GetKioskAppIconCacheDir() = 0;

  // Invoked when kiosk app data or status has changed.
  virtual void OnKioskAppDataChanged(const std::string& app_id) = 0;

  // Invoked when failed to load web store data of an app.
  virtual void OnKioskAppDataLoadFailure(const std::string& app_id) = 0;

  // Invoked when the data which is stored in ExternalCache for current app is
  // damaged.
  virtual void OnExternalCacheDamaged(const std::string& app_id) = 0;
};

}  // namespace ash

#endif  // CHROME_BROWSER_ASH_APP_MODE_KIOSK_APP_DATA_DELEGATE_H_
