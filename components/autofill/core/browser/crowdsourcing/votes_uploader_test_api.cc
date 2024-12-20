// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/crowdsourcing/votes_uploader_test_api.h"

#include "base/run_loop.h"
#include "base/test/scoped_run_loop_timeout.h"

namespace autofill {

testing::AssertionResult VotesUploaderTestApi::FlushPendingVotes(
    base::TimeDelta timeout) {
  if (!votes_uploader_->task_runner_) {
    return testing::AssertionFailure()
           << "The BrowserAutofillManager::vote_upload_task_runner_ is not "
              "initialized yet. Maybe you need to wait for a submission?";
  }
  base::test::ScopedRunLoopTimeout run_loop_timeout(FROM_HERE, timeout);
  base::RunLoop run_loop;
  votes_uploader_->task_runner_->PostTaskAndReply(FROM_HERE, base::DoNothing(),
                                                  run_loop.QuitClosure());
  run_loop.Run();
  return run_loop.AnyQuitCalled()
             ? testing::AssertionSuccess()
             : testing::AssertionFailure() << "RunLoop timed out";
}

}  // namespace autofill
