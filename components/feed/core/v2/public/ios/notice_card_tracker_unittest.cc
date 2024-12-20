// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feed/core/v2/public/ios/notice_card_tracker.h"

#include "base/test/scoped_feature_list.h"
#include "components/feed/core/common/pref_names.h"
#include "components/feed/feed_feature_list.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ios_feed {
namespace {

class IOSNoticeCardTrackerTest : public testing::Test {
 public:
  void SetUp() override {
    feed::RegisterProfilePrefs(profile_prefs_.registry());
  }

 protected:
  TestingPrefServiceSimple profile_prefs_;
};

TEST_F(IOSNoticeCardTrackerTest,
       TrackingNoticeCardActionsDoesntUpdateCountsWhenNoNoticeCard) {
  NoticeCardTracker tracker(&profile_prefs_);
  feed::prefs::SetLastFetchHadNoticeCard(profile_prefs_, false);

  // Generate enough views to reach the acknowlegement threshold, but there was
  // no notice card in the feed.
  const int notice_card_index = 0;
  tracker.OnSliceViewed(notice_card_index);
  tracker.OnSliceViewed(notice_card_index);
  tracker.OnSliceViewed(notice_card_index);

  EXPECT_FALSE(tracker.HasAcknowledgedNoticeCard());
}

TEST_F(IOSNoticeCardTrackerTest,
       TrackingNoticeCardActionsDoesntUpdateCountsForNonNoticeCard) {
  NoticeCardTracker tracker(&profile_prefs_);

  // Generate enough views to reach the acknowlegement threshold, but the views
  // were not on the notice card.
  const int non_notice_card_index = 1;
  tracker.OnSliceViewed(non_notice_card_index);
  tracker.OnSliceViewed(non_notice_card_index);
  tracker.OnSliceViewed(non_notice_card_index);

  EXPECT_FALSE(tracker.HasAcknowledgedNoticeCard());
}

TEST_F(IOSNoticeCardTrackerTest,
       AcknowledgedNoticeCardWhenEnoughViewsAndNoticeCardAt1stPos) {
  NoticeCardTracker tracker(&profile_prefs_);

  const int notice_card_index = 0;
  tracker.OnSliceViewed(notice_card_index);
  tracker.OnSliceViewed(notice_card_index);
  tracker.OnSliceViewed(notice_card_index);

  EXPECT_TRUE(tracker.HasAcknowledgedNoticeCard());
}

TEST_F(IOSNoticeCardTrackerTest,
       DontAcknowledgedNoticeCardWhenNotEnoughViewsNorClicks) {
  NoticeCardTracker tracker(&profile_prefs_);

  // Generate views but not enough to reach the threshold.
  const int notice_card_index = 0;
  tracker.OnSliceViewed(notice_card_index);
  tracker.OnSliceViewed(notice_card_index);

  EXPECT_FALSE(tracker.HasAcknowledgedNoticeCard());
}

TEST_F(IOSNoticeCardTrackerTest,
       DontAcknowledgedNoticeCardFromViewsCountWhenThresholdIsZero) {
  NoticeCardTracker tracker(&profile_prefs_);
  EXPECT_FALSE(tracker.HasAcknowledgedNoticeCard());
}

TEST_F(IOSNoticeCardTrackerTest,
       DontAcknowledgedNoticeCardFromClicksCountWhenThresholdIsZero) {
  NoticeCardTracker tracker(&profile_prefs_);
  EXPECT_FALSE(tracker.HasAcknowledgedNoticeCard());
}

}  // namespace
}  // namespace ios_feed
