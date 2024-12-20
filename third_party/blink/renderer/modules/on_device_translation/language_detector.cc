// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/on_device_translation/language_detector.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/bindings/core/v8/idl_types.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_language_detection_result.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_language_detector.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/modules/ai/on_device_translation/ai_language_detector.h"
#include "third_party/blink/renderer/platform/bindings/exception_code.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/language_detection/language_detection_model.h"

namespace blink {

LanguageDetector::LanguageDetector(
    LanguageDetectionModel* language_detection_model)
    : language_detection_model_(language_detection_model) {}

void LanguageDetector::Trace(Visitor* visitor) const {
  visitor->Trace(language_detection_model_);
  ScriptWrappable::Trace(visitor);
}

// TODO(crbug.com/349927087): The new version is AILanguageDetector::detect().
// Delete this old version.
ScriptPromise<IDLSequence<LanguageDetectionResult>> LanguageDetector::detect(
    ScriptState* script_state,
    const WTF::String& input,
    ExceptionState& exception_state) {
  if (!script_state->ContextIsValid()) {
    exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                      "The execution context is not valid.");
    return ScriptPromise<IDLSequence<LanguageDetectionResult>>();
  }

  auto* resolver = MakeGarbageCollected<
      ScriptPromiseResolver<IDLSequence<LanguageDetectionResult>>>(
      script_state);

  language_detection_model_->DetectLanguage(
      input, WTF::BindOnce(AILanguageDetector::OnDetectComplete,
                           WrapPersistent(resolver)));
  return resolver->Promise();
}

}  // namespace blink
