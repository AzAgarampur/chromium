// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SETTINGS_UI_BUNDLED_TABLE_CELL_CATALOG_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_SETTINGS_UI_BUNDLED_TABLE_CELL_CATALOG_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/settings/ui_bundled/settings_root_table_view_controller.h"

// TableCellCatalogViewController is a Debug-only settings screen which serves
// as a catalog of the various UITableViewCells that are used by the app.
@interface TableCellCatalogViewController : SettingsRootTableViewController

- (instancetype)init NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithStyle:(UITableViewStyle)style NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_SETTINGS_UI_BUNDLED_TABLE_CELL_CATALOG_VIEW_CONTROLLER_H_
