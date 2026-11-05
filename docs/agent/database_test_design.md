# Database interface test design

This document records the final design of the implemented GoogleTest suite for
[`Database`](../src/core/types/database/database.h). The suite is implemented in
[`database_test.cpp`](../tests/unit/core/db/database_test.cpp) and follows the
project's [unit testing conventions](unit_testing_conventions.md) and
[`xstring_test.cpp`](../tests/unit/libs/string/xstring_test.cpp) assertion style.
The interface header and its shared types define the contract; this document
explains the coverage, fixture architecture, and assertion strategy.

The suite applies the same public-interface expectations to every registered
implementation. The archived implementation contains 263 parameterized test
definitions: 184 unit tests and 79 component tests in 15 categories. ArrayDb
and NaiveDb are registered, producing 526 CTest cases. Unsupported optional
capabilities result in explicit skips.

## Scope and test layers

Unit tests target one member method at a time with the smallest supported
public setup and observation path. A setter needs a reader, and a reader needs
seeded data; these dependencies are explicit. Unit cases remain single-threaded.

Component tests exercise methods that solvers use together: lifecycle and
persistent status, record updates, externally synchronized and internally
synchronized solving, checkpoint recovery, resident and permanent readers,
segmentation, and bounded combined workflows. Small method subsets establish
behavior before mixed stress tests exercise their interactions.

The scenario tables describe coverage families. A family can expand into
separate ordinary/concurrent modes, method-specific cases, or input partitions
in the implementation. The tables contain 200 scenario families, including six
segmentation regressions. Finite domains are exercised within those cases.

## Final interface decisions

| Concern | Contract and test consequence |
| --- | --- |
| Required capabilities | Only Init and Finalize are mandatory. Every optional target, setup operation, observer, and cleanup method is checked before use. |
| Database lifecycle | Only one Database may be initialized in a process at a time. Init while initialized is undefined. Finalize is void, always completes successfully in its defined states, and is a successful no-op before initialization and after finalization; it is also valid after failed Init. Destroy all live probes and finish all operations before Finalize. |
| Defaults and naming | Init accepts null aux for default settings and a null tier-name callback for fallback naming. FlushSolvingTier accepts null aux for default behavior. The fixture's custom tier namer is deterministic and produces distinct valid names; callback timing and count are opaque. |
| Initial contents | Newly created solving records and segment-buffer fields are unknown until explicitly written. SetValue initializes only value; SetRemoteness initializes only remoteness. Every field observed, compared, used as a counter, or persisted by a test is seeded explicitly. |
| Record methods | Setters preserve fields they do not write. Maximize replaces a pair only for a strictly greater comparator result; equal-ranked distinct pairs remain unchanged. Getter results and writer returns are checked against independent logical records. |
| Comparison callbacks | Comparators provide a stable ordering, are safe for concurrent calls, and do not reenter Database. Only the sign of the comparison result matters; callback count and invocation argument order are opaque. |
| Undecided-child counters | For undecided records, remoteness is the counter: decrement and clear return the preceding count, with decrement saturating at zero. For decided records, counter methods return zero and preserve both fields. The generic suite uses only the shared supported range. |
| Concurrent records | Each of the nine documented solving-record operations is individually atomic on a concurrent tier. Loaded getters on that solving tier inherit the corresponding solving getter's guarantee. Overlapping calls have no fixed thread order; separate getters do not provide one atomic pair snapshot. |
| Checkpoint mode | Checkpoints support regular, externally synchronized solving tiers only. All solving writes are paused during checkpoint operations. CheckpointLoad creates a regular solving tier and requires no active solving tier or segment-buffer context. |
| Checkpoint data | Saving overwrites the checkpoint without modifying input status bytes or changing tier solved status. Restoring uses the original tier size and, for a nonempty status buffer, the same status format and size. Mismatched tier sizes are undefined behavior. |
| Checkpoint zero-size status | Save permits either null or non-null status with size zero, without guaranteeing success. Load permits null status with size zero without guaranteeing success, but rejects non-null status with size zero with kIllegalArgumentError. Either method rejects null status with positive size with kIllegalArgumentError. |
| Checkpoint absence | Saving without a solving tier accepts any normal API return. Loading a missing checkpoint returns an error without changing status bytes or residency. Removing an absent checkpoint returns kFileSystemError. |
| Resident tiers | IsTierLoaded includes the active solving tier, including a checkpoint-restored tier. LoadTier uses the tier's original size. Resident archive tiers use UnloadTier; solving tiers use FreeSolvingTier. Repeated active loading and size mismatches are undefined. |
| Probe lifecycle | ProbeInit(nullptr) returns kIllegalArgumentError. Calling ProbeInit on an already initialized probe is undefined; destroy it before initializing the same storage again. ProbeDestroy(nullptr) and repeated destruction after a successful destroy are successful no-ops. |
| Probe read failures | Valid failed reads return kErrorValue or kErrorRemoteness. Destroy the probe even after a failed read and before Finalize; do not retry or continue probing. No shared-probe concurrency guarantee is assumed. |
| Memory estimates | TierMemUsage bounds peak memory for LoadTier and CreateSolvingTier; ConcurrentTierMemUsage bounds concurrent creation. Bounds include temporary memory. Zero-sized tiers are valid; negative tier/size, overflow, and other errors return SIZE_MAX. Generic tests check portable inputs and error sentinels without measuring private allocation formulas. |
| Segment buffers | Buffer count and saved segment count are independent. Zero maximum capacity, zero buffer count, and zero buffer size are valid. Negative creation parameters or a count above MaxNumBuffers return kIllegalArgumentError. Invalid active-buffer getter/setter accesses are undefined. |
| Segment storage | Saved segment indices form the consecutive set 0..n-1. Global position equals segment index times segment capacity plus offset. A successful Flush replaces the saved contents of that index; Load overwrites the destination buffer. Negative Load/Flush indices and inactive buffers have checked errors. |
| Consolidation | Consolidate runs before FreeBuffers in the same initialized lifetime. Segment context, including tier identity, ends at release. Nonempty inputs cover the logical tier completely; offsets beyond its end are inaccessible and ignored during consolidation. Every nonnegative empty-tier shape is valid and publishes a normal zero-position archive without requiring segment flushes. |
| Release behavior | FreeSolvingTier and FreeBuffers succeed without their corresponding context and after successful release. FreeBuffers without a segment context preserves an ordinary or concurrent solving tier. Zero-buffer contexts still release their tier identity. |
| Persistent status | Segment files alone leave TierStatus missing; a successful normal flush or consolidation makes it solved. Checkpoint status is separate. Final archives permit every legal value/remoteness combination, including undecided unreachable positions. SetGameSolved is an idempotent caller declaration, independent of tier enumeration. |
| Errors and cleanup | Stop normal database use after failure. No retry, rollback, or continued-solving guarantee is assumed. Join workers, destroy initialized probes, and use documented finalization cleanup. Failed probe lifecycle operations can leave ownership unspecified, so cleanup does not retry them or finalize with a potentially live probe. |
| Paths and representation | GetPath identifies the initialized sandbox and is null outside that lifetime. Exact spelling, pointer identity, storage layout, compression, private probe fields, and callback counts are outside the test oracle. |

## Data domains and boundary selection

Valid Tier identifiers, Position indices, and stored remotenesses are
nonnegative. Positions must also be within the relevant logical tier. Existing
negative constants retain their meanings as invalid identifiers or error
results; their values are unchanged. Shared declarations are in
[base.h](../src/core/types/base.h), [constants.h](../src/core/constants.h), and
[constants.c](../src/core/constants.c).

