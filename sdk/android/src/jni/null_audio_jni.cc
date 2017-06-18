/*
 *  Copyright 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "webrtc/sdk/android/src/jni/audio_jni.h"

namespace webrtc_jni {

rtc::scoped_refptr<webrtc::AudioDecoderFactory> CreateAudioDecoderFactory() {
  return rtc::scoped_refptr<webrtc::AudioDecoderFactory>();
}

rtc::scoped_refptr<webrtc::AudioEncoderFactory> CreateAudioEncoderFactory() {
  return rtc::scoped_refptr<webrtc::AudioEncoderFactory>();
}

}  // namespace webrtc_jni
