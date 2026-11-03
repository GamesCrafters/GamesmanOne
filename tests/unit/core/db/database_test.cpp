/**
 * @file database_test.cpp
 * @author OpenAI Codex
 * @brief Implementation-independent Database interface tests.
 *
 * @copyright GamesCrafters research and development group
 * SPDX-License-Identifier: GPL-3.0-or-later
 * (see root COPYING file or accompanying source file)
 */

#include <gtest/gtest.h>
#include <sys/resource.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <climits>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <ostream>
#include <random>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

extern "C" {
#include "core/constants.h"
#include "core/db/arraydb/arraydb.h"
#include "core/db/naivedb/naivedb.h"
#include "core/types/database/database.h"
#include "core/types/gamesman_status.h"
}

namespace {

constexpr char kChildRootVariable[] = "GAMESMAN_DATABASE_TEST_TEMP_ROOT";
constexpr int kSkippedChildExitCode = 77;
constexpr Tier kStoredTier = 11;
constexpr Tier kMissingTier = 29;
constexpr int kSegmentBufferBudget = 16;
constexpr int kSegmentSize = 17;
constexpr Value kLegalValues[] = {kUndecided, kLose, kDraw, kTie, kWin};
constexpr int kRepresentativeRemotenesses[] = {0, 1, kRemotenessMax / 2,
                                               kRemotenessMax};
constexpr int kBoundaryRemotenesses[] = {0, kRemotenessMax};

using CreateTierFunc = int (*)(Tier, int64_t);
using RecordCompare = int (*)(Value, int, Value, int);

struct DatabaseImplementation {
    const Database *database;
    const char *label;
};

void PrintTo(const DatabaseImplementation &implementation,
             std::ostream *output) {
    *output << implementation.label;
}

struct ExpectedRecord {
    Value value;
    int remoteness;
};

void PrintTo(const ExpectedRecord &record, std::ostream *output) {
    *output << '(' << record.value << ", " << record.remoteness << ')';
}

// Supplies a valid callback without depending on a database's file layout.
int GetTestTierName(Tier tier, char name[kDbFileNameLengthMax + 1]) {
    const int length =
        std::snprintf(name, kDbFileNameLengthMax + 1, "tier_%" PRITier, tier);
    return length >= 0 && length <= kDbFileNameLengthMax ? kSuccess
                                                         : kRuntimeError;
}

std::vector<ExpectedRecord> AllRecords() {
    std::vector<ExpectedRecord> records;
    records.reserve(kNumValues * (kRemotenessMax + 1));
    for (Value value : kLegalValues) {
        for (int remoteness = 0; remoteness <= kRemotenessMax; ++remoteness) {
            records.push_back({value, remoteness});
        }
    }
    return records;
}

std::vector<ExpectedRecord> UndecidedRecords(int first_count) {
    std::vector<ExpectedRecord> records;
    for (int count = first_count; count <= kRemotenessMax; ++count) {
        records.push_back({kUndecided, count});
    }
    return records;
}

std::vector<ExpectedRecord> DecidedRecords() {
    std::vector<ExpectedRecord> records;
    for (const auto &record : AllRecords()) {
        if (record.value != kUndecided) {
            records.push_back(record);
        }
    }
    return records;
}

// These callbacks define public orderings without accessing the database.
int ComparePairs(Value v1, int r1, Value v2, int r2) {
    if (v1 != v2) {
        return v1 < v2 ? -1 : 1;
    }
    return (r1 > r2) - (r1 < r2);
}

int CompareValuesOnly(Value v1, int, Value v2, int) {
    return (v1 > v2) - (v1 < v2);
}

int CompareRemotenessesOnly(Value, int r1, Value, int r2) {
    return (r1 > r2) - (r1 < r2);
}

int CompareReversedValues(Value v1, int r1, Value v2, int r2) {
    return v1 == v2 ? CompareRemotenessesOnly(v1, r1, v2, r2)
                    : (v1 < v2 ? 1 : -1);
}

int CompareSmallerRemotenessFirst(Value v1, int r1, Value v2, int r2) {
    return r1 == r2 ? CompareValuesOnly(v1, r1, v2, r2) : (r1 < r2 ? 1 : -1);
}

int CompareNonUnitSigns(Value v1, int r1, Value v2, int r2) {
    const int order = ComparePairs(v1, r1, v2, r2);
    return order == 0 ? 0 : (order < 0 ? -17 : 23);
}

int CompareExtremeSigns(Value v1, int r1, Value v2, int r2) {
    const int order = ComparePairs(v1, r1, v2, r2);
    return order == 0 ? 0 : (order < 0 ? INT_MIN : INT_MAX);
}

// One event describes one atomic public operation on a seeded position.
// Workers exclusively own their event slots; inspect histories only after join.
enum class ConcurrentRecordOperation {
    kSetValue,
    kSetRemoteness,
    kSetPair,
    kMaximize,
    kDecrement,
    kClear,
    kGetValue,
    kGetRemoteness,
    kGetCount,
};

struct ConcurrentRecordCall {
    ConcurrentRecordOperation operation;
    Position position;
    ExpectedRecord candidate = {kUndecided, 0};
    int result = 0;
    uint64_t begin = 0;
    uint64_t end = 0;
};

void PrintTo(const ConcurrentRecordCall &call, std::ostream *output) {
    constexpr const char *names[] = {
        "SetValue", "SetRemoteness", "SetValueRemoteness",
        "Maximize", "Decrement",     "Clear",
        "GetValue", "GetRemoteness", "GetCount"};
    *output << names[static_cast<size_t>(call.operation)] << '('
            << call.position << ", ";
    PrintTo(call.candidate, output);
    *output << ") returned " << call.result << " during [" << call.begin << ", "
            << call.end << ']';
}

// Maximize calls use ComparePairs. Their oracle uses a direct strict order,
// rather than calling the callback or deriving expectations from DB reads.
bool ApplyConcurrentRecordCall(const ConcurrentRecordCall &call,
                               ExpectedRecord &record) {
    int result = 0;
    switch (call.operation) {
        case ConcurrentRecordOperation::kSetValue:
            result = kSuccess;
            record.value = call.candidate.value;
            break;
        case ConcurrentRecordOperation::kSetRemoteness:
            result = kSuccess;
            record.remoteness = call.candidate.remoteness;
            break;
        case ConcurrentRecordOperation::kSetPair:
            result = kSuccess;
            record = call.candidate;
            break;
        case ConcurrentRecordOperation::kMaximize:
            result = call.candidate.value > record.value ||
                     (call.candidate.value == record.value &&
                      call.candidate.remoteness > record.remoteness);
            if (result != 0) {
                record = call.candidate;
            }
            break;
        case ConcurrentRecordOperation::kDecrement:
            result = record.value == kUndecided ? record.remoteness : 0;
            if (result > 0) {
                --record.remoteness;
            }
            break;
        case ConcurrentRecordOperation::kClear:
            result = record.value == kUndecided ? record.remoteness : 0;
            if (record.value == kUndecided) {
                record.remoteness = 0;
            }
            break;
        case ConcurrentRecordOperation::kGetValue:
            result = static_cast<int>(record.value);
            break;
        case ConcurrentRecordOperation::kGetRemoteness:
            result = record.remoteness;
            break;
        case ConcurrentRecordOperation::kGetCount:
            result = record.value == kUndecided ? record.remoteness : 0;
            break;
    }
    return result == call.result;
}

// Tiny histories permit exhaustive legal serialization. Returning false for an
// oversized/malformed history avoids silently weakening the oracle.
bool HasLegalConcurrentRecordSerialization(
    const std::vector<ExpectedRecord> &initial,
    const std::vector<ConcurrentRecordCall> &calls,
    const std::vector<ExpectedRecord> &final) {
    constexpr size_t kMaxCalls = 8;
    if (calls.size() > kMaxCalls || initial.size() != final.size()) {
        return false;
    }
    std::vector<unsigned> predecessors(calls.size(), 0);
    for (size_t i = 0; i < calls.size(); ++i) {
        if (calls[i].begin >= calls[i].end || calls[i].position < 0 ||
            static_cast<uint64_t>(calls[i].position) >= initial.size()) {
            return false;
        }
        for (size_t j = 0; j < calls.size(); ++j) {
            if (calls[j].end < calls[i].begin) {
                predecessors[i] |= 1u << j;
            }
        }
    }
    const unsigned all = (1u << calls.size()) - 1;
    const auto search = [&](const auto &self, unsigned used,
                            const std::vector<ExpectedRecord> &state) -> bool {
        if (used == all) {
            for (size_t i = 0; i < state.size(); ++i) {
                if (state[i].value != final[i].value ||
                    state[i].remoteness != final[i].remoteness) {
                    return false;
                }
            }
            return true;
        }
        for (size_t i = 0; i < calls.size(); ++i) {
            const unsigned bit = 1u << i;
            if ((used & bit) != 0 || (predecessors[i] & ~used) != 0) {
                continue;
            }
            auto next = state;
            if (ApplyConcurrentRecordCall(calls[i], next[calls[i].position]) &&
                self(self, used | bit, next)) {
                return true;
            }
        }
        return false;
    };
    return search(search, 0, initial);
}

// Keeps re-executed children below a directory the parent can always remove.
class ScopedChildRoot {
   public:
    explicit ScopedChildRoot(const std::string &path) {
        if (const char *previous = std::getenv(kChildRootVariable)) {
            previous_ = previous;
        }
        error_ = setenv(kChildRootVariable, path.c_str(), 1);
    }

    ~ScopedChildRoot() {
        if (error_ == 0) {
            const int result =
                previous_ ? setenv(kChildRootVariable, previous_->c_str(), 1)
                          : unsetenv(kChildRootVariable);
            EXPECT_EQ(result, 0) << "Could not restore the child sandbox root.";
        }
    }

    int Error() const { return error_; }

   private:
    std::optional<std::string> previous_;
    int error_;
};

class DatabaseTest : public ::testing::TestWithParam<DatabaseImplementation> {
   protected:
    void SetUp() override {
        ASSERT_NE(GetParam().database, nullptr);
        std::error_code error;
        const char *parent_root = std::getenv(kChildRootVariable);
        const std::filesystem::path root =
            parent_root ? std::filesystem::path(parent_root)
                        : std::filesystem::temp_directory_path(error);
        ASSERT_FALSE(error) << error.message();

        const std::string pattern =
            (root / "gamesman-database-XXXXXX").string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');
        char *directory = mkdtemp(buffer.data());
        ASSERT_NE(directory, nullptr) << std::strerror(errno);
        sandbox_ = directory;
    }

    void TearDown() override {
        if (!sandbox_.empty()) {
            std::error_code error;
            std::filesystem::remove_all(sandbox_, error);
            EXPECT_FALSE(error) << error.message();
        }
    }

    const Database &Db() const { return *GetParam().database; }

    // Fresh processes also isolate implementations whose finalizers crash.
    // Re-execution avoids inheriting OpenMP runtime threads across fork.
    void RunIsolated(const std::function<void()> &body,
                     const char *skip_reason = "Runtime capability missing.") {
        ASSERT_NE(Db().Init, nullptr) << "Init is required.";
        ASSERT_NE(Db().Finalize, nullptr) << "Finalize is required.";
        ScopedChildRoot root(sandbox_);
        ASSERT_EQ(root.Error(), 0) << std::strerror(errno);
        GTEST_FLAG_SET(death_test_style, "threadsafe");

        bool skipped = false;
        const auto completed = [&skipped](int status) {
            skipped = ::testing::ExitedWithCode(kSkippedChildExitCode)(status);
            return skipped || ::testing::ExitedWithCode(EXIT_SUCCESS)(status);
        };
        ASSERT_EXIT(
            {
                const struct rlimit core_limit = {};
                if (setrlimit(RLIMIT_CORE, &core_limit) != 0) {
                    std::perror("setrlimit");
                    std::exit(EXIT_FAILURE);
                }
                body();
                const auto *result = ::testing::UnitTest::GetInstance()
                                         ->current_test_info()
                                         ->result();
                const int parts_before_cleanup = result->total_part_count();
                ReportFailures(0);
                CleanupDatabase();
                ReportFailures(parts_before_cleanup);
                std::exit(HasFailure()  ? EXIT_FAILURE
                          : IsSkipped() ? kSkippedChildExitCode
                                        : EXIT_SUCCESS);
            },
            completed, "");
        if (skipped) {
            GTEST_SKIP() << skip_reason;
        }
    }

    void Initialize(GetTierNameFunc get_tier_name = nullptr) {
        ASSERT_NO_FATAL_FAILURE(InitializeAt(sandbox_, get_tier_name));
    }

    void InitializeAt(const std::filesystem::path &path,
                      GetTierNameFunc get_tier_name = nullptr) {
        initialization_path_ = path.string();
        initialization_tier_namer_ = get_tier_name;
        initialization_attempted_ = true;
        ASSERT_EQ(Db().Init("database_test", 0, initialization_path_.c_str(),
                            get_tier_name, nullptr),
                  kSuccess);
    }

    // Every additional session sandbox remains below the parent-owned root.
    void CreatePrivateSandbox(std::filesystem::path &path) {
        const std::string pattern =
            (std::filesystem::path(sandbox_) / "session-XXXXXX").string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');
        char *directory = mkdtemp(buffer.data());
        ASSERT_NE(directory, nullptr) << std::strerror(errno);
        path = directory;
    }

    void ExpectSandboxPath() const {
        if (Db().GetPath == nullptr) {
            return;
        }
        const char *path = Db().GetPath();
        ASSERT_NE(path, nullptr);
        std::error_code error;
        const bool equivalent =
            std::filesystem::equivalent(path, initialization_path_, error);
        ASSERT_FALSE(error) << error.message();
        EXPECT_TRUE(equivalent)
            << "GetPath should identify the initialized sandbox directory.";
    }

    void ExpectFinalizedPath() const {
        if (Db().GetPath != nullptr) {
            EXPECT_EQ(Db().GetPath(), nullptr);
        }
    }

    void Finalize() {
        initialization_attempted_ = false;
        solving_tier_active_ = false;
        loaded_tiers_.clear();
        buffers_active_ = false;
        finalize_only_ = false;
        Db().Finalize();
    }

    void RestartDatabase() {
        const std::filesystem::path path = initialization_path_;
        const GetTierNameFunc get_tier_name = initialization_tier_namer_;
        Finalize();
        ASSERT_NO_FATAL_FAILURE(InitializeAt(path, get_tier_name));
    }

    // Expected errors also end normal use of the initialized database.
    void StopAfterError() { finalize_only_ = true; }

    const char *MissingCreationDependency() const {
        if (Db().CreateSolvingTier == nullptr &&
            Db().CreateConcurrentSolvingTier == nullptr) {
            return "No solving-tier creation method is supported.";
        }
        return nullptr;
    }

    const char *MissingRecordDependency() const {
        if (const char *missing = MissingCreationDependency()) {
            return missing;
        }
        if (Db().SetValueRemoteness == nullptr &&
            (Db().SetValue == nullptr || Db().SetRemoteness == nullptr)) {
            return "No complete value/remoteness writer is supported.";
        }
        return nullptr;
    }

    const char *MissingArchiveDependency() const {
        if (const char *missing = MissingRecordDependency()) {
            return missing;
        }
        return Db().FlushSolvingTier == nullptr
                   ? "FlushSolvingTier is not supported."
                   : nullptr;
    }

    const char *MissingCheckpointDependency() const {
        if (Db().CreateSolvingTier == nullptr) {
            return "Regular solving-tier creation is not supported.";
        }
        if (const char *missing = MissingRecordDependency()) {
            return missing;
        }
        return Db().CheckpointSave == nullptr
                   ? "CheckpointSave is not supported."
                   : nullptr;
    }

    void CreateTier(CreateTierFunc create, int64_t size) {
        ASSERT_NE(create, nullptr);
        ASSERT_EQ(create(kStoredTier, size), kSuccess);
        solving_tier_active_ = true;
    }

    void CreateTier(int64_t size) {
        const auto create = record_create_ ? record_create_
                            : Db().CreateSolvingTier
                                ? Db().CreateSolvingTier
                                : Db().CreateConcurrentSolvingTier;
        ASSERT_NO_FATAL_FAILURE(CreateTier(create, size));
    }

    // A mode-specific case reports independently from the other creation mode.
    void RunWithSolvingMode(CreateTierFunc create,
                            const std::function<void()> &body) {
        if (create == nullptr) {
            GTEST_SKIP() << "Requested solving-tier creation mode is not "
                            "supported.";
        }
        ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
            record_create_ = create;
            ASSERT_NO_FATAL_FAILURE(Initialize());
            body();
        }));
    }

    // Each mode gets an independent initialized lifetime, without workers.
    void RunWithSolvingModes(const std::function<void()> &body) {
        const struct {
            CreateTierFunc create;
            const char *name;
        } modes[] = {{Db().CreateSolvingTier, "ordinary"},
                     {Db().CreateConcurrentSolvingTier, "concurrent"}};
        for (const auto &mode : modes) {
            if (mode.create == nullptr) {
                continue;
            }
            SCOPED_TRACE(mode.name);
            ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
                record_create_ = mode.create;
                ASSERT_NO_FATAL_FAILURE(Initialize());
                body();
            }));
        }
    }

    void WriteRecord(Position position, const ExpectedRecord &record) {
        if (Db().SetValueRemoteness != nullptr) {
            ASSERT_EQ(Db().SetValueRemoteness(position, record.value,
                                              record.remoteness),
                      kSuccess);
        } else {
            ASSERT_NE(Db().SetValue, nullptr);
            ASSERT_NE(Db().SetRemoteness, nullptr);
            ASSERT_EQ(Db().SetValue(position, record.value), kSuccess);
            ASSERT_EQ(Db().SetRemoteness(position, record.remoteness),
                      kSuccess);
        }
    }

    void CreateAndPopulate(Tier tier,
                           const std::vector<ExpectedRecord> &records,
                           CreateTierFunc create = nullptr) {
        if (create == nullptr) {
            create = record_create_ ? record_create_
                     : Db().CreateSolvingTier
                         ? Db().CreateSolvingTier
                         : Db().CreateConcurrentSolvingTier;
        }
        ASSERT_NE(create, nullptr);
        ASSERT_EQ(create(tier, static_cast<int64_t>(records.size())), kSuccess);
        solving_tier_active_ = true;
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            const Position position = static_cast<Position>(i);
            ASSERT_NO_FATAL_FAILURE(WriteRecord(position, records[i]));
        }
    }

    void AssertRecord(Position position, const ExpectedRecord &expected) {
        SCOPED_TRACE(position);
        ASSERT_EQ(Db().GetValue(position), expected.value);
        ASSERT_EQ(Db().GetRemoteness(position), expected.remoteness);
    }

    void AssertRecords(const std::vector<ExpectedRecord> &expected) {
        for (size_t i = 0; i < expected.size(); ++i) {
            ASSERT_NO_FATAL_FAILURE(
                AssertRecord(static_cast<Position>(i), expected[i]));
        }
    }

    void AssertNeighborhood(const std::vector<ExpectedRecord> &expected,
                            size_t position) {
        const size_t first = position == 0 ? 0 : position - 1;
        const size_t last =
            position + 1 < expected.size() ? position + 1 : position;
        for (size_t i = first; i <= last; ++i) {
            ASSERT_NO_FATAL_FAILURE(
                AssertRecord(static_cast<Position>(i), expected[i]));
        }
    }

    // Serialize the script and its reference state with the same external lock.
    // Even a fatal worker assertion joins all threads before returning.
    void RunExternallySynchronizedWorkers(
        const std::function<void(int, int)> &operation) {
        constexpr int kWorkerCount = 4;
        constexpr int kStepsPerWorker = 32;
        std::mutex mutex;
        std::condition_variable ready;
        bool started = false;
        bool stopped = false;
        std::string creation_error;
        std::vector<std::thread> workers;
        workers.reserve(kWorkerCount);
        try {
            for (int worker = 0; worker < kWorkerCount; ++worker) {
                workers.emplace_back([&, worker] {
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        ready.wait(lock, [&] { return started; });
                    }
                    for (int step = 0; step < kStepsPerWorker; ++step) {
                        std::lock_guard<std::mutex> lock(mutex);
                        if (stopped) {
                            return;
                        }
                        SCOPED_TRACE(::testing::Message() << "Worker " << worker
                                                          << ", step " << step);
                        operation(worker, step);
                        if (HasFailure()) {
                            stopped = true;
                            return;
                        }
                    }
                });
            }
        } catch (const std::system_error &error) {
            creation_error = error.what();
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopped = !creation_error.empty();
            started = true;
        }
        ready.notify_all();
        for (auto &worker : workers) {
            worker.join();
        }
        ASSERT_TRUE(creation_error.empty()) << creation_error;
        ASSERT_FALSE(HasFailure())
            << "An externally synchronized worker failed.";
    }

    // The start gate joins workers only at launch; record calls remain
    // unlocked. Never leave joinable threads behind after an assertion or
    // launch failure.
    void RunConcurrentRecordWorkers(
        int worker_count, int steps,
        const std::function<void(int, int)> &operation) {
        ASSERT_GT(worker_count, 0);
        ASSERT_GT(steps, 0);
        std::mutex gate;
        std::condition_variable ready;
        bool started = false;
        std::atomic<bool> stopped{false};
        std::string creation_error;
        std::vector<std::thread> workers;
        workers.reserve(worker_count);
        try {
            for (int worker = 0; worker < worker_count; ++worker) {
                workers.emplace_back([&, worker] {
                    {
                        std::unique_lock<std::mutex> lock(gate);
                        ready.wait(lock, [&] { return started; });
                    }
                    for (int step = 0; step < steps; ++step) {
                        if (stopped.load()) {
                            return;
                        }
                        SCOPED_TRACE(::testing::Message() << "Worker " << worker
                                                          << ", step " << step);
                        operation(worker, step);
                        if (HasFailure()) {
                            stopped.store(true);
                            return;
                        }
                    }
                });
            }
        } catch (const std::system_error &error) {
            creation_error = error.what();
            stopped.store(true);
        }
        {
            std::lock_guard<std::mutex> lock(gate);
            started = true;
        }
        ready.notify_all();
        for (auto &worker : workers) {
            worker.join();
        }
        ASSERT_TRUE(creation_error.empty()) << creation_error;
        ASSERT_FALSE(HasFailure()) << "A concurrent record worker failed.";
    }

    void ReadSolvingRecords(std::vector<ExpectedRecord> &records) {
        ASSERT_NE(Db().GetValue, nullptr);
        ASSERT_NE(Db().GetRemoteness, nullptr);
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            records[i].value = Db().GetValue(static_cast<Position>(i));
            ASSERT_NE(records[i].value, kErrorValue);
            records[i].remoteness =
                Db().GetRemoteness(static_cast<Position>(i));
            ASSERT_NE(records[i].remoteness, kErrorRemoteness);
        }
    }

    void AssertConcurrentRecordHistory(
        const std::vector<ExpectedRecord> &initial,
        const std::vector<ConcurrentRecordCall> &calls,
        const std::vector<ExpectedRecord> &final) {
        ASSERT_TRUE(
            HasLegalConcurrentRecordSerialization(initial, calls, final))
            << "Initial records: " << ::testing::PrintToString(initial)
            << "\nCalls: " << ::testing::PrintToString(calls)
            << "\nFinal records: " << ::testing::PrintToString(final);
    }

    // Sequence stamps constrain the oracle without serializing database calls.
    void RunConcurrentRecordHistory(std::vector<ConcurrentRecordCall> &calls,
                                    int worker_count, int steps) {
        ASSERT_EQ(calls.size(), static_cast<size_t>(worker_count * steps));
        std::atomic<uint64_t> sequence{0};
        ASSERT_NO_FATAL_FAILURE(RunConcurrentRecordWorkers(
            worker_count, steps, [&](int worker, int step) {
                auto &call = calls[worker * steps + step];
                const auto &candidate = call.candidate;
                // Validate optional capabilities before stamping the call.
                switch (call.operation) {
                    case ConcurrentRecordOperation::kSetValue:
                        ASSERT_NE(Db().SetValue, nullptr);
                        break;
                    case ConcurrentRecordOperation::kSetRemoteness:
                        ASSERT_NE(Db().SetRemoteness, nullptr);
                        break;
                    case ConcurrentRecordOperation::kSetPair:
                        ASSERT_NE(Db().SetValueRemoteness, nullptr);
                        break;
                    case ConcurrentRecordOperation::kMaximize:
                        ASSERT_NE(Db().MaximizeValueRemoteness, nullptr);
                        break;
                    case ConcurrentRecordOperation::kDecrement:
                        ASSERT_NE(Db().DecrementNumUndecidedChildren, nullptr);
                        break;
                    case ConcurrentRecordOperation::kClear:
                        ASSERT_NE(Db().ClearNumUndecidedChildren, nullptr);
                        break;
                    case ConcurrentRecordOperation::kGetValue:
                        ASSERT_NE(Db().GetValue, nullptr);
                        break;
                    case ConcurrentRecordOperation::kGetRemoteness:
                        ASSERT_NE(Db().GetRemoteness, nullptr);
                        break;
                    case ConcurrentRecordOperation::kGetCount:
                        ASSERT_NE(Db().GetNumUndecidedChildren, nullptr);
                        break;
                }
                call.begin = sequence.fetch_add(1);
                switch (call.operation) {
                    case ConcurrentRecordOperation::kSetValue:
                        call.result =
                            Db().SetValue(call.position, candidate.value);
                        break;
                    case ConcurrentRecordOperation::kSetRemoteness:
                        call.result = Db().SetRemoteness(call.position,
                                                         candidate.remoteness);
                        break;
                    case ConcurrentRecordOperation::kSetPair:
                        call.result = Db().SetValueRemoteness(
                            call.position, candidate.value,
                            candidate.remoteness);
                        break;
                    case ConcurrentRecordOperation::kMaximize:
                        call.result = Db().MaximizeValueRemoteness(
                            call.position, candidate.value,
                            candidate.remoteness, ComparePairs);
                        break;
                    case ConcurrentRecordOperation::kDecrement:
                        call.result =
                            Db().DecrementNumUndecidedChildren(call.position);
                        break;
                    case ConcurrentRecordOperation::kClear:
                        call.result =
                            Db().ClearNumUndecidedChildren(call.position);
                        break;
                    case ConcurrentRecordOperation::kGetValue:
                        call.result = Db().GetValue(call.position);
                        break;
                    case ConcurrentRecordOperation::kGetRemoteness:
                        call.result = Db().GetRemoteness(call.position);
                        break;
                    case ConcurrentRecordOperation::kGetCount:
                        call.result =
                            Db().GetNumUndecidedChildren(call.position);
                        break;
                }
                call.end = sequence.fetch_add(1);
                switch (call.operation) {
                    case ConcurrentRecordOperation::kSetValue:
                    case ConcurrentRecordOperation::kSetRemoteness:
                    case ConcurrentRecordOperation::kSetPair:
                        ASSERT_EQ(call.result, kSuccess);
                        break;
                    case ConcurrentRecordOperation::kGetValue:
                        ASSERT_NE(call.result, kErrorValue);
                        break;
                    case ConcurrentRecordOperation::kGetRemoteness:
                        ASSERT_NE(call.result, kErrorRemoteness);
                        break;
                    default:
                        break;
                }
            }));
    }

    // Explicit expected results keep the oracle separate from DB storage.
    void CheckMaximize(const ExpectedRecord &old,
                       const ExpectedRecord &candidate, RecordCompare compare,
                       bool should_replace, Position position = 1) {
        SCOPED_TRACE(::testing::Message()
                     << "Old (" << old.value << ", " << old.remoteness
                     << "), candidate (" << candidate.value << ", "
                     << candidate.remoteness << ")");
        SCOPED_TRACE(position);
        std::vector<ExpectedRecord> expected = {
            {kDraw, 23}, {kUndecided, 47}, {kLose, 71}};
        expected[position] = old;
        for (size_t i = 0; i < expected.size(); ++i) {
            ASSERT_NO_FATAL_FAILURE(
                WriteRecord(static_cast<Position>(i), expected[i]));
        }
        ASSERT_EQ(Db().MaximizeValueRemoteness(position, candidate.value,
                                               candidate.remoteness, compare),
                  should_replace);
        if (should_replace) {
            expected[position] = candidate;
        }
        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
    }

    void CheckCounters(int (*operation)(Position),
                       const std::vector<ExpectedRecord> &before,
                       const std::vector<int> &results,
                       const std::vector<ExpectedRecord> &after) {
        ASSERT_EQ(before.size(), results.size());
        ASSERT_EQ(before.size(), after.size());
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, before));
        auto expected = before;
        for (size_t i = 0; i < before.size(); ++i) {
            SCOPED_TRACE(i);
            ASSERT_EQ(operation(static_cast<Position>(i)), results[i]);
            expected[i] = after[i];
            ASSERT_NO_FATAL_FAILURE(AssertNeighborhood(expected, i));
        }
        ASSERT_NO_FATAL_FAILURE(AssertRecords(after));
    }

    void ReleaseSolvingTier() {
        if (Db().FreeSolvingTier != nullptr) {
            const int error = Db().FreeSolvingTier();
            cleanup_failed_ = error != kSuccess;
            ASSERT_EQ(error, kSuccess);
            solving_tier_active_ = false;
        } else {
            // Mandatory lifecycle methods can release optional solving state.
            ASSERT_NO_FATAL_FAILURE(RestartDatabase());
        }
    }

    void SaveTier(Tier tier, const std::vector<ExpectedRecord> &records) {
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(tier, records));
        ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
    }

    void CreateSegmentBuffers(int count, int64_t size) {
        ASSERT_NE(Db().segmentation.CreateBuffers, nullptr);
        ASSERT_EQ(Db().segmentation.CreateBuffers(kStoredTier, count, size),
                  kSuccess);
        buffers_active_ = true;
    }

    void ReleaseSegmentBuffers() {
        if (Db().segmentation.FreeBuffers != nullptr) {
            const int error = Db().segmentation.FreeBuffers();
            cleanup_failed_ = error != kSuccess;
            ASSERT_EQ(error, kSuccess);
            buffers_active_ = false;
        } else {
            ASSERT_NO_FATAL_FAILURE(RestartDatabase());
        }
    }

    void WriteSegmentBuffer(int buffer,
                            const std::vector<ExpectedRecord> &records) {
        ASSERT_NE(Db().segmentation.SetValueRemoteness, nullptr);
        SCOPED_TRACE(buffer);
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            Db().segmentation.SetValueRemoteness(
                buffer, static_cast<int64_t>(i), records[i].value,
                records[i].remoteness);
        }
    }

    void AssertSegmentBuffer(int buffer,
                             const std::vector<ExpectedRecord> &records) {
        ASSERT_NE(Db().segmentation.GetValue, nullptr);
        ASSERT_NE(Db().segmentation.GetRemoteness, nullptr);
        SCOPED_TRACE(buffer);
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            ASSERT_EQ(
                Db().segmentation.GetValue(buffer, static_cast<int64_t>(i)),
                records[i].value);
            ASSERT_EQ(Db().segmentation.GetRemoteness(buffer,
                                                      static_cast<int64_t>(i)),
                      records[i].remoteness);
        }
    }

    void SaveCheckpoint(Tier tier, const std::vector<ExpectedRecord> &records,
                        const std::vector<unsigned char> &status) {
        ASSERT_NE(Db().CreateSolvingTier, nullptr);
        ASSERT_NE(Db().CheckpointSave, nullptr);
        ASSERT_FALSE(status.empty());
        ASSERT_NO_FATAL_FAILURE(
            CreateAndPopulate(tier, records, Db().CreateSolvingTier));
        ASSERT_EQ(Db().CheckpointSave(status.data(), status.size()), kSuccess);
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
    }

    void LoadCheckpoint(Tier tier, int64_t size,
                        std::vector<unsigned char> &status) {
        ASSERT_NE(Db().CheckpointLoad, nullptr);
        ASSERT_FALSE(status.empty());
        ASSERT_EQ(Db().CheckpointLoad(tier, size, status.data(), status.size()),
                  kSuccess);
        solving_tier_active_ = true;
    }

    void LoadResidentTier(Tier tier, int64_t size) {
        ASSERT_NE(Db().LoadTier, nullptr);
        ASSERT_EQ(Db().LoadTier(tier, size), kSuccess);
        loaded_tiers_.push_back(tier);
    }

    void UnloadResidentTier(Tier tier) {
        ASSERT_NE(Db().UnloadTier, nullptr);
        const int error = Db().UnloadTier(tier);
        cleanup_failed_ = error != kSuccess;
        ASSERT_EQ(error, kSuccess);
        const auto found =
            std::find(loaded_tiers_.begin(), loaded_tiers_.end(), tier);
        ASSERT_NE(found, loaded_tiers_.end());
        loaded_tiers_.erase(found);
    }

    void AssertLoadedRecords(Tier tier,
                             const std::vector<ExpectedRecord> &records) {
        ASSERT_NE(Db().GetValueFromLoaded, nullptr);
        ASSERT_NE(Db().GetRemotenessFromLoaded, nullptr);
        SCOPED_TRACE(tier);
        for (size_t i = 0; i < records.size(); ++i) {
            const Position position = static_cast<Position>(i);
            SCOPED_TRACE(position);
            ASSERT_EQ(Db().GetValueFromLoaded(tier, position),
                      records[i].value);
            ASSERT_EQ(Db().GetRemotenessFromLoaded(tier, position),
                      records[i].remoteness);
        }
    }

    DbProbe *Probe(size_t index = 0) { return &probes_[index]; }

    void InitializeProbe(size_t index = 0) {
        ASSERT_LT(index, probes_.size());
        ASSERT_NE(Db().ProbeInit, nullptr);
        const int error = Db().ProbeInit(Probe(index));
        // A failed (re)initialization leaves probe ownership unspecified.
        // End the isolated process rather than retrying cleanup or finalizing
        // with a potentially live probe.
        cleanup_failed_ = error != kSuccess;
        ASSERT_EQ(error, kSuccess);
        probe_initialized_[index] = true;
    }

    void DestroyProbe(size_t index = 0) {
        ASSERT_LT(index, probes_.size());
        ASSERT_NE(Db().ProbeDestroy, nullptr);
        const int error = Db().ProbeDestroy(Probe(index));
        cleanup_failed_ = error != kSuccess;
        ASSERT_EQ(error, kSuccess);
        probe_initialized_[index] = false;
    }

    // One available reader suffices to observe a probe lifecycle operation.
    void AssertProbeRecord(TierPosition position,
                           const ExpectedRecord &expected) {
        if (Db().ProbeValue != nullptr) {
            ASSERT_EQ(Db().ProbeValue(Probe(), position), expected.value);
        } else {
            ASSERT_NE(Db().ProbeRemoteness, nullptr);
            ASSERT_EQ(Db().ProbeRemoteness(Probe(), position),
                      expected.remoteness);
        }
    }

    void VerifyArchive(Tier tier, const std::vector<ExpectedRecord> &records) {
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            const TierPosition position = {tier, static_cast<Position>(i)};
            ASSERT_EQ(Db().ProbeValue(Probe(), position), records[i].value);
            ASSERT_EQ(Db().ProbeRemoteness(Probe(), position),
                      records[i].remoteness);
        }
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }

   private:
    void ReportFailures(int first_part) const {
        const auto *result =
            ::testing::UnitTest::GetInstance()->current_test_info()->result();
        for (int i = first_part; i < result->total_part_count(); ++i) {
            const auto &part = result->GetTestPartResult(i);
            if (part.failed()) {
                std::fprintf(stderr, "%s:%d: %s\n",
                             part.file_name() ? part.file_name() : "<test>",
                             part.line_number(), part.message());
            }
        }
    }

    void CleanupDatabase() {
        if (cleanup_failed_) {
            return;
        }
        for (size_t i = 0; i < probes_.size(); ++i) {
            if (!probe_initialized_[i]) {
                continue;
            }
            const int error = Db().ProbeDestroy(Probe(i));
            EXPECT_EQ(error, kSuccess);
            if (error != kSuccess) {
                return;
            }
            probe_initialized_[i] = false;
        }
        if (HasFailure() || finalize_only_) {
            // After an operation fails, only documented cleanup is permitted.
            if (initialization_attempted_) {
                Finalize();
            }
            return;
        }
        if (solving_tier_active_ && Db().FreeSolvingTier != nullptr) {
            const int error = Db().FreeSolvingTier();
            EXPECT_EQ(error, kSuccess);
            if (error != kSuccess) {
                return;
            }
            solving_tier_active_ = false;
        }
        while (!loaded_tiers_.empty() && Db().UnloadTier != nullptr) {
            const int error = Db().UnloadTier(loaded_tiers_.back());
            EXPECT_EQ(error, kSuccess);
            if (error != kSuccess) {
                return;
            }
            loaded_tiers_.pop_back();
        }
        if (buffers_active_ && Db().segmentation.FreeBuffers != nullptr) {
            const int error = Db().segmentation.FreeBuffers();
            EXPECT_EQ(error, kSuccess);
            if (error != kSuccess) {
                return;
            }
            buffers_active_ = false;
        }
        if (initialization_attempted_) {
            Finalize();
        }
    }

   protected:
    bool solving_tier_active_ = false;
    bool buffers_active_ = false;

   private:
    std::string sandbox_;
    std::string initialization_path_;
    GetTierNameFunc initialization_tier_namer_ = nullptr;
    bool initialization_attempted_ = false;
    // Stable storage remains alive if a fatal read assertion exits its caller.
    std::array<DbProbe, 3> probes_ = {};
    std::array<bool, 3> probe_initialized_ = {};
    bool cleanup_failed_ = false;
    bool finalize_only_ = false;
    std::vector<Tier> loaded_tiers_;
    CreateTierFunc record_create_ = nullptr;
};

