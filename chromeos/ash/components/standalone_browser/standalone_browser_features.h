// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_ASH_COMPONENTS_STANDALONE_BROWSER_STANDALONE_BROWSER_FEATURES_H_
#define CHROMEOS_ASH_COMPONENTS_STANDALONE_BROWSER_STANDALONE_BROWSER_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace ash::standalone_browser::features {

// On adding a new flag, please sort in the lexicographical order by
// the variable name.

COMPONENT_EXPORT(CHROMEOS_ASH_COMPONENTS_STANDALONE_BROWSER)
BASE_DECLARE_FEATURE(kLacrosForSupervisedUsers);

COMPONENT_EXPORT(CHROMEOS_ASH_COMPONENTS_STANDALONE_BROWSER)
BASE_DECLARE_FEATURE(kLacrosOnly);

COMPONENT_EXPORT(CHROMEOS_ASH_COMPONENTS_STANDALONE_BROWSER)
BASE_DECLARE_FEATURE(kLacrosProfileMigrationForceOff);

}  // namespace ash::standalone_browser::features

#endif  // CHROMEOS_ASH_COMPONENTS_STANDALONE_BROWSER_STANDALONE_BROWSER_FEATURES_H_
