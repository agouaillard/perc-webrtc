/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_RTP_HEADERS_H_
#define API_RTP_HEADERS_H_

#include <stddef.h>
#include <string.h>
#include <string>
#include <vector>

#include "absl/types/optional.h"
#include "api/array_view.h"
#include "api/video/video_content_type.h"
#include "api/video/video_rotation.h"
#include "api/video/video_timing.h"

#include "common_types.h"  // NOLINT(build/include)
#include "rtc_base/checks.h"
#include "rtc_base/deprecation.h"
#include "typedefs.h"  // NOLINT(build/include)

namespace webrtc {

// Class to represent the value of RTP header extensions that are
// variable-length strings (e.g., RtpStreamId and RtpMid).
// Unlike std::string, it can be copied with memcpy and cleared with memset.
//
// Empty value represents unset header extension (use empty() to query).
class StringRtpHeaderExtension {
 public:
  // String RTP header extensions are limited to 16 bytes because it is the
  // maximum length that can be encoded with one-byte header extensions.
  static constexpr size_t kMaxSize = 16;

  static bool IsLegalName(rtc::ArrayView<const char> name);

  StringRtpHeaderExtension() { value_[0] = 0; }
  explicit StringRtpHeaderExtension(rtc::ArrayView<const char> value) {
    Set(value.data(), value.size());
  }
  StringRtpHeaderExtension(const StringRtpHeaderExtension&) = default;
  StringRtpHeaderExtension& operator=(const StringRtpHeaderExtension&) =
      default;

  bool empty() const { return value_[0] == 0; }
  const char* data() const { return value_; }
  size_t size() const { return strnlen(value_, kMaxSize); }

  void Set(rtc::ArrayView<const uint8_t> value) {
    Set(reinterpret_cast<const char*>(value.data()), value.size());
  }
  void Set(const char* data, size_t size);

  friend bool operator==(const StringRtpHeaderExtension& lhs,
                         const StringRtpHeaderExtension& rhs) {
    return strncmp(lhs.value_, rhs.value_, kMaxSize) == 0;
  }
  friend bool operator!=(const StringRtpHeaderExtension& lhs,
                         const StringRtpHeaderExtension& rhs) {
    return !(lhs == rhs);
  }

 private:
  char value_[kMaxSize];
};

// StreamId represents RtpStreamId which is a string.
typedef StringRtpHeaderExtension StreamId;

// Mid represents RtpMid which is a string.
typedef StringRtpHeaderExtension Mid;


// For Frame Marking RTP Header Extension:
// https://tools.ietf.org/html/draft-ietf-avtext-framemarking-05
// encrypted media payload, an RTP switch can do codec-agnostic
// selective forwarding without decrypting the payload
//   o  S: Start of Frame (1 bit) - MUST be 1 in the first packet in a
//      frame; otherwise MUST be 0.
//   o  E: End of Frame (1 bit) - MUST be 1 in the last packet in a frame;
//      otherwise MUST be 0.
//   o  I: Independent Frame (1 bit) - MUST be 1 for frames that can be
//      decoded independent of prior frames, e.g. intra-frame, VPX
//      keyframe, H.264 IDR [RFC6184], H.265 IDR/CRA/BLA/RAP [RFC7798];
//      otherwise MUST be 0.
//   o  D: Discardable Frame (1 bit) - MUST be 1 for frames that can be
//      discarded, and still provide a decodable media stream; otherwise
//      MUST be 0.
//   o  B: Base Layer Sync (1 bit) - MUST be 1 if this frame only depends
//      on the base layer; otherwise MUST be 0.  If no scalability is
//      used, this MUST be 0.
//   o  TID: Temporal ID (3 bits) - The base temporal layer starts with 0,
//      and increases with 1 for each higher temporal layer/sub-layer.  If
//      no scalability is used, this MUST be 0.
//   o  LID: Layer ID (8 bits) - Identifies the spatial and quality layer
//      encoded.  If no scalability is used, this MUST be 0 or omitted.
//      When omitted, TL0PICIDX MUST also be omitted.
//   o  TL0PICIDX: Temporal Layer 0 Picture Index (8 bits) - Running index
//      of base temporal layer 0 frames when TID is 0.  When TID is not 0,
//      this indicates a dependency on the given index.  If no scalability
//      is used, this MUST be 0 or omitted.  When omitted, LID MUST also
//      be omitted.
struct FrameMarks {
  bool start_of_frame = false;;
  bool end_of_frame = false;
  bool independent = false;
  bool discardable = false;
  bool base_layer_sync = false;
  uint8_t temporal_layer_id = 0;
  uint8_t layer_id = 0;
  int16_t tl0_pic_idx = 0;
};

struct RTPHeaderExtension {
  RTPHeaderExtension();
  RTPHeaderExtension(const RTPHeaderExtension& other);
  RTPHeaderExtension& operator=(const RTPHeaderExtension& other);