#define REQUIRE_DATABASE_MEMBER(member)                      \
    do {                                                     \
        if (Db().member == nullptr) {                        \
            GTEST_SKIP() << #member << " is not supported."; \
        }                                                    \
    } while (false)

#define REQUIRE_DATABASE_ARCHIVE()                              \
    do {                                                        \
        if (const char *missing = MissingArchiveDependency()) { \
            GTEST_SKIP() << missing;                            \
        }                                                       \
    } while (false)

#define REQUIRE_DATABASE_RECORDS()                             \
    do {                                                       \
        if (const char *missing = MissingRecordDependency()) { \
            GTEST_SKIP() << missing;                           \
        }                                                      \
    } while (false)

#define REQUIRE_DATABASE_RECORD_READERS()       \
    do {                                        \
        REQUIRE_DATABASE_MEMBER(GetValue);      \
        REQUIRE_DATABASE_MEMBER(GetRemoteness); \
    } while (false)

#define REQUIRE_DATABASE_CREATION()                              \
    do {                                                         \
        if (const char *missing = MissingCreationDependency()) { \
            GTEST_SKIP() << missing;                             \
        }                                                        \
    } while (false)

#define REQUIRE_DATABASE_CHECKPOINT()                              \
    do {                                                           \
        if (const char *missing = MissingCheckpointDependency()) { \
            GTEST_SKIP() << missing;                               \
        }                                                          \
    } while (false)

#define REQUIRE_DATABASE_PROBE_READER()                                      \
    do {                                                                     \
        if (Db().ProbeValue == nullptr && Db().ProbeRemoteness == nullptr) { \
            GTEST_SKIP() << "No probe record reader is supported.";          \
        }                                                                    \
    } while (false)

// Verifies bounded termination, nonempty content, and whitespace exclusion.
TEST_P(DatabaseTest, UnitDescriptorHasValidInternalName) {
    const char *end = static_cast<const char *>(
        std::memchr(Db().name, '\0', sizeof(Db().name)));
    ASSERT_NE(end, nullptr) << "Internal name must be NUL-terminated.";
    EXPECT_GT(end - Db().name, 0);
    EXPECT_LE(end - Db().name, kDbNameLengthMax);
    for (const char *character = Db().name; character != end; ++character) {
        EXPECT_FALSE(std::isspace(static_cast<unsigned char>(*character)))
            << "Whitespace at index " << character - Db().name;
    }
}

// Verifies the formal name is terminated and nonempty; spaces are allowed.
TEST_P(DatabaseTest, UnitDescriptorHasValidFormalName) {
    const char *end = static_cast<const char *>(
        std::memchr(Db().formal_name, '\0', sizeof(Db().formal_name)));
    ASSERT_NE(end, nullptr) << "Formal name must be NUL-terminated.";
    EXPECT_GT(end - Db().formal_name, 0);
    EXPECT_LE(end - Db().formal_name, kDbFormalNameLengthMax);
}

// Requires only the two mandatory lifecycle function pointers.
TEST_P(DatabaseTest, UnitDescriptorProvidesRequiredLifecycleMembers) {
    EXPECT_NE(Db().Init, nullptr);
    EXPECT_NE(Db().Finalize, nullptr);
}

// Initializes with a valid tier namer and the default auxiliary settings.
TEST_P(DatabaseTest, UnitInitSucceedsWithTierNamer) {
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize(GetTestTierName));
        ExpectSandboxPath();
    }));
}

// Accepts a null tier namer to select the implementation's fallback.
TEST_P(DatabaseTest, UnitInitAcceptsNullTierNamer) {
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ExpectSandboxPath();
    }));
}

// Queries an actually uninitialized database in a fresh process.
TEST_P(DatabaseTest, UnitGetPathBeforeInitialization) {
    REQUIRE_DATABASE_MEMBER(GetPath);
    ASSERT_NO_FATAL_FAILURE(
        RunIsolated([&] { EXPECT_EQ(Db().GetPath(), nullptr); }));
}

// Checks directory identity without requiring a particular path spelling.
TEST_P(DatabaseTest, UnitGetPathAfterInitialization) {
    REQUIRE_DATABASE_MEMBER(GetPath);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ExpectSandboxPath();
    }));
}

// Finalization makes the path unavailable without dereferencing old pointers.
TEST_P(DatabaseTest, UnitGetPathAfterFinalization) {
    REQUIRE_DATABASE_MEMBER(GetPath);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        Finalize();
        ExpectFinalizedPath();
    }));
}

// Finalization owns an ordinary solving tier without a preceding free call.
TEST_P(DatabaseTest, UnitFinalizeWithOrdinarySolvingTier) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(Db().CreateSolvingTier(kStoredTier, 17), kSuccess);
        solving_tier_active_ = true;
        Finalize();
        ExpectFinalizedPath();
    }));
}

// Finalization also owns concurrent solving-tier allocations.
TEST_P(DatabaseTest, UnitFinalizeWithConcurrentSolvingTier) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(Db().CreateConcurrentSolvingTier(kStoredTier, 17), kSuccess);
        solving_tier_active_ = true;
        Finalize();
        ExpectFinalizedPath();
    }));
}

// A loaded tier remains owned by Finalize rather than an explicit unload.
TEST_P(DatabaseTest, UnitFinalizeWithLoadedTier) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(LoadTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, {{kWin, 7}}));
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(kStoredTier, 1));
        Finalize();
        ExpectFinalizedPath();
    }));
}

// Finalization releases active segment buffers without calling FreeBuffers.
TEST_P(DatabaseTest, UnitFinalizeWithOwnedBuffers) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            ASSERT_EQ(Db().segmentation.CreateBuffers(kStoredTier, 1, 17),
                      kSuccess);
            buffers_active_ = true;
            Finalize();
            ExpectFinalizedPath();
        },
        "No positive segment-buffer capacity."));
}

// Every legal value/remoteness pair survives flushing and memory release.
TEST_P(DatabaseTest, UnitFlushSolvingTierWritesCurrentContents) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, records));
        ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, records));
    }));
}

// A fresh tier with no saved data reports missing.
TEST_P(DatabaseTest, UnitTierStatusReportsMissing) {
    REQUIRE_DATABASE_MEMBER(TierStatus);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        EXPECT_EQ(Db().TierStatus(kMissingTier), kDbTierStatusMissing);
    }));
}

// A successfully generated normal archive reports solved.
TEST_P(DatabaseTest, UnitTierStatusReportsSolved) {
    REQUIRE_DATABASE_MEMBER(TierStatus);
    REQUIRE_DATABASE_ARCHIVE();
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, {{kWin, 7}}));
        EXPECT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
    }));
}

// Status queries distinguish stored and missing tiers in either query order.
TEST_P(DatabaseTest, UnitTierStatusSelectsRequestedTier) {
    REQUIRE_DATABASE_MEMBER(TierStatus);
    REQUIRE_DATABASE_ARCHIVE();
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, {{kWin, 7}}));
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        ASSERT_EQ(Db().TierStatus(kMissingTier), kDbTierStatusMissing);
        EXPECT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
    }));
}

// A fresh game has not yet been declared solved.
TEST_P(DatabaseTest, UnitGameStatusReportsIncomplete) {
    REQUIRE_DATABASE_MEMBER(GameStatus);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        EXPECT_EQ(Db().GameStatus(), kDbGameStatusIncomplete);
    }));
}

// GameStatus recognizes a solved declaration used only as checked setup.
TEST_P(DatabaseTest, UnitGameStatusReportsSolved) {
    REQUIRE_DATABASE_MEMBER(GameStatus);
    REQUIRE_DATABASE_MEMBER(SetGameSolved);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(Db().SetGameSolved(), kSuccess);
        EXPECT_EQ(Db().GameStatus(), kDbGameStatusSolved);
    }));
}

// Declaring completion changes the public game status to solved.
TEST_P(DatabaseTest, UnitSetGameSolvedRecordsDeclaration) {
    REQUIRE_DATABASE_MEMBER(SetGameSolved);
    REQUIRE_DATABASE_MEMBER(GameStatus);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(Db().SetGameSolved(), kSuccess);
        EXPECT_EQ(Db().GameStatus(), kDbGameStatusSolved);
    }));
}

// Finalization before any initialization is a successful no-op.
TEST_P(DatabaseTest, UnitFinalizeBeforeInitialization) {
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        Finalize();
        ExpectFinalizedPath();
    }));
}

// Repeated successful finalization leaves the database uninitialized.
TEST_P(DatabaseTest, UnitFinalizeRepeated) {
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        Finalize();
        Finalize();
        ExpectFinalizedPath();
    }));
}

// Repeated flushes preserve logical contents across the entire legal domain.
TEST_P(DatabaseTest, UnitFlushSolvingTierRepeated) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, records));
        ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
        ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, records));
    }));
}

// Repeated solved declarations succeed without requiring a status observer.
TEST_P(DatabaseTest, UnitSetGameSolvedRepeated) {
    REQUIRE_DATABASE_MEMBER(SetGameSolved);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(Db().SetGameSolved(), kSuccess);
        ASSERT_EQ(Db().SetGameSolved(), kSuccess);
        if (Db().GameStatus != nullptr) {
            EXPECT_EQ(Db().GameStatus(), kDbGameStatusSolved);
        }
    }));
}

// Ordinary creation supports small single, even, and non-power-of-two tiers.
TEST_P(DatabaseTest, UnitCreateSolvingTierSupportedSizes) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    for (const int64_t size : {int64_t{1}, int64_t{2}, int64_t{17}}) {
        SCOPED_TRACE(size);
        ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            ASSERT_NO_FATAL_FAILURE(CreateTier(Db().CreateSolvingTier, size));
            if (Db().IsTierLoaded != nullptr) {
                EXPECT_TRUE(Db().IsTierLoaded(kStoredTier));
            }
        }));
    }
}

// Concurrent creation exposes the same supported size partitions.
TEST_P(DatabaseTest, UnitCreateConcurrentSolvingTierSupportedSizes) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    for (const int64_t size : {int64_t{1}, int64_t{2}, int64_t{17}}) {
        SCOPED_TRACE(size);
        ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            ASSERT_NO_FATAL_FAILURE(
                CreateTier(Db().CreateConcurrentSolvingTier, size));
            if (Db().IsTierLoaded != nullptr) {
                EXPECT_TRUE(Db().IsTierLoaded(kStoredTier));
            }
        }));
    }
}

// All requested ordinary records are distinct and accessible after seeding.
TEST_P(DatabaseTest, UnitCreateSolvingTierProvidesRequestedExtent) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(GetValue);
    REQUIRE_DATABASE_MEMBER(GetRemoteness);
    if (const char *missing = MissingRecordDependency()) {
        GTEST_SKIP() << missing;
    }
    for (const int64_t size : {int64_t{1}, int64_t{2}, int64_t{17}}) {
        SCOPED_TRACE(size);
        ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            ASSERT_NO_FATAL_FAILURE(CreateTier(Db().CreateSolvingTier, size));
            const Value values[] = {kWin, kLose, kDraw, kTie, kUndecided};
            std::vector<ExpectedRecord> records;
            for (Position position = 0; position < size; ++position) {
                const ExpectedRecord record = {
                    values[position % kNumValues],
                    static_cast<int>((position * 11 + 3) %
                                     (kRemotenessMax + 1))};
                records.push_back(record);
                ASSERT_NO_FATAL_FAILURE(WriteRecord(position, record));
            }
            ASSERT_NO_FATAL_FAILURE(AssertRecords(records));
        }));
    }
}

// Concurrent storage also provides every requested record without aliasing.
TEST_P(DatabaseTest, UnitCreateConcurrentSolvingTierProvidesRequestedExtent) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(GetValue);
    REQUIRE_DATABASE_MEMBER(GetRemoteness);
    if (const char *missing = MissingRecordDependency()) {
        GTEST_SKIP() << missing;
    }
    for (const int64_t size : {int64_t{1}, int64_t{2}, int64_t{17}}) {
        SCOPED_TRACE(size);
        ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            ASSERT_NO_FATAL_FAILURE(
                CreateTier(Db().CreateConcurrentSolvingTier, size));
            const Value values[] = {kWin, kLose, kDraw, kTie, kUndecided};
            std::vector<ExpectedRecord> records;
            for (Position position = 0; position < size; ++position) {
                const ExpectedRecord record = {
                    values[position % kNumValues],
                    static_cast<int>((position * 11 + 3) %
                                     (kRemotenessMax + 1))};
                records.push_back(record);
                ASSERT_NO_FATAL_FAILURE(WriteRecord(position, record));
            }
            ASSERT_NO_FATAL_FAILURE(AssertRecords(records));
        }));
    }
}

// Empty ordinary tiers can be created and freed without accessing a record.
TEST_P(DatabaseTest, UnitCreateSolvingTierZeroSize) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(CreateTier(Db().CreateSolvingTier, 0));
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_TRUE(Db().IsTierLoaded(kStoredTier));
        }
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
    }));
}

// Empty concurrent tiers have the same creation and release contract.
TEST_P(DatabaseTest, UnitCreateConcurrentSolvingTierZeroSize) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(Db().CreateConcurrentSolvingTier, 0));
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_TRUE(Db().IsTierLoaded(kStoredTier));
        }
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
    }));
}

// A negative ordinary size has its documented checked error.
TEST_P(DatabaseTest, UnitCreateSolvingTierNegativeSize) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(Db().CreateSolvingTier(kStoredTier, -1),
                  kIllegalArgumentError);
    }));
}

// The signed minimum is a separate checked case and isolated lifetime.
TEST_P(DatabaseTest, UnitCreateSolvingTierMinimumSize) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(Db().CreateSolvingTier(kStoredTier, INT64_MIN),
                  kIllegalArgumentError);
    }));
}

// Negative concurrent sizes return the same explicit argument error.
TEST_P(DatabaseTest, UnitCreateConcurrentSolvingTierNegativeSize) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(Db().CreateConcurrentSolvingTier(kStoredTier, -1),
                  kIllegalArgumentError);
    }));
}

// Concurrent signed-minimum rejection is independent of the minus-one case.
TEST_P(DatabaseTest, UnitCreateConcurrentSolvingTierMinimumSize) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(Db().CreateConcurrentSolvingTier(kStoredTier, INT64_MIN),
                  kIllegalArgumentError);
    }));
}

// Freeing with no active tier succeeds without requiring any creation API.
TEST_P(DatabaseTest, UnitFreeSolvingTierWithoutTier) {
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
    }));
}

// Explicit release removes an ordinary tier's in-memory residency.
TEST_P(DatabaseTest, UnitFreeSolvingTierOrdinaryTier) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(CreateTier(Db().CreateSolvingTier, 17));
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
    }));
}

// Explicit release also removes a concurrently created tier's residency.
TEST_P(DatabaseTest, UnitFreeSolvingTierConcurrentTier) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(Db().CreateConcurrentSolvingTier, 17));
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
    }));
}

// Releasing an already freed ordinary tier remains a successful no-op.
TEST_P(DatabaseTest, UnitFreeSolvingTierRepeatedOrdinaryTier) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(CreateTier(Db().CreateSolvingTier, 17));
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
    }));
}

// Repeated release is also guaranteed after concurrent creation.
TEST_P(DatabaseTest, UnitFreeSolvingTierRepeatedConcurrentTier) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(Db().CreateConcurrentSolvingTier, 17));
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
    }));
}

// Setting any legal value preserves every legal remoteness.
TEST_P(DatabaseTest, UnitSetValueEveryLegalValue) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(SetValue);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const auto desired = AllRecords();
        auto expected = desired;
        for (auto &record : expected) {
            record.value = record.value == kWin ? kLose : kWin;
        }
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (size_t i = 0; i < desired.size(); ++i) {
            SCOPED_TRACE(i);
            ASSERT_EQ(Db().SetValue(static_cast<Position>(i), desired[i].value),
                      kSuccess);
            expected[i] = desired[i];
            ASSERT_NO_FATAL_FAILURE(AssertNeighborhood(expected, i));
        }
        ASSERT_NO_FATAL_FAILURE(AssertRecords(desired));
    }));
}

// Includes all 25 old/new transitions, unchanged values, and remoteness edges.
TEST_P(DatabaseTest, UnitSetValueEveryValueTransition) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(SetValue);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        std::vector<ExpectedRecord> before;
        std::vector<ExpectedRecord> after;
        for (Value old : kLegalValues) {
            for (Value next : kLegalValues) {
                for (int remoteness : kRepresentativeRemotenesses) {
                    before.push_back({old, remoteness});
                    after.push_back({next, remoteness});
                }
            }
        }
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, before));
        auto expected = before;
        for (size_t i = 0; i < after.size(); ++i) {
            SCOPED_TRACE(i);
            ASSERT_EQ(Db().SetValue(static_cast<Position>(i), after[i].value),
                      kSuccess);
            expected[i] = after[i];
            ASSERT_NO_FATAL_FAILURE(AssertNeighborhood(expected, i));
        }
        ASSERT_NO_FATAL_FAILURE(AssertRecords(after));
    }));
}

// Every remoteness can replace a different one while preserving each value.
TEST_P(DatabaseTest, UnitSetRemotenessEveryLegalRemoteness) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const auto desired = AllRecords();
        auto expected = desired;
        for (auto &record : expected) {
            record.remoteness = (record.remoteness + 1) % (kRemotenessMax + 1);
        }
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (size_t i = 0; i < desired.size(); ++i) {
            SCOPED_TRACE(i);
            ASSERT_EQ(Db().SetRemoteness(static_cast<Position>(i),
                                         desired[i].remoteness),
                      kSuccess);
            expected[i] = desired[i];
            ASSERT_NO_FATAL_FAILURE(AssertNeighborhood(expected, i));
        }
        ASSERT_NO_FATAL_FAILURE(AssertRecords(desired));
    }));
}

// Replacements and repeated writes preserve neighbors at all address edges.
TEST_P(DatabaseTest, UnitSetRemotenessRepeatedAndReplacementWrites) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        std::vector<ExpectedRecord> expected(17, {kDraw, 19});
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (Position position : {0, 8, 16}) {
            SCOPED_TRACE(position);
            for (Value value : kLegalValues) {
                for (int old : kRepresentativeRemotenesses) {
                    for (int next : kRepresentativeRemotenesses) {
                        expected[position] = {value, old};
                        ASSERT_NO_FATAL_FAILURE(
                            WriteRecord(position, expected[position]));
                        ASSERT_EQ(Db().SetRemoteness(position, next), kSuccess);
                        expected[position].remoteness = next;
                        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                        ASSERT_EQ(Db().SetRemoteness(position, next), kSuccess);
                        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                    }
                }
            }
        }
    }));
}

// Combined writes replace both fields across the complete supported domain.
TEST_P(DatabaseTest, UnitSetValueRemotenessEveryLegalPair) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const auto desired = AllRecords();
        auto expected = desired;
        for (auto &record : expected) {
            record.value = record.value == kWin ? kLose : kWin;
            record.remoteness = (record.remoteness + 1) % (kRemotenessMax + 1);
        }
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (size_t i = 0; i < desired.size(); ++i) {
            SCOPED_TRACE(i);
            ASSERT_EQ(Db().SetValueRemoteness(static_cast<Position>(i),
                                              desired[i].value,
                                              desired[i].remoteness),
                      kSuccess);
            expected[i] = desired[i];
            ASSERT_NO_FATAL_FAILURE(AssertNeighborhood(expected, i));
        }
        ASSERT_NO_FATAL_FAILURE(AssertRecords(desired));
    }));
}

// Writing each identical pair twice remains successful and changes no records.
TEST_P(DatabaseTest, UnitSetValueRemotenessRepeatedPair) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const auto records = AllRecords();
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, records));
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            for (int repetition = 0; repetition < 2; ++repetition) {
                ASSERT_EQ(Db().SetValueRemoteness(static_cast<Position>(i),
                                                  records[i].value,
                                                  records[i].remoteness),
                          kSuccess);
                ASSERT_NO_FATAL_FAILURE(AssertNeighborhood(records, i));
            }
        }
        ASSERT_NO_FATAL_FAILURE(AssertRecords(records));
    }));
}

// Repeated reads return every legal value; the other getter is optional.
TEST_P(DatabaseTest, UnitGetValueEveryLegalResult) {
    REQUIRE_DATABASE_CREATION();
    REQUIRE_DATABASE_MEMBER(GetValue);
    if (Db().SetValueRemoteness == nullptr && Db().SetValue == nullptr) {
        GTEST_SKIP() << "No value writer is supported.";
    }
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const auto records = AllRecords();
        const bool complete_pairs = MissingRecordDependency() == nullptr;
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(static_cast<int64_t>(records.size())));
        for (size_t i = 0; i < records.size(); ++i) {
            const Position position = static_cast<Position>(i);
            if (complete_pairs) {
                ASSERT_NO_FATAL_FAILURE(WriteRecord(position, records[i]));
            } else {
                ASSERT_EQ(Db().SetValue(position, records[i].value), kSuccess);
            }
        }
        for (int pass = 0; pass < 2; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t i = 0; i < records.size(); ++i) {
                SCOPED_TRACE(i);
                const Position position = static_cast<Position>(i);
                ASSERT_EQ(Db().GetValue(position), records[i].value);
                if (complete_pairs && Db().GetRemoteness != nullptr) {
                    ASSERT_EQ(Db().GetRemoteness(position),
                              records[i].remoteness);
                }
            }
        }
    }));
}

// Remoteness reads include undecided records and do not change their fields.
TEST_P(DatabaseTest, UnitGetRemotenessEveryLegalResult) {
    REQUIRE_DATABASE_CREATION();
    REQUIRE_DATABASE_MEMBER(GetRemoteness);
    if (Db().SetValueRemoteness == nullptr && Db().SetRemoteness == nullptr) {
        GTEST_SKIP() << "No remoteness writer is supported.";
    }
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const auto records = AllRecords();
        const bool complete_pairs = MissingRecordDependency() == nullptr;
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(static_cast<int64_t>(records.size())));
        for (size_t i = 0; i < records.size(); ++i) {
            const Position position = static_cast<Position>(i);
            if (complete_pairs) {
                ASSERT_NO_FATAL_FAILURE(WriteRecord(position, records[i]));
            } else {
                ASSERT_EQ(Db().SetRemoteness(position, records[i].remoteness),
                          kSuccess);
            }
        }
        for (int pass = 0; pass < 2; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t i = 0; i < records.size(); ++i) {
                SCOPED_TRACE(i);
                const Position position = static_cast<Position>(i);
                ASSERT_EQ(Db().GetRemoteness(position), records[i].remoteness);
                if (complete_pairs && Db().GetValue != nullptr) {
                    ASSERT_EQ(Db().GetValue(position), records[i].value);
                }
            }
        }
    }));
}

// A greater candidate replaces both fields without changing neighboring pairs.
TEST_P(DatabaseTest, UnitMaximizeValueRemotenessGreaterCandidate) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        ASSERT_NO_FATAL_FAILURE(CreateTier(3));
        for (Position position : {0, 1, 2}) {
            ASSERT_NO_FATAL_FAILURE(CheckMaximize(
                {kLose, 79}, {kWin, 2}, ComparePairs, true, position));
        }
    }));
}

// A lower-ranked candidate leaves both original fields intact.
TEST_P(DatabaseTest, UnitMaximizeValueRemotenessSmallerCandidate) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        ASSERT_NO_FATAL_FAILURE(CreateTier(3));
        for (Position position : {0, 1, 2}) {
            ASSERT_NO_FATAL_FAILURE(CheckMaximize(
                {kWin, 2}, {kLose, 79}, ComparePairs, false, position));
        }
    }));
}

// Equality is not a replacement, including either remoteness boundary.
TEST_P(DatabaseTest, UnitMaximizeValueRemotenessEqualCandidate) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        ASSERT_NO_FATAL_FAILURE(CreateTier(3));
        for (Value value : kLegalValues) {
            for (int remoteness : kBoundaryRemotenesses) {
                const ExpectedRecord pair = {value, remoteness};
                ASSERT_NO_FATAL_FAILURE(
                    CheckMaximize(pair, pair, ComparePairs, false));
            }
        }
    }));
}

// Every ordered value pair is compared at an equal, initialized remoteness.
TEST_P(DatabaseTest, UnitMaximizeValueRemotenessComparesValues) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        ASSERT_NO_FATAL_FAILURE(CreateTier(3));
        for (Value old : kLegalValues) {
            for (Value candidate : kLegalValues) {
                ASSERT_NO_FATAL_FAILURE(CheckMaximize(
                    {old, 17}, {candidate, 17}, ComparePairs, candidate > old));
            }
        }
    }));
}

// Equal values let lesser, equal, and greater remotenesses determine the
// result.
TEST_P(DatabaseTest, UnitMaximizeValueRemotenessComparesRemotenesses) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        ASSERT_NO_FATAL_FAILURE(CreateTier(3));
        for (Value value : kLegalValues) {
            for (int old : kRepresentativeRemotenesses) {
                for (int candidate : kRepresentativeRemotenesses) {
                    ASSERT_NO_FATAL_FAILURE(
                        CheckMaximize({value, old}, {value, candidate},
                                      ComparePairs, candidate > old));
                }
            }
        }
    }));
}

// Comparator preferences override natural value and remoteness ordering.
TEST_P(DatabaseTest, UnitMaximizeValueRemotenessHonorsAlternateOrdering) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        ASSERT_NO_FATAL_FAILURE(CreateTier(3));
        ASSERT_NO_FATAL_FAILURE(CheckMaximize(
            {kWin, 0}, {kLose, kRemotenessMax}, CompareReversedValues, true));
        ASSERT_NO_FATAL_FAILURE(CheckMaximize(
            {kLose, kRemotenessMax}, {kWin, 0}, CompareReversedValues, false));
        ASSERT_NO_FATAL_FAILURE(CheckMaximize(
            {kTie, 7}, {kTie, 3}, CompareSmallerRemotenessFirst, true));
        ASSERT_NO_FATAL_FAILURE(CheckMaximize(
            {kWin, 7}, {kLose, 3}, CompareSmallerRemotenessFirst, true));
        ASSERT_NO_FATAL_FAILURE(CheckMaximize(
            {kLose, 3}, {kWin, 7}, CompareSmallerRemotenessFirst, false));
        ASSERT_NO_FATAL_FAILURE(CheckMaximize(
            {kWin, 0}, {kLose, kRemotenessMax}, CompareRemotenessesOnly, true));
    }));
}