| Domain | Coverage |
| --- | --- |
| Values | V = {kUndecided, kLose, kDraw, kTie, kWin}; all 25 ordered old/new value transitions are covered, including unchanged-value writes. |
| Remoteness | R = 0..kRemotenessMax, currently 0..1023. Every implementation supports this minimum domain. V times R contains 5,120 pairs. Larger implementation-specific domains are excluded. |
| Counters | Exercise every count in R. A backend may support larger counters; callers must consult its specification, and this suite does not infer its maximum. |
| Tier sizes and addresses | Sizes zero, one, two, and small non-power-of-two sizes cover empty/singleton/multiple-record behavior. First, interior, and last positions are sampled without duplicate addresses for small tiers. Empty tiers have no legal position to read or write. |
| Negative boundaries | Use named kIllegalTier and kIllegalPosition when referring to invalid identifiers. Estimator checked errors also sample -2 and INT64_MIN; negative sizes include signed minima. Invalid record accesses remain excluded as undefined behavior. |
| Error returns | Require exact codes only where the interface specifies them. A generic nonzero-error contract does not require every implementation to emit every enumerated error. |

## Fixture architecture and coverage policy

DatabaseTest is a GoogleTest parameterized fixture. The implementation list is
the extension point for additional Database descriptors; PascalCase Unit and
Component prefixes support filtering while preserving one suite name.

Stateful cases run in re-executed child processes through GoogleTest's
threadsafe exit-test mode. This isolates global backend state and crashes and
avoids inheriting an active OpenMP runtime across fork. Expected normal exits,
API assertion failures, crashes, and runtime capability skips remain distinct.
Child failures are forwarded to the parent.

The parent atomically creates a private temporary root with mkdtemp. Children
and additional sandbox sessions allocate fresh directories beneath that owned
root. Parent teardown removes only that root, including artifacts left after a
child failure. Parallel CTest processes therefore cannot share temporary paths.
Sequential sessions within a case preserve the single-active-Database rule.

Fixture state tracks solving ownership, loaded tiers, segment contexts, and
initialized probes. Probe objects occupy stable storage so cleanup can destroy
them even when a fatal read assertion exits a helper. Helpers reject attempts
to initialize an active probe. Cleanup respects the stop-after-error contract
and the requirement to destroy probes before finalization.

| Concern | Design |
| --- | --- |
| Implementation selection | Run the same fixture against each real Database descriptor. Init and Finalize must exist; every other target, setup, readback, and cleanup pointer is checked before calling it. Report `GTEST_SKIP` with the missing dependency; do not silently pass or require optional methods to exist. |
| Default settings | Initialize and flush with null aux for default behavior and valid ordinary game/variant/path inputs. No backend-specific config or aux factory. |
| Unit isolation | One target method per concrete case. Use checked public setup and the smallest available observer. A setter needs a getter and vice versa; record these necessary dependencies instead of inspecting private storage. |
| Public disk fixtures | Generate tiny archives, checkpoints, and segments through supported public methods, using explicit independent reference data. No prebuilt backend blobs, filename assumptions, byte corruption, or private adapters. Optional dependencies can make an individual case unavailable. |
| Checkpoint fixture reloads | Create checkpoint fixtures using regular solving tiers only. Free any active solving tier before every CheckpointLoad, including unit overwrite/binary-status setup helpers. No segment-buffer context may be active. A saved checkpoint does not itself free the solving tier. Always use the original created tier size. |
| Unique temporary storage | Atomically allocate a fresh private directory per test and implementation, e.g. mkdtemp. All artifacts live below it. Test names, timestamps, and PIDs alone are insufficient. Remove only that directory during teardown. |
| Single active Database | At most one backend is initialized at a time within a process. Parallel CTest processes use independent sandboxes; in-process tests serialize complete Init-to-Finalize lifetimes, including tests of different backends. |
| Cleanup and errors | Join workers and manually destroy every initialized probe before Finalize, including probes whose reads failed. Error cleanup is permitted; do not retry reads or continue solving after failure. Finalize before init, twice, and after failed Init is valid and void. |
| Optional lifecycle observers | Init/Finalize cases run even when GetPath is absent; guard that additional observation. Other cases require their own supported setup/observer/release methods. |
| Finite values | V has five values: kUndecided, kLose, kDraw, kTie, kWin. Exhaust 5 × 5 = 25 old/new value transitions and V × R, where R = 0..kRemotenessMax (currently 1023): 5,120 pairs. Wider backend domains are out of scope. |
| Counters | Exhaust counts in R, which every implementation supports; do not infer or test a backend's larger maximum. Callers must consult its specification before using larger counts. |
| Initialization | Newly allocated fields are unknown until explicitly written. SetValue initializes only value; SetRemoteness only remoteness. Seed every field used by an assertion, maximize, counter operation, or persisted logical record. Loads restore saved data. Never assert zero/default contents. |
| Addressing | Use sizes 0, 1, 2, and a small non-power-of-two size; collapse duplicate first/interior/last addresses for tiny tiers. Size-zero tests do not access a position. Use distinct nonnegative tier IDs. Estimator rejection cases use kIllegalTier, another negative tier, and INT64_MIN. Invalid position identifiers are named kIllegalPosition; invalid record accesses remain excluded as UB. |
| Checked versus undefined inputs | Test only documented checked errors. Exclude invalid solving-record inputs, repeated active creation/loading, mode switches while active, checkpoints with concurrent solving tiers, checkpoint tier-size mismatches, reinitialization of active probes, invalid existing-tier probe positions, corrupt probe reads, and invalid segment getter/setter indices. |
| Memory estimates | Treat estimates as peak bounds, including ordinary creation for TierMemUsage. Check valid requests and SIZE_MAX errors only; do not measure RSS, assume bytes per record, inject OOM, or assert backend allocation counts. Actual numerical bound accuracy remains implementation-specific coverage. |
| General diagnostics | Generic leak/race sanitizers can supplement execution without private hooks; instrument the linked implementation too. Leak checks do not become method-specific byte-accounting assertions. |
| Concurrent phases | Keep unit cases single-threaded. Components use bounded workers, barriers and fixed seeds. Each supported concurrent record call is atomic; no fixed order for overlapping calls. Comparison callbacks must be stable, thread-safe, and must not reenter Database. Equal ranking of distinct pairs is permitted. Join before lifecycle or I/O operations. Checkpoints require a regular solving context; joining concurrent workers does not make their solving tier checkpoint-compatible. |
| Concurrent readers | Each loaded getter on the concurrent solving tier has the corresponding solving getter's guarantee. Two separate getters do not form an atomic pair snapshot; compare full pairs after joining or under an external lock. Probes access disk independently; no shared-probe or archive-replacement race guarantee is inferred. |
| Segments | Indices are the consecutive set 0..n-1; buffer count and saved segment count are independent. Position = seg_idx × segment_size + offset. Successful Flush replaces its saved index. Consolidate before FreeBuffers in the same initialized lifetime. Empty consolidation requires no prior segment flushes; verify solved TierStatus without inspecting physical storage. Never access offsets outside the logical tier. |
| Zero segmentation capacity | A reported maximum of zero is valid. Run zero-count/size context cases; skip positive-buffer cases with an explicit capacity reason. No backend-specific expected maximum is used. |
| Opaque details | No physical archive/segment-file inspection, allocation formula, DbProbe field/cache-geometry inspection, callback count, or comparator invocation order defines the oracle. Persisted values/remotenesses use public readers; completion uses TierStatus. Empty tiers have no legal probe positions. GetPath comparisons concern the designated directory, not spelling or pointer identity. |
| Fault and corruption branches | Backend-hook-based allocation/I/O failures, known-corrupt archives, status-check errors, and exact memory accounting are excluded. Their return contracts remain documented; this generic suite does not claim those branches were exercised. |

## Category map

| Group | Category | Test definitions |
| --- | --- | ---: |
| Unit | Descriptor, lifecycle, path, and status | 23 |
| Unit | Solving tier allocation and release | 15 |
| Unit | Record methods | 28 |
| Unit | Memory estimates | 7 |
| Unit | Checkpoints | 20 |
| Unit | Loaded tiers | 35 |
| Unit | Probes | 12 |
| Unit | Segmentation API | 44 |
| Component | Lifecycle, naming, and persistent status | 8 |
| Component | Serial and externally synchronized record operations | 15 |
| Component | Concurrent record operations | 21 |
| Component | Checkpoint workflows | 9 |
| Component | Loaded-tier and probe workflows | 9 |
| Component | Segmentation and consolidation | 15 |
| Component | Combined workflows | 2 |
| Total | 8 unit categories and 7 component categories | 263 |

