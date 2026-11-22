/**
 * @file arraydb_test.cpp
 * @brief Tests for behavior specific to ArrayDb.
 *
 * @copyright GamesCrafters research and development group
 * SPDX-License-Identifier: GPL-3.0-or-later
 * (see root COPYING file or accompanying source file)
 */

#include <gtest/gtest.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

extern "C" {
#include "core/db/arraydb/arraydb.h"
#include "core/types/gamesman_status.h"
}

namespace {

constexpr Tier kActiveTier = 11;
constexpr Tier kOtherTier = 29;
using CreateTierFunc = int (*)(Tier, int64_t);

class ArrayDbTest : public ::testing::Test {
   protected:
    void SetUp() override {
        std::error_code error;
        const auto temp = std::filesystem::temp_directory_path(error);
        ASSERT_FALSE(error) << error.message();
        std::string pattern = (temp / "gamesman-arraydb-XXXXXX").string();
        const char *path = mkdtemp(pattern.data());
        ASSERT_NE(path, nullptr) << std::strerror(errno);
        sandbox_ = path;
    }

    void TearDown() override {
        if (init_attempted_) {
            Finalize();
        }
        if (!sandbox_.empty()) {
            std::error_code error;
            std::filesystem::remove_all(sandbox_, error);
            EXPECT_FALSE(error) << error.message();
        }
    }

    void Initialize(ArrayDbOptions *options = nullptr) {
        init_attempted_ = true;
        ASSERT_EQ(kArrayDb.Init("arraydb_test", 0, sandbox_.c_str(), nullptr,
                                options),
                  kSuccess);
    }

    void Finalize() {
        kArrayDb.Finalize();
        init_attempted_ = false;
    }

    void WriteArchive(Tier tier, ArrayDbOptions *options, std::string &bytes) {
        constexpr int64_t kSize = 8192;
        ASSERT_NO_FATAL_FAILURE(Initialize(options));
        ASSERT_EQ(kArrayDb.CreateSolvingTier(tier, kSize), kSuccess);
        for (Position position = 0; position < kSize; ++position) {
            ASSERT_EQ(kArrayDb.SetValueRemoteness(position, kWin, 7), kSuccess);
        }
        ASSERT_EQ(kArrayDb.FlushSolvingTier(nullptr), kSuccess);
        Finalize();

        // Inspect the encoded output to distinguish the compression settings.
        const auto archive = sandbox_ / (std::to_string(tier) + ".adb.xz");
        std::ifstream input(archive, std::ios::binary);
        ASSERT_TRUE(input.is_open()) << archive;
        bytes.assign(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
        ASSERT_FALSE(bytes.empty()) << archive;
    }

    void ExpectCreationRejection(CreateTierFunc create_active,
                                 CreateTierFunc create_attempt) {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(create_active(kActiveTier, 1), kSuccess);
        ASSERT_EQ(kArrayDb.SetValueRemoteness(0, kWin, 7), kSuccess);

        ASSERT_EQ(create_attempt(kOtherTier, 1), kRuntimeError);
        ASSERT_TRUE(kArrayDb.IsTierLoaded(kActiveTier));
        EXPECT_FALSE(kArrayDb.IsTierLoaded(kOtherTier));
        EXPECT_EQ(kArrayDb.GetValue(0), kWin);
        EXPECT_EQ(kArrayDb.GetRemoteness(0), 7);
    }

   private:
    std::filesystem::path sandbox_;
    bool init_attempted_ = false;
};

// Null options restore the documented compression defaults after custom use.
TEST_F(ArrayDbTest, NullOptionsResetCompressionDefaults) {
    ArrayDbOptions custom{};
    custom.block_size = 4096;
    custom.lzma_level = 1;
    custom.lzma_extreme = 1;
    ArrayDbOptions defaults{};
    defaults.block_size = 1 << 20;
    defaults.lzma_level = 6;
    std::string custom_archive;
    std::string implicit_archive;
    std::string explicit_archive;
    ASSERT_NO_FATAL_FAILURE(WriteArchive(1, &custom, custom_archive));
    ASSERT_NO_FATAL_FAILURE(WriteArchive(2, nullptr, implicit_archive));
    ASSERT_NO_FATAL_FAILURE(WriteArchive(3, &defaults, explicit_archive));
    EXPECT_NE(custom_archive, implicit_archive);
    EXPECT_EQ(implicit_archive, explicit_archive);
}

// An ordinary create cannot replace an active concurrent tier.
TEST_F(ArrayDbTest, OrdinaryCreationRejectsActiveConcurrentTier) {
    ExpectCreationRejection(kArrayDb.CreateConcurrentSolvingTier,
                            kArrayDb.CreateSolvingTier);
}

// A concurrent create cannot replace an active ordinary tier.
TEST_F(ArrayDbTest, ConcurrentCreationRejectsActiveOrdinaryTier) {
    ExpectCreationRejection(kArrayDb.CreateSolvingTier,
                            kArrayDb.CreateConcurrentSolvingTier);
}

// Zero allocated segment buffers still reserve the solving context.
TEST_F(ArrayDbTest, OrdinaryCreationRejectsZeroBufferContext) {
    ASSERT_NO_FATAL_FAILURE(Initialize());
    ASSERT_EQ(kArrayDb.segmentation.CreateBuffers(kActiveTier, 0, 1), kSuccess);
    EXPECT_EQ(kArrayDb.CreateSolvingTier(kOtherTier, 1), kRuntimeError);
    EXPECT_FALSE(kArrayDb.IsTierLoaded(kOtherTier));
}

}  // namespace