// Sign, rather than magnitude or subtraction, controls comparisons.
TEST_P(DatabaseTest, UnitMaximizeValueRemotenessUsesComparisonSign) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        ASSERT_NO_FATAL_FAILURE(CreateTier(3));
        for (RecordCompare compare :
             {CompareNonUnitSigns, CompareExtremeSigns}) {
            ASSERT_NO_FATAL_FAILURE(
                CheckMaximize({kLose, 7}, {kWin, 3}, compare, true));
            ASSERT_NO_FATAL_FAILURE(
                CheckMaximize({kWin, 3}, {kLose, 7}, compare, false));
            ASSERT_NO_FATAL_FAILURE(
                CheckMaximize({kTie, 0}, {kTie, 0}, compare, false));
        }
    }));
}

// Exhaust candidates against every old value and representative remotenesses.
TEST_P(DatabaseTest, UnitMaximizeValueRemotenessDomainBoundaries) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        ASSERT_NO_FATAL_FAILURE(CreateTier(3));
        const auto candidates = AllRecords();
        for (Value old_value : kLegalValues) {
            for (int old_remoteness : kRepresentativeRemotenesses) {
                for (const auto &candidate : candidates) {
                    const bool replace =
                        candidate.value > old_value ||
                        (candidate.value == old_value &&
                         candidate.remoteness > old_remoteness);
                    ASSERT_NO_FATAL_FAILURE(
                        CheckMaximize({old_value, old_remoteness}, candidate,
                                      ComparePairs, replace));
                }
            }
        }
    }));
}

// Equivalent ranks retain distinct original fields instead of copying them.
TEST_P(DatabaseTest, UnitMaximizeValueRemotenessEquivalentRanks) {
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        ASSERT_NO_FATAL_FAILURE(CreateTier(3));
        for (Value value : kLegalValues) {
            ASSERT_NO_FATAL_FAILURE(CheckMaximize(
                {value, 0}, {value, kRemotenessMax}, CompareValuesOnly, false));
            ASSERT_NO_FATAL_FAILURE(CheckMaximize(
                {value, kRemotenessMax}, {value, 0}, CompareValuesOnly, false));
        }
        for (Value old : kLegalValues) {
            for (Value candidate : kLegalValues) {
                if (old == candidate) {
                    continue;
                }
                ASSERT_NO_FATAL_FAILURE(
                    CheckMaximize({old, 19}, {candidate, 19},
                                  CompareRemotenessesOnly, false));
            }
        }
    }));
}

// Returns each preceding positive count and subtracts exactly one.
TEST_P(DatabaseTest, UnitDecrementNumUndecidedChildrenPositive) {
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const auto before = UndecidedRecords(1);
        auto after = before;
        std::vector<int> results;
        results.reserve(before.size());
        for (size_t i = 0; i < before.size(); ++i) {
            results.push_back(before[i].remoteness);
            --after[i].remoteness;
        }
        ASSERT_NO_FATAL_FAILURE(CheckCounters(
            Db().DecrementNumUndecidedChildren, before, results, after));
    }));
}

// A zero counter remains zero rather than underflowing.
TEST_P(DatabaseTest, UnitDecrementNumUndecidedChildrenZero) {
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const std::vector<ExpectedRecord> records = {{kUndecided, 0}};
        ASSERT_NO_FATAL_FAILURE(CheckCounters(
            Db().DecrementNumUndecidedChildren, records, {0}, records));
    }));
}

// Decided values preserve their remoteness across the entire legal domain.
TEST_P(DatabaseTest, UnitDecrementNumUndecidedChildrenDecidedValues) {
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const auto records = DecidedRecords();
        const std::vector<int> results(records.size(), 0);
        ASSERT_NO_FATAL_FAILURE(CheckCounters(
            Db().DecrementNumUndecidedChildren, records, results, records));
    }));
}

// Clearing returns the old positive count and writes exactly zero.
TEST_P(DatabaseTest, UnitClearNumUndecidedChildrenPositive) {
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const auto before = UndecidedRecords(1);
        auto after = before;
        std::vector<int> results;
        results.reserve(before.size());
        for (size_t i = 0; i < before.size(); ++i) {
            results.push_back(before[i].remoteness);
            after[i].remoteness = 0;
        }
        ASSERT_NO_FATAL_FAILURE(CheckCounters(Db().ClearNumUndecidedChildren,
                                              before, results, after));
    }));
}

// An already-zero undecided counter returns zero without changing the record.
TEST_P(DatabaseTest, UnitClearNumUndecidedChildrenZero) {
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const std::vector<ExpectedRecord> records = {{kUndecided, 0}};
        ASSERT_NO_FATAL_FAILURE(CheckCounters(Db().ClearNumUndecidedChildren,
                                              records, {0}, records));
    }));
}

// Clearing decided positions preserves both fields for every legal remoteness.
TEST_P(DatabaseTest, UnitClearNumUndecidedChildrenDecidedValues) {
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const auto records = DecidedRecords();
        const std::vector<int> results(records.size(), 0);
        ASSERT_NO_FATAL_FAILURE(CheckCounters(Db().ClearNumUndecidedChildren,
                                              records, results, records));
    }));
}

// Every supported count is readable without changing its underlying record.
TEST_P(DatabaseTest, UnitGetNumUndecidedChildrenUndecidedCounts) {
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const auto records = UndecidedRecords(0);
        std::vector<int> results;
        results.reserve(records.size());
        for (const auto &record : records) {
            results.push_back(record.remoteness);
        }
        ASSERT_NO_FATAL_FAILURE(CheckCounters(Db().GetNumUndecidedChildren,
                                              records, results, records));
    }));
}

// Decided positions report zero even when their remoteness is positive.
TEST_P(DatabaseTest, UnitGetNumUndecidedChildrenDecidedValues) {
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        const auto records = DecidedRecords();
        const std::vector<int> results(records.size(), 0);
        ASSERT_NO_FATAL_FAILURE(CheckCounters(Db().GetNumUndecidedChildren,
                                              records, results, records));
    }));
}

// Boundary and interior decrements affect only the selected position.
TEST_P(DatabaseTest, UnitDecrementNumUndecidedChildrenPositionIsolation) {
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        std::vector<ExpectedRecord> expected;
        for (int i = 0; i < 17; ++i) {
            expected.push_back({kLegalValues[i % kNumValues],
                                (i * 53 + 17) % (kRemotenessMax + 1)});
        }
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (const Position position :
             {Position{0}, Position{8}, Position{16}}) {
            SCOPED_TRACE(position);
            const int count = static_cast<int>(position) + 1;
            expected[position] = {kUndecided, count};
            ASSERT_NO_FATAL_FAILURE(WriteRecord(position, expected[position]));
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(position), count);
            --expected[position].remoteness;
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }
    }));
}

// Clearing a first, interior, or last record leaves all other records intact.
TEST_P(DatabaseTest, UnitClearNumUndecidedChildrenPositionIsolation) {
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        std::vector<ExpectedRecord> expected;
        for (int i = 0; i < 17; ++i) {
            expected.push_back({kLegalValues[i % kNumValues],
                                (i * 53 + 17) % (kRemotenessMax + 1)});
        }
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (const Position position :
             {Position{0}, Position{8}, Position{16}}) {
            SCOPED_TRACE(position);
            const int count = static_cast<int>(position) + 1;
            expected[position] = {kUndecided, count};
            ASSERT_NO_FATAL_FAILURE(WriteRecord(position, expected[position]));
            ASSERT_EQ(Db().ClearNumUndecidedChildren(position), count);
            expected[position].remoteness = 0;
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }
    }));
}

// Counter reads select the requested position and never mutate any record.
TEST_P(DatabaseTest, UnitGetNumUndecidedChildrenPositionIsolation) {
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        std::vector<ExpectedRecord> expected;
        for (int i = 0; i < 17; ++i) {
            expected.push_back({kLegalValues[i % kNumValues],
                                (i * 53 + 17) % (kRemotenessMax + 1)});
        }
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (const Position position :
             {Position{0}, Position{8}, Position{16}}) {
            SCOPED_TRACE(position);
            const int count = static_cast<int>(position) + 1;
            expected[position] = {kUndecided, count};
            ASSERT_NO_FATAL_FAILURE(WriteRecord(position, expected[position]));
            ASSERT_EQ(Db().GetNumUndecidedChildren(position), count);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }
    }));
}

// Valid requests yield usable estimates without prescribing storage geometry.
TEST_P(DatabaseTest, UnitTierMemUsageValidRequests) {
    REQUIRE_DATABASE_MEMBER(TierMemUsage);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const Tier tiers[] = {0, kStoredTier, kMissingTier};
        const int64_t sizes[] = {0, 1, 2, 17};
        for (const Tier tier : tiers) {
            for (const int64_t size : sizes) {
                SCOPED_TRACE(::testing::Message()
                             << "Tier " << tier << ", size " << size);
                ASSERT_NE(Db().TierMemUsage(tier, size), SIZE_MAX);
            }
        }
    }));
}

// Concurrent estimates support the same valid identifier and size partitions.
TEST_P(DatabaseTest, UnitConcurrentTierMemUsageValidRequests) {
    REQUIRE_DATABASE_MEMBER(ConcurrentTierMemUsage);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const Tier tiers[] = {0, kStoredTier, kMissingTier};
        const int64_t sizes[] = {0, 1, 2, 17};
        for (const Tier tier : tiers) {
            for (const int64_t size : sizes) {
                SCOPED_TRACE(::testing::Message()
                             << "Tier " << tier << ", size " << size);
                ASSERT_NE(Db().ConcurrentTierMemUsage(tier, size), SIZE_MAX);
            }
        }
    }));
}

// Each negative ordinary tier is rejected in its own initialized lifetime.
TEST_P(DatabaseTest, UnitTierMemUsageRejectsNegativeTiers) {
    REQUIRE_DATABASE_MEMBER(TierMemUsage);
    const Tier tiers[] = {kIllegalTier, -2, INT64_MIN};
    for (const Tier tier : tiers) {
        SCOPED_TRACE(tier);
        ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            ASSERT_EQ(Db().TierMemUsage(tier, 17), SIZE_MAX);
        }));
    }
}

// Each negative ordinary size is independently rejected with a valid tier.
TEST_P(DatabaseTest, UnitTierMemUsageRejectsNegativeSizes) {
    REQUIRE_DATABASE_MEMBER(TierMemUsage);
    const int64_t sizes[] = {-1, -2, INT64_MIN};
    for (const int64_t size : sizes) {
        SCOPED_TRACE(size);
        ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            ASSERT_EQ(Db().TierMemUsage(kStoredTier, size), SIZE_MAX);
        }));
    }
}

// Concurrent estimates reject sampled negative tiers before tier creation.
TEST_P(DatabaseTest, UnitConcurrentTierMemUsageRejectsNegativeTiers) {
    REQUIRE_DATABASE_MEMBER(ConcurrentTierMemUsage);
    const Tier tiers[] = {kIllegalTier, -2, INT64_MIN};
    for (const Tier tier : tiers) {
        SCOPED_TRACE(tier);
        ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            ASSERT_EQ(Db().ConcurrentTierMemUsage(tier, 17), SIZE_MAX);
        }));
    }
}

// Negative concurrent sizes have the same explicit sentinel return contract.
TEST_P(DatabaseTest, UnitConcurrentTierMemUsageRejectsNegativeSizes) {
    REQUIRE_DATABASE_MEMBER(ConcurrentTierMemUsage);
    const int64_t sizes[] = {-1, -2, INT64_MIN};
    for (const int64_t size : sizes) {
        SCOPED_TRACE(size);
        ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            ASSERT_EQ(Db().ConcurrentTierMemUsage(kStoredTier, size), SIZE_MAX);
        }));
    }
}

// The ordinary estimate can be queried before allocating the solving tier.
TEST_P(DatabaseTest, UnitTierMemUsageBeforeOrdinaryCreation) {
    REQUIRE_DATABASE_MEMBER(TierMemUsage);
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NE(Db().TierMemUsage(kStoredTier, 17), SIZE_MAX);
        ASSERT_NO_FATAL_FAILURE(CreateTier(Db().CreateSolvingTier, 17));
        if (Db().FreeSolvingTier != nullptr) {
            ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        } else {
            Finalize();
        }
    }));
}

// A new sandbox has no checkpoint for an otherwise valid tier.
TEST_P(DatabaseTest, UnitCheckpointExistsAbsent) {
    REQUIRE_DATABASE_MEMBER(CheckpointExists);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        EXPECT_FALSE(Db().CheckpointExists(kStoredTier));
    }));
}

// Existence queries distinguish a saved tier from a different missing tier.
TEST_P(DatabaseTest, UnitCheckpointExistsPresent) {
    REQUIRE_DATABASE_MEMBER(CheckpointExists);
    REQUIRE_DATABASE_CHECKPOINT();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        ASSERT_NO_FATAL_FAILURE(
            SaveCheckpoint(kStoredTier, {{kWin, 7}}, {0x19, 0x00, 0xe3}));
        EXPECT_TRUE(Db().CheckpointExists(kStoredTier));
        EXPECT_FALSE(Db().CheckpointExists(kMissingTier));
    }));
}

// Saving preserves the input bytes and every legal record in the snapshot.
TEST_P(DatabaseTest, UnitCheckpointSaveRecordsAndStatus) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const auto records = AllRecords();
        std::vector<unsigned char> status = {0x00, 0x91, 0xff, 0x00, 0x37};
        const auto original_status = status;
        ASSERT_NO_FATAL_FAILURE(SaveCheckpoint(kStoredTier, records, status));
        ASSERT_EQ(status, original_status);
        std::vector<unsigned char> restored_status(status.size(), 0x5a);
        ASSERT_NO_FATAL_FAILURE(
            LoadCheckpoint(kStoredTier, static_cast<int64_t>(records.size()),
                           restored_status));
        ASSERT_EQ(restored_status, original_status);
        ASSERT_NO_FATAL_FAILURE(AssertRecords(records));
    }));
}

// A later save replaces old records and a shorter status with a longer one.
TEST_P(DatabaseTest, UnitCheckpointSaveOverwritesShortStatusWithLongStatus) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const std::vector<ExpectedRecord> before = {
            {kUndecided, 9}, {kLose, 0}, {kWin, kRemotenessMax}};
        const std::vector<ExpectedRecord> after = {
            {kTie, kRemotenessMax}, {kDraw, 1}, {kUndecided, 0}};
        const std::vector<unsigned char> old_status = {0x19, 0x00, 0x7f};
        std::vector<unsigned char> new_status(33);
        for (size_t i = 0; i < new_status.size(); ++i) {
            new_status[i] = static_cast<unsigned char>(3 * i);
        }
        const auto original_status = new_status;
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, before));
        ASSERT_EQ(Db().CheckpointSave(old_status.data(), old_status.size()),
                  kSuccess);
        for (size_t i = 0; i < after.size(); ++i) {
            ASSERT_NO_FATAL_FAILURE(
                WriteRecord(static_cast<Position>(i), after[i]));
        }
        ASSERT_EQ(Db().CheckpointSave(new_status.data(), new_status.size()),
                  kSuccess);
        ASSERT_EQ(new_status, original_status);
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        std::vector<unsigned char> restored_status(new_status.size(), 0x5a);
        ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
            kStoredTier, static_cast<int64_t>(after.size()), restored_status));
        ASSERT_EQ(restored_status, original_status);
        ASSERT_NO_FATAL_FAILURE(AssertRecords(after));
    }));
}

// Shortening status exposes only the newest logical payload after restoration.
TEST_P(DatabaseTest, UnitCheckpointSaveOverwritesLongStatusWithShortStatus) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const std::vector<ExpectedRecord> before = {
            {kUndecided, 9}, {kLose, 0}, {kWin, kRemotenessMax}};
        const std::vector<ExpectedRecord> after = {
            {kTie, kRemotenessMax}, {kDraw, 1}, {kUndecided, 0}};
        const std::vector<unsigned char> old_status(33, 0xc7);
        std::vector<unsigned char> new_status = {0xd2, 0x00, 0x7f};
        const auto original_status = new_status;
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, before));
        ASSERT_EQ(Db().CheckpointSave(old_status.data(), old_status.size()),
                  kSuccess);
        for (size_t i = 0; i < after.size(); ++i) {
            ASSERT_NO_FATAL_FAILURE(
                WriteRecord(static_cast<Position>(i), after[i]));
        }
        ASSERT_EQ(Db().CheckpointSave(new_status.data(), new_status.size()),
                  kSuccess);
        ASSERT_EQ(new_status, original_status);
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        std::vector<unsigned char> restored_status(new_status.size(), 0x5a);
        ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
            kStoredTier, static_cast<int64_t>(after.size()), restored_status));
        ASSERT_EQ(restored_status, original_status);
        ASSERT_NO_FATAL_FAILURE(AssertRecords(after));
    }));
}

// Status is binary data, including NULs, all byte values, and odd lengths.
TEST_P(DatabaseTest, UnitCheckpointSaveBinaryStatus) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const std::vector<ExpectedRecord> records = {
            {kUndecided, 7}, {kDraw, 0}, {kWin, kRemotenessMax}};
        std::vector<std::vector<unsigned char>> statuses = {
            {0x00},
            std::vector<unsigned char>(5, 0x00),
            std::vector<unsigned char>(7, 0xff),
            {0x12, 0x00, 0x34, 0x56, 0x00, 0x78, 0x9a, 0x00}};
        std::vector<unsigned char> all_bytes;
        for (int value = 0; value < 256; ++value) {
            all_bytes.push_back(static_cast<unsigned char>(value));
        }
        statuses.push_back(all_bytes);
        for (const auto &status : statuses) {
            SCOPED_TRACE(status.size());
            const auto original_status = status;
            ASSERT_NO_FATAL_FAILURE(
                SaveCheckpoint(kStoredTier, records, status));
            ASSERT_EQ(status, original_status);
            std::vector<unsigned char> restored_status(status.size(), 0x5a);
            ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
                kStoredTier, static_cast<int64_t>(records.size()),
                restored_status));
            ASSERT_EQ(restored_status, original_status);
            if (Db().GetValue != nullptr && Db().GetRemoteness != nullptr) {
                ASSERT_NO_FATAL_FAILURE(AssertRecords(records));
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        }
    }));
}

// Loading creates a readable solving tier and restores the exact status.
TEST_P(DatabaseTest, UnitCheckpointLoadRestoresFixture) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const std::vector<ExpectedRecord> records = {
            {kUndecided, 6}, {kWin, kRemotenessMax}, {kDraw, 0}};
        const std::vector<unsigned char> status = {0x00, 0xff, 0x31};
        ASSERT_NO_FATAL_FAILURE(SaveCheckpoint(kStoredTier, records, status));
        std::vector<unsigned char> restored_status(status.size(), 0x5a);
        ASSERT_NO_FATAL_FAILURE(
            LoadCheckpoint(kStoredTier, static_cast<int64_t>(records.size()),
                           restored_status));
        ASSERT_EQ(restored_status, status);
        ASSERT_NO_FATAL_FAILURE(AssertRecords(records));
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_TRUE(Db().IsTierLoaded(kStoredTier));
        }
    }));
}

// Missing-checkpoint loading explicitly preserves status and residency.
TEST_P(DatabaseTest, UnitCheckpointLoadMissingDoesNothing) {
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        std::vector<unsigned char> status(13, 0x5a);
        const auto original_status = status;
        const bool was_loaded =
            Db().IsTierLoaded != nullptr && Db().IsTierLoaded(kMissingTier);
        StopAfterError();
        ASSERT_NE(
            Db().CheckpointLoad(kMissingTier, 1, status.data(), status.size()),
            kSuccess);
        ASSERT_EQ(status, original_status);
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_EQ(Db().IsTierLoaded(kMissingTier), was_loaded);
        }
    }));
}

// Loading writes the status payload without touching either guard region.
TEST_P(DatabaseTest, UnitCheckpointLoadOutputBounds) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const std::vector<ExpectedRecord> records = {{kWin, 7}};
        const std::vector<unsigned char> status = {0x01, 0x00, 0x99, 0xff,
                                                   0x42};
        ASSERT_NO_FATAL_FAILURE(SaveCheckpoint(kStoredTier, records, status));
        constexpr size_t kGuardSize = 8;
        std::vector<unsigned char> restored_status(
            2 * kGuardSize + status.size(), 0xa7);
        auto expected = restored_status;
        for (size_t i = 0; i < status.size(); ++i) {
            expected[kGuardSize + i] = status[i];
        }
        ASSERT_EQ(Db().CheckpointLoad(kStoredTier, 1,
                                      restored_status.data() + kGuardSize,
                                      status.size()),
                  kSuccess);
        solving_tier_active_ = true;
        ASSERT_EQ(restored_status, expected);
    }));
}

// Removing an existing checkpoint makes its existence query false.
TEST_P(DatabaseTest, UnitCheckpointRemoveExisting) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointRemove);
    REQUIRE_DATABASE_MEMBER(CheckpointExists);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        ASSERT_NO_FATAL_FAILURE(
            SaveCheckpoint(kStoredTier, {{kWin, 7}}, {0x19}));
        ASSERT_EQ(Db().CheckpointRemove(kStoredTier), kSuccess);
        EXPECT_FALSE(Db().CheckpointExists(kStoredTier));
    }));
}

// Removing a checkpoint absent from a fresh sandbox has the specified error.
TEST_P(DatabaseTest, UnitCheckpointRemoveAbsent) {
    REQUIRE_DATABASE_MEMBER(CheckpointRemove);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        ASSERT_EQ(Db().CheckpointRemove(kStoredTier), kFileSystemError);
    }));
}

// Successful removal is followed by the same absent-checkpoint error.
TEST_P(DatabaseTest, UnitCheckpointRemoveAlreadyRemoved) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointRemove);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        ASSERT_NO_FATAL_FAILURE(
            SaveCheckpoint(kStoredTier, {{kWin, 7}}, {0x19}));
        ASSERT_EQ(Db().CheckpointRemove(kStoredTier), kSuccess);
        StopAfterError();
        ASSERT_EQ(Db().CheckpointRemove(kStoredTier), kFileSystemError);
    }));
}

// Removing one tier leaves another tier's records and status intact.
TEST_P(DatabaseTest, UnitCheckpointRemovePreservesOtherTiers) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointRemove);
    REQUIRE_DATABASE_MEMBER(CheckpointExists);
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const std::vector<ExpectedRecord> other_records = {
            {kUndecided, 5}, {kTie, kRemotenessMax}, {kDraw, 0}};
        const std::vector<unsigned char> other_status = {0x73, 0x00, 0x91,
                                                         0xff};
        ASSERT_NO_FATAL_FAILURE(
            SaveCheckpoint(kStoredTier, {{kWin, 7}}, {0x19}));
        ASSERT_NO_FATAL_FAILURE(
            SaveCheckpoint(kMissingTier, other_records, other_status));
        ASSERT_EQ(Db().CheckpointRemove(kStoredTier), kSuccess);
        ASSERT_FALSE(Db().CheckpointExists(kStoredTier));
        ASSERT_TRUE(Db().CheckpointExists(kMissingTier));
        std::vector<unsigned char> restored_status(other_status.size(), 0x5a);
        ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
            kMissingTier, static_cast<int64_t>(other_records.size()),
            restored_status));
        ASSERT_EQ(restored_status, other_status);
        ASSERT_NO_FATAL_FAILURE(AssertRecords(other_records));
    }));
}

// A null, zero-length save is permitted without promising success.
TEST_P(DatabaseTest, UnitCheckpointSaveZeroStatus) {
    REQUIRE_DATABASE_CHECKPOINT();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        ASSERT_NO_FATAL_FAILURE(
            CreateAndPopulate(kStoredTier, {{kUndecided, 7}}));
        if (Db().CheckpointSave(nullptr, 0) != kSuccess) {
            StopAfterError();
        }
    }));
}

// A zero-status load uses a matching checkpoint when its preparation succeeds.
TEST_P(DatabaseTest, UnitCheckpointLoadZeroStatus) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            ASSERT_NO_FATAL_FAILURE(
                CreateAndPopulate(kStoredTier, {{kUndecided, 7}}));
            if (Db().CheckpointSave(nullptr, 0) != kSuccess) {
                StopAfterError();
                GTEST_SKIP() << "Could not prepare a zero-status checkpoint.";
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
            if (Db().CheckpointLoad(kStoredTier, 1, nullptr, 0) == kSuccess) {
                solving_tier_active_ = true;
            } else {
                StopAfterError();
            }
        },
        "Could not prepare a zero-status checkpoint."));
}

// A non-null pointer is permitted for an empty status, without promising
// success.
TEST_P(DatabaseTest, UnitCheckpointSaveZeroSizeWithBuffer) {
    REQUIRE_DATABASE_CHECKPOINT();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, {{kWin, 7}}));
        unsigned char status = 0x5a;
        if (Db().CheckpointSave(&status, 0) != kSuccess) {
            StopAfterError();
        }
    }));
}

// A non-null load status with zero length is a checked argument error.
TEST_P(DatabaseTest, UnitCheckpointLoadZeroSizeWithBuffer) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        ASSERT_NO_FATAL_FAILURE(
            SaveCheckpoint(kStoredTier, {{kWin, 7}}, {0x19}));
        unsigned char status = 0x5a;
        StopAfterError();
        ASSERT_EQ(Db().CheckpointLoad(kStoredTier, 1, &status, 0),
                  kIllegalArgumentError);
    }));
}

// Saving rejects a null status pointer with positive length.
TEST_P(DatabaseTest, UnitCheckpointSaveNullStatusWithPositiveSize) {
    REQUIRE_DATABASE_CHECKPOINT();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, {{kWin, 7}}));
        StopAfterError();
        ASSERT_EQ(Db().CheckpointSave(nullptr, 1), kIllegalArgumentError);
    }));
}

// Loading rejects a null status pointer with an otherwise matching length.
TEST_P(DatabaseTest, UnitCheckpointLoadNullStatusWithPositiveSize) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        ASSERT_NO_FATAL_FAILURE(
            SaveCheckpoint(kStoredTier, {{kWin, 7}}, {0x19}));
        StopAfterError();
        ASSERT_EQ(Db().CheckpointLoad(kStoredTier, 1, nullptr, 1),
                  kIllegalArgumentError);
    }));
}

// Any normal API return is accepted; crashes and sanitizer exits still fail.
TEST_P(DatabaseTest, UnitCheckpointSaveWithoutSolvingTier) {
    REQUIRE_DATABASE_MEMBER(CheckpointSave);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        (void)Db().CheckpointSave(nullptr, 0);
    }));
}

// A valid saved tier becomes resident and exposes an available public view.
TEST_P(DatabaseTest, UnitLoadTierValidArchive) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(LoadTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> records = {
            {kUndecided, 0}, {kWin, 17}, {kLose, kRemotenessMax}};
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, records));
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(
            kStoredTier, static_cast<int64_t>(records.size())));
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_TRUE(Db().IsTierLoaded(kStoredTier));
        }
        if (Db().GetValueFromLoaded != nullptr) {
            EXPECT_EQ(Db().GetValueFromLoaded(kStoredTier, 1),
                      records[1].value);
        } else if (Db().GetRemotenessFromLoaded != nullptr) {
            EXPECT_EQ(Db().GetRemotenessFromLoaded(kStoredTier, 1),
                      records[1].remoteness);
        }
    }));
}

// Unloading a previously loaded archive removes its in-memory residency.
TEST_P(DatabaseTest, UnitUnloadTierPreviouslyLoaded) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(LoadTier);
    REQUIRE_DATABASE_MEMBER(UnloadTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, {{kWin, 7}}));
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(kStoredTier, 1));
        ASSERT_NO_FATAL_FAILURE(UnloadResidentTier(kStoredTier));
        if (Db().IsTierLoaded != nullptr) {
            EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
    }));
}

// UnloadTier rejects an ordinary solving tier.
TEST_P(DatabaseTest, UnitUnloadTierRejectsOrdinarySolvingTier) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(UnloadTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(CreateTier(Db().CreateSolvingTier, 17));
        StopAfterError();
        ASSERT_EQ(Db().UnloadTier(kStoredTier), kIllegalArgumentError);
    }));
}

// UnloadTier rejects a concurrent solving tier.
TEST_P(DatabaseTest, UnitUnloadTierRejectsConcurrentSolvingTier) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(UnloadTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(Db().CreateConcurrentSolvingTier, 17));
        StopAfterError();
        ASSERT_EQ(Db().UnloadTier(kStoredTier), kIllegalArgumentError);
    }));
}

// Checkpoint restoration still creates a solving tier, not an unloadable tier.
TEST_P(DatabaseTest, UnitUnloadTierRejectsCheckpointSolvingTier) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_MEMBER(UnloadTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> records = {
            {kUndecided, 0}, {kWin, 17}, {kLose, kRemotenessMax}};
        const std::vector<unsigned char> status = {0x52, 0x00, 0xff};
        ASSERT_NO_FATAL_FAILURE(SaveCheckpoint(kStoredTier, records, status));
        std::vector<unsigned char> restored(status.size(), 0);
        ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
            kStoredTier, static_cast<int64_t>(records.size()), restored));
        StopAfterError();
        ASSERT_EQ(Db().UnloadTier(kStoredTier), kIllegalArgumentError);
    }));
}

// Fresh tiers are not resident, without requiring any storage-writing method.
TEST_P(DatabaseTest, UnitIsTierLoadedAbsent) {
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
        EXPECT_FALSE(Db().IsTierLoaded(kMissingTier));
    }));
}

// A saved archive does not by itself establish in-memory residency.
TEST_P(DatabaseTest, UnitIsTierLoadedArchiveOnly) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, {{kWin, 7}}));
        EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
        EXPECT_FALSE(Db().IsTierLoaded(kMissingTier));
    }));
}

// Loading an archive is one documented route to in-memory residency.
TEST_P(DatabaseTest, UnitIsTierLoadedLoadedArchive) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(LoadTier);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, {{kWin, 7}}));
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(kStoredTier, 1));
        EXPECT_TRUE(Db().IsTierLoaded(kStoredTier));
    }));
}

// An ordinary solving tier is resident before any record is read.
TEST_P(DatabaseTest, UnitIsTierLoadedOrdinarySolvingTier) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(CreateTier(Db().CreateSolvingTier, 17));
        EXPECT_TRUE(Db().IsTierLoaded(kStoredTier));
    }));
}

// A concurrent solving tier is resident before any record is read.
TEST_P(DatabaseTest, UnitIsTierLoadedConcurrentSolvingTier) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(Db().CreateConcurrentSolvingTier, 17));
        EXPECT_TRUE(Db().IsTierLoaded(kStoredTier));
    }));
}

// A checkpoint-restored solving tier is included in residency queries.
TEST_P(DatabaseTest, UnitIsTierLoadedCheckpointSolvingTier) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> records = {
            {kUndecided, 0}, {kWin, 17}, {kLose, kRemotenessMax}};
        const std::vector<unsigned char> status = {0x52, 0x00, 0xff};
        ASSERT_NO_FATAL_FAILURE(SaveCheckpoint(kStoredTier, records, status));
        std::vector<unsigned char> restored(status.size(), 0);
        ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
            kStoredTier, static_cast<int64_t>(records.size()), restored));
        EXPECT_TRUE(Db().IsTierLoaded(kStoredTier));
    }));
}

// A successfully unloaded archive is no longer resident.
TEST_P(DatabaseTest, UnitIsTierLoadedAfterUnload) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(LoadTier);
    REQUIRE_DATABASE_MEMBER(UnloadTier);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, {{kWin, 7}}));
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(kStoredTier, 1));
        ASSERT_NO_FATAL_FAILURE(UnloadResidentTier(kStoredTier));
        EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
    }));
}

// Freeing a ordinary solving tier removes its residency.
TEST_P(DatabaseTest, UnitIsTierLoadedAfterOrdinaryFree) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(CreateTier(Db().CreateSolvingTier, 17));
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
    }));
}

// Freeing a concurrent solving tier removes its residency.
TEST_P(DatabaseTest, UnitIsTierLoadedAfterConcurrentFree) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(Db().CreateConcurrentSolvingTier, 17));
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
    }));
}

