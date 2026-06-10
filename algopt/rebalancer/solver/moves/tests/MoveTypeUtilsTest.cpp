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

#include "algopt/rebalancer/solver/moves/MoveTypeUtils.h"
#include "algopt/rebalancer/solver/tests/IdConverterTestUtils.h"

#include <folly/container/irange.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class MoveTypeUtilsTest : public ::testing::Test {
  void SetUp() override {
    rng_.seed(std::random_device()());
  }

 protected:
  std::mt19937 rng_;
};

TEST_F(MoveTypeUtilsTest, RandomSampleZeroSamples) {
  std::vector<entities::ContainerId> containerIds = {
      container(1), container(2), container(3), container(4), container(5)};

  auto sampleSet = getRandomSample(containerIds, 0, rng_);
  EXPECT_EQ(sampleSet.size(), 0);
}

TEST_F(MoveTypeUtilsTest, RandomSampleWithoutContainerToExclude) {
  std::vector<entities::ContainerId> containerIds = {
      container(1), container(2), container(3), container(4), container(5)};

  auto sampleSet = getRandomSample(containerIds, 3, rng_);
  EXPECT_EQ(sampleSet.size(), 3);
  for (auto containerId : sampleSet) {
    EXPECT_TRUE(
        std::find(containerIds.begin(), containerIds.end(), containerId) !=
        containerIds.end());
  }
}

TEST_F(MoveTypeUtilsTest, RandomSampleWithContainerToExclude) {
  std::vector<entities::ContainerId> containerIds = {
      container(1), container(2), container(3), container(4), container(5)};

  auto sampleSet =
      getRandomSample(containerIds, 5, rng_, std::make_optional(container(4)));
  EXPECT_EQ(sampleSet.size(), 4);
  EXPECT_FALSE(sampleSet.contains(container(4)));
}

TEST_F(MoveTypeUtilsTest, RandomSampleWithReplacementZeroSamples) {
  std::vector<entities::ContainerId> containerIds = {
      container(1), container(2), container(3), container(4), container(5)};

  auto sampleVec = getRandomSampleWithReplacement(containerIds, 0, rng_);
  EXPECT_EQ(sampleVec.size(), 0);
}

TEST_F(
    MoveTypeUtilsTest,
    RandomSampleWithReplacementWithoutContainerToExclude) {
  std::vector<entities::ContainerId> containerIds = {
      container(1), container(2), container(3), container(4), container(5)};

  auto sampleVec = getRandomSampleWithReplacement(containerIds, 3, rng_);
  EXPECT_EQ(sampleVec.size(), 3);
  for (auto containerId : sampleVec) {
    EXPECT_TRUE(
        std::find(containerIds.begin(), containerIds.end(), containerId) !=
        containerIds.end());
  }
}

TEST_F(MoveTypeUtilsTest, RandomSampleWithReplacementWithContainerToExclude) {
  std::vector<entities::ContainerId> containerIds = {
      container(1), container(2), container(3), container(4), container(5)};

  auto sampleVec = getRandomSampleWithReplacement(
      containerIds, 5, rng_, std::make_optional(container(4)));
  EXPECT_EQ(sampleVec.size(), 5);
  for (auto containerId : sampleVec) {
    EXPECT_FALSE(containerId == container(4));
  }
}

TEST_F(
    MoveTypeUtilsTest,
    RandomSampleWithReplacementOneSampleWithContainerToExcludeFromLargeInput) {
  std::vector<entities::ContainerId> containerIds;
  containerIds.reserve(1000);
  for (const auto i : folly::irange(1000)) {
    containerIds.push_back(container(i));
  }
  auto checkExists = [&](std::vector<entities::ContainerId>& sampledContainers,
                         entities::ContainerId excludedContainer) {
    for (auto& sampledContainer : sampledContainers) {
      EXPECT_TRUE(sampledContainer != excludedContainer);
    }
  };

  auto sampleVec = getRandomSampleWithReplacement(
      containerIds, 1, rng_, std::make_optional(container(4)));
  EXPECT_EQ(sampleVec.size(), 1);
  checkExists(sampleVec, container(4));
}

