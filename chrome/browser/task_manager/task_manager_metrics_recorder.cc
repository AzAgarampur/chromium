// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/task_manager/task_manager_metrics_recorder.h"

#include "base/metrics/histogram_macros.h"

namespace task_manager {

// Records a new entry for the "TaskManager.Opened" UMA metric. If StartAction
// is kOther, no event is recorded.
void RecordNewOpenEvent(StartAction type) {
  if (type == StartAction::kOther) {
    return;
  }
  UMA_HISTOGRAM_ENUMERATION(kStartActionHistogram, type);
}

void RecordCloseEvent(const base::TimeTicks& start_time,
                      const base::TimeTicks& end_time) {
  UMA_HISTOGRAM_LONG_TIMES_100(kClosedElapsedTimeHistogram,
                               end_time - start_time);
}

}  // namespace task_manager
