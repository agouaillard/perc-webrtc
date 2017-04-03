/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_DOUBLE_PERC_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_DOUBLE_PERC_H_

#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "webrtc/typedefs.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_packet.h"

// Forward declaration to avoid pulling in libsrtp headers here
struct srtp_ctx_t_;

namespace webrtc {

class DoublePERC {
 public:
  DoublePERC();
  ~DoublePERC();

  
  bool SetOutboundKey(int cs, const uint8_t* key, size_t len);
  bool SetInboundKey(int cs, const uint8_t* key, size_t len);
  bool Encrypt(rtp::Packet *packet);
  bool Decrypt(uint8_t* payload,size_t* payload_length);
  
 private:
  bool SetKey(int type, int cs, const uint8_t* key, size_t len);
  bool ProtectRtp(void* data, int in_len, int max_len, int* out_len);
  bool UnprotectRtp(void* data, int in_len, int* out_len);  
  srtp_ctx_t_* session_;
  int rtp_auth_tag_len_;
  int rtcp_auth_tag_len_;
  RTC_DISALLOW_COPY_AND_ASSIGN(DoublePERC);  
};

}
#endif /* WEBRTC_MODULES_RTP_RTCP_SOURCE_DOUBLE_PERC_H_ */

