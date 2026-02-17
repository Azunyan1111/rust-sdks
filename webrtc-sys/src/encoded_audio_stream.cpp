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

#include "livekit/encoded_audio_stream.h"

#include <memory>

#include "api/make_ref_counted.h"

namespace livekit_ffi {

EncodedAudioFrameTransformer::EncodedAudioFrameTransformer(
    rust::Box<EncodedAudioFrameObserverWrapper> observer)
    : observer_(std::move(observer)) {}

void EncodedAudioFrameTransformer::Transform(
    std::unique_ptr<webrtc::TransformableFrameInterface> frame) {
  auto data = frame->GetData();
  rust::Slice<const uint8_t> payload(data.data(), data.size());

  observer_->on_encoded_audio_frame(payload, frame->GetTimestamp(),
                                    frame->GetPayloadType(),
                                    frame->GetMimeType());

  webrtc::scoped_refptr<webrtc::TransformedFrameCallback> callback;
  {
    webrtc::MutexLock lock(&mutex_);
    callback = callback_;
  }

  if (callback) {
    callback->OnTransformedFrame(std::move(frame));
  }
}

void EncodedAudioFrameTransformer::RegisterTransformedFrameCallback(
    webrtc::scoped_refptr<webrtc::TransformedFrameCallback> callback) {
  webrtc::MutexLock lock(&mutex_);
  callback_ = std::move(callback);
}

void EncodedAudioFrameTransformer::UnregisterTransformedFrameCallback() {
  webrtc::MutexLock lock(&mutex_);
  callback_ = nullptr;
}

NativeEncodedAudioSink::NativeEncodedAudioSink(
    rust::Box<EncodedAudioFrameObserverWrapper> observer)
    : transformer_(
          webrtc::make_ref_counted<EncodedAudioFrameTransformer>(std::move(observer))) {}

std::shared_ptr<NativeEncodedAudioSink> new_native_encoded_audio_sink(
    rust::Box<EncodedAudioFrameObserverWrapper> observer) {
  return std::make_shared<NativeEncodedAudioSink>(std::move(observer));
}

void set_encoded_audio_sink(std::shared_ptr<RtpReceiver> receiver,
                            std::shared_ptr<NativeEncodedAudioSink> sink) {
  if (!receiver) {
    return;
  }

  receiver->rtc_receiver()->SetDepacketizerToDecoderFrameTransformer(
      sink ? sink->frame_transformer() : nullptr);
}

void clear_encoded_audio_sink(std::shared_ptr<RtpReceiver> receiver) {
  if (!receiver) {
    return;
  }
  receiver->rtc_receiver()->SetDepacketizerToDecoderFrameTransformer(nullptr);
}

}  // namespace livekit_ffi
