/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/rtp_rtcp/source/rtp_header_extensions.h"

#include "webrtc/base/checks.h"
#include "webrtc/base/logging.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_cvo.h"
#include "webrtc/modules/rtp_rtcp/source/byte_io.h"

namespace webrtc {
// Absolute send time in RTP streams.
//
// The absolute send time is signaled to the receiver in-band using the
// general mechanism for RTP header extensions [RFC5285]. The payload
// of this extension (the transmitted value) is a 24-bit unsigned integer
// containing the sender's current time in seconds as a fixed point number
// with 18 bits fractional part.
//
// The form of the absolute send time extension block:
//
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |  ID   | len=2 |              absolute send time               |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
constexpr RTPExtensionType AbsoluteSendTime::kId;
constexpr uint8_t AbsoluteSendTime::kMaxValueSizeBytes;
constexpr const char* AbsoluteSendTime::kUri;

bool AbsoluteSendTime::Parse(const uint8_t* data, 
                             uint8_t length,
                             uint32_t* time_24bits) {
  RTC_DCHECK(length);
  RTC_DCHECK_LE(length,kMaxValueSizeBytes);
  *time_24bits = ByteReader<uint32_t, 3>::ReadBigEndian(data);
  return true;
}

bool AbsoluteSendTime::Write(uint8_t* data, int64_t time_ms) {
  ByteWriter<uint32_t, 3>::WriteBigEndian(data, MsTo24Bits(time_ms));
  return true;
}

// An RTP Header Extension for Client-to-Mixer Audio Level Indication
//
// https://datatracker.ietf.org/doc/draft-lennox-avt-rtp-audio-level-exthdr/
//
// The form of the audio level extension block:
//
//    0                   1
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |  ID   | len=0 |V|   level     |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
constexpr RTPExtensionType AudioLevel::kId;
constexpr uint8_t AudioLevel::kMaxValueSizeBytes;
constexpr const char* AudioLevel::kUri;

bool AudioLevel::Parse(const uint8_t* data,
                       uint8_t length,
                       bool* voice_activity,
                       uint8_t* audio_level) {
  RTC_DCHECK(length);
  RTC_DCHECK_LE(length,kMaxValueSizeBytes);
  *voice_activity = (data[0] & 0x80) != 0;
  *audio_level = data[0] & 0x7F;
  return true;
}

bool AudioLevel::Write(uint8_t* data,
                       bool voice_activity,
                       uint8_t audio_level) {
  RTC_CHECK_LE(audio_level, 0x7f);
  data[0] = (voice_activity ? 0x80 : 0x00) | audio_level;
  return true;
}

// From RFC 5450: Transmission Time Offsets in RTP Streams.
//
// The transmission time is signaled to the receiver in-band using the
// general mechanism for RTP header extensions [RFC5285]. The payload
// of this extension (the transmitted value) is a 24-bit signed integer.
// When added to the RTP timestamp of the packet, it represents the
// "effective" RTP transmission time of the packet, on the RTP
// timescale.
//
// The form of the transmission offset extension block:
//
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |  ID   | len=2 |              transmission offset              |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
constexpr RTPExtensionType TransmissionOffset::kId;
constexpr uint8_t TransmissionOffset::kMaxValueSizeBytes;
constexpr const char* TransmissionOffset::kUri;

bool TransmissionOffset::Parse(const uint8_t* data,
                               uint8_t length, 
                               int32_t* rtp_time) {
  RTC_DCHECK(length);
  RTC_DCHECK_LE(length,kMaxValueSizeBytes);
  *rtp_time = ByteReader<int32_t, 3>::ReadBigEndian(data);
  return true;
}

bool TransmissionOffset::Write(uint8_t* data, int32_t rtp_time) {
  RTC_DCHECK_LE(rtp_time, 0x00ffffff);
  ByteWriter<int32_t, 3>::WriteBigEndian(data, rtp_time);
  return true;
}

//   0                   1                   2
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |  ID   | L=1   |transport wide sequence number |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
constexpr RTPExtensionType TransportSequenceNumber::kId;
constexpr uint8_t TransportSequenceNumber::kMaxValueSizeBytes;
constexpr const char* TransportSequenceNumber::kUri;

bool TransportSequenceNumber::Parse(const uint8_t* data,
                                    uint8_t length, 
                                    uint16_t* value) {
  RTC_DCHECK(length);
  RTC_DCHECK_LE(length,kMaxValueSizeBytes);
  *value = ByteReader<uint16_t>::ReadBigEndian(data);
  return true;
}

bool TransportSequenceNumber::Write(uint8_t* data, uint16_t value) {
  ByteWriter<uint16_t>::WriteBigEndian(data, value);
  return true;
}

// Coordination of Video Orientation in RTP streams.
//
// Coordination of Video Orientation consists in signaling of the current
// orientation of the image captured on the sender side to the receiver for
// appropriate rendering and displaying.
//
//    0                   1
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |  ID   | len=0 |0 0 0 0 C F R R|
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
constexpr RTPExtensionType VideoOrientation::kId;
constexpr uint8_t VideoOrientation::kMaxValueSizeBytes;
constexpr const char* VideoOrientation::kUri;

bool VideoOrientation::Parse(const uint8_t* data,
                             uint8_t length, 
                             VideoRotation* rotation) {
  RTC_DCHECK(length);
  RTC_DCHECK_LE(length,kMaxValueSizeBytes);
  *rotation = ConvertCVOByteToVideoRotation(data[0]);
  return true;
}

bool VideoOrientation::Write(uint8_t* data, VideoRotation rotation) {
  data[0] = ConvertVideoRotationToCVOByte(rotation);
  return true;
}

bool VideoOrientation::Parse(const uint8_t* data,
                             uint8_t length, 
                             uint8_t* value) {
  RTC_DCHECK(length);
  RTC_DCHECK_LE(length,kMaxValueSizeBytes);
  *value = data[0];
  return true;
}

bool VideoOrientation::Write(uint8_t* data, uint8_t value) {
  data[0] = value;
  return true;
}

//   0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |  ID   | len=2 |   MIN delay           |   MAX delay           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
constexpr RTPExtensionType PlayoutDelayLimits::kId;
constexpr uint8_t PlayoutDelayLimits::kMaxValueSizeBytes;
constexpr const char* PlayoutDelayLimits::kUri;

bool PlayoutDelayLimits::Parse(const uint8_t* data,
                               uint8_t length,
                               PlayoutDelay* playout_delay) {
  RTC_DCHECK(length);
  RTC_DCHECK_LE(length,kMaxValueSizeBytes);
  RTC_DCHECK(playout_delay);
  uint32_t raw = ByteReader<uint32_t, 3>::ReadBigEndian(data);
  uint16_t min_raw = (raw >> 12);
  uint16_t max_raw = (raw & 0xfff);
  if (min_raw > max_raw)
    return false;
  playout_delay->min_ms = min_raw * kGranularityMs;
  playout_delay->max_ms = max_raw * kGranularityMs;
  return true;
}

bool PlayoutDelayLimits::Write(uint8_t* data,
                               const PlayoutDelay& playout_delay) {
  RTC_DCHECK_LE(0, playout_delay.min_ms);
  RTC_DCHECK_LE(playout_delay.min_ms, playout_delay.max_ms);
  RTC_DCHECK_LE(playout_delay.max_ms, kMaxMs);
  // Convert MS to value to be sent on extension header.
  uint32_t min_delay = playout_delay.min_ms / kGranularityMs;
  uint32_t max_delay = playout_delay.max_ms / kGranularityMs;
  ByteWriter<uint32_t, 3>::WriteBigEndian(data, (min_delay << 12) | max_delay);
  return true;
}


// For Frame Marking RTP Header Extension:
// 
// https://tools.ietf.org/html/draft-ietf-avtext-framemarking-04#page-4
// This extensions provides meta-information about the RTP streams outside the
// encrypted media payload, an RTP switch can do codec-agnostic
// selective forwarding without decrypting the payload
//
// for Non-Scalable Streams
// 
//     0                   1
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |  ID=? |  L=0  |S|E|I|D|0 0 0 0|
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// for Scalable Streams
// 
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |  ID=? |  L=2  |S|E|I|D|B| TID |   LID         |    TL0PICIDX  |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
constexpr RTPExtensionType FrameMarking::kId;
constexpr uint8_t FrameMarking::kMaxValueSizeBytes;
constexpr const char* FrameMarking::kUri;

bool FrameMarking::Parse(const uint8_t* data,
                         uint8_t length,
                         FrameMarks* frame_marks) {
  RTC_DCHECK(frame_marks);
  RTC_DCHECK(length);
  RTC_DCHECK_LE(length,kMaxValueSizeBytes);
  // Set frame marking data
  frame_marks->startOfFrame = data[0] & 0x80;
  frame_marks->endOfFrame = data[0] & 0x40;
  frame_marks->independent = data[0] & 0x20;
  frame_marks->discardable = data[0] & 0x10;
  
  // Check variable length
  if (length==1) {
    // We are non-scalable
    frame_marks->baseLayerSync = 0;
    frame_marks->temporalLayerId = 0;
    frame_marks->spatialLayerId = 0;
    frame_marks->tl0PicIdx = 0;
  } else if (length==3) {
    // Set scalable parts
    frame_marks->baseLayerSync = data[0] & 0x08;
    frame_marks->temporalLayerId = data[0] & 0x07;
    frame_marks->spatialLayerId = data[1];
    frame_marks->tl0PicIdx = data[2]; 
  } else {
    // Incorrect length
    return false;
  } 
  return true;
}

uint8_t FrameMarking::GetSize(const FrameMarks& frame_marks)
{
  // Check if it is scalable
  if (frame_marks.baseLayerSync
      || (frame_marks.temporalLayerId
            && frame_marks.temporalLayerId != kNoTemporalIdx)
      || (frame_marks.spatialLayerId 
            && frame_marks.spatialLayerId != kNoSpatialIdx)
      || (frame_marks.tl0PicIdx
            && frame_marks.tl0PicIdx != (uint8_t)kNoTl0PicIdx)
  )
    return 3;
  else
    return 1;
}

bool FrameMarking::Write(uint8_t* data, const FrameMarks& frame_marks) {
  data[0] = frame_marks.startOfFrame ? 0x80 : 0x00;
  data[0] |= frame_marks.endOfFrame ? 0x40 : 0x00;
  data[0] |= frame_marks.independent ? 0x20 : 0x00;
  data[0] |= frame_marks.discardable ? 0x10 : 0x00;
  
  // Check if it is scalable
  if (frame_marks.baseLayerSync
       || (frame_marks.temporalLayerId 
            && frame_marks.temporalLayerId != kNoTemporalIdx)
       || (frame_marks.spatialLayerId
            && frame_marks.spatialLayerId != kNoSpatialIdx)
       || (frame_marks.tl0PicIdx 
            && frame_marks.tl0PicIdx != (uint8_t)kNoTl0PicIdx)
   ){
    data[0] |= frame_marks.baseLayerSync ? 0x08 : 0x00;
    data[0] |= (frame_marks.temporalLayerId & 0x07);
    data[1] = frame_marks.spatialLayerId;
    data[2] = frame_marks.tl0PicIdx;
  }
  return true;
}

}  // namespace webrtc
