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

use crate::imp::encoded_audio_stream as stream_imp;

#[derive(Debug, Clone)]
pub struct EncodedAudioFrame {
    pub data: Vec<u8>,
    pub rtp_timestamp: u32,
    pub payload_type: u8,
    pub mime_type: String,
}

#[cfg(not(target_arch = "wasm32"))]
pub mod native {
    use std::{
        fmt::{Debug, Formatter},
        pin::Pin,
        task::{Context, Poll},
    };

    use livekit_runtime::Stream;

    use super::{stream_imp, EncodedAudioFrame};
    use crate::rtp_receiver::RtpReceiver;

    pub struct NativeEncodedAudioStream {
        pub(crate) handle: stream_imp::NativeEncodedAudioStream,
    }

    impl Debug for NativeEncodedAudioStream {
        fn fmt(&self, f: &mut Formatter) -> std::fmt::Result {
            f.debug_struct("NativeEncodedAudioStream")
                .field("receiver", &self.receiver())
                .finish()
        }
    }

    impl NativeEncodedAudioStream {
        pub fn new(receiver: RtpReceiver) -> Self {
            Self { handle: stream_imp::NativeEncodedAudioStream::new(receiver) }
        }

        pub fn receiver(&self) -> RtpReceiver {
            self.handle.receiver()
        }

        pub fn close(&mut self) {
            self.handle.close()
        }
    }

    impl Stream for NativeEncodedAudioStream {
        type Item = EncodedAudioFrame;

        fn poll_next(self: Pin<&mut Self>, cx: &mut Context) -> Poll<Option<Self::Item>> {
            Pin::new(&mut self.get_mut().handle).poll_next(cx)
        }
    }
}
