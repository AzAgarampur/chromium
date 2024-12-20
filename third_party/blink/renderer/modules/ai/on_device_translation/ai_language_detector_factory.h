// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_AI_ON_DEVICE_TRANSLATION_AI_LANGUAGE_DETECTOR_FACTORY_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_AI_ON_DEVICE_TRANSLATION_AI_LANGUAGE_DETECTOR_FACTORY_H_

#include "third_party/blink/renderer/bindings/modules/v8/v8_ai_language_detector_create_options.h"
#include "third_party/blink/renderer/core/execution_context/execution_context_lifecycle_observer.h"
#include "third_party/blink/renderer/modules/ai/on_device_translation/ai_language_detector.h"
#include "third_party/blink/renderer/modules/ai/on_device_translation/ai_language_detector_capabilities.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

namespace blink {
// `ExecutionContextClient` gives us access to the browser interface broker.
class AILanguageDetectorFactory final : public ScriptWrappable,
                                        ExecutionContextClient {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit AILanguageDetectorFactory(ExecutionContext* context);

  void Trace(Visitor* visitor) const override;

  // Creates an `AILanguageDetector`, with a model ready to use.
  ScriptPromise<AILanguageDetector> create(
      ScriptState* script_state,
      AILanguageDetectorCreateOptions* options,
      ExceptionState& exception_state);

  ScriptPromise<AILanguageDetectorCapabilities> capabilities(
      ScriptState* script_state);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_AI_ON_DEVICE_TRANSLATION_AI_LANGUAGE_DETECTOR_FACTORY_H_
