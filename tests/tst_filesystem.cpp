/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "cxxkit/tools/filesystem.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace cxxkit;

TEST(TemporaryDirectory, testTempDir)
{
    filesystem::path tempPath;
    {
        // TemporaryDirectory t;
        // tempPath = t.path();
        // REQUIRE(fs::exists(fs::path(t.path())));
        // REQUIRE(fs::is_directory(t.path()));
    }
    // REQUIRE(!fs::exists(tempPath));
}