All categories and scenario families are represented in the implementation.
Optional capabilities can skip a scenario for a particular backend without
changing its generic contract.

## Group 1: unit tests

### Descriptor, lifecycle, path, and status

Use a fresh sandbox and successful `Init` where required. Pre-initialization
path/finalization cases opt out of automatic initialization. Never initialize
a second instance while the first lifetime is active. Init uses null aux.
Additional observations and all optional setup/cleanup operations are guarded.
No instance-specific aux value is assumed.

| Scenario | Setup and action; minimal extra members for units | Required assertions |
| --- | --- | --- |
| `UnitDescriptorHasValidInternalName` | Inspect the real descriptor using a bounded terminator search before treating its array as a string. | NUL termination within `kDbNameLengthMax + 1`; length 1 through `kDbNameLengthMax`; no whitespace. Use a defined whitespace classification with unsigned characters. |
| `UnitDescriptorHasValidFormalName` | Bounded inspection of `formal_name`. | NUL termination; length 1 through `kDbFormalNameLengthMax`. Spaces are allowed; do not invent trimming requirements. |
| `UnitDescriptorProvidesRequiredLifecycleMembers` | Inspect the real descriptor. | Only `Init` and `Finalize` must be non-null. Other pointers may be null; check every target and dependency before using it. |
| `UnitInitSucceedsWithTierNamer` | Valid game/variant/private sandbox; deterministic valid callback; `aux = nullptr`. | Returns `kSuccess`. If `GetPath` exists, its result identifies the supplied sandbox. No callback timing/count assertion. |
| `UnitInitAcceptsNullTierNamer` | Same valid fixture with null tier namer and null aux. | Returns `kSuccess`; fallback supported without requiring GetPath or storage methods. |
| `UnitGetPathBeforeInitialization` | Fresh isolated backend process before any successful `Init`. | Returns `nullptr`. |
| `UnitGetPathAfterInitialization` | Initialize with the existing private sandbox; call optional `GetPath`. | Non-null result identifies the same directory. No exact spelling, pointer identity, or normalization requirement. |
| `UnitGetPathAfterFinalization` | Successful `Init`, then `Finalize`. | Returns `nullptr`. Never dereference a pointer saved before finalization. |
| `UnitFinalizeWithOwnedTiers` | Separately create an ordinary/concurrent solving tier or load a tier through supported public setup; destroy probes first; call Finalize directly. | Finalize completes successfully; optional GetPath returns null. A generic leak-sanitizer run may supplement this; no backend allocation-count assertion. |
| `UnitFinalizeWithOwnedBuffers` | Create segment buffers through supported public methods; finalize without a live probe. | Successful completion; optional GetPath returns null; generic leak checks may supplement the run. |
| `UnitFlushSolvingTierWritesCurrentContents` | Explicitly populate all records; flush; free memory; observe with a fresh probe. Pass nullptr as flush aux. | Successful flush persists each reference pair; no archive-format assertions. |
| `UnitTierStatusReportsMissing` | Fresh sandbox; queried tier has no archive. | Exactly `kDbTierStatusMissing`. |
| `UnitTierStatusReportsSolved` | Valid normally generated archive fixture. | Exactly `kDbTierStatusSolved`. |
| `UnitTierStatusSelectsRequestedTier` | One normally saved solved tier and one absent tier, with distinct IDs. | Each query returns the requested tier status; no corrupt-file fixture. |
| `UnitGameStatusReportsIncomplete` | Fresh game/variant/sandbox with no solved declaration. | Exactly `kDbGameStatusIncomplete`. |
| `UnitGameStatusReportsSolved` | Fixture representing the game's solved declaration; minimal setup may use checked `SetGameSolved`. | Exactly `kDbGameStatusSolved`. |
| `UnitSetGameSolvedRecordsDeclaration` | Initialized game; invoke `SetGameSolved`; observer: `GameStatus`. | `kSuccess`; status is solved. No requirement to derive this from a count of tiers, which the interface never receives. |
| `UnitFinalizeBeforeInitialization` | Fresh isolated lifetime; call Finalize before Init. | Successful no-op; no return assertion because Finalize is void. |
| `UnitFinalizeRepeated` | Initialize; finalize successfully; finalize again. | Second call completes successfully; optional GetPath remains null. |
| `UnitFlushSolvingTierRepeated` | Create and populate a tier; flush twice without changing records; free and probe. Pass nullptr as flush aux. | Both successful calls preserve the same logical records; no byte-identity requirement. |
| `UnitSetGameSolvedRepeated` | Declare solved twice using the optional method; observe with GameStatus when supported. | Both normal calls succeed; status stays solved. |

### Solving tier allocation and release

| Scenario | Setup and action; minimal extra members for units | Required assertions |
| --- | --- | --- |
| `UnitCreateSolvingTierSupportedSizes` | Initialized DB; create tiers of size 1, 2, and a small non-power-of-two size. Guard optional residency observation. | `kSuccess`; if IsTierLoaded exists, requested tier is loaded. No assertion about default records. |
| `UnitCreateConcurrentSolvingTierSupportedSizes` | Same independent cases with concurrent creation. Guard optional residency observation. | `kSuccess`; if IsTierLoaded exists, requested tier is loaded. |
| `UnitCreateTierProvidesRequestedExtent` | For both modes, seed first/interior/last records with `SetValueRemoteness`; read with getters. | Every requested record is accessible and distinct; exact values survive. |
| `UnitCreateTierZeroSize` | In separate lifetimes, call each supported creation mode with size zero; never access a position. | Normal `kSuccess`; release the zero-sized solving tier with the proper free method. |
| `UnitCreateTierNegativeSize` | Separate cases for each creation mode and sizes -1 and INT64_MIN; no active solving context. | Exactly `kIllegalArgumentError`; finalize afterward. |
| `UnitFreeSolvingTierWithoutTier` | Initialized DB with no solving tier; call free. | `kSuccess`; no-op. |
| `UnitFreeSolvingTierEachCreationMode` | Create ordinary or concurrent tier, then free; guard optional IsTierLoaded observation. | `kSuccess`; if the residency observer exists, the tier is no longer loaded. |
| `UnitFreeSolvingTierRepeated` | Create a supported solving tier; free successfully; call free again. | Both calls return `kSuccess`; optional residency observer is false. |

### Record methods