TEST_F(
    MoveTypeUtilsTest,
    RandomSampleWithReplacementMany999SamplesWithContainerToExcludeFromLargeInput) {
  std::vector<entities::ContainerId> containerIds;
  containerIds.reserve(1000);
  for (const auto i : folly::irange(1000)) {
    containerIds.push_back(container(i));
  }
  auto checkExists = [&](std::vector<entities::ContainerId>& sampledContainers,
                         entities::ContainerId excludedContainer) {
    for (auto& sampledContainer : sampledContainers) {
      EXPECT_TRUE(sampledContainer != excludedContainer);
    }
  };

  auto sampleVec = getRandomSampleWithReplacement(
      containerIds, 999, rng_, std::make_optional(container(4)));
  EXPECT_EQ(sampleVec.size(), 999);
  checkExists(sampleVec, container(4));
}

TEST_F(
    MoveTypeUtilsTest,
    RandomSampleWithReplacementChecksRandomnessFromLargeInput) {
  std::vector<entities::ContainerId> containerIds;
  containerIds.reserve(1000);
  for (const auto i : folly::irange(1000)) {
    containerIds.push_back(container(i));
  }

  auto sampleVec1 = getRandomSampleWithReplacement(containerIds, 500, rng_);
  EXPECT_EQ(sampleVec1.size(), 500);

  auto sampleVec2 = getRandomSampleWithReplacement(containerIds, 500, rng_);
  EXPECT_EQ(sampleVec2.size(), 500);

  // we expect the two sets to not be identical if randomness is working as
  // expected
  EXPECT_FALSE(
      std::set(sampleVec1.begin(), sampleVec1.end()) ==
      std::set(sampleVec2.begin(), sampleVec2.end()));
}

TEST_F(
    MoveTypeUtilsTest,
    RandomSampleOneSampleWithContainerToExcludeFromLargeInput) {
  std::vector<entities::ContainerId> containerIds;
  containerIds.reserve(1000);
  for (const auto i : folly::irange(1000)) {
    containerIds.push_back(container(i));
  }

  auto sampleSet =
      getRandomSample(containerIds, 1, rng_, std::make_optional(container(4)));
  EXPECT_EQ(sampleSet.size(), 1);
  EXPECT_FALSE(sampleSet.contains(container(4)));
}

TEST_F(
    MoveTypeUtilsTest,
    RandomSampleMany999SamplesWithContainerToExcludeFromLargeInput) {
  std::vector<entities::ContainerId> containerIds;
  containerIds.reserve(1000);
  for (const auto i : folly::irange(1000)) {
    containerIds.push_back(container(i));
  }

  auto sampleSet = getRandomSample(
      containerIds, 999, rng_, std::make_optional(container(4)));
  EXPECT_EQ(sampleSet.size(), 999);
  EXPECT_FALSE(sampleSet.contains(container(4)));
}

TEST_F(MoveTypeUtilsTest, RandomSampleChecksRandomnessFromLargeInput) {
  std::vector<entities::ContainerId> containerIds;
  containerIds.reserve(1000);
  for (const auto i : folly::irange(1000)) {
    containerIds.push_back(container(i));
  }

  auto sampleSet1 = getRandomSample(containerIds, 500, rng_);
  EXPECT_EQ(sampleSet1.size(), 500);

  auto sampleSet2 = getRandomSample(containerIds, 500, rng_);
  EXPECT_EQ(sampleSet2.size(), 500);

  // we expect the two sets to not be identical if randomness is working as
  // expected
  EXPECT_FALSE(
      std::set(sampleSet1.begin(), sampleSet1.end()) ==
      std::set(sampleSet2.begin(), sampleSet2.end()));
}

TEST_F(MoveTypeUtilsTest, RandomObjectsSampleZeroSamples) {
  std::vector<entities::ObjectId> objectIds = {
      object(1), object(2), object(3), object(4), object(5)};

  auto sampleSet = getRandomSample(objectIds, 0, rng_);
  EXPECT_EQ(sampleSet.size(), 0);
}

