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

class Rebalancer < Formula
  desc "Library for balanced assignment optimization"
  homepage "https://github.com/facebookincubator/rebalancer"
  url "https://github.com/facebookincubator/rebalancer/archive/refs/tags/v0.0.0.tar.gz"
  sha256 ""
  license "Apache-2.0"

  bottle do
    root_url "https://github.com/facebookincubator/rebalancer/releases/download/v#{version}"
    sha256 cellar: :any, arm64_sonoma: ""
  end

  def install
    prefix.install Dir["*"]
  end

  test do
    assert_predicate lib/"librebalancer.dylib", :exist?
    # Verify the library loads (dlopen) without crashing.
    # We don't run a full example binary because they require XPRESS,
    # a commercial LP solver not bundled in the build.
    system "python3", "-c", "import ctypes; ctypes.CDLL('#{lib}/librebalancer.dylib')"
  end
end
