/*
 * Copyright 2025 LiveKit, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>

#include "api/frame_transformer_interface.h"
#include "api/scoped_refptr.h"
#include "livekit/rtp_receiver.h"
#include "livekit/webrtc.h"
#include "rtc_base/synchronization/mutex.h"
#include "rust/cxx.h"

namespace livekit_ffi {
class EncodedAudioFrameObserverWrapper;
class NativeEncodedAudioSink;
}
#include "webrtc-sys/src/encoded_audio_stream.rs.h"

namespace livekit_ffi {

class EncodedAudioFrameTransformer : public webrtc::FrameTransformerInterface {
 public:
  explicit EncodedAudioFrameTransformer(
      rust::Box<EncodedAudioFrameObserverWrapper> observer);

  void Transform(
      std::unique_ptr<webrtc::TransformableFrameInterface> frame) override;
  void RegisterTransformedFrameCallback(
      webrtc::scoped_refptr<webrtc::TransformedFrameCallback> callback) override;
  void UnregisterTransformedFrameCallback() override;

 private:
  friend class webrtc::RefCountedObject<EncodedAudioFrameTransformer>;
  ~EncodedAudioFrameTransformer() override = default;

  rust::Box<EncodedAudioFrameObserverWrapper> observer_;
  mutable webrtc::Mutex mutex_;
  webrtc::scoped_refptr<webrtc::TransformedFrameCallback> callback_;
};

class NativeEncodedAudioSink {
 public:
  explicit NativeEncodedAudioSink(
      rust::Box<EncodedAudioFrameObserverWrapper> observer);

  webrtc::scoped_refptr<webrtc::FrameTransformerInterface> frame_transformer()
      const {
    return transformer_;
  }

 private:
  webrtc::scoped_refptr<EncodedAudioFrameTransformer> transformer_;
};

std::shared_ptr<NativeEncodedAudioSink> new_native_encoded_audio_sink(
    rust::Box<EncodedAudioFrameObserverWrapper> observer);

void set_encoded_audio_sink(std::shared_ptr<RtpReceiver> receiver,
                            std::shared_ptr<NativeEncodedAudioSink> sink);
void clear_encoded_audio_sink(std::shared_ptr<RtpReceiver> receiver);

}  // namespace livekit_ffi