TEST_F(MoveTypeUtilsTest, RandomObjectsSampleWithoutObjectsToExclude) {
  std::vector<entities::ObjectId> objectIds = {
      object(1), object(2), object(3), object(4), object(5)};

  auto sampleSet = getRandomSample(objectIds, 3, rng_);
  EXPECT_EQ(sampleSet.size(), 3);
  for (auto objectId : sampleSet) {
    EXPECT_TRUE(
        std::find(objectIds.begin(), objectIds.end(), objectId) !=
        objectIds.end());
  }
}

TEST_F(MoveTypeUtilsTest, RandomObjectsSampleWithObjectsToExclude) {
  std::vector<entities::ObjectId> objectIds = {
      object(1), object(2), object(3), object(4), object(5)};

  auto sampleSet =
      getRandomSample(objectIds, 5, rng_, std::make_optional(objectIds.at(4)));
  EXPECT_EQ(sampleSet.size(), 4);
  EXPECT_FALSE(sampleSet.contains(objectIds.at(4)));
  EXPECT_TRUE(sampleSet.contains(objectIds.at(3)));
}

TEST_F(MoveTypeUtilsTest, RandomObjectsSampleLargeInputOneSample) {
  std::vector<entities::ObjectId> largeObjectIds;
  largeObjectIds.reserve(10000);
  for (const auto i : folly::irange(10000)) {
    largeObjectIds.push_back(object(i));
  }

  auto sampleSet = getRandomSample(
      largeObjectIds, 1, rng_, std::make_optional(largeObjectIds.at(4)));
  EXPECT_EQ(sampleSet.size(), 1);
  EXPECT_FALSE(sampleSet.contains(largeObjectIds.at(4)));
}

TEST_F(MoveTypeUtilsTest, RandomObjectsSampleLargeInputAllSamples) {
  std::vector<entities::ObjectId> largeObjectIds;
  largeObjectIds.reserve(10000);
  for (const auto i : folly::irange(10000)) {
    largeObjectIds.push_back(object(i));
  }

  auto sampleSet = getRandomSample(
      largeObjectIds, 10000, rng_, std::make_optional(largeObjectIds.at(4)));
  EXPECT_EQ(sampleSet.size(), 9999);
  EXPECT_FALSE(sampleSet.contains(largeObjectIds.at(4)));
  EXPECT_TRUE(sampleSet.contains(largeObjectIds.at(3)));
}

TEST_F(MoveTypeUtilsTest, RandomSampleManyTimesNoIdToExclude) {
  const std::vector<entities::ContainerId> ids = {
      container(1), container(2), container(3), container(4), container(5)};

  constexpr int kTries = 100;

  std::map<entities::ContainerId, int> idToSampledCount;
  for (const auto _ : folly::irange(kTries)) {
    const auto samples = getRandomSample(ids, /*sampleSize=*/3, rng_);
    for (const auto id : samples) {
      idToSampledCount[id]++;
    }
  }

  for (const auto& [id, count] : idToSampledCount) {
    EXPECT_GE(count, 1);
  }
}

TEST_F(
    MoveTypeUtilsTest,
    RandomSampleManyTimesIdToExcludeWithinSetSampleSizeEqualsCollectionSize) {
  const std::vector<entities::ContainerId> ids = {
      container(1), container(2), container(3), container(4), container(5)};

  constexpr int kTries = 100;

  std::map<entities::ContainerId, int> idToSampledCount;
  for (const auto _ : folly::irange(kTries)) {
    const auto samples =
        getRandomSample(ids, /*sampleSize=*/5, rng_, container(2));
    for (const auto id : samples) {
      idToSampledCount[id]++;
    }
  }

  // we expect only containers 1, 3, 4, 5 to be sampled
  EXPECT_EQ(4, idToSampledCount.size());
  EXPECT_FALSE(idToSampledCount.contains(container(2)));
  for (const auto& [id, count] : idToSampledCount) {
    EXPECT_EQ(count, kTries);
  }
}