// The proper solving-tier release removes a checkpoint-restored tier.
TEST_P(DatabaseTest, UnitIsTierLoadedAfterCheckpointFree) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> records = {
            {kUndecided, 0}, {kWin, 17}, {kLose, kRemotenessMax}};
        const std::vector<unsigned char> status = {0x52, 0x00, 0xff};
        ASSERT_NO_FATAL_FAILURE(SaveCheckpoint(kStoredTier, records, status));
        std::vector<unsigned char> restored(status.size(), 0);
        ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
            kStoredTier, static_cast<int64_t>(records.size()), restored));
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
    }));
}

// Loaded value reads cover all legal value/remoteness pairs.
TEST_P(DatabaseTest, UnitGetValueFromLoadedMatrix) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(LoadTier);
    REQUIRE_DATABASE_MEMBER(GetValueFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, records));
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(
            kStoredTier, static_cast<int64_t>(records.size())));
        for (int pass = 0; pass < 2; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t i = 0; i < records.size(); ++i) {
                SCOPED_TRACE(i);
                const Position position = static_cast<Position>(i);
                ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, position),
                          records[i].value);
                if (Db().GetRemotenessFromLoaded != nullptr) {
                    ASSERT_EQ(
                        Db().GetRemotenessFromLoaded(kStoredTier, position),
                        records[i].remoteness);
                }
            }
        }
    }));
}

// The loaded value view reads current ordinary solving records.
TEST_P(DatabaseTest, UnitGetValueFromLoadedOrdinarySolvingTier) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(GetValueFromLoaded);
    if (Db().SetValueRemoteness == nullptr && Db().SetValue == nullptr) {
        GTEST_SKIP() << "No value writer is supported.";
    }
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        const bool complete_pairs = MissingRecordDependency() == nullptr;
        ASSERT_NO_FATAL_FAILURE(CreateTier(
            Db().CreateSolvingTier, static_cast<int64_t>(records.size())));
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            const Position position = static_cast<Position>(i);
            if (complete_pairs) {
                ASSERT_NO_FATAL_FAILURE(WriteRecord(position, records[i]));
            } else {
                ASSERT_EQ(Db().SetValue(position, records[i].value), kSuccess);
            }
        }
        for (int pass = 0; pass < 2; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t i = 0; i < records.size(); ++i) {
                SCOPED_TRACE(i);
                const Position position = static_cast<Position>(i);
                ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, position),
                          records[i].value);
                if (complete_pairs && Db().GetRemotenessFromLoaded != nullptr) {
                    ASSERT_EQ(
                        Db().GetRemotenessFromLoaded(kStoredTier, position),
                        records[i].remoteness);
                }
            }
        }
    }));
}

// The loaded value view reads current concurrent solving records.
TEST_P(DatabaseTest, UnitGetValueFromLoadedConcurrentSolvingTier) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(GetValueFromLoaded);
    if (Db().SetValueRemoteness == nullptr && Db().SetValue == nullptr) {
        GTEST_SKIP() << "No value writer is supported.";
    }
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        const bool complete_pairs = MissingRecordDependency() == nullptr;
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(Db().CreateConcurrentSolvingTier,
                       static_cast<int64_t>(records.size())));
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            const Position position = static_cast<Position>(i);
            if (complete_pairs) {
                ASSERT_NO_FATAL_FAILURE(WriteRecord(position, records[i]));
            } else {
                ASSERT_EQ(Db().SetValue(position, records[i].value), kSuccess);
            }
        }
        for (int pass = 0; pass < 2; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t i = 0; i < records.size(); ++i) {
                SCOPED_TRACE(i);
                const Position position = static_cast<Position>(i);
                ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, position),
                          records[i].value);
                if (complete_pairs && Db().GetRemotenessFromLoaded != nullptr) {
                    ASSERT_EQ(
                        Db().GetRemotenessFromLoaded(kStoredTier, position),
                        records[i].remoteness);
                }
            }
        }
    }));
}

// The loaded value view includes every checkpoint-restored pair.
TEST_P(DatabaseTest, UnitGetValueFromLoadedCheckpointSolvingTier) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_MEMBER(GetValueFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        const std::vector<unsigned char> status = {0x52, 0x00, 0xff};
        ASSERT_NO_FATAL_FAILURE(SaveCheckpoint(kStoredTier, records, status));
        std::vector<unsigned char> restored(status.size(), 0);
        ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
            kStoredTier, static_cast<int64_t>(records.size()), restored));
        for (int pass = 0; pass < 2; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t i = 0; i < records.size(); ++i) {
                SCOPED_TRACE(i);
                const Position position = static_cast<Position>(i);
                ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, position),
                          records[i].value);
                if (Db().GetRemotenessFromLoaded != nullptr) {
                    ASSERT_EQ(
                        Db().GetRemotenessFromLoaded(kStoredTier, position),
                        records[i].remoteness);
                }
            }
        }
    }));
}

// A nonresident missing tier returns the loaded-value error sentinel.
TEST_P(DatabaseTest, UnitGetValueFromLoadedNonresidentTier) {
    REQUIRE_DATABASE_MEMBER(GetValueFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, 0), kErrorValue);
    }));
}

// An archive alone does not enable the loaded-value accessor.
TEST_P(DatabaseTest, UnitGetValueFromLoadedArchiveOnlyTier) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(GetValueFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, {{kWin, 7}}));
        StopAfterError();
        ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, 0), kErrorValue);
    }));
}

// Loaded value queries distinguish resident tiers at identical positions.
TEST_P(DatabaseTest, UnitGetValueFromLoadedDistinguishesTiers) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(LoadTier);
    REQUIRE_DATABASE_MEMBER(GetValueFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> first = {
            {kUndecided, 0}, {kWin, 17}, {kLose, kRemotenessMax}};
        const std::vector<ExpectedRecord> second = {
            {kTie, 19}, {kLose, 0}, {kDraw, kRemotenessMax - 1}};
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, first));
        ASSERT_NO_FATAL_FAILURE(SaveTier(kMissingTier, second));
        ASSERT_NO_FATAL_FAILURE(
            LoadResidentTier(kStoredTier, static_cast<int64_t>(first.size())));
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(
            kMissingTier, static_cast<int64_t>(second.size())));
        for (size_t i = 0; i < first.size(); ++i) {
            SCOPED_TRACE(i);
            const Position position = static_cast<Position>(i);
            ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, position),
                      first[i].value);
            ASSERT_EQ(Db().GetValueFromLoaded(kMissingTier, position),
                      second[i].value);
            ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, position),
                      first[i].value);
        }
    }));
}

// Loaded remoteness reads cover all legal value/remoteness pairs.
TEST_P(DatabaseTest, UnitGetRemotenessFromLoadedMatrix) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(LoadTier);
    REQUIRE_DATABASE_MEMBER(GetRemotenessFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, records));
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(
            kStoredTier, static_cast<int64_t>(records.size())));
        for (int pass = 0; pass < 2; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t i = 0; i < records.size(); ++i) {
                SCOPED_TRACE(i);
                const Position position = static_cast<Position>(i);
                ASSERT_EQ(Db().GetRemotenessFromLoaded(kStoredTier, position),
                          records[i].remoteness);
                if (Db().GetValueFromLoaded != nullptr) {
                    ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, position),
                              records[i].value);
                }
            }
        }
    }));
}

// The loaded remoteness view reads current ordinary solving records.
TEST_P(DatabaseTest, UnitGetRemotenessFromLoadedOrdinarySolvingTier) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(GetRemotenessFromLoaded);
    if (Db().SetValueRemoteness == nullptr && Db().SetRemoteness == nullptr) {
        GTEST_SKIP() << "No remoteness writer is supported.";
    }
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        const bool complete_pairs = MissingRecordDependency() == nullptr;
        ASSERT_NO_FATAL_FAILURE(CreateTier(
            Db().CreateSolvingTier, static_cast<int64_t>(records.size())));
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            const Position position = static_cast<Position>(i);
            if (complete_pairs) {
                ASSERT_NO_FATAL_FAILURE(WriteRecord(position, records[i]));
            } else {
                ASSERT_EQ(Db().SetRemoteness(position, records[i].remoteness),
                          kSuccess);
            }
        }
        for (int pass = 0; pass < 2; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t i = 0; i < records.size(); ++i) {
                SCOPED_TRACE(i);
                const Position position = static_cast<Position>(i);
                ASSERT_EQ(Db().GetRemotenessFromLoaded(kStoredTier, position),
                          records[i].remoteness);
                if (complete_pairs && Db().GetValueFromLoaded != nullptr) {
                    ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, position),
                              records[i].value);
                }
            }
        }
    }));
}

// The loaded remoteness view reads current concurrent solving records.
TEST_P(DatabaseTest, UnitGetRemotenessFromLoadedConcurrentSolvingTier) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(GetRemotenessFromLoaded);
    if (Db().SetValueRemoteness == nullptr && Db().SetRemoteness == nullptr) {
        GTEST_SKIP() << "No remoteness writer is supported.";
    }
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        const bool complete_pairs = MissingRecordDependency() == nullptr;
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(Db().CreateConcurrentSolvingTier,
                       static_cast<int64_t>(records.size())));
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            const Position position = static_cast<Position>(i);
            if (complete_pairs) {
                ASSERT_NO_FATAL_FAILURE(WriteRecord(position, records[i]));
            } else {
                ASSERT_EQ(Db().SetRemoteness(position, records[i].remoteness),
                          kSuccess);
            }
        }
        for (int pass = 0; pass < 2; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t i = 0; i < records.size(); ++i) {
                SCOPED_TRACE(i);
                const Position position = static_cast<Position>(i);
                ASSERT_EQ(Db().GetRemotenessFromLoaded(kStoredTier, position),
                          records[i].remoteness);
                if (complete_pairs && Db().GetValueFromLoaded != nullptr) {
                    ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, position),
                              records[i].value);
                }
            }
        }
    }));
}

// The loaded remoteness view includes every checkpoint-restored pair.
TEST_P(DatabaseTest, UnitGetRemotenessFromLoadedCheckpointSolvingTier) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_MEMBER(GetRemotenessFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        const std::vector<unsigned char> status = {0x52, 0x00, 0xff};
        ASSERT_NO_FATAL_FAILURE(SaveCheckpoint(kStoredTier, records, status));
        std::vector<unsigned char> restored(status.size(), 0);
        ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
            kStoredTier, static_cast<int64_t>(records.size()), restored));
        for (int pass = 0; pass < 2; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t i = 0; i < records.size(); ++i) {
                SCOPED_TRACE(i);
                const Position position = static_cast<Position>(i);
                ASSERT_EQ(Db().GetRemotenessFromLoaded(kStoredTier, position),
                          records[i].remoteness);
                if (Db().GetValueFromLoaded != nullptr) {
                    ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, position),
                              records[i].value);
                }
            }
        }
    }));
}

// A nonresident missing tier returns the loaded-remoteness error sentinel.
TEST_P(DatabaseTest, UnitGetRemotenessFromLoadedNonresidentTier) {
    REQUIRE_DATABASE_MEMBER(GetRemotenessFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        ASSERT_EQ(Db().GetRemotenessFromLoaded(kStoredTier, 0),
                  kErrorRemoteness);
    }));
}

// An archive alone does not enable the loaded-remoteness accessor.
TEST_P(DatabaseTest, UnitGetRemotenessFromLoadedArchiveOnlyTier) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(GetRemotenessFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, {{kWin, 7}}));
        StopAfterError();
        ASSERT_EQ(Db().GetRemotenessFromLoaded(kStoredTier, 0),
                  kErrorRemoteness);
    }));
}

// Loaded remoteness queries distinguish resident tiers at identical positions.
TEST_P(DatabaseTest, UnitGetRemotenessFromLoadedDistinguishesTiers) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(LoadTier);
    REQUIRE_DATABASE_MEMBER(GetRemotenessFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> first = {
            {kUndecided, 0}, {kWin, 17}, {kLose, kRemotenessMax}};
        const std::vector<ExpectedRecord> second = {
            {kTie, 19}, {kLose, 0}, {kDraw, kRemotenessMax - 1}};
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, first));
        ASSERT_NO_FATAL_FAILURE(SaveTier(kMissingTier, second));
        ASSERT_NO_FATAL_FAILURE(
            LoadResidentTier(kStoredTier, static_cast<int64_t>(first.size())));
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(
            kMissingTier, static_cast<int64_t>(second.size())));
        for (size_t i = 0; i < first.size(); ++i) {
            SCOPED_TRACE(i);
            const Position position = static_cast<Position>(i);
            ASSERT_EQ(Db().GetRemotenessFromLoaded(kStoredTier, position),
                      first[i].remoteness);
            ASSERT_EQ(Db().GetRemotenessFromLoaded(kMissingTier, position),
                      second[i].remoteness);
            ASSERT_EQ(Db().GetRemotenessFromLoaded(kStoredTier, position),
                      first[i].remoteness);
        }
    }));
}

// A missing normal archive has its explicit runtime-error result.
TEST_P(DatabaseTest, UnitLoadTierMissingArchive) {
    REQUIRE_DATABASE_MEMBER(LoadTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        ASSERT_EQ(Db().LoadTier(kStoredTier, 1), kRuntimeError);
    }));
}

// LoadTier rejects the current ordinary solving tier.
TEST_P(DatabaseTest, UnitLoadTierRejectsOrdinarySolvingTier) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(LoadTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(CreateTier(Db().CreateSolvingTier, 17));
        StopAfterError();
        ASSERT_EQ(Db().LoadTier(kStoredTier, 17), kIllegalArgumentError);
    }));
}

// LoadTier rejects the current concurrent solving tier.
TEST_P(DatabaseTest, UnitLoadTierRejectsConcurrentSolvingTier) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(LoadTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(Db().CreateConcurrentSolvingTier, 17));
        StopAfterError();
        ASSERT_EQ(Db().LoadTier(kStoredTier, 17), kIllegalArgumentError);
    }));
}

// LoadTier also rejects a checkpoint-restored solving tier.
TEST_P(DatabaseTest, UnitLoadTierRejectsCheckpointSolvingTier) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_MEMBER(LoadTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> records = {
            {kUndecided, 0}, {kWin, 17}, {kLose, kRemotenessMax}};
        const std::vector<unsigned char> status = {0x52, 0x00, 0xff};
        ASSERT_NO_FATAL_FAILURE(SaveCheckpoint(kStoredTier, records, status));
        std::vector<unsigned char> restored(status.size(), 0);
        ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
            kStoredTier, static_cast<int64_t>(records.size()), restored));
        StopAfterError();
        ASSERT_EQ(
            Db().LoadTier(kStoredTier, static_cast<int64_t>(records.size())),
            kIllegalArgumentError);
    }));
}

// UnloadTier rejects a tier that has never been resident.
TEST_P(DatabaseTest, UnitUnloadTierRejectsNonresidentTier) {
    REQUIRE_DATABASE_MEMBER(UnloadTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        ASSERT_EQ(Db().UnloadTier(kStoredTier), kIllegalArgumentError);
    }));
}

// A saved archive still cannot be unloaded before it is resident.
TEST_P(DatabaseTest, UnitUnloadTierRejectsArchiveOnlyTier) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(UnloadTier);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, {{kWin, 7}}));
        StopAfterError();
        ASSERT_EQ(Db().UnloadTier(kStoredTier), kIllegalArgumentError);
    }));
}

// Successful initialization produces a usable probe through an available view.
TEST_P(DatabaseTest, UnitProbeInitValidStorage) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_PROBE_READER();
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> records = {{kWin, 7}};
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, records));
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        ASSERT_NO_FATAL_FAILURE(
            AssertProbeRecord({kStoredTier, 0}, records[0]));
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// Initialization rejects null storage without creating a real probe.
TEST_P(DatabaseTest, UnitProbeInitNull) {
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        ASSERT_EQ(Db().ProbeInit(nullptr), kIllegalArgumentError);
    }));
}

// Null destruction succeeds without requiring the initialization operation.
TEST_P(DatabaseTest, UnitProbeDestroyNull) {
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(Db().ProbeDestroy(nullptr), kSuccess);
    }));
}

// A successfully initialized probe can be destroyed before its first read.
TEST_P(DatabaseTest, UnitProbeDestroyUnused) {
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// Destruction releases a probe after successful reads from permanent storage.
TEST_P(DatabaseTest, UnitProbeDestroyAfterReads) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_PROBE_READER();
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> records = {{kWin, 7},
                                                     {kLose, kRemotenessMax}};
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, records));
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        ASSERT_NO_FATAL_FAILURE(
            AssertProbeRecord({kStoredTier, 0}, records[0]));
        ASSERT_NO_FATAL_FAILURE(
            AssertProbeRecord({kStoredTier, 1}, records[1]));
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// Value probing returns every legal fixture value in either access direction.
TEST_P(DatabaseTest, UnitProbeValueMatrix) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, records));
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        for (const bool reverse : {false, true}) {
            SCOPED_TRACE(reverse ? "reverse" : "forward");
            for (size_t offset = 0; offset < records.size(); ++offset) {
                const size_t index =
                    reverse ? records.size() - 1 - offset : offset;
                SCOPED_TRACE(index);
                const TierPosition position = {kStoredTier,
                                               static_cast<Position>(index)};
                ASSERT_EQ(Db().ProbeValue(Probe(), position),
                          records[index].value);
            }
        }
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// Remoteness probing preserves every legal pair's independent expected field.
TEST_P(DatabaseTest, UnitProbeRemotenessMatrix) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, records));
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        for (const bool reverse : {false, true}) {
            SCOPED_TRACE(reverse ? "reverse" : "forward");
            for (size_t offset = 0; offset < records.size(); ++offset) {
                const size_t index =
                    reverse ? records.size() - 1 - offset : offset;
                SCOPED_TRACE(index);
                const TierPosition position = {kStoredTier,
                                               static_cast<Position>(index)};
                ASSERT_EQ(Db().ProbeRemoteness(Probe(), position),
                          records[index].remoteness);
            }
        }
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// A missing tier produces the value error sentinel, then permits destruction.
TEST_P(DatabaseTest, UnitProbeValueNotFound) {
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        StopAfterError();
        ASSERT_EQ(Db().ProbeValue(Probe(), {kMissingTier, 0}), kErrorValue);
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// A missing tier produces the remoteness error sentinel without further reads.
TEST_P(DatabaseTest, UnitProbeRemotenessNotFound) {
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        StopAfterError();
        ASSERT_EQ(Db().ProbeRemoteness(Probe(), {kMissingTier, 0}),
                  kErrorRemoteness);
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// Reinitializing an unused probe needs no archive writer or record reader.
TEST_P(DatabaseTest, UnitProbeInitRepeated) {
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// Reinitializing the same object after reads leaves a usable probe.
TEST_P(DatabaseTest, UnitProbeInitRepeatedAfterReads) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_PROBE_READER();
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> records = {
            {kWin, 7}, {kUndecided, 0}, {kLose, kRemotenessMax}};
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, records));
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        ASSERT_NO_FATAL_FAILURE(
            AssertProbeRecord({kStoredTier, 0}, records[0]));
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        ASSERT_NO_FATAL_FAILURE(
            AssertProbeRecord({kStoredTier, 2}, records[2]));
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// Destroying the same successfully released object again remains a no-op.
TEST_P(DatabaseTest, UnitProbeDestroyRepeated) {
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// A value read error leaves destruction available for mandatory cleanup.
TEST_P(DatabaseTest, UnitProbeDestroyAfterFailedValueRead) {
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        StopAfterError();
        ASSERT_EQ(Db().ProbeValue(Probe(), {kMissingTier, 0}), kErrorValue);
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// A remoteness read error also requires successful destruction before finalize.
TEST_P(DatabaseTest, UnitProbeDestroyAfterFailedRemotenessRead) {
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        StopAfterError();
        ASSERT_EQ(Db().ProbeRemoteness(Probe(), {kMissingTier, 0}),
                  kErrorRemoteness);
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// The reported capacity is nonnegative; zero is a supported result.
TEST_P(DatabaseTest, UnitSegmentationMaxNumBuffersReportsCapacity) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        EXPECT_GE(Db().segmentation.MaxNumBuffers(), 0);
    }));
}

// One explicitly initialized position is usable in a supported buffer.
TEST_P(DatabaseTest, UnitSegmentationCreateBuffersSinglePosition) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.GetValue);
    REQUIRE_DATABASE_MEMBER(segmentation.GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, 1));
            const std::vector<ExpectedRecord> records = {
                {kUndecided, kRemotenessMax}};
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, records));
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(0, records));
        },
        "No positive segment-buffer capacity."));
}

// Distinct buffers expose all requested positions without aliasing.
TEST_P(DatabaseTest, UnitSegmentationCreateBuffersMultipleBuffers) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.GetValue);
    REQUIRE_DATABASE_MEMBER(segmentation.GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum < 2) {
                GTEST_SKIP() << "Fewer than two segment buffers are supported.";
            }
            const int count = maximum < 3 ? maximum : 3;
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(count, kSegmentSize));
            std::vector<std::vector<ExpectedRecord>> expected(count);
            for (int buffer = 0; buffer < count; ++buffer) {
                for (int64_t offset = 0; offset < kSegmentSize; ++offset) {
                    expected[buffer].push_back(
                        {kLegalValues[(buffer + offset) % kNumValues],
                         static_cast<int>((buffer * 53 + offset * 11 + 3) %
                                          (kRemotenessMax + 1))});
                }
                ASSERT_NO_FATAL_FAILURE(
                    WriteSegmentBuffer(buffer, expected[buffer]));
            }
            for (int buffer = 0; buffer < count; ++buffer) {
                SCOPED_TRACE(buffer);
                ASSERT_NO_FATAL_FAILURE(
                    AssertSegmentBuffer(buffer, expected[buffer]));
            }
        },
        "Fewer than two segment buffers are supported."));
}

// The maximum count is accepted when it fits the bounded fixture budget.
TEST_P(DatabaseTest, UnitSegmentationCreateBuffersAtMaximum) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum > kSegmentBufferBudget) {
                GTEST_SKIP() << "Reported capacity exceeds the test budget.";
            }
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(maximum, 3));
            if (Db().segmentation.SetValueRemoteness == nullptr ||
                Db().segmentation.GetValue == nullptr ||
                Db().segmentation.GetRemoteness == nullptr) {
                return;
            }
            std::vector<std::vector<ExpectedRecord>> expected(maximum);
            for (int buffer = 0; buffer < maximum; ++buffer) {
                for (int offset = 0; offset < 3; ++offset) {
                    expected[buffer].push_back(
                        {kLegalValues[(buffer + offset) % kNumValues],
                         (buffer * 53 + offset * 11 + 3) %
                             (kRemotenessMax + 1)});
                }
                ASSERT_NO_FATAL_FAILURE(
                    WriteSegmentBuffer(buffer, expected[buffer]));
            }
            for (int buffer = 0; buffer < maximum; ++buffer) {
                SCOPED_TRACE(buffer);
                ASSERT_NO_FATAL_FAILURE(
                    AssertSegmentBuffer(buffer, expected[buffer]));
            }
        },
        "Reported segment-buffer capacity exceeds the test budget."));
}

// Zero buffers establish a valid context without any record accesses.
TEST_P(DatabaseTest, UnitSegmentationCreateBuffersZeroCount) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(0, kSegmentSize));
    }));
}

// Zero-sized buffers are valid even when only a zero-buffer context is
// possible.
TEST_P(DatabaseTest, UnitSegmentationCreateBuffersZeroSize) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        int count = 0;
        if (Db().segmentation.MaxNumBuffers != nullptr) {
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            count = maximum == 0 ? 0 : 1;
        }
        ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(count, 0));
    }));
}

// The all-zero buffer count and segment size are independently valid.
TEST_P(DatabaseTest, UnitSegmentationCreateBuffersZeroCountAndSize) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(0, 0));
    }));
}

// A negative buffer count has its specified checked argument error.
TEST_P(DatabaseTest, UnitSegmentationCreateBuffersNegativeCount) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        ASSERT_EQ(
            Db().segmentation.CreateBuffers(kStoredTier, -1, kSegmentSize),
            kIllegalArgumentError);
    }));
}

// The minimum int buffer count is rejected without requesting storage.
TEST_P(DatabaseTest, UnitSegmentationCreateBuffersMinimumCount) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        ASSERT_EQ(
            Db().segmentation.CreateBuffers(kStoredTier, INT_MIN, kSegmentSize),
            kIllegalArgumentError);
    }));
}

// Negative size is rejected even with an otherwise valid zero count.
TEST_P(DatabaseTest, UnitSegmentationCreateBuffersNegativeSize) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        ASSERT_EQ(Db().segmentation.CreateBuffers(kStoredTier, 0, -1),
                  kIllegalArgumentError);
    }));
}

// The minimum int64_t segment size has the same checked argument error.
TEST_P(DatabaseTest, UnitSegmentationCreateBuffersMinimumSize) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        ASSERT_EQ(Db().segmentation.CreateBuffers(kStoredTier, 0, INT64_MIN),
                  kIllegalArgumentError);
    }));
}

// A representable request above capacity is rejected within a safe budget.
TEST_P(DatabaseTest, UnitSegmentationCreateBuffersAboveCapacityZeroSize) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == INT_MAX || maximum >= kSegmentBufferBudget) {
                GTEST_SKIP()
                    << "Cannot exceed capacity within the test budget.";
            }
            const int count = maximum + 1;
            StopAfterError();
            ASSERT_EQ(Db().segmentation.CreateBuffers(kStoredTier, count, 0),
                      kIllegalArgumentError);
        },
        "Cannot exceed segment-buffer capacity within the safe test budget."));
}

// A representable request above capacity is rejected within a safe budget.
TEST_P(DatabaseTest, UnitSegmentationCreateBuffersAboveCapacitySinglePosition) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == INT_MAX || maximum >= kSegmentBufferBudget) {
                GTEST_SKIP()
                    << "Cannot exceed capacity within the test budget.";
            }
            const int count = maximum + 1;
            StopAfterError();
            ASSERT_EQ(Db().segmentation.CreateBuffers(kStoredTier, count, 1),
                      kIllegalArgumentError);
        },
        "Cannot exceed segment-buffer capacity within the safe test budget."));
}

// Successful release accepts either positive or zero-count buffer contexts.
TEST_P(DatabaseTest, UnitSegmentationFreeBuffersSucceeds) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.FreeBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        int count = 0;
        if (Db().segmentation.MaxNumBuffers != nullptr) {
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            count = maximum == 0 ? 0 : 1;
        }
        ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(count, kSegmentSize));
        ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
    }));
}

// A formerly active index is rejected after successful buffer release.
TEST_P(DatabaseTest, UnitSegmentationFreeBuffersMakesIndicesInactive) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    REQUIRE_DATABASE_MEMBER(segmentation.Load);
    REQUIRE_DATABASE_MEMBER(segmentation.FreeBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, kSegmentSize));
            std::vector<ExpectedRecord> records;
            for (int64_t offset = 0; offset < kSegmentSize; ++offset) {
                records.push_back({kLegalValues[offset % kNumValues],
                                   static_cast<int>((offset * 11 + 3) %
                                                    (kRemotenessMax + 1))});
            }
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, records));
            ASSERT_EQ(Db().segmentation.Flush(0, 0), kSuccess);
            ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
            StopAfterError();
            ASSERT_EQ(Db().segmentation.Load(0, 0), kIllegalArgumentError);
        },
        "No positive segment-buffer capacity."));
}

// Releasing buffers with no context is a successful no-op.
TEST_P(DatabaseTest, UnitSegmentationFreeBuffersWithoutContext) {
    REQUIRE_DATABASE_MEMBER(segmentation.FreeBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
    }));
}

// Repeated successful buffer release remains valid after the context is gone.
TEST_P(DatabaseTest, UnitSegmentationFreeBuffersRepeated) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.FreeBuffers);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        int count = 0;
        if (Db().segmentation.MaxNumBuffers != nullptr) {
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            count = maximum == 0 ? 0 : 1;
        }
        ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(count, kSegmentSize));
        ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
        ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
    }));
}

// Segment value reads cover every legal pair without another getter dependency.
TEST_P(DatabaseTest, UnitSegmentationGetValueAllLegalValues) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.GetValue);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            const auto records = AllRecords();
            ASSERT_NO_FATAL_FAILURE(
                CreateSegmentBuffers(1, static_cast<int64_t>(records.size())));
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, records));
            for (size_t index = 0; index < records.size(); ++index) {
                SCOPED_TRACE(index);
                ASSERT_EQ(
                    Db().segmentation.GetValue(0, static_cast<int64_t>(index)),
                    records[index].value);
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
        },
        "No positive segment-buffer capacity."));
}

// Successive base-five patterns distinguish every buffer/offset using values.
TEST_P(DatabaseTest, UnitSegmentationGetValueSelectsBufferAndOffset) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.GetValue);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum < 2) {
                GTEST_SKIP() << "Fewer than two segment buffers supported.";
            }
            const int count = std::min(maximum, 3);
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(count, kSegmentSize));
            const size_t addresses = static_cast<size_t>(count * kSegmentSize);
            for (size_t place = 1; place < addresses; place *= kNumValues) {
                SCOPED_TRACE(place);
                std::vector<std::vector<ExpectedRecord>> expected(count);
                for (int buffer = 0; buffer < count; ++buffer) {
                    for (int64_t offset = 0; offset < kSegmentSize; ++offset) {
                        const size_t address =
                            static_cast<size_t>(buffer * kSegmentSize + offset);
                        expected[buffer].push_back(
                            {kLegalValues[(address / place) % kNumValues],
                             static_cast<int>(address % (kRemotenessMax + 1))});
                    }
                    ASSERT_NO_FATAL_FAILURE(
                        WriteSegmentBuffer(buffer, expected[buffer]));
                }
                for (int buffer = count - 1; buffer >= 0; --buffer) {
                    SCOPED_TRACE(buffer);
                    for (int64_t offset = 0; offset < kSegmentSize; ++offset) {
                        SCOPED_TRACE(offset);
                        ASSERT_EQ(Db().segmentation.GetValue(buffer, offset),
                                  expected[buffer][offset].value);
                    }
                }
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
        },
        "Fewer than two segment buffers supported."));
}

// Remoteness reads cover all values without requiring the segment value getter.
TEST_P(DatabaseTest, UnitSegmentationGetRemotenessLegalDomain) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            const auto records = AllRecords();
            ASSERT_NO_FATAL_FAILURE(
                CreateSegmentBuffers(1, static_cast<int64_t>(records.size())));
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, records));
            for (size_t index = 0; index < records.size(); ++index) {
                SCOPED_TRACE(index);
                ASSERT_EQ(Db().segmentation.GetRemoteness(
                              0, static_cast<int64_t>(index)),
                          records[index].remoteness);
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
        },
        "No positive segment-buffer capacity."));
}

// Distinct remotenesses identify every initialized buffer and valid offset.
TEST_P(DatabaseTest, UnitSegmentationGetRemotenessSelectsBufferAndOffset) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum < 2) {
                GTEST_SKIP() << "Fewer than two segment buffers supported.";
            }
            const int count = std::min(maximum, 3);
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(count, kSegmentSize));
            std::vector<std::vector<ExpectedRecord>> expected(count);
            for (int buffer = 0; buffer < count; ++buffer) {
                for (int64_t offset = 0; offset < kSegmentSize; ++offset) {
                    const int address =
                        static_cast<int>(buffer * kSegmentSize + offset);
                    expected[buffer].push_back(
                        {kLegalValues[address % kNumValues], address});
                }
                ASSERT_NO_FATAL_FAILURE(
                    WriteSegmentBuffer(buffer, expected[buffer]));
            }
            for (int buffer = count - 1; buffer >= 0; --buffer) {
                SCOPED_TRACE(buffer);
                for (int64_t offset = kSegmentSize - 1; offset >= 0; --offset) {
                    SCOPED_TRACE(offset);
                    ASSERT_EQ(Db().segmentation.GetRemoteness(buffer, offset),
                              expected[buffer][offset].remoteness);
                }
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
        },
        "Fewer than two segment buffers supported."));
}

