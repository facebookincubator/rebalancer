// Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include "algopt/rebalancer/algopt_common/Timer.h"

#include <chrono>
#include <optional>
#include <stdexcept>
#include <utility>

namespace facebook::algopt {

[[nodiscard]] double seconds_since_epoch() {
  return std::chrono::duration_cast<std::chrono::duration<double>>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

/*
 * Note: prefer using folly::stop_watch instead.
 */
Timer::Timer(bool autoStart, NowFn now) : now_(std::move(now)) {
  if (autoStart) {
    start();
  }
}

[[nodiscard]] bool Timer::is_running() const {
  return start_time.has_value();
}

Timer::TimePoint Timer::start() {
  if (is_running()) [[unlikely]] {
    throw std::runtime_error("Timer is already running");
  }
  start_time = now_();
  return start_time.value();
}

Timer::Duration Timer::stop() {
  if (!is_running()) [[unlikely]] {
    throw std::runtime_error("Timer is not running");
  }

  cumulative_time += now_() - start_time.value();
  start_time.reset();
  return cumulative_time;
}

/// Returns the cumulative time tracked by the timer as a clock duration,
/// resets the cumulative time to 0, and starts the timer running if it was
/// previously running.
Timer::Duration Timer::reset() {
  const auto ret = get();
  cumulative_time = Duration{};
  if (is_running()) {
    start_time = now_();
  }
  return ret;
}

/// Returns the cumulative time tracked by the timer in seconds,
/// resets the cumulative time to 0, and starts the timer running.
[[nodiscard]] double Timer::resetSeconds() {
  return std::chrono::duration_cast<std::chrono::duration<double>>(reset())
      .count();
}

[[nodiscard]] Timer::Duration Timer::get() const {
  if (is_running()) {
    return cumulative_time + (now_() - start_time.value());
  } else {
    return cumulative_time;
  }
}

[[nodiscard]] double Timer::getSeconds() const {
  return std::chrono::duration_cast<std::chrono::duration<double>>(get())
      .count();
}

[[nodiscard]] double Timer::getMilliseconds() const {
  return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
             get())
      .count();
}

TimerScope::TimerScope(Timer& timer) : timer_(timer) {
  timer_.start();
}

TimerScope::~TimerScope() {
  timer_.stop();
}

} // namespace facebook::algopt
