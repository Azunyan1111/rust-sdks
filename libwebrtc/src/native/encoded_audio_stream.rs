// Copyright 2025 LiveKit, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use std::{
    pin::Pin,
    sync::Arc,
    task::{Context, Poll},
};

use cxx::SharedPtr;
use livekit_runtime::Stream;
use tokio::sync::mpsc;
use webrtc_sys::encoded_audio_stream as sys_eas;

use crate::{encoded_audio_stream::EncodedAudioFrame, rtp_receiver::RtpReceiver};

pub struct NativeEncodedAudioStream {
    native_sink: SharedPtr<sys_eas::ffi::NativeEncodedAudioSink>,
    rtp_receiver: RtpReceiver,
    frame_rx: mpsc::UnboundedReceiver<EncodedAudioFrame>,
}

impl NativeEncodedAudioStream {
    pub fn new(rtp_receiver: RtpReceiver) -> Self {
        let (frame_tx, frame_rx) = mpsc::unbounded_channel();
        let observer = Arc::new(EncodedAudioFrameObserver { frame_tx });
        let native_sink = sys_eas::ffi::new_native_encoded_audio_sink(Box::new(
            sys_eas::EncodedAudioFrameObserverWrapper::new(observer),
        ));

        sys_eas::ffi::set_encoded_audio_sink(
            rtp_receiver.handle.sys_handle.clone(),
            native_sink.clone(),
        );

        Self { native_sink, rtp_receiver, frame_rx }
    }

    pub fn receiver(&self) -> RtpReceiver {
        self.rtp_receiver.clone()
    }

    pub fn close(&mut self) {
        if !self.native_sink.is_null() {
            sys_eas::ffi::clear_encoded_audio_sink(self.rtp_receiver.handle.sys_handle.clone());
        }

        self.frame_rx.close();
    }
}

impl Drop for NativeEncodedAudioStream {
    fn drop(&mut self) {
        self.close();
    }
}

impl Stream for NativeEncodedAudioStream {
    type Item = EncodedAudioFrame;

    fn poll_next(mut self: Pin<&mut Self>, cx: &mut Context) -> Poll<Option<Self::Item>> {
        self.frame_rx.poll_recv(cx)
    }
}

pub struct EncodedAudioFrameObserver {
    frame_tx: mpsc::UnboundedSender<EncodedAudioFrame>,
}

impl sys_eas::EncodedAudioFrameObserver for EncodedAudioFrameObserver {
    fn on_encoded_audio_frame(
        &self,
        data: &[u8],
        rtp_timestamp: u32,
        payload_type: u8,
        mime_type: String,
    ) {
        let _ = self.frame_tx.send(EncodedAudioFrame {
            data: data.to_vec(),
            rtp_timestamp,
            payload_type,
            mime_type,
        });
    }
}