| Scenario | Setup and action; minimal extra members for units | Required assertions |
| --- | --- | --- |
| `UnitSetValueEveryLegalValue` | Seed with combined setter; set each legal value while retaining each legal remoteness in R; observe value/remoteness getters. | `kSuccess`; exact new value; every seeded remoteness preserved. |
| `UnitSetValueEveryValueTransition` | All `5 × 5 = 25` ordered `(old_value, new_value)` pairs from V, including unchanged values; seed remoteness first. | Exact new value; remoteness and neighbors unchanged. |
| `UnitSetRemotenessEveryLegalRemoteness` | Seed each legal value; set every `r` in global remoteness range; observe both getters. | `kSuccess`; exact remoteness; value unchanged. The global minimum range is required. |
| `UnitSetRemotenessRepeatedAndReplacementWrites` | Same and differing old/new remotenesses, including zero and maximum. | Successful idempotent/replacement writes; no neighbor changes. |
| `UnitSetValueRemotenessEveryLegalPair` | Set each legal value/remoteness combination; observe both getters. | `kSuccess`; both fields exact; no neighbor changes. |
| `UnitSetValueRemotenessRepeatedPair` | Write identical pair twice. | Both writes succeed; pair retained. |
| `UnitGetValueEveryLegalResult` | Seed all `V × R` pairs with combined setter; include first/interior/last positions. | Exact result; reads do not change records. |
| `UnitGetRemotenessEveryLegalResult` | Seed all `V × R` pairs using combined setter, including undecided values. | Exact remoteness; reads do not change records. The global minimum range is required. |
| `UnitMaximizeValueRemotenessGreaterCandidate` | Seed old pair; comparator ranks candidate greater; observe both getters. | `true`; exact candidate replaces old; neighbors unchanged. |
| `UnitMaximizeValueRemotenessSmallerCandidate` | Candidate ranks lower than seeded old pair. | `false`; exact original pair retained. |
| `UnitMaximizeValueRemotenessEqualCandidate` | Candidate equals seeded pair. | `false`; original pair retained. |
| `UnitMaximizeValueRemotenessComparesValues` | Equal remotenesses; vary old/new values across legal-value pairs. | Replacement and boolean follow supplied comparison. |
| `UnitMaximizeValueRemotenessComparesRemotenesses` | Equal values; lesser/equal/greater remotenesses. | Exact comparator-selected pair and boolean. |
| `UnitMaximizeValueRemotenessHonorsAlternateOrdering` | Comparators reverse enum ordering or prefer smaller remoteness; include conflicting field preferences. | Supplied comparator determines result. |
| `UnitMaximizeValueRemotenessUsesComparisonSign` | Valid comparators return negative/positive magnitudes other than one, zero, and `INT_MIN`/`INT_MAX` directly without arithmetic overflow. | Sign determines result; no assertion about callback call count or invocation argument order. |
| `UnitMaximizeValueRemotenessDomainBoundaries` | Candidate pairs exhaust `V × R`; old pairs cover each value and representative remotenesses including both boundaries. | Exact selected pair and correct boolean against the independent comparator model. |
| `UnitMaximizeValueRemotenessEquivalentRanks` | Comparator gives rank zero to distinct initialized pairs; compare before/after with getters. | `false`; original value and remoteness remain unchanged. |
| `UnitDecrementNumUndecidedChildrenPositive` | Seed `(kUndecided,n)` with combined setter for each positive `n` in `R`; observe getters. | Returns old `n`; counter becomes `n-1`; value unchanged. This covers the shared remoteness domain, not a claimed maximum counter capacity. |
| `UnitDecrementNumUndecidedChildrenZero` | Seed `(kUndecided,0)`. | Returns zero; record unchanged; no underflow. |
| `UnitDecrementNumUndecidedChildrenDecidedValues` | Each decided value with every remoteness in `R`. | Returns zero; value and remoteness unchanged. |
| `UnitClearNumUndecidedChildrenPositive` | Seed `(kUndecided,n)` for each positive `n` in `R`. | Returns old `n`; counter becomes zero; value unchanged. |
| `UnitClearNumUndecidedChildrenZero` | Seed `(kUndecided,0)`. | Returns zero; record unchanged. |
| `UnitClearNumUndecidedChildrenDecidedValues` | Each decided value with every remoteness in `R`. | Returns zero; both fields unchanged. |
| `UnitGetNumUndecidedChildrenUndecidedCounts` | Seed `(kUndecided,n)` for every `n` in `R`, including zero. | Exact count returned; record unchanged. |
| `UnitGetNumUndecidedChildrenDecidedValues` | Each decided value with every remoteness in `R`. | Returns zero; underlying remoteness remains unchanged. |
| `UnitCounterMethodsPositionIsolation` | Target each counter method at first/interior/last positions with distinct neighbor data. | Only intended record changes; getter changes none. |

### Memory estimates

| Scenario | Setup and action; minimal extra members for units | Required assertions |
| --- | --- | --- |
| `UnitTierMemUsageValidRequests` | For ordinary valid tier IDs and small nonnegative sizes including zero, call optional TierMemUsage. | A usable estimate rather than SIZE_MAX under normal conditions. Interpret it as a peak bound; no formula, monotonicity, RSS, or allocation-accounting assertions. |
| `UnitConcurrentTierMemUsageValidRequests` | Same small nonnegative request partitions for optional ConcurrentTierMemUsage. | A usable peak estimate rather than SIZE_MAX under normal conditions; no numeric byte formula. |
| `UnitMemoryEstimateRejectedInputs` | For each estimator, separately test negative sizes and negative tiers kIllegalTier, -2, and INT64_MIN, with other inputs valid. The named sentinel and representative negative values are invalid; no sentinel value changes. | Exactly `SIZE_MAX` for every negative tier/size case. Do not assume every large representable positive size overflows. |
| `UnitTierMemUsageBeforeOrdinaryCreation` | Query estimate, then create/free a small ordinary tier using supported methods. | Both APIs usable in this sequence; the estimate contract covers ordinary creation as well as loading. This smoke test does not independently measure the numerical bound. |

### Checkpoints

All solving-tier fixtures in this category use regular creation. The
checkpoint API does not support concurrent solving tiers, and restoration
always creates a regular tier.

| Scenario | Setup and action; minimal extra members for units | Required assertions |
| --- | --- | --- |
| `UnitCheckpointExistsAbsent` | Initialize fresh storage; query a tier with no checkpoint. | `false`. |
| `UnitCheckpointExistsPresent` | Prepare one valid checkpoint fixture; query its tier. | `true`; query of a different absent tier returns `false`. |
| `UnitCheckpointSaveRecordsAndStatus` | Create/populate solving tier; save a positive-sized binary status; free; restore via checked CheckpointLoad and getters. | `kSuccess`; exact checkpoint records and status; const input bytes unchanged. |
| `UnitCheckpointSaveOverwritesExisting` | Prepare old checkpoint; save changed records/status, including short-to-long and long-to-short status payloads. Free the solving tier, then reload with the new matching size/format. | `kSuccess`; restored logical records/status match the newest save. No assertion about old physical trailing file bytes. |
| `UnitCheckpointSaveBinaryStatus` | Save blobs containing embedded NUL, all-zero, all-`0xff`, bytes `0..255`, and lengths not divisible by machine word size; minimally reload. | Exact bytes and length round trip; no string truncation. |
| `UnitCheckpointLoadRestoresFixture` | Valid checkpoint fixture; no current solving tier; matching tier size and status format/size. Use solving getters as observers. | `kSuccess`; exact records/status; solving tier made available. |
| `UnitCheckpointLoadMissingDoesNothing` | No checkpoint; sentinel-fill output status; capture baseline residency. Call load. | Nonzero; status and residency unchanged. |
| `UnitCheckpointLoadOutputBounds` | Valid fixture; output status buffer has prefix/suffix guards beyond required payload. | Exact payload restored; guard bytes unchanged. |
| `UnitCheckpointRemoveExisting` | Valid checkpoint fixture; remove; minimally observe `CheckpointExists`. | `kSuccess`; checkpoint absent. |
| `UnitCheckpointRemoveAbsent` | No checkpoint, including state after an earlier successful removal. | Exactly `kFileSystemError`. |
| `UnitCheckpointRemovePreservesOtherTiers` | Checkpoint fixtures for distinct tiers; remove one. | Target absent; other checkpoint retains exact data. |
| `UnitCheckpointZeroStatus` | Separate supported save/load scenarios with `(status=nullptr, status_size=0)`; checkpoint load has no active solving context. | Call is permitted, but success is not guaranteed and no fixed result code is asserted. End the lifetime on failure. |
| `UnitCheckpointSaveZeroSizeWithBuffer` | Active solving tier; pass a valid non-null status buffer with status_size zero. | Allowed call with no prescribed API return code; crashes and sanitizer failures still fail. End the lifetime on an error return. |
| `UnitCheckpointLoadZeroSizeWithBuffer` | Valid checkpoint; no active solving context; pass a valid non-null status buffer with status_size zero. | Exactly `kIllegalArgumentError`. |
| `UnitCheckpointSaveNullStatusWithPositiveSize` | Active solving tier; call CheckpointSave(nullptr, 1), then finalize without retry. | Exactly `kIllegalArgumentError`; no mutation/rollback assertions. |
| `UnitCheckpointLoadNullStatusWithPositiveSize` | Valid checkpoint; no active solving context; request the correct tier size but pass null status with positive status_size. | Exactly `kIllegalArgumentError`; do not infer a failure-state or retry guarantee. |
| `UnitCheckpointSaveWithoutSolvingTier` | Initialize without creating a solving tier; save with null status and zero size. | Accept any normal API return; crashes and sanitizer failures fail the test. No checkpoint/residency side effects are asserted. |

### Loaded tiers