// The void segment setter preserves every independently expected legal pair.
TEST_P(DatabaseTest, UnitSegmentationSetValueRemotenessLegalPairs) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.GetValue);
    REQUIRE_DATABASE_MEMBER(segmentation.GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            const auto records = AllRecords();
            ASSERT_NO_FATAL_FAILURE(
                CreateSegmentBuffers(1, static_cast<int64_t>(records.size())));
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, records));
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(0, records));
            ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
        },
        "No positive segment-buffer capacity."));
}

// All value transitions and remoteness edges replace a previously seeded pair.
TEST_P(DatabaseTest, UnitSegmentationSetValueRemotenessOverwritesPair) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.GetValue);
    REQUIRE_DATABASE_MEMBER(segmentation.GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, kSegmentSize));
            std::vector<ExpectedRecord> expected;
            for (int64_t offset = 0; offset < kSegmentSize; ++offset) {
                expected.push_back({kLegalValues[offset % kNumValues],
                                    static_cast<int>(offset)});
            }
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, expected));
            const int remotenesses[] = {0, kRemotenessMax};
            const int64_t target = kSegmentSize / 2;
            for (const Value old_value : kLegalValues) {
                for (const Value new_value : kLegalValues) {
                    for (const int old_remoteness : remotenesses) {
                        for (const int new_remoteness : remotenesses) {
                            SCOPED_TRACE(::testing::Message()
                                         << old_value << ',' << old_remoteness
                                         << " -> " << new_value << ','
                                         << new_remoteness);
                            Db().segmentation.SetValueRemoteness(
                                0, target, old_value, old_remoteness);
                            expected[target] = {old_value, old_remoteness};
                            ASSERT_NO_FATAL_FAILURE(
                                AssertSegmentBuffer(0, expected));
                            Db().segmentation.SetValueRemoteness(
                                0, target, new_value, new_remoteness);
                            expected[target] = {new_value, new_remoteness};
                            ASSERT_NO_FATAL_FAILURE(
                                AssertSegmentBuffer(0, expected));
                        }
                    }
                }
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
        },
        "No positive segment-buffer capacity."));
}

// A target write preserves initialized neighbors and every other active buffer.
TEST_P(DatabaseTest, UnitSegmentationSetValueRemotenessPreservesOtherRecords) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.GetValue);
    REQUIRE_DATABASE_MEMBER(segmentation.GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum < 2) {
                GTEST_SKIP() << "Fewer than two segment buffers supported.";
            }
            const int count = std::min(maximum, 3);
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(count, kSegmentSize));
            std::vector<std::vector<ExpectedRecord>> expected(count);
            for (int buffer = 0; buffer < count; ++buffer) {
                for (int64_t offset = 0; offset < kSegmentSize; ++offset) {
                    const int address =
                        static_cast<int>(buffer * kSegmentSize + offset);
                    expected[buffer].push_back(
                        {kLegalValues[address % kNumValues], address});
                }
                ASSERT_NO_FATAL_FAILURE(
                    WriteSegmentBuffer(buffer, expected[buffer]));
            }
            const int64_t offsets[] = {0, kSegmentSize / 2, kSegmentSize - 1};
            for (int buffer = 0; buffer < count; ++buffer) {
                SCOPED_TRACE(buffer);
                for (const int64_t offset : offsets) {
                    SCOPED_TRACE(offset);
                    auto &target = expected[buffer][offset];
                    target = {kLegalValues[(target.value + 1) % kNumValues],
                              kRemotenessMax - target.remoteness};
                    Db().segmentation.SetValueRemoteness(
                        buffer, offset, target.value, target.remoteness);
                    for (int observed = 0; observed < count; ++observed) {
                        SCOPED_TRACE(observed);
                        ASSERT_NO_FATAL_FAILURE(
                            AssertSegmentBuffer(observed, expected[observed]));
                    }
                }
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
        },
        "Fewer than two segment buffers supported."));
}

// Distinct logical patterns expose both buffer aliasing and offset mistakes.
std::vector<ExpectedRecord> SegmentStorageRecords(int variant) {
    std::vector<ExpectedRecord> records;
    records.reserve(static_cast<size_t>(kSegmentSize));
    for (int64_t offset = 0; offset < kSegmentSize; ++offset) {
        const int index = static_cast<int>(offset);
        const int remoteness =
            offset == 0 ? 0
            : offset + 1 == kSegmentSize
                ? kRemotenessMax
                : (37 * index + 83 * variant) % (kRemotenessMax + 1);
        records.push_back(
            {kLegalValues[(index + variant) % kNumValues], remoteness});
    }
    return records;
}

// Loading replaces overwritten memory with the explicitly saved records.
TEST_P(DatabaseTest, UnitSegmentationLoadRestoresSavedSegment) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    REQUIRE_DATABASE_MEMBER(segmentation.Load);
    REQUIRE_DATABASE_MEMBER(segmentation.GetValue);
    REQUIRE_DATABASE_MEMBER(segmentation.GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, kSegmentSize));
            const auto saved = SegmentStorageRecords(1);
            const auto replacement = SegmentStorageRecords(2);
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, saved));
            // Intended tier: three full segments, with disk indices 0..2.
            ASSERT_EQ(Db().segmentation.Flush(0, 2), kSuccess);
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, replacement));
            ASSERT_EQ(Db().segmentation.Load(0, 2), kSuccess);
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(0, saved));
        },
        "No positive segment-buffer capacity."));
}

// Loading into either buffer preserves the other buffer's initialized data.
TEST_P(DatabaseTest, UnitSegmentationLoadSelectsDestinationBuffer) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    REQUIRE_DATABASE_MEMBER(segmentation.Load);
    REQUIRE_DATABASE_MEMBER(segmentation.GetValue);
    REQUIRE_DATABASE_MEMBER(segmentation.GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum < 2) {
                GTEST_SKIP() << "Fewer than two segment buffers are supported.";
            }
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(2, kSegmentSize));
            const auto saved = SegmentStorageRecords(1);
            const auto second = SegmentStorageRecords(2);
            const auto replacement = SegmentStorageRecords(3);
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, saved));
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(1, second));
            // Disk index 2 is independent of both active buffer indices.
            ASSERT_EQ(Db().segmentation.Flush(0, 2), kSuccess);
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, replacement));
            ASSERT_EQ(Db().segmentation.Load(1, 2), kSuccess);
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(0, replacement));
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(1, saved));
            ASSERT_EQ(Db().segmentation.Load(0, 2), kSuccess);
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(0, saved));
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(1, saved));
        },
        "Fewer than two segment buffers are supported."));
}

// Inactive destinations are checked with an otherwise valid saved segment.
TEST_P(DatabaseTest, UnitSegmentationLoadRejectsInactiveBuffer) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    REQUIRE_DATABASE_MEMBER(segmentation.Load);
    for (const int buffer : {-1, 1}) {
        SCOPED_TRACE(buffer);
        ASSERT_NO_FATAL_FAILURE(RunIsolated(
            [&] {
                ASSERT_NO_FATAL_FAILURE(Initialize());
                const int maximum = Db().segmentation.MaxNumBuffers();
                ASSERT_GE(maximum, 0);
                if (maximum == 0) {
                    GTEST_SKIP() << "No positive segment-buffer capacity.";
                }
                ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, kSegmentSize));
                ASSERT_NO_FATAL_FAILURE(
                    WriteSegmentBuffer(0, SegmentStorageRecords(1)));
                ASSERT_EQ(Db().segmentation.Flush(0, 2), kSuccess);
                StopAfterError();
                ASSERT_EQ(Db().segmentation.Load(buffer, 2),
                          kIllegalArgumentError);
            },
            "No positive segment-buffer capacity."));
        if (IsSkipped()) {
            return;
        }
    }
}

// Inactive-buffer validation does not require a tier or segment context.
TEST_P(DatabaseTest, UnitSegmentationLoadRejectsBufferBeforeCreation) {
    REQUIRE_DATABASE_MEMBER(segmentation.Load);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        ASSERT_EQ(Db().segmentation.Load(0, 0), kIllegalArgumentError);
    }));
}

// An unsaved index inside the intended tier range reports a filesystem error.
TEST_P(DatabaseTest, UnitSegmentationLoadMissingSegment) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.Load);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, kSegmentSize));
            // Intended tier: three full segments; index 1 has not been saved.
            StopAfterError();
            ASSERT_EQ(Db().segmentation.Load(0, 1), kFileSystemError);
        },
        "No positive segment-buffer capacity."));
}

// Flushed records survive replacement of their source memory buffer.
TEST_P(DatabaseTest, UnitSegmentationFlushSavesBufferContents) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    REQUIRE_DATABASE_MEMBER(segmentation.Load);
    REQUIRE_DATABASE_MEMBER(segmentation.GetValue);
    REQUIRE_DATABASE_MEMBER(segmentation.GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, kSegmentSize));
            const auto saved = SegmentStorageRecords(1);
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, saved));
            ASSERT_EQ(Db().segmentation.Flush(0, 2), kSuccess);
            ASSERT_NO_FATAL_FAILURE(
                WriteSegmentBuffer(0, SegmentStorageRecords(3)));
            ASSERT_EQ(Db().segmentation.Load(0, 2), kSuccess);
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(0, saved));
        },
        "No positive segment-buffer capacity."));
}

// Selected source buffers determine disk records regardless of disk index.
TEST_P(DatabaseTest, UnitSegmentationFlushSelectsSourceBuffer) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    REQUIRE_DATABASE_MEMBER(segmentation.Load);
    REQUIRE_DATABASE_MEMBER(segmentation.GetValue);
    REQUIRE_DATABASE_MEMBER(segmentation.GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum < 2) {
                GTEST_SKIP() << "Fewer than two segment buffers are supported.";
            }
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(2, kSegmentSize));
            const auto first = SegmentStorageRecords(1);
            const auto second = SegmentStorageRecords(2);
            const auto replacement = SegmentStorageRecords(3);
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, first));
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(1, second));
            ASSERT_EQ(Db().segmentation.Flush(1, 0), kSuccess);
            ASSERT_EQ(Db().segmentation.Flush(0, 2), kSuccess);
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, replacement));
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(1, replacement));
            ASSERT_EQ(Db().segmentation.Load(0, 0), kSuccess);
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(0, second));
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(1, replacement));
            ASSERT_EQ(Db().segmentation.Load(1, 2), kSuccess);
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(0, second));
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(1, first));
        },
        "Fewer than two segment buffers are supported."));
}

// An inactive source is an argument error without requiring record accessors.
TEST_P(DatabaseTest, UnitSegmentationFlushRejectsInactiveBuffer) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    for (const int buffer : {-1, 1}) {
        SCOPED_TRACE(buffer);
        ASSERT_NO_FATAL_FAILURE(RunIsolated(
            [&] {
                ASSERT_NO_FATAL_FAILURE(Initialize());
                const int maximum = Db().segmentation.MaxNumBuffers();
                ASSERT_GE(maximum, 0);
                if (maximum == 0) {
                    GTEST_SKIP() << "No positive segment-buffer capacity.";
                }
                ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, kSegmentSize));
                StopAfterError();
                ASSERT_EQ(Db().segmentation.Flush(buffer, 2),
                          kIllegalArgumentError);
            },
            "No positive segment-buffer capacity."));
        if (IsSkipped()) {
            return;
        }
    }
}

// An inactive flush is checked before any buffer context is established.
TEST_P(DatabaseTest, UnitSegmentationFlushRejectsBufferBeforeCreation) {
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        StopAfterError();
        ASSERT_EQ(Db().segmentation.Flush(0, 0), kIllegalArgumentError);
    }));
}

// Releasing buffers makes their former source indices inactive.
TEST_P(DatabaseTest, UnitSegmentationFlushRejectsBufferAfterFree) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.FreeBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, kSegmentSize));
            ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
            StopAfterError();
            ASSERT_EQ(Db().segmentation.Flush(0, 0), kIllegalArgumentError);
        },
        "No positive segment-buffer capacity."));
}

// Negative load indices are checked independently in fresh active contexts.
TEST_P(DatabaseTest, UnitSegmentationLoadRejectsNegativeDiskIndices) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.Load);
    for (const int segment : {-1, INT_MIN}) {
        SCOPED_TRACE(segment);
        ASSERT_NO_FATAL_FAILURE(RunIsolated(
            [&] {
                ASSERT_NO_FATAL_FAILURE(Initialize());
                const int maximum = Db().segmentation.MaxNumBuffers();
                ASSERT_GE(maximum, 0);
                if (maximum == 0) {
                    GTEST_SKIP() << "No positive segment-buffer capacity.";
                }
                ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, kSegmentSize));
                StopAfterError();
                ASSERT_EQ(Db().segmentation.Load(0, segment),
                          kIllegalArgumentError);
            },
            "No positive segment-buffer capacity."));
        if (IsSkipped()) {
            return;
        }
    }
}

// A valid, initialized flush source still rejects every tested negative index.
TEST_P(DatabaseTest, UnitSegmentationFlushRejectsNegativeDiskIndices) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    for (const int segment : {-1, INT_MIN}) {
        SCOPED_TRACE(segment);
        ASSERT_NO_FATAL_FAILURE(RunIsolated(
            [&] {
                ASSERT_NO_FATAL_FAILURE(Initialize());
                const int maximum = Db().segmentation.MaxNumBuffers();
                ASSERT_GE(maximum, 0);
                if (maximum == 0) {
                    GTEST_SKIP() << "No positive segment-buffer capacity.";
                }
                ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, kSegmentSize));
                ASSERT_NO_FATAL_FAILURE(
                    WriteSegmentBuffer(0, SegmentStorageRecords(1)));
                StopAfterError();
                ASSERT_EQ(Db().segmentation.Flush(0, segment),
                          kIllegalArgumentError);
            },
            "No positive segment-buffer capacity."));
        if (IsSkipped()) {
            return;
        }
    }
}

// Replacing one saved index restores its newest records after memory changes.
TEST_P(DatabaseTest, UnitSegmentationFlushOverwritesExistingIndex) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    REQUIRE_DATABASE_MEMBER(segmentation.Load);
    REQUIRE_DATABASE_MEMBER(segmentation.GetValue);
    REQUIRE_DATABASE_MEMBER(segmentation.GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, kSegmentSize));
            const auto old = SegmentStorageRecords(1);
            const auto replacement = SegmentStorageRecords(2);
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, old));
            ASSERT_EQ(Db().segmentation.Flush(0, 1), kSuccess);
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, replacement));
            ASSERT_EQ(Db().segmentation.Flush(0, 1), kSuccess);
            ASSERT_NO_FATAL_FAILURE(
                WriteSegmentBuffer(0, SegmentStorageRecords(3)));
            ASSERT_EQ(Db().segmentation.Load(0, 1), kSuccess);
            ASSERT_NO_FATAL_FAILURE(AssertSegmentBuffer(0, replacement));
        },
        "No positive segment-buffer capacity."));
}

// A single saved segment consolidates every legal pair into a normal archive.
TEST_P(DatabaseTest, UnitSegmentationConsolidateSingleSegment) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    REQUIRE_DATABASE_MEMBER(segmentation.Consolidate);
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            const auto records = AllRecords();
            ASSERT_NO_FATAL_FAILURE(
                CreateSegmentBuffers(1, static_cast<int64_t>(records.size())));
            ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, records));
            ASSERT_EQ(Db().segmentation.Flush(0, 0), kSuccess);
            if (Db().TierStatus != nullptr) {
                ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
            }
            ASSERT_EQ(Db().segmentation.Consolidate(
                          static_cast<int64_t>(records.size()), 1),
                      kSuccess);
            if (Db().TierStatus != nullptr) {
                ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
            ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, records));
        },
        "No positive segment-buffer capacity."));
}

// Concatenation follows disk segment order when one buffer saves all segments.
TEST_P(DatabaseTest, UnitSegmentationConsolidateMultipleSegments) {
    REQUIRE_DATABASE_MEMBER(segmentation.MaxNumBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(segmentation.Flush);
    REQUIRE_DATABASE_MEMBER(segmentation.Consolidate);
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated(
        [&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            const int maximum = Db().segmentation.MaxNumBuffers();
            ASSERT_GE(maximum, 0);
            if (maximum == 0) {
                GTEST_SKIP() << "No positive segment-buffer capacity.";
            }
            constexpr int kNumSegments = 3;
            std::vector<ExpectedRecord> records;
            for (int i = 0; i < kNumSegments * kSegmentSize; ++i) {
                records.push_back({kLegalValues[i % kNumValues],
                                   (19 * i) % (kRemotenessMax + 1)});
            }
            records.front().remoteness = 0;
            records.back().remoteness = kRemotenessMax;
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(1, kSegmentSize));
            for (int segment = 0; segment < kNumSegments; ++segment) {
                SCOPED_TRACE(segment);
                const auto first = records.begin() + segment * kSegmentSize;
                const std::vector<ExpectedRecord> part(first,
                                                       first + kSegmentSize);
                ASSERT_NO_FATAL_FAILURE(WriteSegmentBuffer(0, part));
                ASSERT_EQ(Db().segmentation.Flush(0, segment), kSuccess);
            }
            ASSERT_EQ(Db().segmentation.Consolidate(
                          static_cast<int64_t>(records.size()), kNumSegments),
                      kSuccess);
            if (Db().TierStatus != nullptr) {
                ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
            ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, records));
        },
        "No positive segment-buffer capacity."));
}

// Empty consolidation requires no buffers, saved segments, or probe positions.
TEST_P(DatabaseTest, UnitSegmentationConsolidateEmptyTier) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.Consolidate);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(0, 0));
        if (Db().TierStatus != nullptr) {
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
        }
        ASSERT_EQ(Db().segmentation.Consolidate(0, 0), kSuccess);
        if (Db().TierStatus != nullptr) {
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        }
        ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
    }));
}

// All nonnegative empty-tier shapes are valid without physical segment files.
TEST_P(DatabaseTest, UnitSegmentationConsolidateEmptyTierShapes) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.Consolidate);
    for (int count : {0, 1, 3}) {
        for (int size : {0, 1, kSegmentSize}) {
            SCOPED_TRACE(::testing::Message()
                         << "Segment count " << count << ", capacity " << size);
            ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
                ASSERT_NO_FATAL_FAILURE(Initialize());
                ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(0, size));
                ASSERT_EQ(Db().segmentation.Consolidate(0, count), kSuccess);
                if (Db().TierStatus != nullptr) {
                    ASSERT_EQ(Db().TierStatus(kStoredTier),
                              kDbTierStatusSolved);
                }
                ASSERT_NO_FATAL_FAILURE(ReleaseSegmentBuffers());
            }));
        }
    }
}

// Negative tier sizes are checked in independent zero-buffer contexts.
TEST_P(DatabaseTest, UnitSegmentationConsolidateRejectsNegativeTierSize) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.Consolidate);
    const int64_t sizes[] = {-1, INT64_MIN};
    for (int64_t size : sizes) {
        SCOPED_TRACE(size);
        ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(0, 0));
            StopAfterError();
            ASSERT_EQ(Db().segmentation.Consolidate(size, 0),
                      kIllegalArgumentError);
        }));
    }
}

// Negative segment counts are checked independently of the valid tier size.
TEST_P(DatabaseTest, UnitSegmentationConsolidateRejectsNegativeSegmentCount) {
    REQUIRE_DATABASE_MEMBER(segmentation.CreateBuffers);
    REQUIRE_DATABASE_MEMBER(segmentation.Consolidate);
    for (int count : {-1, INT_MIN}) {
        SCOPED_TRACE(count);
        ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
            ASSERT_NO_FATAL_FAILURE(Initialize());
            ASSERT_NO_FATAL_FAILURE(CreateSegmentBuffers(0, 0));
            StopAfterError();
            ASSERT_EQ(Db().segmentation.Consolidate(0, count),
                      kIllegalArgumentError);
        }));
    }
}

// Initialization and finalization work with optional path observations only.
TEST_P(DatabaseTest, ComponentBasicLifecycle) {
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_NO_FATAL_FAILURE(ExpectSandboxPath());
        Finalize();
        ASSERT_NO_FATAL_FAILURE(ExpectFinalizedPath());
    }));
}

// The explicit solved declaration survives a complete database restart.
TEST_P(DatabaseTest, ComponentGameSolvedDeclarationPersists) {
    REQUIRE_DATABASE_MEMBER(SetGameSolved);
    REQUIRE_DATABASE_MEMBER(GameStatus);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(Db().GameStatus(), kDbGameStatusIncomplete);
        ASSERT_EQ(Db().SetGameSolved(), kSuccess);
        ASSERT_EQ(Db().GameStatus(), kDbGameStatusSolved);
        Finalize();
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_EQ(Db().GameStatus(), kDbGameStatusSolved);
    }));
}

// Ordinary residency is independent of the archive's solved status.
TEST_P(DatabaseTest, ComponentTierSolvedStatusLifecycle) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(TierStatus);
    REQUIRE_DATABASE_MEMBER(FlushSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const std::vector<ExpectedRecord> records = {
            {kUndecided, 0}, {kUndecided, kRemotenessMax},
            {kLose, 0},      {kLose, kRemotenessMax},
            {kDraw, 0},      {kDraw, kRemotenessMax},
            {kTie, 0},       {kTie, kRemotenessMax},
            {kWin, 0},       {kWin, kRemotenessMax}};
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, records));
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
        }
        ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
        }
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
    }));
}

// Concurrent-mode allocation follows the same serial status lifecycle.
TEST_P(DatabaseTest, ComponentTierSolvedStatusLifecycleConcurrent) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(TierStatus);
    REQUIRE_DATABASE_MEMBER(FlushSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            const std::vector<ExpectedRecord> records = {
                {kUndecided, 0}, {kUndecided, kRemotenessMax},
                {kLose, 0},      {kLose, kRemotenessMax},
                {kDraw, 0},      {kDraw, kRemotenessMax},
                {kTie, 0},       {kTie, kRemotenessMax},
                {kWin, 0},       {kWin, kRemotenessMax}};
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, records));
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
            if (Db().IsTierLoaded != nullptr) {
                ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
            }
            ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
            if (Db().IsTierLoaded != nullptr) {
                ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
            if (Db().IsTierLoaded != nullptr) {
                ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
            }
        }));
}

// Restart clears actual resident tiers while preserving the saved archive.
TEST_P(DatabaseTest, ComponentLifecycleRestartsWithFreshResidency) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    REQUIRE_DATABASE_MEMBER(TierStatus);
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> records = {
            {kUndecided, 0},
            {kLose, 1},
            {kDraw, kRemotenessMax / 2},
            {kTie, kRemotenessMax},
            {kWin, 7}};
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, records));
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        if (Db().LoadTier != nullptr) {
            ASSERT_NO_FATAL_FAILURE(LoadResidentTier(
                kStoredTier, static_cast<int64_t>(records.size())));
            ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
        }
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(
            kMissingTier, {{kLose, 17}, {kUndecided, kRemotenessMax}}));
        ASSERT_TRUE(Db().IsTierLoaded(kMissingTier));
        Finalize();
        ASSERT_NO_FATAL_FAILURE(ExpectFinalizedPath());
        ASSERT_NO_FATAL_FAILURE(Initialize());
        ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        ASSERT_FALSE(Db().IsTierLoaded(kMissingTier));
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, records));
    }));
}

// Default tier naming preserves distinct tiers within the same sandbox.
TEST_P(DatabaseTest, ComponentFallbackTierNamesRoundTrip) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize(nullptr));
        const std::vector<ExpectedRecord> first = {{kUndecided, 0},
                                                   {kLose, 1},
                                                   {kDraw, kRemotenessMax / 2},
                                                   {kTie, kRemotenessMax},
                                                   {kWin, 7}};
        const std::vector<ExpectedRecord> second = {{kWin, kRemotenessMax},
                                                    {kTie, 0},
                                                    {kLose, 17},
                                                    {kDraw, 1},
                                                    {kUndecided, 3}};
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, first));
        ASSERT_NO_FATAL_FAILURE(SaveTier(kMissingTier, second));
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, first));
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kMissingTier, second));
        if (Db().TierStatus != nullptr) {
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
            ASSERT_EQ(Db().TierStatus(kMissingTier), kDbTierStatusSolved);
        }
    }));
}

// Reusing a deterministic custom tier namer preserves both archives on restart.
TEST_P(DatabaseTest, ComponentCustomTierNamesRoundTrip) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize(GetTestTierName));
        const std::vector<ExpectedRecord> first = {{kUndecided, 0},
                                                   {kLose, 1},
                                                   {kDraw, kRemotenessMax / 2},
                                                   {kTie, kRemotenessMax},
                                                   {kWin, 7}};
        const std::vector<ExpectedRecord> second = {{kWin, kRemotenessMax},
                                                    {kTie, 0},
                                                    {kLose, 17},
                                                    {kDraw, 1},
                                                    {kUndecided, 3}};
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, first));
        ASSERT_NO_FATAL_FAILURE(SaveTier(kMissingTier, second));
        Finalize();
        ASSERT_NO_FATAL_FAILURE(Initialize(GetTestTierName));
        ASSERT_NO_FATAL_FAILURE(ExpectSandboxPath());
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kMissingTier, second));
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, first));
        if (Db().TierStatus != nullptr) {
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
            ASSERT_EQ(Db().TierStatus(kMissingTier), kDbTierStatusSolved);
        }
    }));
}

// Independent sandboxes preserve contrasting records and distinct tier status.
TEST_P(DatabaseTest, ComponentSandboxIsolation) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(TierStatus);
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        std::filesystem::path first_path;
        std::filesystem::path second_path;
        ASSERT_NO_FATAL_FAILURE(CreatePrivateSandbox(first_path));
        ASSERT_NO_FATAL_FAILURE(CreatePrivateSandbox(second_path));
        const std::vector<ExpectedRecord> first = {{kUndecided, 0},
                                                   {kLose, 1},
                                                   {kDraw, kRemotenessMax / 2},
                                                   {kTie, kRemotenessMax},
                                                   {kWin, 7}};
        const std::vector<ExpectedRecord> second = {{kWin, kRemotenessMax},
                                                    {kTie, 0},
                                                    {kLose, 17},
                                                    {kDraw, 1},
                                                    {kUndecided, 3}};
        const std::vector<ExpectedRecord> first_only = {{kDraw, 0}};

        ASSERT_NO_FATAL_FAILURE(InitializeAt(first_path));
        ASSERT_NO_FATAL_FAILURE(ExpectSandboxPath());
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
        ASSERT_EQ(Db().TierStatus(kMissingTier), kDbTierStatusMissing);
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, first));
        ASSERT_NO_FATAL_FAILURE(SaveTier(kMissingTier, first_only));
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        ASSERT_EQ(Db().TierStatus(kMissingTier), kDbTierStatusSolved);
        Finalize();

        ASSERT_NO_FATAL_FAILURE(InitializeAt(second_path));
        ASSERT_NO_FATAL_FAILURE(ExpectSandboxPath());
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
        ASSERT_EQ(Db().TierStatus(kMissingTier), kDbTierStatusMissing);
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, second));
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        ASSERT_EQ(Db().TierStatus(kMissingTier), kDbTierStatusMissing);
        Finalize();

        ASSERT_NO_FATAL_FAILURE(InitializeAt(first_path));
        ASSERT_NO_FATAL_FAILURE(ExpectSandboxPath());
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        ASSERT_EQ(Db().TierStatus(kMissingTier), kDbTierStatusSolved);
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, first));
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kMissingTier, first_only));
        Finalize();

        ASSERT_NO_FATAL_FAILURE(InitializeAt(second_path));
        ASSERT_NO_FATAL_FAILURE(ExpectSandboxPath());
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        ASSERT_EQ(Db().TierStatus(kMissingTier), kDbTierStatusMissing);
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, second));
        Finalize();
        ASSERT_NO_FATAL_FAILURE(ExpectFinalizedPath());
    }));
}

// Value-only solving preserves independently seeded remotenesses throughout.
TEST_P(DatabaseTest, ComponentSerialSetterFamiliesValueOnly) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected(17);
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(static_cast<int64_t>(expected.size())));
        for (size_t i = 0; i < expected.size(); ++i) {
            expected[i].remoteness = kRepresentativeRemotenesses[i % 4];
            ASSERT_EQ(Db().SetRemoteness(static_cast<Position>(i),
                                         expected[i].remoteness),
                      kSuccess);
        }
        // Do not observe any value until every value has been initialized.
        for (size_t i = 0; i < expected.size(); ++i) {
            expected[i].value = kLegalValues[i % kNumValues];
            ASSERT_EQ(
                Db().SetValue(static_cast<Position>(i), expected[i].value),
                kSuccess);
        }
        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        for (int pass = 1; pass <= kNumValues; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t step = 0; step < expected.size(); ++step) {
                const size_t i = (step * 5) % expected.size();
                SCOPED_TRACE(i);
                const Value value = kLegalValues[(i + pass) % kNumValues];
                ASSERT_EQ(Db().SetValue(static_cast<Position>(i), value),
                          kSuccess);
                expected[i].value = value;
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            }
        }
    }));
}

// Remoteness-only solving preserves each explicitly seeded value across passes.
TEST_P(DatabaseTest, ComponentSerialSetterFamiliesRemotenessOnly) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected(17);
        ASSERT_NO_FATAL_FAILURE(
            CreateTier(static_cast<int64_t>(expected.size())));
        for (size_t i = 0; i < expected.size(); ++i) {
            expected[i].value = kLegalValues[i % kNumValues];
            ASSERT_EQ(
                Db().SetValue(static_cast<Position>(i), expected[i].value),
                kSuccess);
        }
        // Do not observe a remoteness until it has been explicitly initialized.
        for (size_t i = 0; i < expected.size(); ++i) {
            expected[i].remoteness = kRepresentativeRemotenesses[i % 4];
            ASSERT_EQ(Db().SetRemoteness(static_cast<Position>(i),
                                         expected[i].remoteness),
                      kSuccess);
        }
        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        for (int pass = 1; pass <= 4; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t step = 0; step < expected.size(); ++step) {
                const size_t i = expected.size() - 1 - step;
                SCOPED_TRACE(i);
                const int remoteness =
                    kRepresentativeRemotenesses[(i + pass) % 4];
                ASSERT_EQ(
                    Db().SetRemoteness(static_cast<Position>(i), remoteness),
                    kSuccess);
                expected[i].remoteness = remoteness;
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            }
        }
    }));
}

// A combined-writer-only workflow needs neither individual field setter.
TEST_P(DatabaseTest, ComponentSerialSetterFamiliesCombined) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected(17);
        for (size_t i = 0; i < expected.size(); ++i) {
            expected[i] = {kLegalValues[i % kNumValues],
                           kRepresentativeRemotenesses[i % 4]};
        }
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        for (int pass = 1; pass <= kNumValues; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t step = 0; step < expected.size(); ++step) {
                const size_t i = (step * 5) % expected.size();
                SCOPED_TRACE(i);
                const ExpectedRecord replacement = {
                    kLegalValues[(i + pass) % kNumValues],
                    kRepresentativeRemotenesses[(i + pass) % 4]};
                ASSERT_EQ(Db().SetValueRemoteness(static_cast<Position>(i),
                                                  replacement.value,
                                                  replacement.remoteness),
                          kSuccess);
                expected[i] = replacement;
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            }
        }
    }));
}

// Setter families can alternate on each record without losing either field.
TEST_P(DatabaseTest, ComponentSerialSetterInteroperability) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected(17, {kDraw, 19});
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (int pass = 0; pass < kNumValues; ++pass) {
            SCOPED_TRACE(pass);
            for (size_t step = 0; step < expected.size(); ++step) {
                const size_t i = (step * 5) % expected.size();
                const Position position = static_cast<Position>(i);
                SCOPED_TRACE(position);
                ExpectedRecord replacement = {
                    kLegalValues[(i + pass) % kNumValues],
                    kRepresentativeRemotenesses[(i + pass) % 4]};
                ASSERT_EQ(Db().SetValueRemoteness(position, replacement.value,
                                                  replacement.remoteness),
                          kSuccess);
                expected[i] = replacement;
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));

                const Value value = kLegalValues[(i + pass + 1) % kNumValues];
                ASSERT_EQ(Db().SetValue(position, value), kSuccess);
                expected[i].value = value;
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));

                const int remoteness =
                    kRepresentativeRemotenesses[(i + pass + 1) % 4];
                ASSERT_EQ(Db().SetRemoteness(position, remoteness), kSuccess);
                expected[i].remoteness = remoteness;
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));

                replacement = {kLegalValues[(i + pass + 2) % kNumValues],
                               kRepresentativeRemotenesses[(i + pass + 2) % 4]};
                ASSERT_EQ(Db().SetValueRemoteness(position, replacement.value,
                                                  replacement.remoteness),
                          kSuccess);
                expected[i] = replacement;
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            }
        }
    }));
}

