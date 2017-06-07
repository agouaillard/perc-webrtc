/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_processing/test/conversational_speech/multiend_call.h"

#include <algorithm>
#include <iterator>

#include "webrtc/base/logging.h"
#include "webrtc/base/pathutils.h"

namespace webrtc {
namespace test {
namespace conversational_speech {

MultiEndCall::MultiEndCall(
    rtc::ArrayView<const Turn> timing, const std::string& audiotracks_path,
    std::unique_ptr<WavReaderAbstractFactory> wavreader_abstract_factory)
        : timing_(timing), audiotracks_path_(audiotracks_path),
          wavreader_abstract_factory_(std::move(wavreader_abstract_factory)) {
  FindSpeakerNames();
  CreateAudioTrackReaders();
  valid_ = CheckTiming();
}

MultiEndCall::~MultiEndCall() = default;

const std::set<std::string>& MultiEndCall::speaker_names() const {
  return speaker_names_;
}

const std::map<std::string, std::unique_ptr<WavReaderInterface>>&
    MultiEndCall::audiotrack_readers() const {
  return audiotrack_readers_;
}

bool MultiEndCall::valid() const {
  return valid_;
}

size_t MultiEndCall::total_duration_samples() const {
  return total_duration_samples_;
}

const std::vector<MultiEndCall::SpeakingTurn>& MultiEndCall::speaking_turns()
    const {
  return speaking_turns_;
}

void MultiEndCall::FindSpeakerNames() {
  RTC_DCHECK(speaker_names_.empty());
  for (const Turn& turn : timing_) {
    speaker_names_.emplace(turn.speaker_name);
  }
}

void MultiEndCall::CreateAudioTrackReaders() {
  RTC_DCHECK(audiotrack_readers_.empty());
  for (const Turn& turn : timing_) {
    auto it = audiotrack_readers_.find(turn.audiotrack_file_name);
    if (it != audiotrack_readers_.end())
      continue;

    // Instance Pathname to retrieve the full path to the audiotrack file.
    const rtc::Pathname audiotrack_file_path(
        audiotracks_path_, turn.audiotrack_file_name);

    // Map the audiotrack file name to a new instance of WavReaderInterface.
    std::unique_ptr<WavReaderInterface> wavreader =
        wavreader_abstract_factory_->Create(audiotrack_file_path.pathname());
    audiotrack_readers_.emplace(
        turn.audiotrack_file_name, std::move(wavreader));
  }
}

bool MultiEndCall::CheckTiming() {
  struct Interval {
    size_t begin;
    size_t end;
  };
  size_t number_of_turns = timing_.size();
  auto millisecond_to_samples = [](int ms, int sr) -> int {
    // Truncation may happen if the sampling rate is not an integer multiple
    // of 1000 (e.g., 44100).
    return ms * sr / 1000;
  };
  auto in_interval = [](size_t value, const Interval& interval) {
    return interval.begin <= value && value < interval.end;
  };
  total_duration_samples_ = 0;
  speaking_turns_.clear();

  // Begin and end timestamps for the last two turns (unit: number of samples).
  Interval second_last_turn = {0, 0};
  Interval last_turn = {0, 0};

  // Initialize map to store speaking turn indices of each speaker (used to
  // detect self cross-talk).
  std::map<std::string, std::vector<size_t>> speaking_turn_indices;
  for (const std::string& speaker_name : speaker_names_) {
    speaking_turn_indices.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(speaker_name),
        std::forward_as_tuple());
  }

  // Parse turns.
  for (size_t turn_index = 0; turn_index < number_of_turns; ++turn_index) {
    const Turn& turn = timing_[turn_index];
    auto it = audiotrack_readers_.find(turn.audiotrack_file_name);
    RTC_CHECK(it != audiotrack_readers_.end())
        << "Audio track reader not created";

    // Begin and end timestamps for the current turn.
    int offset_samples = millisecond_to_samples(
        turn.offset, it->second->SampleRate());
    std::size_t begin_timestamp = last_turn.end + offset_samples;
    std::size_t end_timestamp = begin_timestamp + it->second->NumSamples();
    LOG(LS_INFO) << "turn #" << turn_index << " " << begin_timestamp
        << "-" << end_timestamp << " ms";

    // The order is invalid if the offset is negative and its absolute value is
    // larger then the duration of the previous turn.
    if (offset_samples < 0 && -offset_samples > static_cast<int>(
        last_turn.end - last_turn.begin)) {
      LOG(LS_ERROR) << "invalid order";
      return false;
    }

    // Cross-talk with 3 or more speakers occurs when the beginning of the
    // current interval falls in the last two turns.
    if (turn_index > 1 && in_interval(begin_timestamp, last_turn)
        && in_interval(begin_timestamp, second_last_turn)) {
      LOG(LS_ERROR) << "cross-talk with 3+ speakers";
      return false;
    }

    // Append turn.
    speaking_turns_.emplace_back(
        turn.speaker_name, turn.audiotrack_file_name,
        begin_timestamp, end_timestamp);

    // Save speaking turn index for self cross-talk detection.
    RTC_DCHECK_EQ(speaking_turns_.size(), turn_index + 1);
    speaking_turn_indices[turn.speaker_name].push_back(turn_index);

    // Update total duration of the consversational speech.
    if (total_duration_samples_ < end_timestamp)
      total_duration_samples_ = end_timestamp;

    // Update and continue with next turn.
    second_last_turn = last_turn;
    last_turn.begin = begin_timestamp;
    last_turn.end = end_timestamp;
  }

  // Detect self cross-talk.
  for (const std::string& speaker_name : speaker_names_) {
    LOG(LS_INFO) << "checking self cross-talk for <"
        << speaker_name << ">";

    // Copy all turns for this speaker to new vector.
    std::vector<SpeakingTurn> speaking_turns_for_name;
    std::copy_if(speaking_turns_.begin(), speaking_turns_.end(),
                 std::back_inserter(speaking_turns_for_name),
                 [&speaker_name](const SpeakingTurn& st){
                   return st.speaker_name == speaker_name; });

    // Check for overlap between adjacent elements.
    // This is a sufficient condition for self cross-talk since the intervals
    // are sorted by begin timestamp.
    auto overlap = std::adjacent_find(
        speaking_turns_for_name.begin(), speaking_turns_for_name.end(),
        [](const SpeakingTurn& a, const SpeakingTurn& b) {
            return a.end > b.begin; });

    if (overlap != speaking_turns_for_name.end()) {
      LOG(LS_ERROR) << "Self cross-talk detected";
      return false;
    }
  }

  return true;
}

}  // namespace conversational_speech
}  // namespace test
}  // namespace webrtc