| Scenario | Setup and action; minimal extra members for units | Required assertions |
| --- | --- | --- |
| `UnitLoadTierValidArchive` | Valid on-disk fixture and correct tier size; load; minimally observe `IsTierLoaded` and one getter. | `kSuccess`; resident; known record readable. |
| `UnitUnloadTierPreviouslyLoaded` | Load valid fixture as setup; unload; observe residency. | `kSuccess`; tier no longer resident. |
| `UnitUnloadTierRejectsSolvingTier` | In separate cases establish ordinary/concurrent/checkpoint-restored solving tier; attempt UnloadTier; finalize. | Exactly `kIllegalArgumentError`; no subsequent continued-solving/retry assertion. |
| `UnitIsTierLoadedAbsent` | Fresh database, and a separate case with an archive existing only on disk. | `false`; disk existence alone is insufficient. |
| `UnitIsTierLoadedEachResidentRoute` | Independently establish residency by `LoadTier`, ordinary create, concurrent create, or checkpoint load. | `true` for each documented route. |
| `UnitIsTierLoadedAfterRelease` | Resident tier released by corresponding unload/free method. | `false` for released tier. |
| `UnitGetValueFromLoadedMatrix` | Known resident fixture from `LoadTier`; query all `V × R` pairs and boundary/interior positions. | Exact fixture values. |
| `UnitGetRemotenessFromLoadedMatrix` | Known resident fixture from `LoadTier`; query all `V × R` pairs. | Exact fixture remotenesses. |
| `UnitLoadedGettersSolvingTier` | Independently establish solving tier through each supported route and populate known data; call both loaded getters. | Exact current in-memory values/remotenesses. |
| `UnitLoadedGettersNonresidentTier` | Tier absent from memory, with and without an archive existing on disk. | `kErrorValue` and `kErrorRemoteness`. |
| `UnitLoadedGettersDistinguishTiers` | Two resident tiers contain different records at the same position. | Each getter addresses the specified tier. |
| `UnitLoadTierMissingArchive` | Fresh sandbox and valid tier/size; no archive exists. | Exactly `kRuntimeError`. |
| `UnitLoadTierRejectsSolvingTier` | Create a solving tier; request LoadTier for that same tier. | Exactly `kIllegalArgumentError`. |
| `UnitUnloadTierRejectsNonresidentTier` | Tier has not been loaded or created; request unload. | Exactly `kIllegalArgumentError`. |

### Probes

Reinitializing an initialized probe is undefined behavior and is excluded.
Destroying it before initializing the same storage again remains valid.

| Scenario | Setup and action; minimal extra members for units | Required assertions |
| --- | --- | --- |
| `UnitProbeInitValidStorage` | Initialized database; valid `DbProbe` storage; initialize and perform a minimal fixture read. | `kSuccess`; initialized probe usable; do not inspect cache fields. |
| `UnitProbeInitNull` | Initialized database; call optional ProbeInit(nullptr); no real probe object is created. End normal use after the checked error. | Exactly `kIllegalArgumentError`. |
| `UnitProbeDestroyNull` | Initialized database; call optional ProbeDestroy(nullptr). | `kSuccess`; successful no-op. |
| `UnitProbeDestroyUnused` | Successfully initialize a probe; destroy it before any read. | `kSuccess`; generic leak sanitizer can supplement, without asserting DbProbe internals. |
| `UnitProbeDestroyAfterReads` | Initialize; read a public-API-generated archive; destroy. | `kSuccess`; generic resource diagnostics can supplement. |
| `UnitProbeValueMatrix` | Initialized probe and valid permanent fixture; query all `V × R` pairs and first/interior/last records. | Exact fixture values. |
| `UnitProbeRemotenessMatrix` | Initialized probe and valid fixture covering all `V × R` pairs. | Exact fixture remotenesses. |
| `UnitProbeValueNotFound` | Initialized probe; query a missing tier with an ordinary position value. Manually destroy the probe before finalization; do not read again. | `kErrorValue`; destroy returns kSuccess normally. No invalid existing-tier position call. |
| `UnitProbeRemotenessNotFound` | Initialized probe; query a missing tier; manually destroy the probe before finalization without another read. | `kErrorRemoteness` from core/constants.h; destroy returns kSuccess normally. |
| `UnitProbeDestroyRepeated` | Initialize; destroy successfully; destroy the same probe again. | Second destruction returns `kSuccess`. |
| `UnitProbeDestroyAfterFailedValueRead` | Initialize probe; use ProbeValue on a missing tier and check kErrorValue; destroy the initialized probe before finalization. | ProbeDestroy returns `kSuccess` normally. No further probing or retry. |
| `UnitProbeDestroyAfterFailedRemotenessRead` | Initialize probe; use ProbeRemoteness on a missing tier and check kErrorRemoteness; destroy before finalization. | ProbeDestroy returns `kSuccess` normally. No further probing or retry. |

### Segmentation API

All cases use the common initialization/finalization fixture. `B` is a
resource-bounded positive buffer count at most `MaxNumBuffers()`; `S` is a
small positive segment capacity. Positive-capacity cases require at least one
supported buffer. If the reported maximum is zero, report positive-buffer cases as skipped
with that explicit capacity reason; zero-buffer context cases remain valid. Explicitly seed every record before reading it. `seg.` below means
`Database::segmentation`. Cases needing distinct buffers require `B >= 2`;
skip them with a capacity reason when the maximum is below two.