TEST_F(
    MoveTypeUtilsTest,
    RandomSampleManyTimesIdToExcludeWithinSetSampleSizeLessThanCollectionSize) {
  const std::vector<entities::ContainerId> ids = {
      container(1), container(2), container(3), container(4), container(5)};

  constexpr int kTries = 100;

  std::map<entities::ContainerId, int> idToSampledCount;
  for (const auto _ : folly::irange(kTries)) {
    const auto samples =
        getRandomSample(ids, /*sampleSize=*/3, rng_, container(2));
    for (const auto id : samples) {
      idToSampledCount[id]++;
    }
  }

  // we expect only containers 1, 3, 4, 5 to be sampled and all of them to
  // have been sampled at least once
  EXPECT_EQ(4, idToSampledCount.size());
  EXPECT_FALSE(idToSampledCount.contains(container(2)));
  for (const auto& [id, count] : idToSampledCount) {
    EXPECT_GT(count, 0);
  }
}

TEST_F(MoveTypeUtilsTest, RandomSampleManyTimesIdToExcludeOutsideOfSet) {
  const std::vector<entities::ContainerId> ids = {
      container(1), container(2), container(3), container(4), container(5)};

  constexpr int kTries = 100;

  std::map<entities::ContainerId, int> idToSampledCount;
  for (const auto _ : folly::irange(kTries)) {
    const auto samples =
        getRandomSample(ids, /*sampleSize=*/5, rng_, container(6));
    EXPECT_EQ(5, samples.size());
    for (const auto id : samples) {
      idToSampledCount[id]++;
    }
  }

  // we expect all the containers to be sampled kTries times since the one to
  // exclude is outside the set
  for (const auto& [id, count] : idToSampledCount) {
    EXPECT_EQ(count, kTries);
  }
}

TEST_F(
    MoveTypeUtilsTest,
    RandomSampleManyTimesIdToExcludeOutsideOfSetSampleSizeMinusOne) {
  const std::vector<entities::ContainerId> ids = {
      container(1), container(2), container(3), container(4), container(5)};

  constexpr int kTries = 100;

  std::map<entities::ContainerId, int> idToSampledCount;
  for (const auto _ : folly::irange(kTries)) {
    const auto samples =
        getRandomSample(ids, /*sampleSize=*/4, rng_, container(6));
    for (const auto id : samples) {
      idToSampledCount[id]++;
    }
  }

  // we expect all the containers to be sampled at least once
  EXPECT_EQ(5, idToSampledCount.size());
  for (const auto& [id, count] : idToSampledCount) {
    EXPECT_GE(count, 1);
  }
}

class EqualSizeRandomSampleTest : public MoveTypeUtilsTest {
 protected:
  static std::vector<entities::ContainerId> createVector(
      int startIndex,
      int endIndex) {
    std::vector<entities::ContainerId> containers;
    for (int i = startIndex; i < endIndex; ++i) {
      containers.push_back(container(i));
    }
    return containers;
  }

  static std::vector<entities::ObjectId> createObjectVector(
      int startIndex,
      int endIndex) {
    std::vector<entities::ObjectId> objects;
    for (int i = startIndex; i < endIndex; ++i) {
      // unless otherwise specified, all object i are in container i
      objects.push_back(object(i));
    }
    return objects;
  }

  template <typename T>
  ReferenceList<const std::vector<T>> toReferenceList(
      const std::vector<std::vector<T>>& containerVectors) {
    ReferenceList<const std::vector<T>> containerIdsList;
    for (auto& vec : containerVectors) {
      containerIdsList.push_back(std::cref(vec));
    }

    return containerIdsList;
  }

