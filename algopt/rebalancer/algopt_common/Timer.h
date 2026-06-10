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

#pragma once

#include <chrono>
#include <functional>
#include <optional>

namespace facebook::algopt {

[[nodiscard]] double seconds_since_epoch();

/*
 * Note: prefer using folly::stop_watch instead.
 */
class Timer {
 public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;
  using Duration = Clock::duration;
  using NowFn = std::function<TimePoint()>;

  explicit Timer(bool autoStart = false, NowFn now = Clock::now);

  [[nodiscard]] bool is_running() const;

  TimePoint start();
  Duration stop();

  /// Returns the cumulative time tracked by the timer as a clock duration and
  /// resets the cumulative time to 0.
  Duration reset();

  /// Returns the cumulative time tracked by the timer in seconds and resets the
  /// cumulative time to 0.
  [[nodiscard]] double resetSeconds();

  [[nodiscard]] Duration get() const;
  [[nodiscard]] double getSeconds() const;
  [[nodiscard]] double getMilliseconds() const;

 private:
  NowFn now_;
  std::optional<std::chrono::time_point<Clock>> start_time;
  Duration cumulative_time{};
};

class TimerScope {
 public:
  explicit TimerScope(Timer& timer);
  ~TimerScope();

  TimerScope(const TimerScope&) = delete;
  TimerScope& operator=(const TimerScope&) = delete;
  TimerScope(TimerScope&&) = delete;
  TimerScope& operator=(TimerScope&&) = delete;

 private:
  Timer& timer_;
};

} // namespace facebook::algopt