// Independent scalar ranks model strict improvement and retention on equality.
TEST_P(DatabaseTest, ComponentSerialMaximizeReferenceFold) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const auto pair_rank = [](const ExpectedRecord &record) {
            return static_cast<int>(record.value) * (kRemotenessMax + 1) +
                   record.remoteness;
        };
        const struct {
            const char *name;
            RecordCompare compare;
            int (*rank)(const ExpectedRecord &);
        } orderings[] = {
            {"pairs", ComparePairs, pair_rank},
            {"reversed values", CompareReversedValues,
             [](const ExpectedRecord &record) {
                 return -static_cast<int>(record.value) * (kRemotenessMax + 1) +
                        record.remoteness;
             }},
            {"values only", CompareValuesOnly,
             [](const ExpectedRecord &record) {
                 return static_cast<int>(record.value);
             }},
            {"remotenesses only", CompareRemotenessesOnly,
             [](const ExpectedRecord &record) { return record.remoteness; }},
            {"smaller remoteness", CompareSmallerRemotenessFirst,
             [](const ExpectedRecord &record) {
                 return -record.remoteness * kNumValues +
                        static_cast<int>(record.value);
             }},
            {"non-unit signs", CompareNonUnitSigns, pair_rank},
            {"extreme signs", CompareExtremeSigns, pair_rank}};
        std::vector<ExpectedRecord> expected(17, {kDraw, 19});
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (const auto &ordering : orderings) {
            SCOPED_TRACE(ordering.name);
            std::vector<ExpectedRecord> ascending;
            for (Value value : kLegalValues) {
                for (int remoteness : kRepresentativeRemotenesses) {
                    ascending.push_back({value, remoteness});
                }
            }
            std::stable_sort(
                ascending.begin(), ascending.end(),
                [&](const ExpectedRecord &left, const ExpectedRecord &right) {
                    return ordering.rank(left) < ordering.rank(right);
                });
            auto descending = ascending;
            std::reverse(descending.begin(), descending.end());
            auto shuffled = ascending;
            std::mt19937 generator(0x5EED);
            std::shuffle(shuffled.begin(), shuffled.end(), generator);
            std::vector<ExpectedRecord> duplicates;
            for (const auto &candidate : shuffled) {
                duplicates.push_back(candidate);
                duplicates.push_back(candidate);
            }
            const struct {
                const char *name;
                const std::vector<ExpectedRecord> *candidates;
            } sequences[] = {{"ascending", &ascending},
                             {"descending", &descending},
                             {"shuffled", &shuffled},
                             {"duplicates", &duplicates}};
            for (const auto &sequence : sequences) {
                SCOPED_TRACE(sequence.name);
                for (Position position : {0, 8, 16}) {
                    SCOPED_TRACE(position);
                    expected[position] = ascending.front();
                    ASSERT_NO_FATAL_FAILURE(
                        WriteRecord(position, expected[position]));
                    for (size_t i = 0; i < sequence.candidates->size(); ++i) {
                        SCOPED_TRACE(i);
                        const ExpectedRecord &candidate =
                            (*sequence.candidates)[i];
                        const bool should_replace =
                            ordering.rank(candidate) >
                            ordering.rank(expected[position]);
                        ASSERT_EQ(Db().MaximizeValueRemoteness(
                                      position, candidate.value,
                                      candidate.remoteness, ordering.compare),
                                  should_replace);
                        if (should_replace) {
                            expected[position] = candidate;
                        }
                        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                    }
                }
            }
        }
    }));
}

// A complete countdown returns each preceding count and then stays at zero.
TEST_P(DatabaseTest, ComponentCounterCountdownThroughZero) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected = {
            {kWin, 37}, {kUndecided, kRemotenessMax}, {kTie, 71}};
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (int count = kRemotenessMax; count > 0; --count) {
            SCOPED_TRACE(count);
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), count);
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), count);
            expected[1].remoteness = count - 1;
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), count - 1);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }
        for (int repetition = 0; repetition < 3; ++repetition) {
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), 0);
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), 0);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }
    }));
}

// Clearing a partially consumed count leaves subsequent decrements at zero.
TEST_P(DatabaseTest, ComponentCounterCountdownAndClear) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected = {
            {kDraw, 13}, {kUndecided, 7}, {kLose, kRemotenessMax}};
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (int count : {7, 6, 5}) {
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), count);
            expected[1].remoteness = count - 1;
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), count - 1);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }
        ASSERT_EQ(Db().ClearNumUndecidedChildren(1), 4);
        expected[1].remoteness = 0;
        ASSERT_EQ(Db().GetNumUndecidedChildren(1), 0);
        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        for (int repetition = 0; repetition < 3; ++repetition) {
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), 0);
            ASSERT_EQ(Db().ClearNumUndecidedChildren(1), 0);
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), 0);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }
    }));
}

// Value-only transitions retain the field that alternates count/remoteness
// roles.
TEST_P(DatabaseTest, ComponentCounterValueStateTransitions) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected = {
            {kDraw, 13}, {kUndecided, 7}, {kWin, 71}};
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (Value decided : kLegalValues) {
            if (decided == kUndecided) {
                continue;
            }
            SCOPED_TRACE(decided);
            expected[1] = {kUndecided, 7};
            ASSERT_NO_FATAL_FAILURE(WriteRecord(1, expected[1]));
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), 7);
            expected[1].remoteness = 6;
            ASSERT_EQ(Db().SetValue(1, decided), kSuccess);
            expected[1].value = decided;
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), 0);
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), 0);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().ClearNumUndecidedChildren(1), 0);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));

            ASSERT_EQ(Db().SetValue(1, kUndecided), kSuccess);
            expected[1].value = kUndecided;
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), 6);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), 6);
            expected[1].remoteness = 5;
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), 5);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().ClearNumUndecidedChildren(1), 5);
            expected[1].remoteness = 0;
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), 0);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }
    }));
}

// Separate remoteness writes change counters only while the value is undecided.
TEST_P(DatabaseTest, ComponentCounterValueStateTransitionsSeparateSetters) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected = {
            {kDraw, 13}, {kUndecided, 7}, {kWin, 71}};
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (Value decided : kLegalValues) {
            if (decided == kUndecided) {
                continue;
            }
            SCOPED_TRACE(decided);
            for (int count : {0, 1, static_cast<int>(kRemotenessMax)}) {
                SCOPED_TRACE(count);
                ASSERT_EQ(Db().SetValue(1, kUndecided), kSuccess);
                expected[1].value = kUndecided;
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                ASSERT_EQ(Db().SetRemoteness(1, count), kSuccess);
                expected[1].remoteness = count;
                ASSERT_EQ(Db().GetNumUndecidedChildren(1), count);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), count);
                expected[1].remoteness = count > 0 ? count - 1 : 0;
                ASSERT_EQ(Db().GetNumUndecidedChildren(1),
                          expected[1].remoteness);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));

                ASSERT_EQ(Db().SetValue(1, decided), kSuccess);
                expected[1].value = decided;
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                ASSERT_EQ(Db().SetRemoteness(1, count), kSuccess);
                expected[1].remoteness = count;
                ASSERT_EQ(Db().GetNumUndecidedChildren(1), 0);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), 0);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                ASSERT_EQ(Db().ClearNumUndecidedChildren(1), 0);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));

                ASSERT_EQ(Db().SetValue(1, kUndecided), kSuccess);
                expected[1].value = kUndecided;
                ASSERT_EQ(Db().GetNumUndecidedChildren(1), count);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                ASSERT_EQ(Db().ClearNumUndecidedChildren(1), count);
                expected[1].remoteness = 0;
                ASSERT_EQ(Db().GetNumUndecidedChildren(1), 0);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), 0);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            }
        }
    }));
}

// Combined writes replace the stored count/remoteness when changing value
// state.
TEST_P(DatabaseTest, ComponentCounterValueStateTransitionsCombined) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected = {
            {kLose, 13}, {kUndecided, 7}, {kTie, 71}};
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (Value decided : kLegalValues) {
            if (decided == kUndecided) {
                continue;
            }
            SCOPED_TRACE(decided);
            for (int remoteness : kRepresentativeRemotenesses) {
                SCOPED_TRACE(remoteness);
                ASSERT_EQ(Db().SetValueRemoteness(1, decided, remoteness),
                          kSuccess);
                expected[1] = {decided, remoteness};
                ASSERT_EQ(Db().GetNumUndecidedChildren(1), 0);
                ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), 0);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                ASSERT_EQ(Db().ClearNumUndecidedChildren(1), 0);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));

                ASSERT_EQ(
                    Db().SetValueRemoteness(1, kUndecided, kRemotenessMax),
                    kSuccess);
                expected[1] = {kUndecided, kRemotenessMax};
                ASSERT_EQ(Db().GetNumUndecidedChildren(1), kRemotenessMax);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                ASSERT_EQ(Db().DecrementNumUndecidedChildren(1),
                          kRemotenessMax);
                expected[1].remoteness = kRemotenessMax - 1;
                ASSERT_EQ(Db().GetNumUndecidedChildren(1), kRemotenessMax - 1);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                ASSERT_EQ(Db().ClearNumUndecidedChildren(1),
                          kRemotenessMax - 1);
                expected[1].remoteness = 0;
                ASSERT_EQ(Db().GetNumUndecidedChildren(1), 0);
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            }
        }
    }));
}

// Comparator-driven transitions use the same counter interpretation rules.
TEST_P(DatabaseTest, ComponentCounterValueStateTransitionsMaximize) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected = {
            {kWin, 13}, {kUndecided, 7}, {kDraw, 71}};
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        for (Value decided : kLegalValues) {
            if (decided == kUndecided) {
                continue;
            }
            SCOPED_TRACE(decided);
            expected[1] = {kUndecided, 7};
            ASSERT_NO_FATAL_FAILURE(WriteRecord(1, expected[1]));
            ASSERT_TRUE(Db().MaximizeValueRemoteness(1, decided, kRemotenessMax,
                                                     ComparePairs));
            expected[1] = {decided, kRemotenessMax};
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), 0);
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), 0);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().ClearNumUndecidedChildren(1), 0);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));

            ASSERT_TRUE(Db().MaximizeValueRemoteness(1, kUndecided, 3,
                                                     CompareReversedValues));
            expected[1] = {kUndecided, 3};
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), 3);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(1), 3);
            expected[1].remoteness = 2;
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), 2);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().ClearNumUndecidedChildren(1), 2);
            expected[1].remoteness = 0;
            ASSERT_EQ(Db().GetNumUndecidedChildren(1), 0);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }
    }));
}

// Interleaved counter calls follow an independent model at every position.
TEST_P(DatabaseTest, ComponentCounterMultiplePositions) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected;
        for (int i = 0; i < 17; ++i) {
            expected.push_back({kLegalValues[i % kNumValues],
                                (i * 53 + 17) % (kRemotenessMax + 1)});
        }
        expected[0] = {kUndecided, 7};
        expected[8] = {kUndecided, kRemotenessMax};
        expected[16] = {kUndecided, 1};
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        enum class Action { kQuery, kDecrement, kClear };
        const struct {
            Position position;
            Action action;
        } steps[] = {{0, Action::kDecrement},  {8, Action::kDecrement},
                     {16, Action::kQuery},     {0, Action::kQuery},
                     {16, Action::kDecrement}, {8, Action::kDecrement},
                     {0, Action::kClear},      {16, Action::kDecrement},
                     {8, Action::kQuery},      {0, Action::kDecrement},
                     {4, Action::kDecrement},  {8, Action::kClear},
                     {16, Action::kClear},     {4, Action::kClear},
                     {0, Action::kClear},      {8, Action::kDecrement},
                     {4, Action::kQuery},      {16, Action::kQuery}};
        for (const auto &step : steps) {
            SCOPED_TRACE(step.position);
            SCOPED_TRACE(static_cast<int>(step.action));
            auto &record = expected[step.position];
            const int preceding =
                record.value == kUndecided ? record.remoteness : 0;
            switch (step.action) {
                case Action::kQuery:
                    ASSERT_EQ(Db().GetNumUndecidedChildren(step.position),
                              preceding);
                    break;
                case Action::kDecrement:
                    ASSERT_EQ(Db().DecrementNumUndecidedChildren(step.position),
                              preceding);
                    if (preceding > 0) {
                        --record.remoteness;
                    }
                    break;
                case Action::kClear:
                    ASSERT_EQ(Db().ClearNumUndecidedChildren(step.position),
                              preceding);
                    if (record.value == kUndecided) {
                        record.remoteness = 0;
                    }
                    break;
            }
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            for (size_t i = 0; i < expected.size(); ++i) {
                SCOPED_TRACE(i);
                const auto &current = expected[i];
                ASSERT_EQ(
                    Db().GetNumUndecidedChildren(static_cast<Position>(i)),
                    current.value == kUndecided ? current.remoteness : 0);
            }
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }
    }));
}

// Mixed record operations retain their exact results in a normal tier archive.
TEST_P(DatabaseTest, ComponentSerialRecordsPersistAfterFlush) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(FlushSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected = {{kUndecided, kRemotenessMax},
                                                {kLose, 0},
                                                {kDraw, 1},
                                                {kTie, kRemotenessMax},
                                                {kWin, kRemotenessMax / 2},
                                                {kUndecided, 3},
                                                {kUndecided, 0}};
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        const auto check = [&] {
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            for (size_t i = 0; i < expected.size(); ++i) {
                SCOPED_TRACE(i);
                ASSERT_EQ(
                    Db().GetNumUndecidedChildren(static_cast<Position>(i)),
                    expected[i].value == kUndecided ? expected[i].remoteness
                                                    : 0);
            }
        };
        ASSERT_NO_FATAL_FAILURE(check());

        ASSERT_EQ(Db().SetValue(1, kWin), kSuccess);
        expected[1].value = kWin;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetRemoteness(2, kRemotenessMax), kSuccess);
        expected[2].remoteness = kRemotenessMax;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetValueRemoteness(3, kLose, 7), kSuccess);
        expected[3] = {kLose, 7};
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_TRUE(Db().MaximizeValueRemoteness(4, kWin, kRemotenessMax,
                                                 ComparePairs));
        expected[4] = {kWin, kRemotenessMax};
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_FALSE(Db().MaximizeValueRemoteness(4, kWin, 0, ComparePairs));
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_FALSE(Db().MaximizeValueRemoteness(4, kWin, kRemotenessMax,
                                                  ComparePairs));
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().DecrementNumUndecidedChildren(0), kRemotenessMax);
        expected[0].remoteness = kRemotenessMax - 1;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().ClearNumUndecidedChildren(5), 3);
        expected[5].remoteness = 0;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().DecrementNumUndecidedChildren(5), 0);
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().ClearNumUndecidedChildren(6), 0);
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().DecrementNumUndecidedChildren(3), 0);
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().ClearNumUndecidedChildren(3), 0);
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetValue(0, kTie), kSuccess);
        expected[0].value = kTie;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetRemoteness(0, 0), kSuccess);
        expected[0].remoteness = 0;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetValueRemoteness(5, kUndecided, kRemotenessMax),
                  kSuccess);
        expected[5] = {kUndecided, kRemotenessMax};
        ASSERT_NO_FATAL_FAILURE(check());

        ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
        if (Db().TierStatus != nullptr) {
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        }
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, expected));
    }));
}

// A shared external lock protects separate setters, both getters, and the
// model.
TEST_P(DatabaseTest, ComponentExternallySynchronizedSetterWorkers) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected = {
            {kUndecided, 0}, {kLose, 1}, {kDraw, kRemotenessMax},
            {kTie, 0},       {kWin, 1},  {kUndecided, kRemotenessMax},
            {kLose, 0},      {kDraw, 1}, {kTie, kRemotenessMax}};
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        ASSERT_NO_FATAL_FAILURE(RunExternallySynchronizedWorkers([&](int worker,
                                                                     int step) {
            const size_t index = (worker + step) % expected.size();
            const Position position = static_cast<Position>(index);
            if ((worker + step) % 2 == 0) {
                const Value value =
                    kLegalValues[(worker * 3 + step) % kNumValues];
                ASSERT_EQ(Db().SetValue(position, value), kSuccess);
                expected[index].value = value;
            } else {
                const int remoteness =
                    (worker * 97 + step * 31) % (kRemotenessMax + 1);
                ASSERT_EQ(Db().SetRemoteness(position, remoteness), kSuccess);
                expected[index].remoteness = remoteness;
            }
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }));
        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
    }));
}

// Every ordinary-tier record method shares one lock; workers finish before I/O.
TEST_P(DatabaseTest, ComponentExternallySynchronizedWorkers) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(FlushSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected = {{kUndecided, kRemotenessMax},
                                                {kLose, 0},
                                                {kDraw, 1},
                                                {kTie, kRemotenessMax},
                                                {kWin, 7},
                                                {kUndecided, 3},
                                                {kUndecided, 0}};
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        ASSERT_NO_FATAL_FAILURE(RunExternallySynchronizedWorkers([&](int worker,
                                                                     int step) {
            const size_t index = (worker + step) % expected.size();
            const Position position = static_cast<Position>(index);
            ExpectedRecord &record = expected[index];
            const ExpectedRecord candidate = {
                kLegalValues[(worker * 3 + step) % kNumValues],
                (worker * 97 + step * 31) % (kRemotenessMax + 1)};
            switch ((worker * 3 + step) % 7) {
                case 0:
                    ASSERT_EQ(Db().SetValue(position, candidate.value),
                              kSuccess);
                    record.value = candidate.value;
                    break;
                case 1:
                    ASSERT_EQ(
                        Db().SetRemoteness(position, candidate.remoteness),
                        kSuccess);
                    record.remoteness = candidate.remoteness;
                    break;
                case 2:
                    ASSERT_EQ(Db().SetValueRemoteness(position, candidate.value,
                                                      candidate.remoteness),
                              kSuccess);
                    record = candidate;
                    break;
                case 3: {
                    const bool improves =
                        candidate.value > record.value ||
                        (candidate.value == record.value &&
                         candidate.remoteness > record.remoteness);
                    ASSERT_EQ(Db().MaximizeValueRemoteness(
                                  position, candidate.value,
                                  candidate.remoteness, ComparePairs),
                              improves);
                    if (improves) {
                        record = candidate;
                    }
                    break;
                }
                case 4: {
                    const int previous =
                        record.value == kUndecided ? record.remoteness : 0;
                    ASSERT_EQ(Db().DecrementNumUndecidedChildren(position),
                              previous);
                    if (previous > 0) {
                        --record.remoteness;
                    }
                    break;
                }
                case 5: {
                    const int previous =
                        record.value == kUndecided ? record.remoteness : 0;
                    ASSERT_EQ(Db().ClearNumUndecidedChildren(position),
                              previous);
                    if (record.value == kUndecided) {
                        record.remoteness = 0;
                    }
                    break;
                }
                case 6:
                    // A read-only step observes the same serialized model.
                    break;
            }
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            for (size_t i = 0; i < expected.size(); ++i) {
                SCOPED_TRACE(i);
                ASSERT_EQ(
                    Db().GetNumUndecidedChildren(static_cast<Position>(i)),
                    expected[i].value == kUndecided ? expected[i].remoteness
                                                    : 0);
            }
        }));
        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
        if (Db().TierStatus != nullptr) {
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        }
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, expected));
    }));
}

// Both allocation modes execute the same serial script in independent
// sandboxes.
TEST_P(DatabaseTest, ComponentConcurrentSequentialParity) {
    REQUIRE_DATABASE_MEMBER(CreateSolvingTier);
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        std::filesystem::path paths[2];
        ASSERT_NO_FATAL_FAILURE(CreatePrivateSandbox(paths[0]));
        ASSERT_NO_FATAL_FAILURE(CreatePrivateSandbox(paths[1]));
        const CreateTierFunc modes[] = {Db().CreateSolvingTier,
                                        Db().CreateConcurrentSolvingTier};
        std::vector<ExpectedRecord> snapshots[2];
        for (int mode = 0; mode < 2; ++mode) {
            SCOPED_TRACE(mode);
            ASSERT_NO_FATAL_FAILURE(InitializeAt(paths[mode]));
            std::vector<ExpectedRecord> expected = {{kUndecided, 7},
                                                    {kLose, 0},
                                                    {kDraw, 1},
                                                    {kTie, kRemotenessMax},
                                                    {kWin, 7}};
            ASSERT_NO_FATAL_FAILURE(
                CreateAndPopulate(kStoredTier, expected, modes[mode]));
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().SetValue(1, kTie), kSuccess);
            expected[1].value = kTie;
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().SetRemoteness(1, kRemotenessMax), kSuccess);
            expected[1].remoteness = kRemotenessMax;
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().SetValueRemoteness(2, kUndecided, 3), kSuccess);
            expected[2] = {kUndecided, 3};
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_TRUE(Db().MaximizeValueRemoteness(3, kWin, 0, ComparePairs));
            expected[3] = {kWin, 0};
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_FALSE(Db().MaximizeValueRemoteness(3, kTie, kRemotenessMax,
                                                      ComparePairs));
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().GetNumUndecidedChildren(0), 7);
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(0), 7);
            expected[0].remoteness = 6;
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().ClearNumUndecidedChildren(0), 6);
            expected[0].remoteness = 0;
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(0), 0);
            ASSERT_EQ(Db().GetNumUndecidedChildren(0), 0);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().DecrementNumUndecidedChildren(4), 0);
            ASSERT_EQ(Db().ClearNumUndecidedChildren(4), 0);
            ASSERT_EQ(Db().GetNumUndecidedChildren(4), 0);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            snapshots[mode].resize(expected.size());
            ASSERT_NO_FATAL_FAILURE(ReadSolvingRecords(snapshots[mode]));
            Finalize();
        }
        ASSERT_EQ(snapshots[0].size(), snapshots[1].size());
        for (size_t i = 0; i < snapshots[0].size(); ++i) {
            SCOPED_TRACE(i);
            EXPECT_EQ(snapshots[0][i].value, snapshots[1][i].value);
            EXPECT_EQ(snapshots[0][i].remoteness, snapshots[1][i].remoteness);
        }
    }));
}

// Independent workers alternate individual setters on their own positions.
TEST_P(DatabaseTest, ComponentConcurrentDisjointPositionsSeparateSetters) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            std::vector<ExpectedRecord> expected(16, {kDraw, 19});
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
            ASSERT_NO_FATAL_FAILURE(
                RunConcurrentRecordWorkers(4, 64, [&](int worker, int step) {
                    const Position position = worker * 4 + step % 4;
                    const Value value =
                        kLegalValues[(worker + step) % kNumValues];
                    ASSERT_EQ(Db().SetValue(position, value), kSuccess);
                    expected[position].value = value;
                    ASSERT_NO_FATAL_FAILURE(
                        AssertRecord(position, expected[position]));
                    const int remoteness =
                        kRepresentativeRemotenesses[(worker + step / 4) % 4];
                    ASSERT_EQ(Db().SetRemoteness(position, remoteness),
                              kSuccess);
                    expected[position].remoteness = remoteness;
                    ASSERT_NO_FATAL_FAILURE(
                        AssertRecord(position, expected[position]));
                }));
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }));
}

// Combined writes on distinct records cannot interfere with another worker.
TEST_P(DatabaseTest, ComponentConcurrentDisjointPositionsCombinedSetter) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            std::vector<ExpectedRecord> expected(16, {kDraw, 19});
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
            ASSERT_NO_FATAL_FAILURE(
                RunConcurrentRecordWorkers(4, 64, [&](int worker, int step) {
                    const Position position = worker * 4 + step % 4;
                    const ExpectedRecord replacement = {
                        kLegalValues[(worker + step) % kNumValues],
                        kRepresentativeRemotenesses[(worker + step / 4) % 4]};
                    ASSERT_EQ(
                        Db().SetValueRemoteness(position, replacement.value,
                                                replacement.remoteness),
                        kSuccess);
                    expected[position] = replacement;
                    ASSERT_NO_FATAL_FAILURE(
                        AssertRecord(position, expected[position]));
                }));
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }));
}

// Disjoint maximize calls have exact per-position folds independent of
// scheduling.
TEST_P(DatabaseTest, ComponentConcurrentDisjointPositionsMaximize) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            std::vector<ExpectedRecord> expected(16, {kUndecided, 0});
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
            ASSERT_NO_FATAL_FAILURE(
                RunConcurrentRecordWorkers(4, 64, [&](int worker, int step) {
                    const Position position = worker * 4 + step % 4;
                    const ExpectedRecord candidate = {
                        kLegalValues[(worker + step) % kNumValues],
                        (step * 17 + worker) % (kRemotenessMax + 1)};
                    const auto &old = expected[position];
                    const bool should_replace =
                        candidate.value > old.value ||
                        (candidate.value == old.value &&
                         candidate.remoteness > old.remoteness);
                    ASSERT_EQ(Db().MaximizeValueRemoteness(
                                  position, candidate.value,
                                  candidate.remoteness, ComparePairs),
                              should_replace);
                    if (should_replace) {
                        expected[position] = candidate;
                    }
                    ASSERT_NO_FATAL_FAILURE(
                        AssertRecord(position, expected[position]));
                }));
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }));
}

// Counter scripts touch disjoint records and preserve each independent model.
TEST_P(DatabaseTest, ComponentConcurrentDisjointPositionsCounters) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            std::vector<ExpectedRecord> expected;
            for (int i = 0; i < 16; ++i) {
                expected.push_back(
                    {kUndecided, kRepresentativeRemotenesses[i % 4]});
            }
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
            ASSERT_NO_FATAL_FAILURE(
                RunConcurrentRecordWorkers(4, 64, [&](int worker, int step) {
                    const Position position = worker * 4 + step % 4;
                    const int before = expected[position].remoteness;
                    switch ((step / 4) % 3) {
                        case 0:
                            ASSERT_EQ(Db().GetNumUndecidedChildren(position),
                                      before);
                            break;
                        case 1:
                            ASSERT_EQ(
                                Db().DecrementNumUndecidedChildren(position),
                                before);
                            if (before > 0) {
                                --expected[position].remoteness;
                            }
                            break;
                        case 2:
                            ASSERT_EQ(Db().ClearNumUndecidedChildren(position),
                                      before);
                            expected[position].remoteness = 0;
                            break;
                    }
                    ASSERT_EQ(Db().GetNumUndecidedChildren(position),
                              expected[position].remoteness);
                    ASSERT_NO_FATAL_FAILURE(
                        AssertRecord(position, expected[position]));
                }));
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }));
}

// A value-only workflow observes only the explicitly initialized value field.
TEST_P(DatabaseTest, ComponentConcurrentSamePositionValueReadersAndWriters) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(GetValue);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            ASSERT_NO_FATAL_FAILURE(CreateTier(1));
            ASSERT_EQ(Db().SetValue(0, kLose), kSuccess);
            const Value allowed[] = {kLose, kTie, kWin};
            ASSERT_NO_FATAL_FAILURE(
                RunConcurrentRecordWorkers(4, 64, [&](int worker, int step) {
                    if (worker % 2 == 0) {
                        ASSERT_EQ(
                            Db().SetValue(0, allowed[(worker + step) % 3]),
                            kSuccess);
                    } else {
                        const Value value = Db().GetValue(0);
                        ASSERT_TRUE(std::find(allowed, allowed + 3, value) !=
                                    allowed + 3);
                        if (Db().GetValueFromLoaded != nullptr) {
                            const Value loaded =
                                Db().GetValueFromLoaded(kStoredTier, 0);
                            ASSERT_TRUE(std::find(allowed, allowed + 3,
                                                  loaded) != allowed + 3);
                        }
                    }
                }));
            const Value value = Db().GetValue(0);
            ASSERT_TRUE(std::find(allowed, allowed + 3, value) != allowed + 3);
        }));
}

// A remoteness-only workflow does not inspect the unwritten value field.
TEST_P(DatabaseTest,
       ComponentConcurrentSamePositionRemotenessReadersAndWriters) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_MEMBER(GetRemoteness);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            ASSERT_NO_FATAL_FAILURE(CreateTier(1));
            ASSERT_EQ(Db().SetRemoteness(0, 0), kSuccess);
            const int allowed[] = {0, kRemotenessMax / 2, kRemotenessMax};
            ASSERT_NO_FATAL_FAILURE(
                RunConcurrentRecordWorkers(4, 64, [&](int worker, int step) {
                    if (worker % 2 == 0) {
                        ASSERT_EQ(
                            Db().SetRemoteness(0, allowed[(worker + step) % 3]),
                            kSuccess);
                    } else {
                        const int remoteness = Db().GetRemoteness(0);
                        ASSERT_TRUE(std::find(allowed, allowed + 3,
                                              remoteness) != allowed + 3);
                        if (Db().GetRemotenessFromLoaded != nullptr) {
                            const int loaded =
                                Db().GetRemotenessFromLoaded(kStoredTier, 0);
                            ASSERT_TRUE(std::find(allowed, allowed + 3,
                                                  loaded) != allowed + 3);
                        }
                    }
                }));
            const int remoteness = Db().GetRemoteness(0);
            ASSERT_TRUE(std::find(allowed, allowed + 3, remoteness) !=
                        allowed + 3);
        }));
}

// While pairs change, separate getters each return a permitted field value.
TEST_P(DatabaseTest, ComponentConcurrentSamePositionPairReadersAndWriters) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            const ExpectedRecord allowed[] = {
                {kLose, 1}, {kTie, kRemotenessMax / 2}, {kWin, kRemotenessMax}};
            ASSERT_NO_FATAL_FAILURE(
                CreateAndPopulate(kStoredTier, {allowed[0]}));
            ASSERT_NO_FATAL_FAILURE(
                RunConcurrentRecordWorkers(4, 64, [&](int worker, int step) {
                    if (worker % 2 == 0) {
                        const auto &replacement = allowed[(worker + step) % 3];
                        ASSERT_EQ(
                            Db().SetValueRemoteness(0, replacement.value,
                                                    replacement.remoteness),
                            kSuccess);
                    } else {
                        const Value value = Db().GetValue(0);
                        ASSERT_TRUE(value == kLose || value == kTie ||
                                    value == kWin);
                        const int remoteness = Db().GetRemoteness(0);
                        ASSERT_TRUE(remoteness == 1 ||
                                    remoteness == kRemotenessMax / 2 ||
                                    remoteness == kRemotenessMax);
                        if (Db().GetValueFromLoaded != nullptr) {
                            const Value loaded =
                                Db().GetValueFromLoaded(kStoredTier, 0);
                            ASSERT_TRUE(loaded == kLose || loaded == kTie ||
                                        loaded == kWin);
                        }
                        if (Db().GetRemotenessFromLoaded != nullptr) {
                            const int loaded =
                                Db().GetRemotenessFromLoaded(kStoredTier, 0);
                            ASSERT_TRUE(loaded == 1 ||
                                        loaded == kRemotenessMax / 2 ||
                                        loaded == kRemotenessMax);
                        }
                    }
                }));
            const Value value = Db().GetValue(0);
            ASSERT_NE(value, kErrorValue);
            const int remoteness = Db().GetRemoteness(0);
            ASSERT_TRUE((value == allowed[0].value &&
                         remoteness == allowed[0].remoteness) ||
                        (value == allowed[1].value &&
                         remoteness == allowed[1].remoteness) ||
                        (value == allowed[2].value &&
                         remoteness == allowed[2].remoteness));
        }));
}

