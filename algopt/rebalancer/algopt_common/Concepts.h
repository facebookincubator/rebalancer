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

#include <thrift/lib/cpp2/op/Get.h>

#include <concepts>
#include <iterator>

// some helper function that will be used to write the concepts
template <typename ThriftStructOrUnion, typename FieldType, size_t Index>
constexpr bool isFieldTypeAssignableAtIndex() {
  using Ordinal = apache::thrift::type::detail::pos_to_ordinal<Index>;
  using ThisField =
      apache::thrift::op::get_native_type<ThriftStructOrUnion, Ordinal>;
  return std::is_assignable_v<ThisField, FieldType>;
}

template <typename ThriftStructOrUnion, typename FieldType, size_t... Index>
constexpr bool isFieldTypeAssignableAtAnyIndex(std::index_sequence<Index...>) {
  return (
      ... ||
      isFieldTypeAssignableAtIndex<ThriftStructOrUnion, FieldType, Index>());
}

// some helper function that will be used to write the concepts
template <typename ThriftStructOrUnion, typename FieldType, size_t Index>
constexpr bool doesFieldTypeMatchAtIndex() {
  using Ordinal = apache::thrift::type::detail::pos_to_ordinal<Index>;
  using ThisField =
      apache::thrift::op::get_native_type<ThriftStructOrUnion, Ordinal>;
  return std::is_same_v<ThisField, FieldType>;
}

template <typename ThriftStructOrUnion, typename FieldType, size_t... Index>
constexpr bool doesFieldTypeMatchAtAnyIndex(std::index_sequence<Index...>) {
  return (
      ... ||
      doesFieldTypeMatchAtIndex<ThriftStructOrUnion, FieldType, Index>());
}

template <typename Container, typename Type1, typename Type2>
concept IsIterableOverPairs = requires(Container c) {
  requires(
      std::same_as<
          typename Container::value_type,
          std::pair<const Type1, Type2>> ||
      std::same_as<typename Container::value_type, std::pair<Type1, Type2>>);

  requires std::same_as<decltype(c.begin()), typename Container::iterator>;
  requires std::same_as<decltype(c.end()), typename Container::iterator>;
};

template <typename Container, typename Key, typename Value>
concept IsMap = requires(Container c) {
  requires std::same_as<typename Container::key_type, Key>;
  requires std::same_as<typename Container::mapped_type, Value>;
  requires std::
      same_as<typename Container::value_type, std::pair<const Key, Value>>;
  requires std::same_as<decltype(c.begin()), typename Container::iterator>;
  requires std::same_as<decltype(c.end()), typename Container::iterator>;
};

template <
    typename Container,
    typename OuterKey,
    typename InnerKey,
    typename InnerValue>
concept IsMapOfMap = requires {
  requires IsMap<Container, OuterKey, typename Container::mapped_type>;
  requires IsMap<typename Container::mapped_type, InnerKey, InnerValue>;
};

// verifies that the ThriftStructOrUnion contains a field  to which FieldType
// can be assigned
template <typename ThriftStructOrUnion, typename FieldType>
concept FieldTypeAssignableToThriftStructOrUnion = requires {
  requires isFieldTypeAssignableAtAnyIndex<ThriftStructOrUnion, FieldType>(
      std::make_integer_sequence<
          size_t,
          apache::thrift::op::num_fields<ThriftStructOrUnion>>{});
};

// verifies that the ThriftStructOrUnion contains a field of type FieldType
template <typename ThriftStructOrUnion, typename FieldType>
concept FieldTypeExistsInThriftStructOrUnion = requires {
  requires doesFieldTypeMatchAtAnyIndex<ThriftStructOrUnion, FieldType>(
      std::make_integer_sequence<
          size_t,
          apache::thrift::op::num_fields<ThriftStructOrUnion>>{});
};
