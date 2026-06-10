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

#include "algopt/rebalancer/treeprof/Event.h"

#include <memory>
#include <string>

namespace facebook::algopt::treeprof {

constexpr int NUM_BYTES_IN_ONE_MB = 1024 * 1024;

class VisualizationFilter {
 public:
  /** returns true if the event qualifies the filter **/
  virtual bool apply(std::shared_ptr<const Event> event) = 0;
  virtual ~VisualizationFilter() = default;
};

/**
  BASE Filters that directly work on the event
*/

class PeakMemoryAtLeastXMBytes : public VisualizationFilter {
 public:
  explicit PeakMemoryAtLeastXMBytes(double threshold);
  virtual bool apply(std::shared_ptr<const Event> event) override;

 private:
  double threshold_;
};

class RuntimeAtLeastX : public VisualizationFilter {
 public:
  explicit RuntimeAtLeastX(double threshold);
  virtual bool apply(std::shared_ptr<const Event> event) override;

 private:
  double threshold_;
};

class EventNameHasWord : public VisualizationFilter {
 public:
  explicit EventNameHasWord(std::string word);
  virtual bool apply(std::shared_ptr<const Event> event) override;

 private:
  std::string word_;
};

/**
  DERIVED Filters that combine base filters using unary and binary logical
  operations
*/

class Not : public VisualizationFilter {
 public:
  explicit Not(std::shared_ptr<VisualizationFilter> filter);
  virtual bool apply(std::shared_ptr<const Event> event) override;

 private:
  std::shared_ptr<VisualizationFilter> filter_;
};

class And : public VisualizationFilter {
 public:
  explicit And(
      std::shared_ptr<VisualizationFilter> first,
      std::shared_ptr<VisualizationFilter> second);
  virtual bool apply(std::shared_ptr<const Event> event) override;

 private:
  std::shared_ptr<VisualizationFilter> first_;
  std::shared_ptr<VisualizationFilter> second_;
};

class Or : public VisualizationFilter {
 public:
  explicit Or(
      std::shared_ptr<VisualizationFilter> first,
      std::shared_ptr<VisualizationFilter> second);
  virtual bool apply(std::shared_ptr<const Event> event) override;

 private:
  std::shared_ptr<VisualizationFilter> first_;
  std::shared_ptr<VisualizationFilter> second_;
};

} // namespace facebook::algopt::treeprof