// Pure comparison and individual reads remain safe during shared-position
// maxima.
TEST_P(DatabaseTest, ComponentConcurrentMaximizeSafety) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            ASSERT_NO_FATAL_FAILURE(
                CreateAndPopulate(kStoredTier, {{kLose, 0}}));
            const Value allowed[] = {kLose, kTie, kWin};
            ASSERT_NO_FATAL_FAILURE(
                RunConcurrentRecordWorkers(4, 64, [&](int worker, int step) {
                    if (worker % 2 == 0) {
                        Db().MaximizeValueRemoteness(
                            0, allowed[(worker + step) % 3], step,
                            ComparePairs);
                    } else {
                        const Value value = Db().GetValue(0);
                        ASSERT_TRUE(std::find(allowed, allowed + 3, value) !=
                                    allowed + 3);
                        const int remoteness = Db().GetRemoteness(0);
                        ASSERT_GE(remoteness, 0);
                        ASSERT_LE(remoteness, 63);
                    }
                }));
            const Value value = Db().GetValue(0);
            ASSERT_TRUE(std::find(allowed, allowed + 3, value) != allowed + 3);
            const int remoteness = Db().GetRemoteness(0);
            ASSERT_GE(remoteness, 0);
            ASSERT_LE(remoteness, 63);
        }));
}

// No reseeding occurs while decrement, clear, and reads overlap on one counter.
TEST_P(DatabaseTest, ComponentConcurrentUndecidedCounterSafety) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            ASSERT_NO_FATAL_FAILURE(
                CreateAndPopulate(kStoredTier, {{kUndecided, kRemotenessMax}}));
            ASSERT_NO_FATAL_FAILURE(
                RunConcurrentRecordWorkers(4, 64, [&](int worker, int) {
                    const int result =
                        worker < 2    ? Db().DecrementNumUndecidedChildren(0)
                        : worker == 2 ? Db().ClearNumUndecidedChildren(0)
                                      : Db().GetNumUndecidedChildren(0);
                    ASSERT_GE(result, 0);
                    ASSERT_LE(result, kRemotenessMax);
                    const int count = Db().GetNumUndecidedChildren(0);
                    ASSERT_GE(count, 0);
                    ASSERT_LE(count, kRemotenessMax);
                }));
            ASSERT_EQ(Db().GetNumUndecidedChildren(0), 0);
            if (Db().GetValue != nullptr) {
                ASSERT_EQ(Db().GetValue(0), kUndecided);
            }
            if (Db().GetRemoteness != nullptr) {
                ASSERT_EQ(Db().GetRemoteness(0), 0);
            }
        }));
}

// Multiple readers see exact fully seeded records with no concurrent writes.
TEST_P(DatabaseTest, ComponentConcurrentReadOnlyRecords) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            const auto expected = AllRecords();
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
            ASSERT_NO_FATAL_FAILURE(
                RunConcurrentRecordWorkers(4, 64, [&](int worker, int step) {
                    const Position position = static_cast<Position>(
                        (step % kNumValues) * (kRemotenessMax + 1) +
                        (worker * 17 + step * 53) % (kRemotenessMax + 1));
                    const auto &record = expected[position];
                    ASSERT_NO_FATAL_FAILURE(AssertRecord(position, record));
                    if (Db().GetValueFromLoaded != nullptr) {
                        ASSERT_EQ(
                            Db().GetValueFromLoaded(kStoredTier, position),
                            record.value);
                    }
                    if (Db().GetRemotenessFromLoaded != nullptr) {
                        ASSERT_EQ(
                            Db().GetRemotenessFromLoaded(kStoredTier, position),
                            record.remoteness);
                    }
                    if (Db().GetNumUndecidedChildren != nullptr) {
                        ASSERT_EQ(
                            Db().GetNumUndecidedChildren(position),
                            record.value == kUndecided ? record.remoteness : 0);
                    }
                }));
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        }));
}

// Pair writes and independent getter calls admit a legal whole-call ordering.
TEST_P(DatabaseTest, ComponentConcurrentPairWriterContention) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            const std::vector<ExpectedRecord> initial = {
                {kDraw, 19}, {kUndecided, 0}, {kTie, 71}};
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, initial));
            for (int round = 0; round < 16; ++round) {
                SCOPED_TRACE(round);
                for (size_t i = 0; i < initial.size(); ++i) {
                    ASSERT_NO_FATAL_FAILURE(
                        WriteRecord(static_cast<Position>(i), initial[i]));
                }
                const ExpectedRecord first = {
                    kLegalValues[round % kNumValues],
                    kRepresentativeRemotenesses[round % 4]};
                const ExpectedRecord second = {
                    kLegalValues[(round + 1) % kNumValues],
                    kRepresentativeRemotenesses[(round + 1) % 4]};
                std::vector<ConcurrentRecordCall> calls = {
                    {ConcurrentRecordOperation::kSetPair, 1, first},
                    {ConcurrentRecordOperation::kGetValue, 1},
                    {ConcurrentRecordOperation::kGetRemoteness, 1},
                    {ConcurrentRecordOperation::kSetPair, 1, second},
                    {ConcurrentRecordOperation::kGetValue, 1},
                    {ConcurrentRecordOperation::kGetRemoteness, 1}};
                ASSERT_NO_FATAL_FAILURE(
                    RunConcurrentRecordHistory(calls, 2, 3));
                std::vector<ExpectedRecord> final(initial.size());
                ASSERT_NO_FATAL_FAILURE(ReadSolvingRecords(final));
                ASSERT_TRUE((final[1].value == first.value &&
                             final[1].remoteness == first.remoteness) ||
                            (final[1].value == second.value &&
                             final[1].remoteness == second.remoteness));
                ASSERT_NO_FATAL_FAILURE(
                    AssertConcurrentRecordHistory(initial, calls, final));
            }
        }));
}

// Independent field writers retain both completed updates under contention.
TEST_P(DatabaseTest, ComponentConcurrentIndependentFieldWriterContention) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            const std::vector<ExpectedRecord> initial = {
                {kDraw, 19}, {kLose, 7}, {kTie, 71}};
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, initial));
            for (int round = 0; round < 16; ++round) {
                SCOPED_TRACE(round);
                for (size_t i = 0; i < initial.size(); ++i) {
                    ASSERT_NO_FATAL_FAILURE(
                        WriteRecord(static_cast<Position>(i), initial[i]));
                }
                const ExpectedRecord desired = {
                    kLegalValues[round % kNumValues],
                    kRepresentativeRemotenesses[round % 4]};
                std::vector<ConcurrentRecordCall> calls = {
                    {ConcurrentRecordOperation::kSetValue, 1, desired},
                    {ConcurrentRecordOperation::kGetRemoteness, 1},
                    {ConcurrentRecordOperation::kGetValue, 1},
                    {ConcurrentRecordOperation::kSetRemoteness, 1, desired},
                    {ConcurrentRecordOperation::kGetValue, 1},
                    {ConcurrentRecordOperation::kGetRemoteness, 1}};
                ASSERT_NO_FATAL_FAILURE(
                    RunConcurrentRecordHistory(calls, 2, 3));
                auto expected = initial;
                expected[1] = desired;
                ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                std::vector<ExpectedRecord> final(initial.size());
                ASSERT_NO_FATAL_FAILURE(ReadSolvingRecords(final));
                ASSERT_NO_FATAL_FAILURE(
                    AssertConcurrentRecordHistory(initial, calls, final));
            }
        }));
}

// One atomic replacement wins when both callers offer the same better pair.
TEST_P(DatabaseTest, ComponentConcurrentMaximizeSameCandidate) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            std::vector<ExpectedRecord> expected(17, {kDraw, 19});
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
            for (Position position : {0, 8, 16}) {
                SCOPED_TRACE(position);
                for (int round = 0; round < 8; ++round) {
                    SCOPED_TRACE(round);
                    expected[position] = {kLose, 0};
                    ASSERT_NO_FATAL_FAILURE(
                        WriteRecord(position, expected[position]));
                    std::vector<int> replacements(2, -1);
                    ASSERT_NO_FATAL_FAILURE(
                        RunConcurrentRecordWorkers(2, 1, [&](int worker, int) {
                            replacements[worker] = Db().MaximizeValueRemoteness(
                                position, kWin, 7, ComparePairs);
                        }));
                    for (int result : replacements) {
                        ASSERT_TRUE(result == 0 || result == 1);
                    }
                    ASSERT_EQ(replacements[0] + replacements[1], 1);
                    expected[position] = {kWin, 7};
                    ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                }
            }
        }));
}

// The larger offer wins regardless of which overlapping call linearizes first.
TEST_P(DatabaseTest, ComponentConcurrentMaximizeDistinctCandidates) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            std::vector<ExpectedRecord> expected(17, {kDraw, 19});
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
            const ExpectedRecord candidates[] = {{kTie, 3}, {kWin, 7}};
            for (Position position : {0, 8, 16}) {
                SCOPED_TRACE(position);
                for (int round = 0; round < 8; ++round) {
                    SCOPED_TRACE(round);
                    expected[position] = {kLose, 0};
                    ASSERT_NO_FATAL_FAILURE(
                        WriteRecord(position, expected[position]));
                    std::vector<int> replacements(2, -1);
                    ASSERT_NO_FATAL_FAILURE(
                        RunConcurrentRecordWorkers(2, 1, [&](int worker, int) {
                            const auto &candidate = candidates[worker];
                            replacements[worker] = Db().MaximizeValueRemoteness(
                                position, candidate.value, candidate.remoteness,
                                ComparePairs);
                        }));
                    // Tie may replace Lose first or observe the already-set
                    // Win.
                    ASSERT_TRUE(replacements[0] == 0 || replacements[0] == 1);
                    ASSERT_EQ(replacements[1], 1);
                    expected[position] = {kWin, 7};
                    ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                }
            }
        }));
}

// Concurrent decrements consume every positive preceding count exactly once.
TEST_P(DatabaseTest, ComponentConcurrentDecrementExactReturns) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            std::vector<ExpectedRecord> expected(17, {kDraw, 19});
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
            constexpr int kWorkers = 4;
            for (Position position : {0, 8, 16}) {
                SCOPED_TRACE(position);
                for (int count : {0, 1, 7, static_cast<int>(kRemotenessMax)}) {
                    SCOPED_TRACE(count);
                    expected[position] = {kUndecided, count};
                    ASSERT_NO_FATAL_FAILURE(
                        WriteRecord(position, expected[position]));
                    const int steps = count / kWorkers + 2;
                    std::vector<std::vector<int>> returns(
                        kWorkers, std::vector<int>(steps, -1));
                    ASSERT_NO_FATAL_FAILURE(RunConcurrentRecordWorkers(
                        kWorkers, steps, [&](int worker, int step) {
                            returns[worker][step] =
                                Db().DecrementNumUndecidedChildren(position);
                        }));
                    std::vector<int> frequencies(count + 1, 0);
                    for (const auto &worker_returns : returns) {
                        for (int result : worker_returns) {
                            ASSERT_GE(result, 0);
                            ASSERT_LE(result, count);
                            ++frequencies[result];
                        }
                    }
                    ASSERT_EQ(frequencies[0], kWorkers * steps - count);
                    for (int previous = 1; previous <= count; ++previous) {
                        SCOPED_TRACE(previous);
                        ASSERT_EQ(frequencies[previous], 1);
                    }
                    expected[position].remoteness = 0;
                    ASSERT_EQ(Db().GetNumUndecidedChildren(position), 0);
                    ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                }
            }
        }));
}

// Exactly one clear consumes the original positive count under contention.
TEST_P(DatabaseTest, ComponentConcurrentClearExactReturns) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            std::vector<ExpectedRecord> expected(17, {kDraw, 19});
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
            constexpr int kWorkers = 4;
            constexpr int kSteps = 8;
            const int counts[] = {1, 7, kRemotenessMax};
            for (Position position : {0, 8, 16}) {
                SCOPED_TRACE(position);
                for (int round = 0; round < 8; ++round) {
                    SCOPED_TRACE(round);
                    const int count = counts[round % 3];
                    SCOPED_TRACE(count);
                    expected[position] = {kUndecided, count};
                    ASSERT_NO_FATAL_FAILURE(
                        WriteRecord(position, expected[position]));
                    std::vector<std::vector<int>> returns(
                        kWorkers, std::vector<int>(kSteps, -1));
                    ASSERT_NO_FATAL_FAILURE(RunConcurrentRecordWorkers(
                        kWorkers, kSteps, [&](int worker, int step) {
                            returns[worker][step] =
                                Db().ClearNumUndecidedChildren(position);
                        }));
                    int original_count_returns = 0;
                    for (const auto &worker_returns : returns) {
                        for (int result : worker_returns) {
                            ASSERT_TRUE(result == 0 || result == count);
                            original_count_returns += result == count;
                        }
                    }
                    ASSERT_EQ(original_count_returns, 1);
                    expected[position].remoteness = 0;
                    ASSERT_EQ(Db().GetNumUndecidedChildren(position), 0);
                    ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                }
            }
        }));
}

// Decrements and clears consume exactly the initial count across any ordering.
TEST_P(DatabaseTest, ComponentConcurrentDecrementClearConservation) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            std::vector<ExpectedRecord> expected(17, {kDraw, 19});
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
            constexpr int kWorkers = 4;
            constexpr int kSteps = 32;
            struct CounterReturn {
                bool clear;
                int previous;
            };
            for (Position position : {0, 8, 16}) {
                SCOPED_TRACE(position);
                for (int count : {0, 1, 7, static_cast<int>(kRemotenessMax)}) {
                    SCOPED_TRACE(count);
                    expected[position] = {kUndecided, count};
                    ASSERT_NO_FATAL_FAILURE(
                        WriteRecord(position, expected[position]));
                    std::vector<std::vector<CounterReturn>> returns(
                        kWorkers, std::vector<CounterReturn>(
                                      kSteps, CounterReturn{false, -1}));
                    ASSERT_NO_FATAL_FAILURE(RunConcurrentRecordWorkers(
                        kWorkers, kSteps, [&](int worker, int step) {
                            auto &result = returns[worker][step];
                            result.clear = (worker + step) % 3 == 0;
                            result.previous =
                                result.clear
                                    ? Db().ClearNumUndecidedChildren(position)
                                    : Db().DecrementNumUndecidedChildren(
                                          position);
                        }));
                    int consumed = 0;
                    for (const auto &worker_returns : returns) {
                        for (const auto &result : worker_returns) {
                            ASSERT_GE(result.previous, 0);
                            ASSERT_LE(result.previous, count);
                            consumed += result.clear ? result.previous
                                                     : result.previous > 0;
                        }
                    }
                    ASSERT_LE(consumed, count);
                    // Include a quiescent drain in the same conservation law.
                    const int drained =
                        Db().ClearNumUndecidedChildren(position);
                    ASSERT_GE(drained, 0);
                    ASSERT_LE(drained, count);
                    ASSERT_EQ(consumed + drained, count);
                    expected[position].remoteness = 0;
                    ASSERT_EQ(Db().GetNumUndecidedChildren(position), 0);
                    ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                }
            }
        }));
}

// Counter contention on fixed decided records leaves both fields unchanged.
TEST_P(DatabaseTest, ComponentConcurrentDecidedCounterContention) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_RECORDS();
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            std::vector<ExpectedRecord> expected(17, {kDraw, 19});
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
            constexpr int kWorkers = 4;
            constexpr int kSteps = 24;
            for (Position position : {0, 8, 16}) {
                SCOPED_TRACE(position);
                for (Value value : kLegalValues) {
                    if (value == kUndecided) {
                        continue;
                    }
                    SCOPED_TRACE(value);
                    for (int remoteness : kRepresentativeRemotenesses) {
                        SCOPED_TRACE(remoteness);
                        expected[position] = {value, remoteness};
                        ASSERT_NO_FATAL_FAILURE(
                            WriteRecord(position, expected[position]));
                        std::vector<std::vector<int>> returns(
                            kWorkers, std::vector<int>(kSteps, -1));
                        ASSERT_NO_FATAL_FAILURE(RunConcurrentRecordWorkers(
                            kWorkers, kSteps, [&](int worker, int step) {
                                int &result = returns[worker][step];
                                switch ((worker + step) % 3) {
                                    case 0:
                                        result =
                                            Db().DecrementNumUndecidedChildren(
                                                position);
                                        break;
                                    case 1:
                                        result = Db().ClearNumUndecidedChildren(
                                            position);
                                        break;
                                    case 2:
                                        result = Db().GetNumUndecidedChildren(
                                            position);
                                        break;
                                }
                            }));
                        for (const auto &worker_returns : returns) {
                            for (int result : worker_returns) {
                                ASSERT_EQ(result, 0);
                            }
                        }
                        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
                    }
                }
            }
        }));
}

// Mixed disjoint scripts have exact models; only joined workers precede I/O.
TEST_P(DatabaseTest, ComponentConcurrentFinalPersistenceAndStatus) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(FlushSolvingTier);
    REQUIRE_DATABASE_MEMBER(TierStatus);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            std::vector<ExpectedRecord> expected;
            for (int i = 0; i < 20; ++i) {
                expected.push_back({kLegalValues[i % kNumValues],
                                    kRepresentativeRemotenesses[i % 4]});
            }
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
            ASSERT_NO_FATAL_FAILURE(
                RunConcurrentRecordWorkers(4, 64, [&](int worker, int step) {
                    const Position position = worker * 5 + step % 5;
                    auto &record = expected[position];
                    const ExpectedRecord candidate = {
                        kLegalValues[(worker + step) % kNumValues],
                        (worker * 97 + step * 31) % (kRemotenessMax + 1)};
                    switch (step % 7) {
                        case 0:
                            ASSERT_EQ(Db().SetValue(position, candidate.value),
                                      kSuccess);
                            record.value = candidate.value;
                            break;
                        case 1:
                            ASSERT_EQ(Db().SetRemoteness(position,
                                                         candidate.remoteness),
                                      kSuccess);
                            record.remoteness = candidate.remoteness;
                            break;
                        case 2:
                            ASSERT_EQ(Db().SetValueRemoteness(
                                          position, candidate.value,
                                          candidate.remoteness),
                                      kSuccess);
                            record = candidate;
                            break;
                        case 3: {
                            const bool improves =
                                candidate.value > record.value ||
                                (candidate.value == record.value &&
                                 candidate.remoteness > record.remoteness);
                            ASSERT_EQ(Db().MaximizeValueRemoteness(
                                          position, candidate.value,
                                          candidate.remoteness, ComparePairs),
                                      improves);
                            if (improves) {
                                record = candidate;
                            }
                            break;
                        }
                        case 4: {
                            const int previous = record.value == kUndecided
                                                     ? record.remoteness
                                                     : 0;
                            ASSERT_EQ(
                                Db().DecrementNumUndecidedChildren(position),
                                previous);
                            if (previous > 0) {
                                --record.remoteness;
                            }
                            break;
                        }
                        case 5: {
                            const int previous = record.value == kUndecided
                                                     ? record.remoteness
                                                     : 0;
                            ASSERT_EQ(Db().ClearNumUndecidedChildren(position),
                                      previous);
                            if (record.value == kUndecided) {
                                record.remoteness = 0;
                            }
                            break;
                        }
                        case 6:
                            break;
                    }
                    ASSERT_NO_FATAL_FAILURE(AssertRecord(position, record));
                    ASSERT_EQ(
                        Db().GetNumUndecidedChildren(position),
                        record.value == kUndecided ? record.remoteness : 0);
                }));
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
            ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
            ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
            ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, expected));
        }));
}

// Fixed inputs explore mixed contended/disjoint histories without fixing order.
TEST_P(DatabaseTest, ComponentConcurrentDeterministicStress) {
    REQUIRE_DATABASE_MEMBER(CreateConcurrentSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(FlushSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(
        RunWithSolvingMode(Db().CreateConcurrentSolvingTier, [&] {
            const ConcurrentRecordOperation operations[] = {
                ConcurrentRecordOperation::kSetValue,
                ConcurrentRecordOperation::kSetRemoteness,
                ConcurrentRecordOperation::kSetPair,
                ConcurrentRecordOperation::kMaximize,
                ConcurrentRecordOperation::kDecrement,
                ConcurrentRecordOperation::kClear,
                ConcurrentRecordOperation::kGetValue,
                ConcurrentRecordOperation::kGetRemoteness,
                ConcurrentRecordOperation::kGetCount};
            const unsigned seeds[] = {0x5EEDu, 0xC0FFEEu, 0x12345678u};
            std::vector<ExpectedRecord> initial(7, {kDraw, 19});
            std::vector<ExpectedRecord> final(initial.size());
            ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, initial));
            for (unsigned seed : seeds) {
                SCOPED_TRACE(::testing::Message() << "Seed " << seed);
                std::mt19937 generator(seed);
                for (int round = 0; round < 9; ++round) {
                    SCOPED_TRACE(round);
                    for (size_t i = 0; i < initial.size(); ++i) {
                        initial[i] = {
                            kLegalValues[generator() % kNumValues],
                            kRepresentativeRemotenesses[generator() % 4]};
                        ASSERT_NO_FATAL_FAILURE(
                            WriteRecord(static_cast<Position>(i), initial[i]));
                    }
                    std::vector<ConcurrentRecordCall> calls;
                    for (int worker = 0; worker < 4; ++worker) {
                        for (int step = 0; step < 2; ++step) {
                            // Alternate contention on 0/1 and
                            // worker-owned 1..4.
                            const Position position = round % 2 == 0
                                                          ? (worker < 2 ? 0 : 1)
                                                          : worker + 1;
                            calls.push_back(
                                {operations[(round * 8 + worker * 2 + step) %
                                            9],
                                 position,
                                 {kLegalValues[generator() % kNumValues],
                                  kRepresentativeRemotenesses[generator() %
                                                              4]}});
                        }
                    }
                    ASSERT_NO_FATAL_FAILURE(
                        RunConcurrentRecordHistory(calls, 4, 2));
                    ASSERT_NO_FATAL_FAILURE(ReadSolvingRecords(final));
                    ASSERT_NO_FATAL_FAILURE(
                        AssertConcurrentRecordHistory(initial, calls, final));
                }
            }
            ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
            if (Db().TierStatus != nullptr) {
                ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
            ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, final));
        }));
}

// A regular solving tier and its binary progress restore through the full API.
TEST_P(DatabaseTest, ComponentCheckpointOrdinaryRoundTrip) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(CheckpointExists);
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const std::vector<ExpectedRecord> expected = {
            {kUndecided, 0},        {kLose, 0}, {kDraw, 1},
            {kTie, kRemotenessMax}, {kWin, 7},  {kUndecided, kRemotenessMax},
            {kUndecided, 3}};
        const std::vector<unsigned char> expected_status = {
            0x00, 0xff, 0x19, 0x7f, 0x00, 0x80, 0xa5};
        auto status = expected_status;
        ASSERT_FALSE(Db().CheckpointExists(kStoredTier));
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, expected));
        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        ASSERT_EQ(Db().CheckpointSave(status.data(), status.size()), kSuccess);
        ASSERT_EQ(status, expected_status);
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
        ASSERT_TRUE(Db().CheckpointExists(kStoredTier));
        std::vector<unsigned char> restored_status(expected_status.size(),
                                                   0x5a);
        ASSERT_NO_FATAL_FAILURE(
            LoadCheckpoint(kStoredTier, static_cast<int64_t>(expected.size()),
                           restored_status));
        ASSERT_EQ(restored_status, expected_status);
        ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
        }
        for (size_t i = 0; i < expected.size(); ++i) {
            SCOPED_TRACE(i);
            const Position position = static_cast<Position>(i);
            const auto &record = expected[i];
            if (Db().GetValueFromLoaded != nullptr) {
                ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, position),
                          record.value);
            }
            if (Db().GetRemotenessFromLoaded != nullptr) {
                ASSERT_EQ(Db().GetRemotenessFromLoaded(kStoredTier, position),
                          record.remoteness);
            }
            if (Db().GetNumUndecidedChildren != nullptr) {
                ASSERT_EQ(Db().GetNumUndecidedChildren(position),
                          record.value == kUndecided ? record.remoteness : 0);
            }
        }
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
    }));
}

// Unsaved changes to records and the caller's status buffer cannot alter A.
TEST_P(DatabaseTest, ComponentCheckpointRestoresSavedSnapshot) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(CheckpointExists);
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const std::vector<ExpectedRecord> saved = {
            {kUndecided, 7},        {kLose, 0}, {kDraw, 1},
            {kTie, kRemotenessMax}, {kWin, 7},  {kUndecided, kRemotenessMax},
            {kUndecided, 0}};
        const std::vector<ExpectedRecord> unsaved = {{kWin, kRemotenessMax},
                                                     {kUndecided, 2},
                                                     {kTie, 7},
                                                     {kLose, 3},
                                                     {kDraw, 0},
                                                     {kUndecided, 1},
                                                     {kWin, 1}};
        const std::vector<unsigned char> saved_status = {0x00, 0xff, 0x19, 0x7f,
                                                         0x00, 0x80, 0xa5};
        auto status = saved_status;
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, saved));
        ASSERT_EQ(Db().CheckpointSave(status.data(), status.size()), kSuccess);
        ASSERT_EQ(status, saved_status);
        ASSERT_EQ(unsaved.size(), saved.size());
        for (size_t i = 0; i < unsaved.size(); ++i) {
            ASSERT_NO_FATAL_FAILURE(
                WriteRecord(static_cast<Position>(i), unsaved[i]));
        }
        for (auto &byte : status) {
            byte = static_cast<unsigned char>(byte ^ 0x5a);
        }
        ASSERT_NE(status, saved_status);
        ASSERT_NO_FATAL_FAILURE(AssertRecords(unsaved));
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        ASSERT_TRUE(Db().CheckpointExists(kStoredTier));
        std::vector<unsigned char> restored_status(saved_status.size(), 0xc7);
        ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
            kStoredTier, static_cast<int64_t>(saved.size()), restored_status));
        ASSERT_EQ(restored_status, saved_status);
        ASSERT_NO_FATAL_FAILURE(AssertRecords(saved));
        if (Db().GetNumUndecidedChildren != nullptr) {
            for (size_t i = 0; i < saved.size(); ++i) {
                SCOPED_TRACE(i);
                ASSERT_EQ(
                    Db().GetNumUndecidedChildren(static_cast<Position>(i)),
                    saved[i].value == kUndecided ? saved[i].remoteness : 0);
            }
        }
    }));
}

// Saving B supersedes A's complete records and differently sized binary status.
TEST_P(DatabaseTest, ComponentCheckpointLatestSaveWins) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(CheckpointExists);
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const std::vector<ExpectedRecord> first = {
            {kUndecided, 7},        {kLose, 0}, {kDraw, 1},
            {kTie, kRemotenessMax}, {kWin, 7},  {kUndecided, kRemotenessMax},
            {kUndecided, 0}};
        const std::vector<ExpectedRecord> latest = {{kWin, kRemotenessMax},
                                                    {kUndecided, 2},
                                                    {kTie, 7},
                                                    {kLose, 3},
                                                    {kDraw, 0},
                                                    {kUndecided, 1},
                                                    {kWin, 1}};
        const std::vector<unsigned char> expected_first_status = {
            0x00, 0xff, 0x19, 0x7f, 0x00, 0x80, 0xa5};
        const std::vector<unsigned char> expected_latest_status = {
            0xff, 0x00, 0x80, 0x01, 0x7f, 0xc7, 0x00, 0x91, 0x00,
            0xa5, 0xfe, 0x03, 0x10, 0x00, 0x7e, 0x81, 0x37};
        auto first_status = expected_first_status;
        auto latest_status = expected_latest_status;
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, first));
        ASSERT_EQ(Db().CheckpointSave(first_status.data(), first_status.size()),
                  kSuccess);
        ASSERT_EQ(first_status, expected_first_status);
        ASSERT_EQ(latest.size(), first.size());
        for (size_t i = 0; i < latest.size(); ++i) {
            ASSERT_NO_FATAL_FAILURE(
                WriteRecord(static_cast<Position>(i), latest[i]));
        }
        ASSERT_NO_FATAL_FAILURE(AssertRecords(latest));
        ASSERT_EQ(
            Db().CheckpointSave(latest_status.data(), latest_status.size()),
            kSuccess);
        ASSERT_EQ(latest_status, expected_latest_status);
        ASSERT_EQ(first_status, expected_first_status);
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        ASSERT_TRUE(Db().CheckpointExists(kStoredTier));
        std::vector<unsigned char> restored_status(
            expected_latest_status.size(), 0x5a);
        ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
            kStoredTier, static_cast<int64_t>(latest.size()), restored_status));
        ASSERT_EQ(restored_status, expected_latest_status);
        ASSERT_NO_FATAL_FAILURE(AssertRecords(latest));
        if (Db().GetNumUndecidedChildren != nullptr) {
            for (size_t i = 0; i < latest.size(); ++i) {
                SCOPED_TRACE(i);
                ASSERT_EQ(
                    Db().GetNumUndecidedChildren(static_cast<Position>(i)),
                    latest[i].value == kUndecided ? latest[i].remoteness : 0);
            }
        }
    }));
}

// A regular checkpoint restores its exact progress after a complete restart.
TEST_P(DatabaseTest, ComponentCheckpointSurvivesReinitialization) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointExists);
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize(GetTestTierName));
        const std::vector<ExpectedRecord> records = {
            {kUndecided, kRemotenessMax},
            {kLose, 0},
            {kDraw, 1},
            {kTie, kRemotenessMax},
            {kWin, 7},
            {kUndecided, 3},
            {kUndecided, 0}};
        std::vector<unsigned char> status = {0x13, 0x00, 0xff, 0x47, 0x80};
        const auto saved_status = status;
        ASSERT_NO_FATAL_FAILURE(
            CreateAndPopulate(kStoredTier, records, Db().CreateSolvingTier));
        ASSERT_EQ(Db().CheckpointSave(status.data(), status.size()), kSuccess);
        ASSERT_EQ(status, saved_status);
        ASSERT_TRUE(Db().CheckpointExists(kStoredTier));
        // Finalize releases the still-active tier; no explicit free is needed.
        Finalize();
        ASSERT_NO_FATAL_FAILURE(Initialize(GetTestTierName));
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
        ASSERT_TRUE(Db().CheckpointExists(kStoredTier));
        std::vector<unsigned char> restored_status(saved_status.size(), 0xa5);
        ASSERT_NO_FATAL_FAILURE(
            LoadCheckpoint(kStoredTier, static_cast<int64_t>(records.size()),
                           restored_status));
        ASSERT_EQ(restored_status, saved_status);
        ASSERT_NO_FATAL_FAILURE(AssertRecords(records));
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
        }
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            const Position position = static_cast<Position>(i);
            const auto &record = records[i];
            if (Db().GetValueFromLoaded != nullptr) {
                ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, position),
                          record.value);
            }
            if (Db().GetRemotenessFromLoaded != nullptr) {
                ASSERT_EQ(Db().GetRemotenessFromLoaded(kStoredTier, position),
                          record.remoteness);
            }
            if (Db().GetNumUndecidedChildren != nullptr) {
                ASSERT_EQ(Db().GetNumUndecidedChildren(position),
                          record.value == kUndecided ? record.remoteness : 0);
            }
        }
    }));
}

// A removed checkpoint cannot initialize a tier or overwrite the output status.
TEST_P(DatabaseTest, ComponentCheckpointRemoveThenLoad) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointExists);
    REQUIRE_DATABASE_MEMBER(CheckpointRemove);
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> records = {
            {kUndecided, 7}, {kLose, 0}, {kWin, kRemotenessMax}};
        const std::vector<unsigned char> saved_status = {0x13, 0x00, 0xff, 0x47,
                                                         0x80};
        ASSERT_NO_FATAL_FAILURE(
            SaveCheckpoint(kStoredTier, records, saved_status));
        ASSERT_TRUE(Db().CheckpointExists(kStoredTier));
        ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        ASSERT_EQ(Db().CheckpointRemove(kStoredTier), kSuccess);
        ASSERT_FALSE(Db().CheckpointExists(kStoredTier));
        std::vector<unsigned char> output(saved_status.size(), 0xa5);
        const auto sentinel = output;
        // Missing-load failure explicitly promises no state changes.
        StopAfterError();
        ASSERT_NE(Db().CheckpointLoad(kStoredTier,
                                      static_cast<int64_t>(records.size()),
                                      output.data(), output.size()),
                  kSuccess);
        EXPECT_EQ(output, sentinel);
        EXPECT_FALSE(Db().IsTierLoaded(kStoredTier));
    }));
}