| Scenario | Setup and action; minimal extra members for units | Required assertions |
| --- | --- | --- |
| `UnitSegmentationMaxNumBuffersReportsCapacity` | Call optional MaxNumBuffers; use its reported capacity to select bounded create/use cases. | A nonnegative count; zero is valid. No expected implementation-specific number or copied adapter oracle. |
| `UnitSegmentationCreateBuffersSinglePosition` | Create one buffer with capacity one; explicitly set and read offset zero. Extra: segment setter/getters, free. | `kSuccess` with adequate resources; requested record is usable. No default contents assertion. |
| `UnitSegmentationCreateBuffersMultipleBuffers` | Create feasible `B` buffers of capacity `S`; seed/read first, interior, and last offsets of every buffer. Extra: segment setter/getters, free. | `kSuccess`; requested buffers and offsets are usable without aliasing. |
| `UnitSegmentationCreateBuffersAtMaximum` | If reported capacity is small enough for the fixture budget, request that count with a small segment size; otherwise report resource-bound omission. At maximum zero, only create/free the context. | Supported count usable under adequate resources; no record access when count is zero, no induced OOM or allocation instrumentation. |
| `UnitSegmentationCreateBuffersZeroCountOrSize` | Separate no-context lifetimes for zero buffers and zero-sized buffers; count never above reported capacity; do not access nonexistent offsets. | Normal `kSuccess`; context can be freed. |
| `UnitSegmentationCreateBuffersNegativeCountOrSize` | Separate cases: negative buffer count with valid size; negative size with valid count, including signed minima. | Exactly `kIllegalArgumentError`. |
| `UnitSegmentationCreateBuffersAboveCapacity` | Query reported maximum M; request M+1 buffers with otherwise valid small sizes, including zero. Only form M+1 when representable as int and within the fixture's resource budget; cover M=0 with a request for one buffer. | Exactly `kIllegalArgumentError`. Report an explicit omission if the maximum cannot be exceeded safely; do not overflow INT_MAX or force OOM. |
| `UnitSegmentationGetValueAllLegalValues` | Seed active records with all `V × R` pairs; read each. Extra: create, setter, free. | Exact seeded values for every legal `Value` regardless of remoteness. |
| `UnitSegmentationGetValueSelectsBufferAndOffset` | Seed distinguishable values across feasible buffers and first/interior/last valid offsets. Extra: create, setter, free. | Returns the requested buffer's requested record. |
| `UnitSegmentationGetRemotenessLegalDomain` | Seed all `V × R` pairs, including zero and the declared maximum, then read each remoteness. Extra: create, setter, free. | Exact seeded remoteness; the project-wide minimum range is supported; no storage-width assumption. |
| `UnitSegmentationGetRemotenessSelectsBufferAndOffset` | Distinct remotenesses across feasible buffers and first/interior/last valid offsets. Extra: create, setter, free. | Returns the requested buffer's requested record. |
| `UnitSegmentationSetValueRemotenessLegalPairs` | Write every legal value with each supported remoteness at valid offsets. Extra: create, both segment getters, free. | Both fields equal the supplied pair; setter has no return value. |
| `UnitSegmentationSetValueRemotenessOverwritesPair` | Explicitly seed pair A, then write distinct pair B. Extra: create, both segment getters, free. | Both fields contain B. |
| `UnitSegmentationSetValueRemotenessPreservesOtherRecords` | Seed neighbors and other buffers; change one target. Extra: create, both segment getters, free. | Target changes; explicitly initialized other records retain their values and remotenesses. |
| `UnitSegmentationLoadRestoresSavedSegment` | Public setup: seed/flush an in-range segment; overwrite destination memory; load into an active buffer; read all logical records. | `kSuccess`; exact saved pairs restored, replacing previous contents. |
| `UnitSegmentationLoadSelectsDestinationBuffer` | Keep multiple buffers with different explicit data; load a valid saved segment into one. Extra: create, setter, flush, getters, free. | Destination receives saved contents; other buffers retain their data. |
| `UnitSegmentationLoadRejectsInactiveBuffer` | Keep source segment valid; try inactive destination indices such as `-1` and `B`, separately. Extra: create, setter, flush, free. | Exactly `kIllegalArgumentError`. Avoid combining inactive destination with missing segment because error precedence is unspecified. |
| `UnitSegmentationLoadMissingSegment` | Active buffer; request a valid segment index in the intended range 0..n-1 that has not yet been saved. | Exactly `kFileSystemError`; do not use an out-of-range index to create a missing-file test. |
| `UnitSegmentationFlushSavesBufferContents` | Fill an active buffer; flush an in-range segment; overwrite memory; reload using public API. | `kSuccess`; reloaded pairs match the reference data; no format-inspection adapter. |
| `UnitSegmentationFlushSelectsSourceBuffer` | Distinct explicit buffer patterns; flush selected sources to distinct segment indices, then reload. Extra: create, setter, load, getters, free. | Saved content comes from the selected buffer, independent of buffer index equaling segment index. |
| `UnitSegmentationFlushRejectsInactiveBuffer` | Flush from a negative or inactive buffer index; segment index otherwise valid. Separate cases avoid error-precedence assumptions. | Exactly `kIllegalArgumentError`. |
| `UnitSegmentationNegativeDiskIndices` | In separate valid-context cases call Load and Flush with an active buffer and seg_idx -1 or INT_MIN. | Exactly `kIllegalArgumentError`. |
| `UnitSegmentationFlushOverwritesExistingIndex` | Save A, write B, successfully flush again to the same valid disk segment index; reload using public methods. | kSuccess; loaded records equal B. Replacing the same index does not add a segment. No rollback/retry assertion. |
| `UnitSegmentationFreeBuffersSucceeds` | Create buffers using public methods; free once; finalization completes cleanup. | `kSuccess` normally; generic leak checks may supplement without backend allocation assertions. |
| `UnitSegmentationFreeBuffersMakesIndicesInactive` | Save a valid segment, free all buffers, then try loading it into a formerly active index. Extra: create, setter, flush, load. | Free succeeds; load returns `kIllegalArgumentError` because destination is no longer active. |
| `UnitSegmentationFreeBuffersWithoutContext` | Initialized database, no segment context created; call FreeBuffers. | `kSuccess`; successful no-op. |
| `UnitSegmentationFreeBuffersRepeated` | Create and free successfully; free again. | Second call returns `kSuccess`. |
| `UnitSegmentationConsolidateSingleSegment` | Flush one fully initialized segment with matching `tier_size`; consolidate; observe normal archive through initialized probe. Extra: create, setter, flush, probe lifecycle/readers, free. | `kSuccess`; archive contains exactly the logical pairs. |
| `UnitSegmentationConsolidateMultipleSegments` | Publicly save full segments at consecutive indices; consolidate before freeing; probe all positions. | `kSuccess`; exact ordered concatenation: global position = seg_idx × segment_size + offset. |
| `UnitSegmentationConsolidateEmptyTier` | Create a zero-buffer, zero-sized context; check optional TierStatus before consolidation; call Consolidate(0,0) without preceding segment flushes, then free. | kSuccess; a fresh tier reports missing before and solved after consolidation when TierStatus exists. No file inspection and no empty-tier probe call. |
| `UnitSegmentationConsolidateEmptyTierShapes` | Cross segment counts 0/1/several with capacities 0/positive at tier_size=0. Create a zero-buffer context, perform no segment flushes, consolidate, and free in separate lifetimes. | kSuccess and solved TierStatus when supported for every shape. No dependency on positive buffer capacity, physical segment-file materialization, or valid positions to probe. |
| `UnitSegmentationConsolidateNegativeArguments` | Separate valid-context cases with negative tier_size or segment count; other argument valid. | Exactly `kIllegalArgumentError`. |
| `UnitSegmentationFreeBuffersClearsZeroBufferContext` | Create a zero-buffer context, free it, then create and populate an ordinary solving tier. | Release clears the segment context, including its tier identity; normal solving and supported residency observations work. |
| `UnitSegmentationLoadRejectsBufferBeforeCreation` | Initialized database with no segment buffers; call Load with buffer zero and a nonnegative segment index. | Exactly `kIllegalArgumentError` for the inactive destination. |
| `UnitSegmentationFlushRejectsBufferBeforeCreation` | Initialized database with no segment buffers; call Flush with buffer zero and a nonnegative segment index. | Exactly `kIllegalArgumentError` for the inactive source. |
| `UnitSegmentationFlushRejectsBufferAfterFree` | Create and free segment buffers; call Flush on a formerly active buffer. | Exactly `kIllegalArgumentError`; freed indices remain inactive. |

The segment getters and setter expressly assume an active buffer and valid offset. Calls violating those assumptions do not belong in portable checked-error tests.

## Group 2: component tests

Each component owns its full lifetime and temporary storage. Exercise small
method subsets first; mixed stress cases supplement their coverage. For
permanent data, compare against the test's reference records, even when two
database access paths are also compared with each other.

### Lifecycle, naming, and persistent status

| Scenario | Setup and action | Required assertions |
| --- | --- | --- |
| `ComponentBasicLifecycle` | Init valid ordinary inputs with null aux; optionally inspect GetPath; Finalize. | Successful lifecycle; if supported, GetPath identifies the sandbox while initialized and is null after finalization. |
| `ComponentLifecycleRestartsWithFreshResidency` | Write/free an archive, finalize, then initialize the same game/variant/path. | On-disk data/status survive; previous in-memory tiers are not resident; a newly initialized probe reads the reference records. |
| `ComponentFallbackTierNamesRoundTrip` | Initialize with null namer; write and read two distinct tiers in one sandbox. | Fallback naming supports correct independent round trips; no assumed filename spelling. |
| `ComponentCustomTierNamesRoundTrip` | Initialize with a valid deterministic callback producing distinct legal names; write, restart with the same callback, and read. | Correct records for each tier; callback-driven storage remains accessible. No assumptions about call count or path layout. |
| `ComponentTierSolvedStatusLifecycle` | Fresh tier: create, explicitly populate, check status, flush, check status, free, check status; repeat for each creation mode. | Missing before final archive is flushed; solved after successful flush and after freeing memory. In-memory residency is separate from disk solved status. |
| `ComponentGameSolvedDeclarationPersists` | Fresh game: check incomplete, call `SetGameSolved`, check solved, finalize/reinitialize same identity, check again. | Declaration changes status to solved and remains readable in the later session. |
| `ComponentSandboxIsolation` | In sequential backend sessions, write different records for the same game/variant/tier in separate private sandboxes; reopen each. | Each sandbox reads its own records and status. Do not overlap module-global initialization. |

### Serial and externally synchronized record operations

