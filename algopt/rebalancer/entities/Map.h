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

#ifndef REBALANCER_OSS_BUILD
#include "multifeed/shared/MFHash.h"
#else
#include <folly/container/F14Map.h>
#endif

namespace facebook {
namespace rebalancer {
namespace entities {

#ifndef REBALANCER_OSS_BUILD
template <class Key, class Value>
using Map = multifeed::MFHashMap<Key, Value>;
#else
template <class Key, class Value>
using Map = folly::F14FastMap<Key, Value>;
#endif

} // namespace entities
} // namespace rebalancer
} // namespace facebook