  struct SamplesInfo {
    std::map<int, int> sampleCountToSetCount;
    std::map<size_t, int> indexToSampleCount;
  };
  template <typename T>
  SamplesInfo getSamplesInfo(
      const PackerSet<T>& samples,
      const std::vector<std::vector<T>>& vectors) {
    std::map<int, int> sampleCountToSetCount;
    std::map<size_t, int> indexToSampleCount;
    for (const auto i : folly::irange(vectors.size())) {
      const auto& ids = vectors.at(i);
      int samplesFromThisSet = 0;
      for (auto& sample : samples) {
        if (std::find(ids.begin(), ids.end(), sample) != ids.end()) {
          samplesFromThisSet++;
        }
      }
      sampleCountToSetCount[samplesFromThisSet]++;
      indexToSampleCount[i] = samplesFromThisSet;
    }
    return {
        .sampleCountToSetCount = sampleCountToSetCount,
        .indexToSampleCount = indexToSampleCount};
  }
};

TEST_F(EqualSizeRandomSampleTest, BasicZeroSamples) {
  std::vector<std::vector<entities::ContainerId>> containerVectors;
  constexpr int nVectors = 5;
  containerVectors.reserve(nVectors);
  for (const auto i : folly::irange(nVectors)) {
    containerVectors.push_back(createVector(i, i + 2));
  }

  constexpr int requiredSampleSize = 0;
  auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(containerVectors), requiredSampleSize, rng_);

  EXPECT_EQ(0, samples.size());
}

TEST_F(EqualSizeRandomSampleTest, BasicAllContainerListsAreEmpty) {
  std::vector<std::vector<entities::ContainerId>> containerVectors;
  constexpr int nVectors = 5;
  containerVectors.reserve(nVectors);
  for (const auto _ : folly::irange(nVectors)) {
    containerVectors.push_back(createVector(0, 0));
  }

  constexpr int requiredSampleSize = 10;
  auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(containerVectors), requiredSampleSize, rng_);

  EXPECT_EQ(0, samples.size());
}

TEST_F(EqualSizeRandomSampleTest, BasicMixedEmptyAndNonEmptyContainerLists) {
  std::vector<std::vector<entities::ContainerId>> containerVectors;
  containerVectors.push_back(createVector(1, 2));
  containerVectors.push_back(createVector(0, 0));
  containerVectors.push_back(createVector(6, 9));
  containerVectors.push_back(createVector(20, 25));
  containerVectors.push_back(createVector(0, 0));

  constexpr int requiredSampleSize = 10;
  auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(containerVectors), requiredSampleSize, rng_);

  // only 9 samples can be taken; we expect 1, 3, 5 samples from one set
  // each, and 2 sets from which we have zero samples
  EXPECT_EQ(9, samples.size());
  auto samplesInfo = getSamplesInfo(samples, containerVectors);
  EXPECT_EQ(
      (std::map<int, int>{{0, 2}, {1, 1}, {3, 1}, {5, 1}}),
      samplesInfo.sampleCountToSetCount);

  // expect zero samples from set1 and set4 since they both are empty
  EXPECT_EQ(0, samplesInfo.indexToSampleCount[1]);
  EXPECT_EQ(0, samplesInfo.indexToSampleCount[4]);
}

TEST_F(
    EqualSizeRandomSampleTest,
    BasicTwoContainerListsWithRequiredSampleSize3) {
  std::vector<std::vector<entities::ContainerId>> containerVectors;
  containerVectors.push_back(createVector(1, 3));
  containerVectors.push_back(createVector(10, 20));

  constexpr int requiredSampleSize = 3;
  auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(containerVectors), requiredSampleSize, rng_);

  // we expect 3 samples in total; 1, 2 samples from one set
  // each
  EXPECT_EQ(3, samples.size());
  auto samplesInfo = getSamplesInfo(samples, containerVectors);
  EXPECT_EQ(
      (std::map<int, int>{{1, 1}, {2, 1}}), samplesInfo.sampleCountToSetCount);
}