| Scenario | Setup and action | Required assertions |
| --- | --- | --- |
| `ComponentSerialSetterFamilies` | Separate scenarios for value-only after remoteness seeding, remoteness-only after value seeding, and combined writes. | Every position matches expected fields; untouched fields retained. |
| `ComponentSerialSetterInteroperability` | Combined write -> value-only -> remoteness-only -> combined write; inspect after each. | Exact per-operation state; neighboring records unaffected. |
| `ComponentSerialMaximizeReferenceFold` | Ascending/descending/shuffled/duplicate candidates under multiple comparators. | Reference maximum after each call; boolean matches strict improvement. |
| `ComponentCounterCountdownAndClear` | Countdown through zero; separately decrement a few times, clear, decrement, clear again. | Exact preceding-count returns; stable zero; no underflow. |
| `ComponentCounterValueStateTransitions` | Undecided count -> decided value through each setter family -> counter methods -> explicitly return to undecided. | Decided operations return zero without changing remoteness; restored undecided interpretation uses retained or newly written remoteness. |
| `ComponentCounterMultiplePositions` | Interleave different counts and clear/decrement/query sequences across first/interior/last records. | Independent per-position reference models hold. |
| `ComponentSerialRecordsPersistAfterFlush` | Explicitly initialize every record; mix setters/maximize/counters; flush, free, probe. | Exact final records persist; no default-initialization assumption. |
| `ComponentExternallySynchronizedWorkers` | Ordinary tier; protect all operations sharing a position with an external lock; join before flush/free. | Reference state and persisted records match serialized operations. |

### Concurrent record operations

| Scenario | Setup and action | Required assertions |
| --- | --- | --- |
| `ComponentConcurrentSequentialParity` | Run deterministic serial operation script in both creation modes with distinct sandboxes. | Identical logical records and public readback; no file-byte equality requirement. |
| `ComponentConcurrentDisjointPositions` | Workers operate on disjoint subsets; exercise each method family before mixing them. | Independent reference models hold; no cross-position interference. |
| `ComponentConcurrentSamePositionReadersAndWriters` | Overlap supported solving setters/getters on a seeded position; include loaded getters on that solving tier when present; join before final readback. | Successful setter calls; individually permitted field results, atomic per-call behavior, and clean race-sanitizer runs. Two getter calls are not one pair snapshot. |
| `ComponentConcurrentMaximizeSafety` | Pure thread-safe comparator; seeded position; maximize and read concurrently. | Legal individual results and no races; exact maximum assertions are covered separately. |
| `ComponentConcurrentUndecidedCounterSafety` | Seed n; overlap decrement, clear, and count reads without reseeding. | No races; returns/counts in 0..n; exact multiset/conservation assertions in dedicated cases. |
| `ComponentConcurrentPairAndIndependentFieldContention` | Separate cases for combined-pair writers and independent value-only/remoteness-only writers on a seeded position; join. | Final pair is one complete combined write; independent setters preserve the other field. Validate a legal serial order respecting completed calls, without fixing overlapping-thread order. |
| `ComponentConcurrentMaximizeSameCandidate` | Initial (kLose,0); comparator orders by value then remoteness; two threads offer (kWin,7). | Exactly one true and one false; final (kWin,7). Both calls are atomic. |
| `ComponentConcurrentMaximizeDistinctCandidates` | Initial (kLose,0); threads offer (kTie,3) and (kWin,7) under the same comparator. | Final (kWin,7). Returns may be (true,true) if Tie wins first or (false,true) if Win wins first; do not require a fixed number of true results. |
| `ComponentConcurrentDecrementExactReturns` | Seed count `n`; more than `n` concurrent decrements. | positive returns are exactly 1..n once each, remaining returns zero; final zero. |
| `ComponentConcurrentClearExactReturns` | Seed positive count; multiple concurrent clears. | one original count, remaining returns zero; final zero. |
| `ComponentConcurrentDecrementClearConservation` | Seed `n`; mix decrements and clears without reseeding; ensure final draining operation. | positive decrement count plus sum of clear returns equals n; final zero; no negative count. |
| `ComponentConcurrentDecidedCountersAndReadOnlyRecords` | Separate fixed-decided counter contention and fully seeded read-only multireader scenarios. | Counter calls zero and records unchanged; read-only answers exact. |
| `ComponentConcurrentFinalPersistenceAndStatus` | Join all workers; inspect state; flush, free, probe/status. | Exact quiescent state persists; solved tier status. |
| `ComponentConcurrentDeterministicStress` | Fixed reported seeds; bounded workers and disjoint/contended positions; atomic-operation model checks; lifecycle/I/O after joins. | Reference results and permitted histories, no races, exact quiescent persistence. No timing-dependent thread-order expectations. |

### Checkpoint workflows

These workflows use regular solving tiers. Concurrent solving and checkpoint
operations are exercised in separate contexts.

| Scenario | Setup and action | Required assertions |
| --- | --- | --- |
| `ComponentCheckpointOrdinaryRoundTrip` | Create ordinary solving tier; write explicit records/status; save; free; check exists; load. | Exact records/status restored; solving and loaded getters agree; `IsTierLoaded` true. |
| `ComponentCheckpointRestoresSavedSnapshot` | Save A; mutate records/status without saving; free; restore. | Saved A restored, including decided records and undecided counters. |
| `ComponentCheckpointLatestSaveWins` | Save A; change records/status; save B; free and restore. | Complete B restored. |
| `ComponentCheckpointSurvivesReinitialization` | Save; finalize; initialize same game/variant/path; check exists; restore. | Checkpoint survives lifecycle restart; exact progress restored. |
| `ComponentCheckpointIndependentTiers` | Save checkpoints for multiple tiers; restore/remove them in varied order. | Each tier/status stays distinct; removing one preserves others. |
| `ComponentCheckpointResumeAndFinish` | Restore mixed decided/counter state; continue setters/maximize/counters; flush; free; probe. | Explicit reference model matches final permanent records; tier reports solved. |
| `ComponentCheckpointRemoveThenLoad` | Save; remove; check exists; attempt load into sentinel status buffer. | Removal succeeds; exists false; load nonzero and changes neither status nor residency. |
| `ComponentCheckpointDoesNotMarkSolved` | Separate fresh and already-solved archive cases; save checkpoint with writes paused; compare tier status before/after. | Checkpoint leaves tier archive status unchanged; checkpoint-only tier reports missing. |

### Loaded-tier and probe workflows

| Scenario | Setup and action | Required assertions |
| --- | --- | --- |
| `ComponentLoadReadUnload` | Load solved tier; read; unload; query residency and loaded getters. | Correct resident reads; after unload, residency false and getters return documented errors. |
| `ComponentLoadedTiersAndSolvingTier` | Load solved tiers; create a distinct solving tier; interleave reads/mutations; free solving tier and unload others. | No tier aliasing; each release uses its proper method. Rejection errors are isolated in unit tests instead of continuing the component after failure. |
| `ComponentUnloadPreservesOtherTiers` | Load distinct tiers; unload one; continue queries on others. | Remaining tiers stay resident with exact records. |
| `ComponentFlushLoadAndProbeAgree` | Solve; flush; free; load; read; unload; probe. | Solving, loaded, and permanent views match the same explicit reference data. |
| `ComponentProbeReadOrders` | Probe initialized archive in forward, reverse, repeated, and fixed-seed shuffled order, including all positions. | Correct results independently of request order/internal buffer geometry. |
| `ComponentProbeTierAndGetterSwitching` | One probe alternates tiers and value/remoteness methods at same/different positions. | No stale tier/cache state; each read matches fixture. |
| `ComponentIndependentProbes` | Initialize multiple probes; interleave their reads sequentially; destroy one; continue using others. | Each works independently; destroying one preserves others. No thread-safety requirement is inferred. |
| `ComponentProbeAfterMemoryRelease` | Write/flush archive; free solving tier; initialize/read/destroy probes in bounded cycles. | Permanent reads work independently of residency; generic leak diagnostics may supplement. |

### Segmentation and consolidation