// Saving progress alone leaves a fresh tier without a completed archive.
TEST_P(DatabaseTest, ComponentCheckpointDoesNotMarkSolvedFreshTier) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(TierStatus);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const std::vector<ExpectedRecord> records = {
            {kUndecided, kRemotenessMax},
            {kLose, 0},
            {kDraw, 1},
            {kTie, kRemotenessMax},
            {kWin, 7}};
        std::vector<unsigned char> status = {0x13, 0x00, 0xff, 0x47, 0x80};
        const auto saved_status = status;
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, records));
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
        ASSERT_EQ(Db().CheckpointSave(status.data(), status.size()), kSuccess);
        ASSERT_EQ(status, saved_status);
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);
        if (Db().CheckpointExists != nullptr) {
            ASSERT_TRUE(Db().CheckpointExists(kStoredTier));
        }
    }));
}

// Saving different progress preserves an existing completed archive and status.
TEST_P(DatabaseTest, ComponentCheckpointDoesNotMarkSolvedExistingArchive) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(FlushSolvingTier);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(TierStatus);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const std::vector<ExpectedRecord> archive = {
            {kUndecided, 0},
            {kLose, 1},
            {kDraw, kRemotenessMax / 2},
            {kTie, kRemotenessMax},
            {kWin, 7}};
        const std::vector<ExpectedRecord> progress = {{kWin, kRemotenessMax},
                                                      {kUndecided, 7},
                                                      {kTie, 0},
                                                      {kLose, 3},
                                                      {kDraw, 1}};
        ASSERT_EQ(archive.size(), progress.size());
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, archive));
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, progress));
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        std::vector<unsigned char> status = {0x13, 0x00, 0xff, 0x47, 0x80};
        const auto saved_status = status;
        ASSERT_EQ(Db().CheckpointSave(status.data(), status.size()), kSuccess);
        ASSERT_EQ(status, saved_status);
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        if (Db().CheckpointExists != nullptr) {
            ASSERT_TRUE(Db().CheckpointExists(kStoredTier));
        }
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        if (Db().ProbeInit != nullptr && Db().ProbeDestroy != nullptr &&
            (Db().ProbeValue != nullptr || Db().ProbeRemoteness != nullptr)) {
            ASSERT_NO_FATAL_FAILURE(InitializeProbe());
            for (size_t i = 0; i < archive.size(); ++i) {
                SCOPED_TRACE(i);
                const TierPosition position = {kStoredTier,
                                               static_cast<Position>(i)};
                if (Db().ProbeValue != nullptr) {
                    ASSERT_EQ(Db().ProbeValue(Probe(), position),
                              archive[i].value);
                }
                if (Db().ProbeRemoteness != nullptr) {
                    ASSERT_EQ(Db().ProbeRemoteness(Probe(), position),
                              archive[i].remoteness);
                }
            }
            ASSERT_NO_FATAL_FAILURE(DestroyProbe());
        }
    }));
}

// Distinct tiers retain their own checkpoint data through varied release order.
TEST_P(DatabaseTest, ComponentCheckpointIndependentTiers) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointExists);
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_MEMBER(CheckpointRemove);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        const Tier tiers[] = {kStoredTier, kMissingTier, 47};
        const std::vector<std::vector<ExpectedRecord>> records = {
            {{kUndecided, 7},
             {kLose, 0},
             {kDraw, kRemotenessMax / 2},
             {kTie, kRemotenessMax},
             {kWin, 1}},
            {{kWin, kRemotenessMax}, {kUndecided, 1}, {kLose, 5}},
            {{kTie, 0},
             {kDraw, kRemotenessMax},
             {kUndecided, kRemotenessMax},
             {kLose, 23},
             {kWin, 3},
             {kUndecided, 0},
             {kTie, 7}}};
        const std::vector<std::vector<unsigned char>> statuses = {
            {0x00, 0xa1, 0x00, 0xff},
            {0xe2, 0x00, 0x33, 0x55, 0xff, 0x81, 0x00},
            {0xff}};
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(tiers[i]);
            ASSERT_FALSE(Db().CheckpointExists(tiers[i]));
            auto input_status = statuses[i];
            ASSERT_NO_FATAL_FAILURE(
                SaveCheckpoint(tiers[i], records[i], input_status));
            ASSERT_EQ(input_status, statuses[i]);
        }
        bool exists[] = {true, true, true};
        // Restore and remove B, C, A, rather than the A, B, C save order.
        for (size_t index : {1u, 2u, 0u}) {
            const Tier tier = tiers[index];
            SCOPED_TRACE(tier);
            for (size_t i = 0; i < records.size(); ++i) {
                ASSERT_EQ(Db().CheckpointExists(tiers[i]), exists[i]);
                if (Db().IsTierLoaded != nullptr) {
                    ASSERT_FALSE(Db().IsTierLoaded(tiers[i]));
                }
            }
            ASSERT_TRUE(Db().CheckpointExists(tier));
            std::vector<unsigned char> restored(statuses[index].size(), 0x5a);
            ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
                tier, static_cast<int64_t>(records[index].size()), restored));
            ASSERT_EQ(restored, statuses[index]);
            ASSERT_NO_FATAL_FAILURE(AssertRecords(records[index]));
            if (Db().IsTierLoaded != nullptr) {
                for (size_t i = 0; i < records.size(); ++i) {
                    ASSERT_EQ(Db().IsTierLoaded(tiers[i]), i == index);
                }
            }
            for (size_t i = 0; i < records[index].size(); ++i) {
                const Position position = static_cast<Position>(i);
                SCOPED_TRACE(position);
                if (Db().GetValueFromLoaded != nullptr) {
                    ASSERT_EQ(Db().GetValueFromLoaded(tier, position),
                              records[index][i].value);
                }
                if (Db().GetRemotenessFromLoaded != nullptr) {
                    ASSERT_EQ(Db().GetRemotenessFromLoaded(tier, position),
                              records[index][i].remoteness);
                }
            }
            ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
            if (Db().IsTierLoaded != nullptr) {
                ASSERT_FALSE(Db().IsTierLoaded(tier));
            }
            ASSERT_EQ(Db().CheckpointRemove(tier), kSuccess);
            exists[index] = false;
            for (size_t i = 0; i < records.size(); ++i) {
                ASSERT_EQ(Db().CheckpointExists(tiers[i]), exists[i]);
            }
        }
    }));
}

// Restored progress supports normal record operations and a permanent archive.
TEST_P(DatabaseTest, ComponentCheckpointResumeAndFinish) {
    REQUIRE_DATABASE_CHECKPOINT();
    REQUIRE_DATABASE_MEMBER(CheckpointExists);
    REQUIRE_DATABASE_MEMBER(CheckpointLoad);
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(SetValue);
    REQUIRE_DATABASE_MEMBER(SetRemoteness);
    REQUIRE_DATABASE_MEMBER(SetValueRemoteness);
    REQUIRE_DATABASE_MEMBER(MaximizeValueRemoteness);
    REQUIRE_DATABASE_MEMBER(DecrementNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(ClearNumUndecidedChildren);
    REQUIRE_DATABASE_MEMBER(GetNumUndecidedChildren);
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(FlushSolvingTier);
    REQUIRE_DATABASE_MEMBER(TierStatus);
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingMode(Db().CreateSolvingTier, [&] {
        std::vector<ExpectedRecord> expected = {
            {kUndecided, 7}, {kUndecided, kRemotenessMax},
            {kUndecided, 0}, {kLose, 0},
            {kDraw, 1},      {kTie, kRemotenessMax},
            {kWin, 7}};
        const std::vector<unsigned char> status = {0x00, 0x01, 0xff, 0x42,
                                                   0x80, 0x00, 0x93};
        auto input_status = status;
        ASSERT_NO_FATAL_FAILURE(
            SaveCheckpoint(kStoredTier, expected, input_status));
        ASSERT_EQ(input_status, status);
        ASSERT_TRUE(Db().CheckpointExists(kStoredTier));
        std::vector<unsigned char> restored(status.size(), 0x5a);
        ASSERT_NO_FATAL_FAILURE(LoadCheckpoint(
            kStoredTier, static_cast<int64_t>(expected.size()), restored));
        ASSERT_EQ(restored, status);
        const auto check = [&] {
            ASSERT_NO_FATAL_FAILURE(AssertRecords(expected));
            for (size_t i = 0; i < expected.size(); ++i) {
                SCOPED_TRACE(i);
                ASSERT_EQ(
                    Db().GetNumUndecidedChildren(static_cast<Position>(i)),
                    expected[i].value == kUndecided ? expected[i].remoteness
                                                    : 0);
            }
        };
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusMissing);

        ASSERT_EQ(Db().DecrementNumUndecidedChildren(0), 7);
        expected[0].remoteness = 6;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().ClearNumUndecidedChildren(1), kRemotenessMax);
        expected[1].remoteness = 0;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().DecrementNumUndecidedChildren(2), 0);
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetValue(0, kLose), kSuccess);
        expected[0].value = kLose;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetRemoteness(0, 0), kSuccess);
        expected[0].remoteness = 0;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetValueRemoteness(1, kWin, kRemotenessMax), kSuccess);
        expected[1] = {kWin, kRemotenessMax};
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_TRUE(Db().MaximizeValueRemoteness(4, kTie, 3, ComparePairs));
        expected[4] = {kTie, 3};
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_FALSE(Db().MaximizeValueRemoteness(4, kDraw, kRemotenessMax,
                                                  ComparePairs));
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetValue(2, kWin), kSuccess);
        expected[2].value = kWin;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetRemoteness(3, kRemotenessMax), kSuccess);
        expected[3].remoteness = kRemotenessMax;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetValueRemoteness(5, kUndecided, 3), kSuccess);
        expected[5] = {kUndecided, 3};
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().ClearNumUndecidedChildren(5), 3);
        expected[5].remoteness = 0;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetRemoteness(5, 1), kSuccess);
        expected[5].remoteness = 1;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().DecrementNumUndecidedChildren(5), 1);
        expected[5].remoteness = 0;
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().SetValue(5, kTie), kSuccess);
        expected[5].value = kTie;
        ASSERT_NO_FATAL_FAILURE(check());
        // Explicitly retain an unreachable undecided record in the archive.
        ASSERT_EQ(Db().SetValueRemoteness(2, kUndecided, 0), kSuccess);
        expected[2] = {kUndecided, 0};
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_TRUE(Db().MaximizeValueRemoteness(6, kWin, kRemotenessMax,
                                                 ComparePairs));
        expected[6] = {kWin, kRemotenessMax};
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().DecrementNumUndecidedChildren(6), 0);
        ASSERT_NO_FATAL_FAILURE(check());
        ASSERT_EQ(Db().ClearNumUndecidedChildren(6), 0);
        ASSERT_NO_FATAL_FAILURE(check());

        ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, expected));
    }));
}

// Loading exposes exact values; unloading ends with the documented error read.
TEST_P(DatabaseTest, ComponentLoadReadUnloadValue) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(LoadTier);
    REQUIRE_DATABASE_MEMBER(UnloadTier);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    REQUIRE_DATABASE_MEMBER(GetValueFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> records = {
            {kUndecided, 0},
            {kLose, 1},
            {kDraw, kRemotenessMax / 2},
            {kTie, kRemotenessMax},
            {kWin, 7},
            {kUndecided, kRemotenessMax},
            {kWin, 0}};
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, records));
        ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(
            kStoredTier, static_cast<int64_t>(records.size())));
        ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            ASSERT_EQ(
                Db().GetValueFromLoaded(kStoredTier, static_cast<Position>(i)),
                records[i].value);
        }
        ASSERT_NO_FATAL_FAILURE(UnloadResidentTier(kStoredTier));
        ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        StopAfterError();
        ASSERT_EQ(Db().GetValueFromLoaded(kStoredTier, 0), kErrorValue);
    }));
}

// The remoteness path has its own terminal unloaded-tier error observation.
TEST_P(DatabaseTest, ComponentLoadReadUnloadRemoteness) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(LoadTier);
    REQUIRE_DATABASE_MEMBER(UnloadTier);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    REQUIRE_DATABASE_MEMBER(GetRemotenessFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const std::vector<ExpectedRecord> records = {
            {kUndecided, 0},
            {kLose, 1},
            {kDraw, kRemotenessMax / 2},
            {kTie, kRemotenessMax},
            {kWin, 7},
            {kUndecided, kRemotenessMax},
            {kWin, 0}};
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, records));
        ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(
            kStoredTier, static_cast<int64_t>(records.size())));
        ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            ASSERT_EQ(Db().GetRemotenessFromLoaded(kStoredTier,
                                                   static_cast<Position>(i)),
                      records[i].remoteness);
        }
        ASSERT_NO_FATAL_FAILURE(UnloadResidentTier(kStoredTier));
        ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        StopAfterError();
        ASSERT_EQ(Db().GetRemotenessFromLoaded(kStoredTier, 0),
                  kErrorRemoteness);
    }));
}

// Read-only archives remain distinct while either solving mode is modified.
TEST_P(DatabaseTest, ComponentLoadedTiersAndSolvingTier) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(LoadTier);
    REQUIRE_DATABASE_MEMBER(UnloadTier);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    REQUIRE_DATABASE_MEMBER(GetValueFromLoaded);
    REQUIRE_DATABASE_MEMBER(GetRemotenessFromLoaded);
    REQUIRE_DATABASE_RECORD_READERS();
    ASSERT_NO_FATAL_FAILURE(RunWithSolvingModes([&] {
        constexpr Tier kSolvingTier = 47;
        const std::vector<ExpectedRecord> first = {{kUndecided, 0},
                                                   {kLose, 1},
                                                   {kDraw, kRemotenessMax / 2},
                                                   {kTie, kRemotenessMax},
                                                   {kWin, 7}};
        const std::vector<ExpectedRecord> second = {
            {kWin, kRemotenessMax}, {kUndecided, 3}, {kLose, 0}};
        std::vector<ExpectedRecord> solving = {{kDraw, 13},
                                               {kUndecided, kRemotenessMax},
                                               {kLose, 0},
                                               {kWin, 7},
                                               {kTie, 1},
                                               {kUndecided, 0},
                                               {kWin, kRemotenessMax}};
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, first));
        ASSERT_NO_FATAL_FAILURE(SaveTier(kMissingTier, second));
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(
            kMissingTier, static_cast<int64_t>(second.size())));
        ASSERT_NO_FATAL_FAILURE(
            LoadResidentTier(kStoredTier, static_cast<int64_t>(first.size())));
        ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
        ASSERT_TRUE(Db().IsTierLoaded(kMissingTier));
        ASSERT_FALSE(Db().IsTierLoaded(kSolvingTier));
        ASSERT_NO_FATAL_FAILURE(AssertLoadedRecords(kStoredTier, first));
        ASSERT_NO_FATAL_FAILURE(AssertLoadedRecords(kMissingTier, second));

        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kSolvingTier, solving));
        const auto check = [&] {
            ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
            ASSERT_TRUE(Db().IsTierLoaded(kMissingTier));
            ASSERT_TRUE(Db().IsTierLoaded(kSolvingTier));
            ASSERT_NO_FATAL_FAILURE(AssertRecords(solving));
            ASSERT_NO_FATAL_FAILURE(AssertLoadedRecords(kSolvingTier, solving));
            ASSERT_NO_FATAL_FAILURE(AssertLoadedRecords(kStoredTier, first));
            ASSERT_NO_FATAL_FAILURE(AssertLoadedRecords(kMissingTier, second));
        };
        ASSERT_NO_FATAL_FAILURE(check());
        for (int pass = 0; pass < kNumValues; ++pass) {
            SCOPED_TRACE(pass);
            for (Position position : {0, 3, 6}) {
                SCOPED_TRACE(position);
                const ExpectedRecord replacement = {
                    kLegalValues[(static_cast<int>(position) + pass) %
                                 kNumValues],
                    kRepresentativeRemotenesses[(position + pass) % 4]};
                ASSERT_NO_FATAL_FAILURE(WriteRecord(position, replacement));
                solving[position] = replacement;
                ASSERT_NO_FATAL_FAILURE(check());
            }
        }

        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        ASSERT_FALSE(Db().IsTierLoaded(kSolvingTier));
        ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
        ASSERT_TRUE(Db().IsTierLoaded(kMissingTier));
        ASSERT_NO_FATAL_FAILURE(AssertLoadedRecords(kStoredTier, first));
        ASSERT_NO_FATAL_FAILURE(AssertLoadedRecords(kMissingTier, second));
        ASSERT_NO_FATAL_FAILURE(UnloadResidentTier(kStoredTier));
        ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        ASSERT_TRUE(Db().IsTierLoaded(kMissingTier));
        ASSERT_NO_FATAL_FAILURE(AssertLoadedRecords(kMissingTier, second));
        ASSERT_NO_FATAL_FAILURE(UnloadResidentTier(kMissingTier));
        ASSERT_FALSE(Db().IsTierLoaded(kMissingTier));
        ASSERT_FALSE(Db().IsTierLoaded(kSolvingTier));
    }));
}

// Unloading varied tier sizes preserves every remaining resident archive.
TEST_P(DatabaseTest, ComponentUnloadPreservesOtherTiers) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(LoadTier);
    REQUIRE_DATABASE_MEMBER(UnloadTier);
    REQUIRE_DATABASE_MEMBER(IsTierLoaded);
    REQUIRE_DATABASE_MEMBER(GetValueFromLoaded);
    REQUIRE_DATABASE_MEMBER(GetRemotenessFromLoaded);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const Tier tiers[] = {kStoredTier, kMissingTier, 47};
        const std::vector<std::vector<ExpectedRecord>> records = {
            {{kUndecided, 0},
             {kLose, 1},
             {kDraw, kRemotenessMax / 2},
             {kTie, kRemotenessMax},
             {kWin, 7}},
            {{kWin, kRemotenessMax}, {kUndecided, 3}, {kLose, 0}},
            {{kTie, 0},
             {kDraw, kRemotenessMax},
             {kUndecided, kRemotenessMax},
             {kLose, 23},
             {kWin, 3},
             {kUndecided, 0},
             {kTie, 7}}};
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(tiers[i]);
            ASSERT_NO_FATAL_FAILURE(SaveTier(tiers[i], records[i]));
            ASSERT_FALSE(Db().IsTierLoaded(tiers[i]));
        }
        // Load C, A, B and release B, C, A to exercise independent residency.
        bool resident[] = {false, false, false};
        const auto check = [&] {
            for (size_t i = 0; i < records.size(); ++i) {
                SCOPED_TRACE(tiers[i]);
                ASSERT_EQ(Db().IsTierLoaded(tiers[i]), resident[i]);
                if (resident[i]) {
                    ASSERT_NO_FATAL_FAILURE(
                        AssertLoadedRecords(tiers[i], records[i]));
                }
                if (Db().TierStatus != nullptr) {
                    ASSERT_EQ(Db().TierStatus(tiers[i]), kDbTierStatusSolved);
                }
            }
        };
        for (size_t index : {2u, 0u, 1u}) {
            SCOPED_TRACE(index);
            ASSERT_NO_FATAL_FAILURE(LoadResidentTier(
                tiers[index], static_cast<int64_t>(records[index].size())));
            resident[index] = true;
            ASSERT_NO_FATAL_FAILURE(check());
        }
        for (size_t index : {1u, 2u, 0u}) {
            SCOPED_TRACE(index);
            ASSERT_NO_FATAL_FAILURE(UnloadResidentTier(tiers[index]));
            resident[index] = false;
            ASSERT_NO_FATAL_FAILURE(check());
        }
    }));
}

// All three public record views match the same explicitly seeded archive.
TEST_P(DatabaseTest, ComponentFlushLoadAndProbeAgree) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(LoadTier);
    REQUIRE_DATABASE_MEMBER(UnloadTier);
    REQUIRE_DATABASE_RECORD_READERS();
    REQUIRE_DATABASE_MEMBER(GetValueFromLoaded);
    REQUIRE_DATABASE_MEMBER(GetRemotenessFromLoaded);
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto records = AllRecords();
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, records));
        ASSERT_NO_FATAL_FAILURE(AssertRecords(records));
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
        }
        ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
        if (Db().TierStatus != nullptr) {
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        }
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
        ASSERT_NO_FATAL_FAILURE(LoadResidentTier(
            kStoredTier, static_cast<int64_t>(records.size())));
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_TRUE(Db().IsTierLoaded(kStoredTier));
        }
        ASSERT_NO_FATAL_FAILURE(AssertLoadedRecords(kStoredTier, records));
        ASSERT_NO_FATAL_FAILURE(UnloadResidentTier(kStoredTier));
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
        ASSERT_NO_FATAL_FAILURE(VerifyArchive(kStoredTier, records));
        if (Db().TierStatus != nullptr) {
            ASSERT_EQ(Db().TierStatus(kStoredTier), kDbTierStatusSolved);
        }
    }));
}

// One probe reads every seeded pair in several public request orders.
TEST_P(DatabaseTest, ComponentProbeReadOrders) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_PROBE_READER();
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const auto expected = AllRecords();
        ASSERT_NO_FATAL_FAILURE(SaveTier(kStoredTier, expected));
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        const auto read = [&](Position position) {
            SCOPED_TRACE(position);
            const TierPosition target = {kStoredTier, position};
            if (Db().ProbeValue != nullptr) {
                ASSERT_EQ(Db().ProbeValue(Probe(), target),
                          expected[position].value);
            }
            if (Db().ProbeRemoteness != nullptr) {
                ASSERT_EQ(Db().ProbeRemoteness(Probe(), target),
                          expected[position].remoteness);
            }
        };
        std::vector<Position> positions;
        positions.reserve(expected.size());
        for (size_t i = 0; i < expected.size(); ++i) {
            positions.push_back(static_cast<Position>(i));
        }
        {
            SCOPED_TRACE("Forward");
            for (Position position : positions) {
                ASSERT_NO_FATAL_FAILURE(read(position));
            }
        }
        {
            SCOPED_TRACE("Reverse");
            for (auto position = positions.rbegin();
                 position != positions.rend(); ++position) {
                ASSERT_NO_FATAL_FAILURE(read(*position));
            }
        }
        {
            SCOPED_TRACE("Repeated");
            for (Position position : positions) {
                ASSERT_NO_FATAL_FAILURE(read(position));
                ASSERT_NO_FATAL_FAILURE(read(position));
            }
        }
        {
            constexpr unsigned kShuffleSeed = 0x4C0FFEEu;
            SCOPED_TRACE(::testing::Message()
                         << "Shuffle seed " << kShuffleSeed);
            std::mt19937 generator(kShuffleSeed);
            std::shuffle(positions.begin(), positions.end(), generator);
            for (Position position : positions) {
                ASSERT_NO_FATAL_FAILURE(read(position));
            }
        }
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// Getter order and tier changes must not reuse another logical record's fields.
TEST_P(DatabaseTest, ComponentProbeTierAndGetterSwitching) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_MEMBER(ProbeValue);
    REQUIRE_DATABASE_MEMBER(ProbeRemoteness);
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const Tier tiers[] = {kStoredTier, kMissingTier, 47};
        std::vector<std::vector<ExpectedRecord>> expected(3);
        for (int i = 0; i < 17; ++i) {
            expected[0].push_back({kLegalValues[i % kNumValues],
                                   (i * 97) % (kRemotenessMax + 1)});
        }
        for (int i = 0; i < 5; ++i) {
            expected[1].push_back(
                {kLegalValues[(i + 2) % kNumValues], kRemotenessMax - i});
        }
        for (int i = 0; i < 33; ++i) {
            expected[2].push_back({kLegalValues[(i + 4) % kNumValues],
                                   (i * 53 + 17) % (kRemotenessMax + 1)});
        }
        for (size_t i = 0; i < expected.size(); ++i) {
            SCOPED_TRACE(tiers[i]);
            ASSERT_NO_FATAL_FAILURE(SaveTier(tiers[i], expected[i]));
        }
        ASSERT_NO_FATAL_FAILURE(InitializeProbe());
        // Adjacent calls also change getters and their target tier or position.
        for (int repeat = 0; repeat < 2; ++repeat) {
            ASSERT_EQ(Db().ProbeValue(Probe(), {tiers[0], 0}),
                      expected[0][0].value);
        }
        ASSERT_EQ(Db().ProbeRemoteness(Probe(), {tiers[1], 4}),
                  expected[1][4].remoteness);
        ASSERT_EQ(Db().ProbeValue(Probe(), {tiers[1], 0}),
                  expected[1][0].value);
        ASSERT_EQ(Db().ProbeRemoteness(Probe(), {tiers[1], 1}),
                  expected[1][1].remoteness);
        ASSERT_EQ(Db().ProbeRemoteness(Probe(), {tiers[2], 32}),
                  expected[2][32].remoteness);
        ASSERT_EQ(Db().ProbeValue(Probe(), {tiers[0], 16}),
                  expected[0][16].value);
        const struct {
            size_t tier_index;
            Position position;
        } requests[] = {{0, 0},  {1, 0}, {2, 0},  {0, 8}, {2, 32}, {1, 4},
                        {2, 16}, {1, 2}, {0, 16}, {1, 1}, {2, 1},  {0, 1},
                        {2, 0},  {1, 0}, {0, 0},  {1, 4}, {0, 16}, {2, 32}};
        for (size_t i = 0; i < sizeof(requests) / sizeof(requests[0]); ++i) {
            SCOPED_TRACE(i);
            const auto &request = requests[i];
            const TierPosition target = {tiers[request.tier_index],
                                         request.position};
            const auto &record = expected[request.tier_index][request.position];
            if (i % 2 == 0) {
                ASSERT_EQ(Db().ProbeValue(Probe(), target), record.value);
                ASSERT_EQ(Db().ProbeRemoteness(Probe(), target),
                          record.remoteness);
            } else {
                ASSERT_EQ(Db().ProbeRemoteness(Probe(), target),
                          record.remoteness);
                ASSERT_EQ(Db().ProbeValue(Probe(), target), record.value);
            }
        }
        ASSERT_NO_FATAL_FAILURE(DestroyProbe());
    }));
}

// Independent probe objects retain their own state across sequential reads.
TEST_P(DatabaseTest, ComponentIndependentProbes) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_PROBE_READER();
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        const Tier tiers[] = {kStoredTier, kMissingTier, 47};
        const size_t sizes[] = {5, 17, 31};
        std::vector<std::vector<ExpectedRecord>> records(3);
        for (size_t tier = 0; tier < records.size(); ++tier) {
            for (size_t i = 0; i < sizes[tier]; ++i) {
                records[tier].push_back(
                    {kLegalValues[(i + tier + 1) % kNumValues],
                     static_cast<int>((i * 53 + tier * 211 + 7) %
                                      (kRemotenessMax + 1))});
            }
            ASSERT_NO_FATAL_FAILURE(SaveTier(tiers[tier], records[tier]));
        }
        for (size_t probe = 0; probe < 3; ++probe) {
            ASSERT_NO_FATAL_FAILURE(InitializeProbe(probe));
        }
        const auto read = [&](size_t probe, size_t tier, size_t index) {
            SCOPED_TRACE(::testing::Message()
                         << "Probe " << probe << ", tier " << tiers[tier]
                         << ", index " << index);
            const TierPosition position = {tiers[tier],
                                           static_cast<Position>(index)};
            if (Db().ProbeValue != nullptr) {
                ASSERT_EQ(Db().ProbeValue(Probe(probe), position),
                          records[tier][index].value);
            }
            if (Db().ProbeRemoteness != nullptr) {
                ASSERT_EQ(Db().ProbeRemoteness(Probe(probe), position),
                          records[tier][index].remoteness);
            }
        };
        // Populate distinct probe state, then switch tiers independently.
        for (size_t probe = 0; probe < 3; ++probe) {
            ASSERT_NO_FATAL_FAILURE(
                read(probe, probe, records[probe].size() - 1));
        }
        for (size_t step = 0; step < 18; ++step) {
            for (size_t probe = 0; probe < 3; ++probe) {
                const size_t tier = (step + probe) % records.size();
                const size_t index = (step * 7 + probe) % records[tier].size();
                ASSERT_NO_FATAL_FAILURE(read(probe, tier, index));
            }
        }
        ASSERT_NO_FATAL_FAILURE(DestroyProbe(1));
        for (size_t step = 0; step < 32; ++step) {
            for (size_t probe : {0u, 2u}) {
                const size_t tier = (step + probe) % records.size();
                const size_t size = records[tier].size();
                const size_t index = step % 3 == 0 ? 0
                                     : step % 3 == 1
                                         ? size - 1
                                         : (step * 11 + probe) % size;
                ASSERT_NO_FATAL_FAILURE(read(probe, tier, index));
            }
        }
        ASSERT_NO_FATAL_FAILURE(DestroyProbe(0));
        // The last surviving probe still reads every record in every tier.
        for (size_t tier = 0; tier < records.size(); ++tier) {
            for (size_t i = records[tier].size(); i > 0; --i) {
                ASSERT_NO_FATAL_FAILURE(read(2, tier, i - 1));
            }
        }
        ASSERT_NO_FATAL_FAILURE(DestroyProbe(2));
    }));
}

// Repeated probe lifetimes require only permanent data after solving memory
// frees.
TEST_P(DatabaseTest, ComponentProbeAfterMemoryRelease) {
    REQUIRE_DATABASE_ARCHIVE();
    REQUIRE_DATABASE_MEMBER(FreeSolvingTier);
    REQUIRE_DATABASE_MEMBER(ProbeInit);
    REQUIRE_DATABASE_MEMBER(ProbeDestroy);
    REQUIRE_DATABASE_PROBE_READER();
    ASSERT_NO_FATAL_FAILURE(RunIsolated([&] {
        ASSERT_NO_FATAL_FAILURE(Initialize());
        std::vector<ExpectedRecord> records;
        for (int i = 0; i < 17; ++i) {
            records.push_back({kLegalValues[i % kNumValues],
                               kRepresentativeRemotenesses[i % 4]});
        }
        ASSERT_NO_FATAL_FAILURE(CreateAndPopulate(kStoredTier, records));
        ASSERT_EQ(Db().FlushSolvingTier(nullptr), kSuccess);
        ASSERT_NO_FATAL_FAILURE(ReleaseSolvingTier());
        if (Db().IsTierLoaded != nullptr) {
            ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
        }
        for (size_t cycle = 0; cycle < 8; ++cycle) {
            SCOPED_TRACE(cycle);
            ASSERT_NO_FATAL_FAILURE(InitializeProbe());
            for (size_t step = 0; step < records.size(); ++step) {
                // Five is coprime with seventeen, so each cycle visits every
                // record.
                const size_t index = (cycle + step * 5) % records.size();
                SCOPED_TRACE(index);
                const TierPosition position = {kStoredTier,
                                               static_cast<Position>(index)};
                if (Db().ProbeValue != nullptr) {
                    ASSERT_EQ(Db().ProbeValue(Probe(), position),
                              records[index].value);
                }
                if (Db().ProbeRemoteness != nullptr) {
                    ASSERT_EQ(Db().ProbeRemoteness(Probe(), position),
                              records[index].remoteness);
                }
            }
            ASSERT_NO_FATAL_FAILURE(DestroyProbe());
            if (Db().IsTierLoaded != nullptr) {
                ASSERT_FALSE(Db().IsTierLoaded(kStoredTier));
            }
        }
    }));
}

#undef REQUIRE_DATABASE_PROBE_READER
#undef REQUIRE_DATABASE_CHECKPOINT
#undef REQUIRE_DATABASE_CREATION
#undef REQUIRE_DATABASE_RECORD_READERS
#undef REQUIRE_DATABASE_RECORDS
#undef REQUIRE_DATABASE_ARCHIVE
#undef REQUIRE_DATABASE_MEMBER

// Register further implementations here; assertions use only Database members.
INSTANTIATE_TEST_SUITE_P(
    Implementations, DatabaseTest,
    ::testing::Values(DatabaseImplementation{&kArrayDb, "ArrayDb"},
                      DatabaseImplementation{&kNaiveDb, "NaiveDb"}),
    [](const ::testing::TestParamInfo<DatabaseImplementation> &info) {
        return info.param.label;
    });

}  // namespace