TEST_F(
    EqualSizeRandomSampleTest,
    BasicThreeContainerListsRequiredSampleSize30) {
  std::vector<std::vector<entities::ContainerId>> containerVectors;
  containerVectors.push_back(createVector(10, 20));
  containerVectors.push_back(createVector(20, 30));
  containerVectors.push_back(createVector(30, 40));

  constexpr int requiredSampleSize = 30;
  auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(containerVectors), requiredSampleSize, rng_);

  // we expect 10 samples from each set
  EXPECT_EQ(30, samples.size());
  auto samplesInfo = getSamplesInfo(samples, containerVectors);
  EXPECT_EQ((std::map<int, int>{{10, 3}}), samplesInfo.sampleCountToSetCount);
}

TEST_F(EqualSizeRandomSampleTest, BasicFourContainerListsRequiredSampleSize5) {
  std::vector<std::vector<entities::ContainerId>> containerVectors;
  containerVectors.push_back(createVector(10, 20));
  containerVectors.push_back(createVector(20, 30));
  containerVectors.push_back(createVector(30, 40));
  containerVectors.push_back(createVector(40, 50));

  constexpr int requiredSampleSize = 5;
  auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(containerVectors), requiredSampleSize, rng_);

  // we expect 5 samples in total; 3 sets will have sample size 1, while one
  // will have size 2
  EXPECT_EQ(5, samples.size());
  auto samplesInfo = getSamplesInfo(samples, containerVectors);
  EXPECT_EQ(
      (std::map<int, int>{{2, 1}, {1, 3}}), samplesInfo.sampleCountToSetCount);
}

TEST_F(EqualSizeRandomSampleTest, WithContainersToExclude) {
  std::vector<std::vector<entities::ContainerId>> containerVectors;
  containerVectors.push_back({container(0)});
  containerVectors.push_back({container(1)});

  constexpr int requiredSampleSize = 1;
  const auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(containerVectors),
      requiredSampleSize,
      rng_,
      /*idToExcludeFromSample=*/std::make_optional(container(1)));

  // we expect container 0 to be sampled, since container 1 is excluded
  EXPECT_EQ(1, samples.size());
  EXPECT_EQ(container(0), *samples.begin());
}

TEST_F(EqualSizeRandomSampleTest, WithContainersToExclude2) {
  std::vector<std::vector<entities::ContainerId>> containerVectors;
  containerVectors.push_back({container(0)});
  containerVectors.push_back({container(1), container(2)});

  constexpr int requiredSampleSize = 1;
  constexpr int kTries = 100;

  std::map<entities::ContainerId, int> idToSampledCount;
  for (const auto _ : folly::irange(kTries)) {
    const auto samples = getEqualSizeRandomSamplesFromEachSetIn(
        toReferenceList(containerVectors),
        requiredSampleSize,
        rng_,
        /*idToExcludeFromSample=*/std::make_optional(container(1)));

    // we do not expect container 1 to be sampled
    const auto sampledId = *samples.begin();
    EXPECT_EQ(1, samples.size());
    EXPECT_FALSE(container(1) == sampledId);
    idToSampledCount[sampledId]++;
  }

  // we expect both container0 and container2 to be sampled at least once each
  EXPECT_EQ(2, idToSampledCount.size());
  EXPECT_GE(idToSampledCount[container(0)], 1);
  EXPECT_GE(idToSampledCount[container(2)], 1);
}

TEST_F(EqualSizeRandomSampleTest, ObjectsSamplingZeroSamples) {
  std::vector<std::vector<entities::ObjectId>> objectVectors;
  constexpr int nVectors = 5;
  objectVectors.reserve(nVectors);
  for (const auto i : folly::irange(nVectors)) {
    objectVectors.push_back(createObjectVector(i, i + 2));
  }

  constexpr int requiredSampleSize = 0;
  auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(objectVectors), requiredSampleSize, rng_);

  EXPECT_EQ(0, samples.size());
}

TEST_F(EqualSizeRandomSampleTest, ObjectsSamplingAllObjectListsAreEmpty) {
  std::vector<std::vector<entities::ObjectId>> objectVectors;
  constexpr int nVectors = 5;
  objectVectors.reserve(nVectors);
  for (const auto _ : folly::irange(nVectors)) {
    objectVectors.push_back(createObjectVector(0, 0));
  }

  constexpr int requiredSampleSize = 10;
  auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(objectVectors), requiredSampleSize, rng_);

  EXPECT_EQ(0, samples.size());
}