| Scenario | Setup and action | Required assertions |
| --- | --- | --- |
| `ComponentSegmentationMinimalLifecycle` | Query capacity; create one buffer; explicitly fill; flush; consolidate; free; probe final archive. | Logical records survive; resulting normal tier archive reports `kDbTierStatusSolved`. |
| `ComponentSegmentationReusesOneBufferForManySegments` | Reuse one buffer for n consecutive segment indices 0..n-1; consolidate before freeing. | Each saved segment keeps its explicit data; global records follow index × segment_size + offset. |
| `ComponentSegmentationKeepsMultipleBuffersIndependent` | Alternate updates, reads, and flushes across feasible buffers with distinct deterministic patterns. | No cross-buffer or cross-offset contamination; every saved segment has its selected source data. |
| `ComponentSegmentationLoadModifyAndResave` | Flush segment A; overwrite memory; load A; modify selected records; flush to another segment index; reload both. | Load restores A; subsequent writes work; independently saved indices remain distinguishable. |
| `ComponentSegmentationOverwriteThenConsolidate` | Save two complete segments; replace the first at the same index with new records; consolidate with segment count still two, then free/probe. | Final archive contains the replacement first segment and original second segment, in index order. |
| `ComponentSegmentationLoadsIntoDifferentBuffers` | Save from buffer zero; load the same segment into another active buffer; alternate loads of different saved segments. | Segment identity is independent of original and destination buffer indices; correct records after each load. |
| `ComponentSegmentationConsolidatesOutOfOrderFlushes` | Flush the complete index set 0..n-1 in shuffled order, without repeated indices; consolidate before freeing. | Same ordered logical tier regardless of flush order. |
| `ComponentSegmentationHandlesPartialFinalSegment` | Example segment size 4 and tier size 10: initialize only logical records 0..9; never read or write the final two padding offsets; flush/consolidate. | Exactly the ten logical records survive; padding is ignored without being initialized or accessed through public getters/setter. |
| `ComponentSegmentationMatchesNormalTierArchive` | Write the same reference tier normally and through consecutive segments in separate sandboxes; consolidate before freeing; load/probe each supported view. | Both have solved status and identical logical records, including allowed undecided positions. No byte comparison; normal flush uses nullptr aux. |
| `ComponentSegmentationPersistsAcrossDatabaseSessions` | Consolidate successfully; finalize; reinitialize the same sandbox; read using probes and loaded-tier getters. | Complete logical data persists and tier remains solved. |
| `ComponentSegmentationTracksTierStatus` | Start with no archive; create/fill/flush segments; consolidate; query tier status before any output and after successful consolidation. | Initially missing; solved after successful consolidation. |
| `ComponentSegmentationIntermediateStatus` | For a fresh nonempty tier, flush first an incomplete set of logical segments, then the complete set; query TierStatus before Consolidate. | kDbTierStatusMissing after both segment-flush stages; kDbTierStatusSolved only after successful Consolidate. No inspection of segment files. |
| `ComponentSegmentationStreamingStress` | Many consecutive saved segments, few buffers, fixed-seed nonrepeating flush order, reloads, and final consolidation before free. | Every logical record matches the independent model; no tail access or time threshold. |
| `ComponentSegmentationFreeBuffersPreservesOrdinarySolvingTier` | Create/populate an ordinary solving tier without an active segment context; call FreeBuffers repeatedly, inspect records and supported residency, flush, free, and read the archive. | FreeBuffers is a successful no-op; solving identity, records, and final persistence remain correct. |
| `ComponentSegmentationFreeBuffersPreservesConcurrentSolvingTier` | Repeat the no-segment-context workflow with a concurrent solving tier; keep lifecycle and I/O calls outside record phases. | FreeBuffers preserves the concurrent solving tier and its final reference archive. |

### Combined workflows

| Scenario | Setup and action | Required assertions |
| --- | --- | --- |
| `ComponentRecoveredSolveFeedsAllReaders` | Load child tiers; create a regular parent solving tier; mutate records/counters; checkpoint/free/restore; continue serially; flush/free and compare supported reader paths. | Exact restored status/records, serial continuation, correct residency, and final reference archive/solved status. |
| `ComponentBoundedMixedStress` | Bounded fixed-seed scripts over supported method subsets; atomic concurrent record phases followed by joins and release; regular-solving checkpoint workflows in separate contexts; lifecycle/I/O. | Independent model after quiescent phases; valid atomic histories during contention; no cross-tier/path contamination or unsupported retries. |

## Assertion strategy and rationale

Reference records are constructed independently of backend storage and readback.
Serial scripts update a scalar model after each operation. Persistent records
are checked against that model through each supported reader; agreement between
two backend readers alone is insufficient.

Concurrent workers synchronize their starts and record invocation/return event
order. Tiny histories contain at most eight calls, permitting an exhaustive
search for a legal serial ordering consistent with calls that completed before
others began. The search uses pure per-record transition rules and checks every
return value and the exact quiescent final state. Fixed seeds, bounded workers,
and disjoint/contended positions make failures diagnosable without assigning an
expected order to overlapping threads. Larger serial batches use their known
order rather than the bounded concurrent-history checker. Workers join before
lifecycle, checkpoint, flush, consolidation, or permanent-reader operations.

| Decision | Rationale and representative assertion |
| --- | --- |
| Exhaustive finite domains | Five old values times five new values gives 25 ordered transitions. Testing unchanged-value transitions catches preservation errors alongside replacement errors. Exhausting V times R checks every required legal record pair. |
| Atomic maximize | Starting at (kLose,0), two calls offering (kWin,7) must produce one true and one false. Offers (kTie,3) and (kWin,7) always finish at Win, but permit either two true returns or only the Win call returning true. Both overlapping orders are legal. |
| Atomic counters | From count one, two decrements return one and zero; from count three, two clears return three and zero. In mixed decrement/clear histories, the number of positive decrement returns plus the sum of clear returns equals the original count after draining. |
| Independent field observations | Each getter is atomic in concurrent mode, but a value read and a later remoteness read may describe different writes. Full-pair equality is asserted after joining workers or under external synchronization. |
| Segment identity | With capacity four, segment two holds global positions eight through eleven. For tier size ten, its final two offsets are inaccessible tail. Reflushing index one replaces index one; it does not add another segment. |
| Logical archive equivalence | Normal flush and segmented consolidation must yield the same logical records and solved status. Public probes and supported loaded getters establish this equivalence without requiring byte-identical files. |
| Portable memory checks | A peak-bound promise includes transient allocation, but the interface exposes no portable byte-accounting hook. Generic checks validate supported requests and SIZE_MAX errors; backend-specific tests establish numerical bound accuracy. |
| Finalization assertions | Finalize has no return value. Tests check successful completion and supported observable postconditions, supplemented by resource diagnostics. |
| Sanitizer diagnostics | ASan, leak checks, and TSan supplement behavioral assertions when available and instrument the linked implementation as well. They do not define private allocation counts, a shared-probe concurrency contract, or timing thresholds. |

## Coverage boundaries

| Coverage | Disposition |
| --- | --- |
| Private allocation/I/O fault hooks, recognized corrupt-file construction, exact byte accounting and implementation-specific aux configurations. | Concrete implementation tests; no adapters or new runtime testing hooks added here. Generic error tests use only deterministic public checked-input/missing-file conditions. |
| OOM codes, TierStatusCorrupted/CheckError, GameStatusCheckError, and unexpected getter failures. | Return contracts remain in the header, but there is no portable way to force these conditions for every implementation. No claim of complete branch coverage. |
| Cleanup after a failed Init. | Finalize is defined in this state, but the suite has no portable input that forces every implementation's Init to fail. Deterministic failure injection belongs in implementation-specific tests. |
| Unknown initial fields, invalid/oversized counters, invalid record inputs, reinitialization of active probes, corrupted probe reads, tail access, and lifecycle overlap. | No default-content assertions or intentional undefined-behavior/death tests. Unwritten contents remain unspecified; invalid accesses explicitly declared UB stay excluded. |
| Retry, rollback, continued solving after failure, power-loss durability, consolidation after FreeBuffers/reinit. | Unsupported workflows excluded. Defined successful repeat operations remain tested. |
| Stronger corruption checks and exact GetPath spelling/lifetime/ownership policy. | Outside the generic contract; directory identity is checked without stronger spelling, lifetime, or ownership guarantees. |
