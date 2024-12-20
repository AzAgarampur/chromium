// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/ai/on_device_translation/ai_language_detector_factory.h"

#include "third_party/blink/renderer/modules/ai/ai.h"
#include "third_party/blink/renderer/modules/ai/on_device_translation/ai_language_detector.h"

namespace blink {
AILanguageDetectorFactory::AILanguageDetectorFactory(ExecutionContext* context)
    : ExecutionContextClient(context) {}

void AILanguageDetectorFactory::Trace(Visitor* visitor) const {
  ScriptWrappable::Trace(visitor);
  ExecutionContextClient::Trace(visitor);
}

namespace {
void HandleCreateDetectorCallback(
    ScriptPromiseResolver<blink::AILanguageDetector>* resolver,
    base::expected<LanguageDetectionModel*, DetectLanguageError> maybe_model) {
  if (maybe_model.has_value()) {
    resolver->Resolve(
        MakeGarbageCollected<AILanguageDetector>(maybe_model.value()));
  } else {
    switch (maybe_model.error()) {
      case DetectLanguageError::kUnavailable:
        resolver->Reject("Model not available");
    }
  }
}
}  // namespace

ScriptPromise<AILanguageDetector> AILanguageDetectorFactory::create(
    ScriptState* script_state,
    AILanguageDetectorCreateOptions* options,
    ExceptionState& exception_state) {
  // TODO(crbug.com/349927087): Take `options` into account.
  if (!script_state->ContextIsValid()) {
    exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                      "The execution context is not valid.");
    return ScriptPromise<AILanguageDetector>();
  }

  auto* resolver =
      MakeGarbageCollected<ScriptPromiseResolver<AILanguageDetector>>(
          script_state);
  LanguageDetectionModel::Create(
      GetExecutionContext()->GetBrowserInterfaceBroker(),
      WTF::BindOnce(&HandleCreateDetectorCallback, WrapPersistent(resolver)));

  return resolver->Promise();
}

ScriptPromise<AILanguageDetectorCapabilities>
AILanguageDetectorFactory::capabilities(ScriptState* script_state) {
  auto* resolver = MakeGarbageCollected<
      ScriptPromiseResolver<AILanguageDetectorCapabilities>>(script_state);

  // Create an AILanguageDetectorCapabilities object
  auto* capabilities = MakeGarbageCollected<AILanguageDetectorCapabilities>();

  // Resolve the promise with the capabilities object
  resolver->Resolve(capabilities);

  return resolver->Promise();
}
}  // namespace blink