TEST_F(EqualSizeRandomSampleTest, ObjectsSamplingSelectSamplesFromEachSet) {
  std::vector<std::vector<entities::ObjectId>> objectVectors;
  objectVectors.push_back(createObjectVector(1, 2));
  objectVectors.push_back(createObjectVector(0, 0));
  objectVectors.push_back(createObjectVector(6, 9));
  objectVectors.push_back(createObjectVector(20, 25));
  objectVectors.push_back(createObjectVector(0, 0));

  constexpr int requiredSampleSize = 10;
  auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(objectVectors), requiredSampleSize, rng_);

  // only 9 samples can be taken; we expect 1, 3, 5 samples from one set
  // each, and 2 sets from which we have zero samples
  EXPECT_EQ(9, samples.size());
  auto samplesInfo = getSamplesInfo(samples, objectVectors);
  EXPECT_EQ(
      (std::map<int, int>{{0, 2}, {1, 1}, {3, 1}, {5, 1}}),
      samplesInfo.sampleCountToSetCount);

  // expect zero samples from set1 and set4 since they both are empty
  EXPECT_EQ(0, samplesInfo.indexToSampleCount[1]);
  EXPECT_EQ(0, samplesInfo.indexToSampleCount[4]);
}

TEST_F(
    EqualSizeRandomSampleTest,
    ObjectsSamplingTwoObjectVectorsRequiredSampleSize3) {
  std::vector<std::vector<entities::ObjectId>> objectVectors;
  objectVectors.push_back(createObjectVector(1, 3));
  objectVectors.push_back(createObjectVector(10, 20));

  constexpr int requiredSampleSize = 3;
  auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(objectVectors), requiredSampleSize, rng_);

  // we expect 3 samples in total; 1, 2 samples from one set
  // each
  EXPECT_EQ(3, samples.size());
  auto samplesInfo = getSamplesInfo(samples, objectVectors);
  EXPECT_EQ(
      (std::map<int, int>{{1, 1}, {2, 1}}), samplesInfo.sampleCountToSetCount);
}

TEST_F(
    EqualSizeRandomSampleTest,
    ObjectsSamplingThreeObjectVectorsRequiredSampleSize30) {
  std::vector<std::vector<entities::ObjectId>> objectVectors;
  objectVectors.push_back(createObjectVector(10, 20));
  objectVectors.push_back(createObjectVector(20, 30));
  objectVectors.push_back(createObjectVector(30, 40));

  constexpr int requiredSampleSize = 30;
  auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(objectVectors), requiredSampleSize, rng_);

  // we expect 10 samples from each set
  EXPECT_EQ(30, samples.size());
  auto samplesInfo = getSamplesInfo(samples, objectVectors);
  EXPECT_EQ((std::map<int, int>{{10, 3}}), samplesInfo.sampleCountToSetCount);
}

TEST_F(
    EqualSizeRandomSampleTest,
    ObjectsSamplingFourObjectVectorsRequiredSampleSize5) {
  std::vector<std::vector<entities::ObjectId>> objectVectors;
  objectVectors.push_back(createObjectVector(10, 20));
  objectVectors.push_back(createObjectVector(20, 30));
  objectVectors.push_back(createObjectVector(30, 40));
  objectVectors.push_back(createObjectVector(40, 50));

  constexpr int requiredSampleSize = 5;
  auto samples = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(objectVectors), requiredSampleSize, rng_);

  // we expect 5 samples in total; 3 sets will have sample size 1, while one
  // will have size 2
  EXPECT_EQ(5, samples.size());
  auto samplesInfo = getSamplesInfo(samples, objectVectors);
  EXPECT_EQ(
      (std::map<int, int>{{2, 1}, {1, 3}}), samplesInfo.sampleCountToSetCount);
}

