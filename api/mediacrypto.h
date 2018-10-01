/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_MEDIACRYPTO_H_
#define API_MEDIACRYPTO_H_

#include "api/mediatypes.h"

namespace webrtc {

class MediaCrypto {
 public:
  virtual ~MediaCrypto() = default;
  virtual bool Encrypt(cricket::MediaType type, uint32_t ssrc, bool first,
                       bool last, bool is_intra, uint8_t* payload,
                       size_t* payload_size) = 0;
  virtual size_t GetMaxEncryptionOverhead() = 0;

  virtual bool Decrypt(cricket::MediaType type, uint32_t ssrc, uint8_t* payload,
                       size_t* payload_size) = 0;

};

}  // namespace webrtc

#endif  // API_MEDIACRYPTO_H_