  bool hasTransmissionTimeOffset;
  int32_t transmissionTimeOffset;
  bool hasAbsoluteSendTime;
  uint32_t absoluteSendTime;
  bool hasTransportSequenceNumber;
  uint16_t transportSequenceNumber;

  // Audio Level includes both level in dBov and voiced/unvoiced bit. See:
  // https://datatracker.ietf.org/doc/draft-lennox-avt-rtp-audio-level-exthdr/
  bool hasAudioLevel;
  bool voiceActivity;
  uint8_t audioLevel;

  // For Coordination of Video Orientation. See
  // http://www.etsi.org/deliver/etsi_ts/126100_126199/126114/12.07.00_60/
  // ts_126114v120700p.pdf
  bool hasVideoRotation;
  VideoRotation videoRotation;

  // TODO(ilnik): Refactor this and one above to be absl::optional() and remove
  // a corresponding bool flag.
  bool hasVideoContentType;
  VideoContentType videoContentType;

  bool has_video_timing;
  VideoSendTiming video_timing;

  PlayoutDelay playout_delay = {-1, -1};

  // For identification of a stream when ssrc is not signaled. See
  // https://tools.ietf.org/html/draft-ietf-avtext-rid-09
  // TODO(danilchap): Update url from draft to release version.
  StreamId stream_id;
  StreamId repaired_stream_id;

  // For identifying the media section used to interpret this RTP packet. See
  // https://tools.ietf.org/html/draft-ietf-mmusic-sdp-bundle-negotiation-38
  Mid mid;

  // For Frame Marking RTP Header Extension:
  // https://tools.ietf.org/html/draft-ietf-avtext-framemarking-05
  bool has_frame_marks;
  FrameMarks frame_marks;
};

struct RTPHeader {
  RTPHeader();
  RTPHeader(const RTPHeader& other);
  RTPHeader& operator=(const RTPHeader& other);

  bool markerBit;
  uint8_t payloadType;
  uint16_t sequenceNumber;
  uint32_t timestamp;
  uint32_t ssrc;
  uint8_t numCSRCs;
  uint32_t arrOfCSRCs[kRtpCsrcSize];
  size_t paddingLength;
  size_t headerLength;
  int payload_type_frequency;
  RTPHeaderExtension extension;
};

// RTCP mode to use. Compound mode is described by RFC 4585 and reduced-size
// RTCP mode is described by RFC 5506.
enum class RtcpMode { kOff, kCompound, kReducedSize };

enum NetworkState {
  kNetworkUp,
  kNetworkDown,
};

struct RtpKeepAliveConfig final {
  // If no packet has been sent for |timeout_interval_ms|, send a keep-alive
  // packet. The keep-alive packet is an empty (no payload) RTP packet with a
  // payload type of 20 as long as the other end has not negotiated the use of
  // this value. If this value has already been negotiated, then some other
  // unused static payload type from table 5 of RFC 3551 shall be used and set
  // in |payload_type|.
  int64_t timeout_interval_ms = -1;
  uint8_t payload_type = 20;

  bool operator==(const RtpKeepAliveConfig& o) const {
    return timeout_interval_ms == o.timeout_interval_ms &&
           payload_type == o.payload_type;
  }
  bool operator!=(const RtpKeepAliveConfig& o) const { return !(*this == o); }
};

}  // namespace webrtc

#endif  // API_RTP_HEADERS_H_