TEST_F(EqualSizeRandomSampleTest, DifferentScopeItemsObjectVectors) {
  constexpr int groups = 60;
  constexpr int idsPerGroup = 1;

  std::vector<std::vector<entities::ObjectId>> objectVectors;
  objectVectors.reserve(groups);
  for (const auto i : folly::irange(groups)) {
    objectVectors.push_back(createObjectVector(i, i + idsPerGroup));
  }

  constexpr int requiredSampleSize = 10;
  auto sample1 = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(objectVectors), requiredSampleSize, rng_);
  auto sample2 = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(objectVectors), requiredSampleSize, rng_);
  auto sample3 = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(objectVectors), requiredSampleSize, rng_);
  auto sample4 = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(objectVectors), requiredSampleSize, rng_);

  EXPECT_EQ(sample1.size(), 10);
  EXPECT_EQ(sample2.size(), 10);
  EXPECT_EQ(sample3.size(), 10);
  EXPECT_EQ(sample4.size(), 10);
  std::unordered_set<entities::ObjectId> mySet;
  for (auto& sample : {sample1, sample2, sample3, sample4}) {
    for (auto container : sample) {
      mySet.insert(container);
    }
  }
  EXPECT_GT(mySet.size(), 10);
}

TEST_F(EqualSizeRandomSampleTest, DifferentScopeItemsContainerVectors) {
  constexpr int groups = 60;
  constexpr int idsPerGroup = 1;

  std::vector<std::vector<entities::ContainerId>> containerVectors;
  containerVectors.reserve(groups);
  for (const auto i : folly::irange(groups)) {
    containerVectors.push_back(createVector(i, i + idsPerGroup));
  }

  constexpr int requiredSampleSize = 10;
  auto sample1 = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(containerVectors), requiredSampleSize, rng_);
  auto sample2 = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(containerVectors), requiredSampleSize, rng_);
  auto sample3 = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(containerVectors), requiredSampleSize, rng_);
  auto sample4 = getEqualSizeRandomSamplesFromEachSetIn(
      toReferenceList(containerVectors), requiredSampleSize, rng_);

  EXPECT_EQ(sample1.size(), 10);
  EXPECT_EQ(sample2.size(), 10);
  EXPECT_EQ(sample3.size(), 10);
  EXPECT_EQ(sample4.size(), 10);
  std::unordered_set<entities::ContainerId> mySet;
  for (auto& sample : {sample1, sample2, sample3, sample4}) {
    for (auto container : sample) {
      mySet.insert(container);
    }
  }
  EXPECT_GT(mySet.size(), 10);
}

TEST_F(MoveTypeUtilsTest, RandomSampleBasicTestWithSetsZeroSamples) {
  auto containerIds = folly::F14FastSet<entities::ContainerId>{
      container(1), container(2), container(3), container(4), container(5)};

  auto sampleSet = getRandomSample(containerIds, 0, rng_);
  EXPECT_EQ(sampleSet.size(), 0);
}

TEST_F(
    MoveTypeUtilsTest,
    RandomSampleBasicTestWithSetsWithoutContainerToExclude) {
  auto containerIds = folly::F14FastSet<entities::ContainerId>{
      container(1), container(2), container(3), container(4), container(5)};

  auto sampleSet = getRandomSample(containerIds, 3, rng_);
  EXPECT_EQ(sampleSet.size(), 3);
  for (auto containerId : sampleSet) {
    EXPECT_TRUE(sampleSet.contains(containerId));
  }
}

TEST_F(MoveTypeUtilsTest, RandomSampleBasicTestWithSetsWithContainerToExclude) {
  auto containerIds = folly::F14FastSet<entities::ContainerId>{
      container(1), container(2), container(3), container(4), container(5)};

  auto sampleSet =
      getRandomSample(containerIds, 5, rng_, std::make_optional(container(4)));
  EXPECT_EQ(sampleSet.size(), 4);
  EXPECT_FALSE(sampleSet.contains(container(4)));
}

} // namespace facebook::rebalancer::packer::tests
