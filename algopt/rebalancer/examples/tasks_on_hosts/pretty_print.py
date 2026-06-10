# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# pyre-strict
from collections import defaultdict
from typing import Mapping

from terminaltables import AsciiTable


def prettyPrint(objectToContainer: Mapping[str, str]) -> None:
    containerToObjects = defaultdict(list)
    for objectName, containerName in objectToContainer.items():
        containerToObjects[containerName].append(objectName)

    table = [[], []]
    for containerName, objectNames in sorted(containerToObjects.items()):
        table[0].append(containerName)
        table[1].append("\n".join(sorted(objectNames)))

    print(AsciiTable(table).table)
