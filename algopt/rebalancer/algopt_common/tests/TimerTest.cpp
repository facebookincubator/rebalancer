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

#include <gtest/gtest.h>

#include <chrono>

namespace facebook::algopt {

namespace {

/// A test clock whose time is advanced manually. Each instance owns its own
/// time state, so tests never share mutable statics.
class ManualClock {
 public:
  void advance(Timer::Duration d) {
    time_ += d;
  }
  Timer::TimePoint now() const {
    return time_;
  }
  /// Returns a NowFn that captures this clock by reference.
  Timer::NowFn asFn() {
    return [this] { return now(); };
  }

 private:
  Timer::TimePoint time_{};
};

} // namespace

TEST(TimerTest, SecondsSinceEpoch) {
  // This is a simple sanity check that seconds_since_epoch returns a reasonable
  // value
  const auto now = seconds_since_epoch();
  // January 1, 2020 in seconds since epoch: 1577836800
  EXPECT_GT(now, 1577836800);
  // January 1, 2040 in seconds since epoch: 2209017600
  EXPECT_LT(now, 2209017600);
}

TEST(TimerTest, DefaultConstructor) {
  const Timer timer;
  EXPECT_FALSE(timer.is_running());
}

TEST(TimerTest, AutoStartConstructor) {
  ManualClock clock;
  const Timer timer(true, clock.asFn());
  EXPECT_TRUE(timer.is_running());
}

TEST(TimerTest, StartStop) {
  Timer timer;
  EXPECT_FALSE(timer.is_running());

  timer.start();
  EXPECT_TRUE(timer.is_running());

  timer.stop();
  EXPECT_FALSE(timer.is_running());
}

TEST(TimerTest, StartAlreadyRunning) {
  ManualClock clock;
  Timer timer(true, clock.asFn());
  EXPECT_THROW(timer.start(), std::runtime_error);
}

TEST(TimerTest, StopNotRunning) {
  ManualClock clock;
  Timer timer(false, clock.asFn());
  EXPECT_THROW(timer.stop(), std::runtime_error);
}

TEST(TimerTest, GetDuration) {
  ManualClock clock;
  Timer timer(false, clock.asFn());
  timer.start();
  clock.advance(std::chrono::seconds(1));
  const auto duration = timer.get();
  EXPECT_EQ(duration.count(), 1'000'000'000);
  timer.stop();
}

TEST(TimerTest, GetSeconds) {
  ManualClock clock;
  Timer timer(false, clock.asFn());
  timer.start();
  clock.advance(std::chrono::seconds(1));
  const double seconds = timer.getSeconds();
  EXPECT_DOUBLE_EQ(seconds, 1.0);
  timer.stop();
}

TEST(TimerTest, GetMilliseconds) {
  ManualClock clock;
  Timer timer(false, clock.asFn());
  timer.start();
  clock.advance(std::chrono::milliseconds(1000));
  const double ms = timer.getMilliseconds();
  EXPECT_DOUBLE_EQ(ms, 1000.0);
  timer.stop();
}

TEST(TimerTest, ResetSeconds) {
  ManualClock clock;
  Timer timer(false, clock.asFn());
  timer.start();
  clock.advance(std::chrono::seconds(1));
  const auto duration = timer.resetSeconds();
  EXPECT_DOUBLE_EQ(duration, 1.0);
  EXPECT_TRUE(timer.is_running());

  // After reset, cumulative time should be reset to 0
  clock.advance(std::chrono::milliseconds(10));
  const auto newDuration = timer.getMilliseconds();
  EXPECT_DOUBLE_EQ(newDuration, 10.0);
}

TEST(TimerTest, ResetSecondsStaysStopped) {
  ManualClock clock;
  Timer timer(true, clock.asFn());
  clock.advance(std::chrono::milliseconds(10));
  timer.stop();
  timer.reset();
  clock.advance(std::chrono::milliseconds(10));
  const auto duration2 = timer.resetSeconds();
  EXPECT_EQ(duration2, 0);
  EXPECT_FALSE(timer.is_running());
}

TEST(TimerTest, CumulativeTime) {
  ManualClock clock;
  Timer timer(false, clock.asFn());

  // First interval
  timer.start();
  clock.advance(std::chrono::milliseconds(100));
  timer.stop();
  const auto firstDuration = timer.get();

  // Untracked time
  clock.advance(std::chrono::milliseconds(300));

  // Second interval
  timer.start();
  clock.advance(std::chrono::milliseconds(100));
  timer.stop();
  const auto totalDuration = timer.get();

  // The timer must have accumulated more time in two intervals than one.
  EXPECT_GT(totalDuration.count(), firstDuration.count());

  // The 300ms gap between intervals must NOT be counted.
  EXPECT_DOUBLE_EQ(timer.getMilliseconds(), 200.0);
}

TEST(TimerTest, TimerScope) {
  ManualClock clock;
  Timer timer(false, clock.asFn());
  EXPECT_FALSE(timer.is_running());

  {
    const TimerScope scope(timer);
    EXPECT_TRUE(timer.is_running());
    clock.advance(std::chrono::milliseconds(100));
  }

  // After scope ends, timer should be stopped.
  EXPECT_FALSE(timer.is_running());
  EXPECT_DOUBLE_EQ(timer.getMilliseconds(), 100.0);
}

TEST(TimerTest, GetWhileRunning) {
  ManualClock clock;
  Timer timer(false, clock.asFn());
  timer.start();
  clock.advance(std::chrono::milliseconds(100));

  // Timer is still running; get() should reflect elapsed time since start.
  const auto duration = timer.getMilliseconds();
  EXPECT_DOUBLE_EQ(duration, 100.0);
  EXPECT_TRUE(timer.is_running());

  timer.stop();
}

TEST(TimerTest, GetWhileStopped) {
  ManualClock clock;
  Timer timer(false, clock.asFn());
  timer.start();
  clock.advance(std::chrono::milliseconds(100));
  timer.stop();

  // Get the duration after stopping
  const auto duration1 = timer.get();

  // Call get() again after more clock advancement — should return the same
  // duration since timer is stopped.
  clock.advance(std::chrono::milliseconds(100));
  const auto duration2 = timer.get();

  EXPECT_EQ(duration1.count(), duration2.count());
}

TEST(TimerTest, FractionalSeconds) {
  ManualClock clock;
  Timer timer(/*autoStart=*/true, clock.asFn());
  clock.advance(std::chrono::milliseconds(500));
  timer.stop();

  EXPECT_DOUBLE_EQ(timer.getSeconds(), 0.5);
}

} // namespace facebook::algopt
