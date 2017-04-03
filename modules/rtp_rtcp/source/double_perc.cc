/*
 *  Copyright 2009 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/rtp_rtcp/source/double_perc.h"

#include <string.h>

#include "third_party/libsrtp/include/srtp.h"
#include "webrtc/modules/rtp_rtcp/source/double_perc.h"
#include "webrtc/base/sslstreamadapter.h"

/* OHB data
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |M|     PT      |       sequence number         |  timestamp    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                  timestamp                    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
static size_t ohb_size = 7;

namespace webrtc {
  
DoublePERC::DoublePERC()
    : session_(nullptr),
      rtp_auth_tag_len_(0),
      rtcp_auth_tag_len_(0) {
}

DoublePERC::~DoublePERC() {
  if (session_) {
    srtp_set_user_data(session_, nullptr);
    srtp_dealloc(session_);
  }
}
bool DoublePERC::SetOutboundKey(int cs, const uint8_t* key, size_t len) {
  return SetKey(ssrc_any_inbound, cs, key, len);
}

bool DoublePERC::SetInboundKey(int cs, const uint8_t* key, size_t len) {
  return SetKey(ssrc_any_outbound, cs, key, len);
}

bool DoublePERC::SetKey(int type, int cs, const uint8_t* key, size_t len) {

  if (session_) {
    LOG(LS_ERROR) << "Failed to create SRTP session: "
                  << "SRTP session already created";
    return false;
  }

  srtp_policy_t policy;
  memset(&policy, 0, sizeof(policy));
  if (cs == rtc::SRTP_AES128_CM_SHA1_80) {
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);
  } else if (cs == rtc::SRTP_AES128_CM_SHA1_32) {
    // RTP HMAC is shortened to 32 bits, but RTCP remains 80 bits.
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy.rtp);
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);
  } else if (cs == rtc::SRTP_AEAD_AES_128_GCM ) {
    srtp_crypto_policy_set_aes_gcm_128_16_auth(&policy.rtp);
    srtp_crypto_policy_set_aes_gcm_128_16_auth(&policy.rtcp);
  } else if (cs == rtc::SRTP_AEAD_AES_256_GCM ) {
    srtp_crypto_policy_set_aes_gcm_256_16_auth(&policy.rtp);
    srtp_crypto_policy_set_aes_gcm_256_16_auth(&policy.rtcp);
  } else {
    LOG(LS_WARNING) << "Failed to create SRTP session: unsupported"
                    << " cipher_suite " << cs;
    return false;
  }

  int expected_key_len;
  int expected_salt_len;
  if (!rtc::GetSrtpKeyAndSaltLengths(cs, &expected_key_len,
      &expected_salt_len)) {
    // This should never happen.
    LOG(LS_WARNING) << "Failed to create SRTP session: unsupported"
                    << " cipher_suite without length information" << cs;
    return false;
  }

  if (!key ||
      len != static_cast<size_t>(expected_key_len + expected_salt_len)) {
    LOG(LS_WARNING) << "Failed to create SRTP session: invalid key";
    return false;
  }

  policy.ssrc.type = static_cast<srtp_ssrc_type_t>(type);
  policy.ssrc.value = 0;
  policy.key = const_cast<uint8_t*>(key);
  // TODO(astor) parse window size from WSH session-param
  policy.window_size = 1024;
  policy.allow_repeat_tx = 1;

  policy.next = nullptr;

  int err = srtp_create(&session_, &policy);
  if (err != srtp_err_status_ok) {
    session_ = nullptr;
    LOG(LS_ERROR) << "Failed to create SRTP session, err=" << err;
    return false;
  }

  srtp_set_user_data(session_, this);
  rtp_auth_tag_len_ = policy.rtp.auth_tag_len;
  rtcp_auth_tag_len_ = policy.rtcp.auth_tag_len;
  return true;
}

bool DoublePERC::ProtectRtp(void* p, int in_len, int max_len, int* out_len) {
  if (!session_) {
    LOG(LS_WARNING) << "Failed to protect SRTP packet: no SRTP Session";
    return false;
  }

  int need_len = in_len + rtp_auth_tag_len_;  // NOLINT
  if (max_len < need_len) {
    LOG(LS_WARNING) << "Failed to protect SRTP packet: The buffer length "
                    << max_len << " is less than the needed " << need_len;
    return false;
  }

  *out_len = in_len;
  int err = srtp_protect(session_, p, out_len);
  if (err != srtp_err_status_ok) {
    LOG(LS_WARNING) << "Failed to encrypt double packet";
    return false;
  }
  return true;
}


bool DoublePERC::UnprotectRtp(void* p, int in_len, int* out_len) {
  
  if (!session_) {
    LOG(LS_WARNING) << "Failed to unprotect SRTP packet: no SRTP Session";
    return false;
  }

  *out_len = in_len;
  int err = srtp_unprotect(session_, p, out_len);

  if (err != srtp_err_status_ok) {
    LOG(LS_WARNING) << "Failed to unprotect SRTP packet, err=" << err;
    return false;
  }
  return true;
}

bool DoublePERC::Encrypt(rtp::Packet *packet)
{
  // Calculate payload size for encrypted version
  size_t size = ohb_size + packet->payload_size() + rtp_auth_tag_len_;
  
  //Check it is enought
  if (size>packet->MaxPayloadSize()) {
    LOG(LS_WARNING) << "Failed to perform DOUBLE PERC"
      << " encrypted size wille exceed max payload size available";
    return false;
  }
  // Alloc temporal buffer for encryption
  uint8_t* inner = (uint8_t*) malloc(size + 1);
  const uint8_t* data = packet->data();
  
  // Innert RTP packet has no padding,csrcs or extensions
  inner[0] = 0x80;
  // Copy the rest of the header
  memcpy(inner + 1, data + 1, ohb_size);
  //Copy the rest of the payload
  memcpy(inner + 1 + ohb_size, packet->payload().data(), packet->payload_size());
  
  // Protect inner rtp packet
  int out_len;
  bool result = ProtectRtp(inner,
                           1 + ohb_size + packet->payload_size(),
                           size,
                           &out_len);
  
  //Set encrypted payload
  if (result) {
    // Allocate new payload size
    uint8_t* buffer = packet->AllocatePayload(out_len - 1);
    
    if (buffer) {
      // Copy the encrypted inner rtp packet except first byte  of rtp header
      memcpy(buffer, inner + 1, out_len - 1);
      // Set new payload size
      packet->SetPayloadSize(out_len - ohb_size);
    } else {
        LOG(LS_WARNING) << "Failed to perform DOUBLE PERC"
          << " could not allocate payload for encrypted data";
        result = false;
    }
  }
  
  //Free aux
  free(inner);
  
  return result;
}

bool DoublePERC::Decrypt(uint8_t* payload,size_t* payload_length) {
  //Check we have enought data on payload
  if (*payload_length < ohb_size + rtp_auth_tag_len_) {
    LOG(LS_WARNING) << "Failed to perform DOUBLE PERC"
      << " encrypted payload is smaller than the minimum possible";
    return false;
  }
  
  // Alloc temporal buffer for decryption
  uint8_t* inner = (uint8_t*) malloc(*payload_length + 1);
  
   // Reconstruct RTP header
  inner[0] = 0x80;
  // Copy the rest of the header
  memcpy(inner+1, payload, *payload_length);
  
  // UnProtect inner rtp packet
  int out_length;
  bool result = UnprotectRtp(inner,
                           1 + *payload_length,
                           &out_length);
 //Set decyrpted payload
  if (result) {
    // Remove the OHB data
    *payload_length = out_length - ohb_size;
    // Copy the encrypted inner rtp packet except first byte  of rtp header
    memcpy(payload, inner + ohb_size, *payload_length);
  } else {
      LOG(LS_WARNING) << "Failed to perform DOUBLE PERC"
        << " could not allocate payload for decrypted data";
      result = false;
  }
  
  //Free aux
  free(inner);
  
  return result;
}

}