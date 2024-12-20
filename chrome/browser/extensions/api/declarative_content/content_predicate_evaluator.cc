// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/declarative_content/content_predicate_evaluator.h"

namespace extensions {

ContentPredicateEvaluator::~ContentPredicateEvaluator() = default;

ContentPredicateEvaluator::ContentPredicateEvaluator() = default;

ContentPredicateEvaluator::Delegate::Delegate() = default;

ContentPredicateEvaluator::Delegate::~Delegate() = default;

}  // namespace extensions
