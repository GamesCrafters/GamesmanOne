# Database interface test plan

This is a design plan, not an implemented test suite. The current
[`database.h`](../src/core/types/database/database.h) incorporates the settled
review decisions. Only generic interface behavior belongs here; concrete
implementations, file formats, allocation formulas, and private fault hooks do
not define expectations. Original substantive review comments are preserved in
the review record below.

All eventual GoogleTest cases use `DatabaseTest`. PascalCase names prefixed
with `Unit` or `Component` allow filtering without changing the suite name.
Follow [`xstring_test.cpp`](../tests/unit/libs/string/xstring_test.cpp) for
comments/assertion style. Rows are scenario templates: grouped methods expand
into independent cases, and finite value domains into parameters/subcases.

## Reading the tables

| Basis | Meaning |
| --- | --- |
| H | Required by the revised header and its shared types/constants. |
| U | Exercises a solver workflow from the original request. |
| Qn | Depends on the corresponding unresolved question below; do not implement an assumed answer. |

Q1 through Q12 are resolved. The latest Q12 correction supersedes the earlier
proposal to migrate sentinel values; all existing constants remain unchanged.
A generic nonzero-error contract does not require every backend to emit every
error enumerator. Tests use exact codes only for explicit cases.

## Open questions for review

No open questions remain from this review. The table is retained for any
further review opinions. All cases below remain design scenarios; no test or
runtime implementation is authorized by resolving the questions.

| ID | Question | Proposed specification / test consequence | Review opinions |
| --- | --- | --- | --- |
| — | None currently. | The latest decisions are reflected in the specifications and plan only. | |

## Decisions from the latest review

Original opinions are preserved verbatim. Q1 through Q12 are closed decisions;
the earlier Q7 migration proposal is explicitly superseded by Q12. Only design
and specification comments are updated; no implementation follows this review.

| ID | Confirmed decision and consequence | Review opinions (preserved) |
| --- | --- | --- |
| Q1 | FlushSolvingTier accepts null aux for default behavior. Generic flush and disk-fixture cases use nullptr without an aux adapter. | Yes, we should explicitly allow `NULL` to be passed for default behavior. |
| Q2 | CheckpointSave and CheckpointLoad return kIllegalArgumentError for null status with positive size. The latest checkpoint correction permits nonnull+zero for Save; Load still rejects nonnull+zero. | Yes, `kIllegalArgumentError` should be returned in this case. |
| Q3 | Superseded for initialization by the latest correction: ProbeInit(nullptr) returns kIllegalArgumentError. ProbeDestroy(nullptr) remains a successful no-op returning kSuccess. Retain independent cases. | Yes, and they should be no-op. |
| Q4 | CreateBuffers rejects num_buffers above MaxNumBuffers with kIllegalArgumentError, including maximum zero. The planned over-capacity test avoids INT_MAX + 1 overflow. | Agreed. |
| Q5 | Successful segment Flush overwrites that index. Load and consolidation use the newest contents; replacement does not increase the segment count. | Yes, it will overwrite the existing segment. |
| Q6 | For tier_size zero, every nonnegative segment count and segment capacity is a valid shape. Nonempty-tier consistency and negative-argument errors are unchanged. Q11 confirms that empty-tier consolidation needs no preceding segment flushes or physical segment-file assumptions. | All combinations are allowed. |
| Q7 | Superseded by the correction in Q12: keep the existing named constants and their values. Valid Tier, Position, and remoteness data are nonnegative; no sentinel migration or legacy-analysis conversion is planned. | Help me replace those values with INT64_MIN and reference those definitions in constants.h in the database specs and our tests. |
| Q8 | Comparison callbacks have stable ordering, are safe for concurrent calls, and must not reenter Database. Equivalent-ranked distinct pairs remain legal; they do not replace the stored pair. | The requirements proposed here are all valid. Please update the specs accordingly. |
| Q9 | Valid probe calls return kErrorValue/kErrorRemoteness on read errors as well as not-found. Invalid/corrupt input preconditions remain UB; no backend-specific error injection is introduced. | ProbeValue() and ProbeRemoteness() should return `kErrorValue` and `kErrorRemoteness` respectively on error. |
| Q10 | An initialized probe must be manually destroyed even after read failure, before Finalize. Cleanup is allowed; retries and continued probing remain unsupported. | Require manually destroying the probe even after failed reads. |
| Q11 | Empty consolidation produces a normal zero-position archive and solved TierStatus without requiring prior segment flushes. Verify public results only; do not inspect segment-file materialization, names, sizes, or layout. An empty tier has no legal position to probe. | Zero-length segments don't necessarily exist on disk, but an empty tier must still be flushed to an archive containing zero positions. You should not be checking the segment files either; the internal handling of the database is a blackbox. The only things that we can test from the outside are 1. the value-remoteness entries at each position accessed through the probing interface, and 2. the tier status which only becomes solved after consolidation. |
| Q12 | Corrected domain: all negative Tier identifiers, Position indices, and remoteness data are invalid. Existing negative error/sentinel results remain unchanged. This supersedes the INT64_MIN migration, so the proposed legacy-analysis conversion is unnecessary. | Correction to the previous comment: we cannot modify the identifiers in constants.h. Because of this, the new plan is to make all negative Tier, Position, and Remoteness values from the entire system. This is currently safe for all games, solvers, and database implementations. Update the specs accordingly. |

## Latest checkpoint contract corrections

These decisions supersede the earlier checkpoint review outcomes below; the
original review opinions remain preserved.

| Concern | Current contract | Test consequence |
| --- | --- | --- |
| Non-null status with zero size in CheckpointSave | Permitted; either success or an error return is allowed. CheckpointLoad's nonnull+zero checked error is unchanged. | Retain the regular-tier save case as a normal-return check without a prescribed API return code. |
| CheckpointSave without a solving tier | Either success or an error return is allowed. The call must return normally. | Accept any normal API return; process crashes and sanitizer failures still fail the test. |
| Consistent tier sizes | A stored tier's creation, checkpoint restoration, and archive loading use the same position count. A mismatched CheckpointLoad size is undefined behavior. | Remove ordinary/concurrent size-mismatch cases and always load checkpoints with the original created tier size. |
| Checkpoint solving mode | The checkpoint API supports only regular, externally synchronized solving tiers. Using it with a concurrent solving tier is undefined behavior; CheckpointLoad always creates a regular tier. | Remove concurrent checkpoint unit cases and the concurrent checkpoint component scenario. Require regular creation for checkpoint fixtures. |

## Valid data and negative error sentinels

The corrected rule applies to logical game data. Negative values remain
available for documented errors and invalid-result sentinels; their existing
names and definitions in [constants.h](../src/core/constants.h) and
[constants.c](../src/core/constants.c) are unchanged. The sentinel migration
and associated saved-analysis compatibility work are withdrawn.

| Type / concern | Settled contract |
| --- | --- |
| Tier | Valid identifiers are nonnegative. Every negative tier is invalid, including kIllegalTier and INT64_MIN. |
| Position | Valid positions are nonnegative and within the relevant tier/game range; negative values, including kIllegalPosition, are invalid data. |
| Remoteness | Valid stored remotenesses are nonnegative and within the supported range. kErrorRemoteness remains a documented negative error return. |
| Shared declarations | Tier and Position domains are documented in [base.h](../src/core/types/base.h); the shared remoteness documentation is in constants.h. No typedef, signature, constant value, or runtime validation code changes here. |
| Checked errors | Both memory estimators return SIZE_MAX for every negative tier or size. Other negative-input behavior follows existing per-method checked-error or undefined-behavior contracts; no new universal rejection code is invented. |
| Planned coverage | Use named sentinels where referring to invalid identifiers/error results. Estimator rejection cases also sample other negative tiers; keep literal INT64_MIN for numeric boundary inputs. Invalid record accesses remain excluded as UB. |
| Scope | This domain rule does not redefine generic signed-integer container keys or the Move type. The former migration/analysis-conversion implementation tasks are no longer part of the plan. |

## Common fixture and coverage rules

| Concern | Revised approach |
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
| Checked versus undefined inputs | Test only documented checked errors. Exclude invalid solving-record inputs, repeated active creation/loading, mode switches while active, checkpoints with concurrent solving tiers, checkpoint tier-size mismatches, invalid existing-tier probe positions, corrupt probe reads, and invalid segment getter/setter indices. |
| Memory estimates | Treat estimates as peak bounds, including ordinary creation for TierMemUsage. Check valid requests and SIZE_MAX errors only; do not measure RSS, assume bytes per record, inject OOM, or assert backend allocation counts. Actual numerical bound accuracy remains implementation-specific coverage. |
| General diagnostics | Generic leak/race sanitizers can supplement execution without private hooks; instrument the linked implementation too. Leak checks do not become method-specific byte-accounting assertions. |
| Concurrent phases | Keep unit cases single-threaded. Components use bounded workers, barriers and fixed seeds. Each confirmed record call is atomic; no fixed order for overlapping calls. Comparison callbacks must be stable, thread-safe, and must not reenter Database. Equal ranking of distinct pairs is permitted. Join before lifecycle or I/O operations. Checkpoints require a regular solving context; joining concurrent workers does not make their solving tier checkpoint-compatible. |
| Concurrent readers | Each loaded getter on the concurrent solving tier has the corresponding solving getter's guarantee. Two separate getters do not form an atomic pair snapshot; compare full pairs after joining or under an external lock. Probes access disk independently; no shared-probe or archive-replacement race guarantee is inferred. |
| Segments | Indices are the consecutive set 0..n-1; buffer count and saved segment count are independent. Position = seg_idx × segment_size + offset. Successful Flush replaces its saved index. Consolidate before FreeBuffers in the same initialized lifetime. Empty consolidation requires no prior segment flushes; verify solved TierStatus without inspecting physical storage. Never access offsets outside the logical tier. |
| Zero segmentation capacity | A reported maximum of zero is valid. Run zero-count/size context cases; skip positive-buffer cases with an explicit capacity reason. No backend-specific expected maximum is used. |
| Opaque details | No archive/segment-file existence, names, sizes, layout, bytes, compression, allocation formula, DbProbe fields/cache geometry, callback count, or comparator order is inspected. Persisted values/remotenesses use public probes; completion uses TierStatus. Empty tiers have no legal probe positions. GetPath comparisons concern the designated directory, not spelling or pointer identity. |
| Fault and corruption branches | Backend-hook-based allocation/I/O failures, known-corrupt archives, status-check errors, and exact memory accounting are excluded. Their return contracts remain documented; this generic suite does not claim those branches were exercised. |

## Category map

| Group | Categories |
| --- | --- |
| Unit | Descriptor/lifecycle/path/status; solving tier creation/release; all nine record methods; estimates; checkpoints; loaded tiers; probes; all segmentation methods. |
| Component | Lifecycle/persistence; setter subsets/counters; atomic concurrent solving; serial checkpoint recovery; resident/disk readers; segmented consolidation/equivalence; bounded combined workflows. |

## Group 1: unit tests

### Descriptor, lifecycle, path, and status

Use a fresh sandbox and successful `Init` where required. Pre-initialization
path/finalization cases opt out of automatic initialization. Never initialize
a second instance while the first lifetime is active. Init uses null aux.
Additional observations and all optional setup/cleanup operations are guarded.
No instance-specific aux value is assumed.

| Planned test name | Setup and action; minimal extra members for units | Required assertions | Basis |
| --- | --- | --- | --- |
| `UnitDescriptorHasValidInternalName` | Inspect the real descriptor using a bounded terminator search before treating its array as a string. | NUL termination within `kDbNameLengthMax + 1`; length 1 through `kDbNameLengthMax`; no whitespace. Use a defined whitespace classification with unsigned characters. | H |
| `UnitDescriptorHasValidFormalName` | Bounded inspection of `formal_name`. | NUL termination; length 1 through `kDbFormalNameLengthMax`. Spaces are allowed; do not invent trimming requirements. | H |
| `UnitDescriptorProvidesRequiredLifecycleMembers` | Inspect the real descriptor. | Only `Init` and `Finalize` must be non-null. Other pointers may be null; check every target and dependency before using it. | H |
| `UnitInitSucceedsWithTierNamer` | Valid game/variant/private sandbox; deterministic valid callback; `aux = nullptr`. | Returns `kSuccess`. If `GetPath` exists, its result identifies the supplied sandbox. No callback timing/count assertion. | H |
| `UnitInitAcceptsNullTierNamer` | Same valid fixture with null tier namer and null aux. | Returns `kSuccess`; fallback supported without requiring GetPath or storage methods. | H |
| `UnitGetPathBeforeInitialization` | Fresh isolated backend process before any successful `Init`. | Returns `nullptr`. | H |
| `UnitGetPathAfterInitialization` | Initialize with the existing private sandbox; call optional `GetPath`. | Non-null result identifies the same directory. No exact spelling, pointer identity, or normalization requirement. | H |
| `UnitGetPathAfterFinalization` | Successful `Init`, then `Finalize`. | Returns `nullptr`. Never dereference a pointer saved before finalization. | H |
| `UnitFinalizeWithOwnedTiers` | Separately create an ordinary/concurrent solving tier or load a tier through supported public setup; destroy probes first; call Finalize directly. | Finalize completes successfully; optional GetPath returns null. A generic leak-sanitizer run may supplement this; no backend allocation-count assertion. | H |
| `UnitFinalizeWithOwnedBuffers` | Create segment buffers through supported public methods; finalize without a live probe. | Successful completion; optional GetPath returns null; generic leak checks may supplement the run. | H |
| `UnitFlushSolvingTierWritesCurrentContents` | Explicitly populate all records; flush; free memory; observe with a fresh probe. Pass nullptr as flush aux. | Successful flush persists each reference pair; no archive-format assertions. | H |
| `UnitTierStatusReportsMissing` | Fresh sandbox; queried tier has no archive. | Exactly `kDbTierStatusMissing`. | H |
| `UnitTierStatusReportsSolved` | Valid normally generated archive fixture. | Exactly `kDbTierStatusSolved`. | H |
| `UnitTierStatusSelectsRequestedTier` | One normally saved solved tier and one absent tier, with distinct IDs. | Each query returns the requested tier status; no corrupt-file fixture. | H |
| `UnitGameStatusReportsIncomplete` | Fresh game/variant/sandbox with no solved declaration. | Exactly `kDbGameStatusIncomplete`. | H |
| `UnitGameStatusReportsSolved` | Fixture representing the game's solved declaration; minimal setup may use checked `SetGameSolved`. | Exactly `kDbGameStatusSolved`. | H |
| `UnitSetGameSolvedRecordsDeclaration` | Initialized game; invoke `SetGameSolved`; observer: `GameStatus`. | `kSuccess`; status is solved. No requirement to derive this from a count of tiers, which the interface never receives. | H |
| `UnitFinalizeBeforeInitialization` | Fresh isolated lifetime; call Finalize before Init. | Successful no-op; no return assertion because Finalize is void. | H |
| `UnitFinalizeRepeated` | Initialize; finalize successfully; finalize again. | Second call completes successfully; optional GetPath remains null. | H |
| `UnitFlushSolvingTierRepeated` | Create and populate a tier; flush twice without changing records; free and probe. Pass nullptr as flush aux. | Both successful calls preserve the same logical records; no byte-identity requirement. | H |
| `UnitSetGameSolvedRepeated` | Declare solved twice using the optional method; observe with GameStatus when supported. | Both normal calls succeed; status stays solved. | H |

### Solving tier allocation and release

| Planned test name | Setup and action; minimal extra members for units | Required assertions | Basis |
| --- | --- | --- | --- |
| `UnitCreateSolvingTierSupportedSizes` | Initialized DB; create tiers of size 1, 2, and a small non-power-of-two size. Guard optional residency observation. | `kSuccess`; if IsTierLoaded exists, requested tier is loaded. No assertion about default records. | H |
| `UnitCreateConcurrentSolvingTierSupportedSizes` | Same independent cases with concurrent creation. Guard optional residency observation. | `kSuccess`; if IsTierLoaded exists, requested tier is loaded. | H |
| `UnitCreateTierProvidesRequestedExtent` | For both modes, seed first/interior/last records with `SetValueRemoteness`; read with getters. | Every requested record is accessible and distinct; exact values survive. | H |
| `UnitCreateTierZeroSize` | In separate lifetimes, call each supported creation mode with size zero; never access a position. | Normal `kSuccess`; release the zero-sized solving tier with the proper free method. | H |
| `UnitCreateTierNegativeSize` | Separate cases for each creation mode and sizes -1 and INT64_MIN; no active solving context. | Exactly `kIllegalArgumentError`; finalize afterward. | H |
| `UnitFreeSolvingTierWithoutTier` | Initialized DB with no solving tier; call free. | `kSuccess`; no-op. | H |
| `UnitFreeSolvingTierEachCreationMode` | Create ordinary or concurrent tier, then free; guard optional IsTierLoaded observation. | `kSuccess`; if the residency observer exists, the tier is no longer loaded. | H |
| `UnitFreeSolvingTierRepeated` | Create a supported solving tier; free successfully; call free again. | Both calls return `kSuccess`; optional residency observer is false. | H |

### Record methods

| Planned test name | Setup and action; minimal extra members for units | Required assertions | Basis |
| --- | --- | --- | --- |
| `UnitSetValueEveryLegalValue` | Seed with combined setter; set each legal value while retaining each legal remoteness in R; observe value/remoteness getters. | `kSuccess`; exact new value; every seeded remoteness preserved. | H |
| `UnitSetValueEveryValueTransition` | All `5 × 5 = 25` ordered `(old_value, new_value)` pairs from V, including unchanged values; seed remoteness first. | Exact new value; remoteness and neighbors unchanged. | H |
| `UnitSetRemotenessEveryLegalRemoteness` | Seed each legal value; set every `r` in global remoteness range; observe both getters. | `kSuccess`; exact remoteness; value unchanged. The global minimum range is confirmed. | H |
| `UnitSetRemotenessRepeatedAndReplacementWrites` | Same and differing old/new remotenesses, including zero and maximum. | Successful idempotent/replacement writes; no neighbor changes. | H |
| `UnitSetValueRemotenessEveryLegalPair` | Set each legal value/remoteness combination; observe both getters. | `kSuccess`; both fields exact; no neighbor changes. | H |
| `UnitSetValueRemotenessRepeatedPair` | Write identical pair twice. | Both writes succeed; pair retained. | H |
| `UnitGetValueEveryLegalResult` | Seed all `V × R` pairs with combined setter; include first/interior/last positions. | Exact result; reads do not change records. | H |
| `UnitGetRemotenessEveryLegalResult` | Seed all `V × R` pairs using combined setter, including undecided values. | Exact remoteness; reads do not change records. The global minimum range is confirmed. | H |
| `UnitMaximizeValueRemotenessGreaterCandidate` | Seed old pair; comparator ranks candidate greater; observe both getters. | `true`; exact candidate replaces old; neighbors unchanged. | H |
| `UnitMaximizeValueRemotenessSmallerCandidate` | Candidate ranks lower than seeded old pair. | `false`; exact original pair retained. | H |
| `UnitMaximizeValueRemotenessEqualCandidate` | Candidate equals seeded pair. | `false`; original pair retained. | H |
| `UnitMaximizeValueRemotenessComparesValues` | Equal remotenesses; vary old/new values across legal-value pairs. | Replacement and boolean follow supplied comparison. | H |
| `UnitMaximizeValueRemotenessComparesRemotenesses` | Equal values; lesser/equal/greater remotenesses. | Exact comparator-selected pair and boolean. | H |
| `UnitMaximizeValueRemotenessHonorsAlternateOrdering` | Comparators reverse enum ordering or prefer smaller remoteness; include conflicting field preferences. | Supplied comparator determines result. | H |
| `UnitMaximizeValueRemotenessUsesComparisonSign` | Valid comparators return negative/positive magnitudes other than one, zero, and `INT_MIN`/`INT_MAX` directly without arithmetic overflow. | Sign determines result; no assertion about callback call count or invocation argument order. | H |
| `UnitMaximizeValueRemotenessDomainBoundaries` | Candidate pairs exhaust `V × R`; old pairs cover each value and representative remotenesses including both boundaries. | Exact selected pair and correct boolean against the independent comparator model. | H |
| `UnitMaximizeValueRemotenessEquivalentRanks` | Comparator gives rank zero to distinct initialized pairs; compare before/after with getters. | `false`; original value and remoteness remain unchanged. | H |
| `UnitDecrementNumUndecidedChildrenPositive` | Seed `(kUndecided,n)` with combined setter for each positive `n` in `R`; observe getters. | Returns old `n`; counter becomes `n-1`; value unchanged. This covers the shared remoteness domain, not a claimed maximum counter capacity. | H |
| `UnitDecrementNumUndecidedChildrenZero` | Seed `(kUndecided,0)`. | Returns zero; record unchanged; no underflow. | H |
| `UnitDecrementNumUndecidedChildrenDecidedValues` | Each decided value with every remoteness in confirmed `R`. | Returns zero; value and remoteness unchanged. | H |
| `UnitClearNumUndecidedChildrenPositive` | Seed `(kUndecided,n)` for each positive `n` in `R`. | Returns old `n`; counter becomes zero; value unchanged. | H |
| `UnitClearNumUndecidedChildrenZero` | Seed `(kUndecided,0)`. | Returns zero; record unchanged. | H |
| `UnitClearNumUndecidedChildrenDecidedValues` | Each decided value with every remoteness in confirmed `R`. | Returns zero; both fields unchanged. | H |
| `UnitGetNumUndecidedChildrenUndecidedCounts` | Seed `(kUndecided,n)` for every `n` in `R`, including zero. | Exact count returned; record unchanged. | H |
| `UnitGetNumUndecidedChildrenDecidedValues` | Each decided value with every remoteness in confirmed `R`. | Returns zero; underlying remoteness remains unchanged. | H |
| `UnitCounterMethodsPositionIsolation` | Target each counter method at first/interior/last positions with distinct neighbor data. | Only intended record changes; getter changes none. | H |

### Memory estimates

| Planned test name | Setup and action; minimal extra members for units | Required assertions | Basis |
| --- | --- | --- | --- |
| `UnitTierMemUsageValidRequests` | For ordinary valid tier IDs and small nonnegative sizes including zero, call optional TierMemUsage. | A usable estimate rather than SIZE_MAX under normal conditions. Interpret it as a peak bound; no formula, monotonicity, RSS, or allocation-accounting assertions. | H |
| `UnitConcurrentTierMemUsageValidRequests` | Same small nonnegative request partitions for optional ConcurrentTierMemUsage. | A usable peak estimate rather than SIZE_MAX under normal conditions; no numeric byte formula. | H |
| `UnitMemoryEstimateRejectedInputs` | For each estimator, separately test negative sizes and negative tiers kIllegalTier, -2, and INT64_MIN, with other inputs valid. The named sentinel and representative negative values are invalid; no sentinel value changes. | Exactly `SIZE_MAX` for every negative tier/size case. Do not assume every large representable positive size overflows. | H |
| `UnitTierMemUsageBeforeOrdinaryCreation` | Query estimate, then create/free a small ordinary tier using supported methods. | Both APIs usable in this sequence; the estimate contract covers ordinary creation as well as loading. This smoke test does not independently measure the numerical bound. | H |

### Checkpoints

All solving-tier fixtures in this category use regular creation. The
checkpoint API does not support concurrent solving tiers, and restoration
always creates a regular tier.

| Planned test name | Setup and action; minimal extra members for units | Required assertions | Basis |
| --- | --- | --- | --- |
| `UnitCheckpointExistsAbsent` | Initialize fresh storage; query a tier with no checkpoint. | `false`. | H |
| `UnitCheckpointExistsPresent` | Prepare one valid checkpoint fixture; query its tier. | `true`; query of a different absent tier returns `false`. | H |
| `UnitCheckpointSaveRecordsAndStatus` | Create/populate solving tier; save a positive-sized binary status; free; restore via checked CheckpointLoad and getters. | `kSuccess`; exact checkpoint records and status; const input bytes unchanged. | H |
| `UnitCheckpointSaveOverwritesExisting` | Prepare old checkpoint; save changed records/status, including short-to-long and long-to-short status payloads. Free the solving tier, then reload with the new matching size/format. | `kSuccess`; restored logical records/status match the newest save. No assertion about old physical trailing file bytes. | H |
| `UnitCheckpointSaveBinaryStatus` | Save blobs containing embedded NUL, all-zero, all-`0xff`, bytes `0..255`, and lengths not divisible by machine word size; minimally reload. | Exact bytes and length round trip; no string truncation. | H |
| `UnitCheckpointLoadRestoresFixture` | Valid checkpoint fixture; no current solving tier; matching tier size and status format/size. Use solving getters as observers. | `kSuccess`; exact records/status; solving tier made available. | H |
| `UnitCheckpointLoadMissingDoesNothing` | No checkpoint; sentinel-fill output status; capture baseline residency. Call load. | Nonzero; status and residency unchanged. | H |
| `UnitCheckpointLoadOutputBounds` | Valid fixture; output status buffer has prefix/suffix guards beyond required payload. | Exact payload restored; guard bytes unchanged. | H |
| `UnitCheckpointRemoveExisting` | Valid checkpoint fixture; remove; minimally observe `CheckpointExists`. | `kSuccess`; checkpoint absent. | H |
| `UnitCheckpointRemoveAbsent` | No checkpoint, including state after an earlier successful removal. | Exactly `kFileSystemError`. | H |
| `UnitCheckpointRemovePreservesOtherTiers` | Checkpoint fixtures for distinct tiers; remove one. | Target absent; other checkpoint retains exact data. | H |
| `UnitCheckpointZeroStatus` | Separate supported save/load scenarios with `(status=nullptr, status_size=0)`; checkpoint load has no active solving context. | Call is permitted, but success is not guaranteed and no fixed result code is asserted. End the lifetime on failure. | H |
| `UnitCheckpointSaveZeroSizeWithBuffer` | Active solving tier; pass a valid non-null status buffer with status_size zero. | Allowed call with no prescribed API return code; crashes and sanitizer failures still fail. End the lifetime on an error return. | H |
| `UnitCheckpointLoadZeroSizeWithBuffer` | Valid checkpoint; no active solving context; pass a valid non-null status buffer with status_size zero. | Exactly `kIllegalArgumentError`. | H |
| `UnitCheckpointSaveNullStatusWithPositiveSize` | Active solving tier; call CheckpointSave(nullptr, 1), then finalize without retry. | Exactly `kIllegalArgumentError`; no mutation/rollback assertions. | H |
| `UnitCheckpointLoadNullStatusWithPositiveSize` | Valid checkpoint; no active solving context; request the correct tier size but pass null status with positive status_size. | Exactly `kIllegalArgumentError`; do not infer a failure-state or retry guarantee. | H |
| `UnitCheckpointSaveWithoutSolvingTier` | Initialize without creating a solving tier; save with null status and zero size. | Accept any normal API return; crashes and sanitizer failures fail the test. No checkpoint/residency side effects are asserted. | H |

### Loaded tiers

| Planned test name | Setup and action; minimal extra members for units | Required assertions | Basis |
| --- | --- | --- | --- |
| `UnitLoadTierValidArchive` | Valid on-disk fixture and correct tier size; load; minimally observe `IsTierLoaded` and one getter. | `kSuccess`; resident; known record readable. | H |
| `UnitUnloadTierPreviouslyLoaded` | Load valid fixture as setup; unload; observe residency. | `kSuccess`; tier no longer resident. | H |
| `UnitUnloadTierRejectsSolvingTier` | In separate cases establish ordinary/concurrent/checkpoint-restored solving tier; attempt UnloadTier; finalize. | Exactly `kIllegalArgumentError`; no subsequent continued-solving/retry assertion. | H |
| `UnitIsTierLoadedAbsent` | Fresh database, and a separate case with an archive existing only on disk. | `false`; disk existence alone is insufficient. | H |
| `UnitIsTierLoadedEachResidentRoute` | Independently establish residency by `LoadTier`, ordinary create, concurrent create, or checkpoint load. | `true` for each documented route. | H |
| `UnitIsTierLoadedAfterRelease` | Resident tier released by corresponding unload/free method. | `false` for released tier. | H |
| `UnitGetValueFromLoadedMatrix` | Known resident fixture from `LoadTier`; query all `V × R` pairs and boundary/interior positions. | Exact fixture values. | H |
| `UnitGetRemotenessFromLoadedMatrix` | Known resident fixture from `LoadTier`; query all `V × R` pairs. | Exact fixture remotenesses. | H |
| `UnitLoadedGettersSolvingTier` | Independently establish solving tier through each supported route and populate known data; call both loaded getters. | Exact current in-memory values/remotenesses. | H |
| `UnitLoadedGettersNonresidentTier` | Tier absent from memory, with and without an archive existing on disk. | `kErrorValue` and `kErrorRemoteness`. | H |
| `UnitLoadedGettersDistinguishTiers` | Two resident tiers contain different records at the same position. | Each getter addresses the specified tier. | H |
| `UnitLoadTierMissingArchive` | Fresh sandbox and valid tier/size; no archive exists. | Exactly `kRuntimeError`. | H |
| `UnitLoadTierRejectsSolvingTier` | Create a solving tier; request LoadTier for that same tier. | Exactly `kIllegalArgumentError`. | H |
| `UnitUnloadTierRejectsNonresidentTier` | Tier has not been loaded or created; request unload. | Exactly `kIllegalArgumentError`. | H |

### Probes

| Planned test name | Setup and action; minimal extra members for units | Required assertions | Basis |
| --- | --- | --- | --- |
| `UnitProbeInitValidStorage` | Initialized database; valid `DbProbe` storage; initialize and perform a minimal fixture read. | `kSuccess`; initialized probe usable; do not inspect cache fields. | H |
| `UnitProbeInitNull` | Initialized database; call optional ProbeInit(nullptr); no real probe object is created. End normal use after the checked error. | Exactly `kIllegalArgumentError`. | H |
| `UnitProbeDestroyNull` | Initialized database; call optional ProbeDestroy(nullptr). | `kSuccess`; successful no-op. | H |
| `UnitProbeDestroyUnused` | Successfully initialize a probe; destroy it before any read. | `kSuccess`; generic leak sanitizer can supplement, without asserting DbProbe internals. | H |
| `UnitProbeDestroyAfterReads` | Initialize; read a public-API-generated archive; destroy. | `kSuccess`; generic resource diagnostics can supplement. | H |
| `UnitProbeValueMatrix` | Initialized probe and valid permanent fixture; query all `V × R` pairs and first/interior/last records. | Exact fixture values. | H |
| `UnitProbeRemotenessMatrix` | Initialized probe and valid fixture covering all `V × R` pairs. | Exact fixture remotenesses. | H |
| `UnitProbeValueNotFound` | Initialized probe; query a missing tier with an ordinary position value. Manually destroy the probe before finalization; do not read again. | `kErrorValue`; destroy returns kSuccess normally. No invalid existing-tier position call. | H |
| `UnitProbeRemotenessNotFound` | Initialized probe; query a missing tier; manually destroy the probe before finalization without another read. | `kErrorRemoteness` from core/constants.h; destroy returns kSuccess normally. | H |
| `UnitProbeInitRepeated` | Initialize probe, optionally read; initialize the same probe again, then read/destroy. | Reinitialization succeeds normally and leaves a usable probe, equivalent to successful destroy then init. | H |
| `UnitProbeDestroyRepeated` | Initialize; destroy successfully; destroy the same probe again. | Second destruction returns `kSuccess`. | H |
| `UnitProbeDestroyAfterFailedValueRead` | Initialize probe; use ProbeValue on a missing tier and check kErrorValue; destroy the initialized probe before finalization. | ProbeDestroy returns `kSuccess` normally. No further probing or retry. | H |
| `UnitProbeDestroyAfterFailedRemotenessRead` | Initialize probe; use ProbeRemoteness on a missing tier and check kErrorRemoteness; destroy before finalization. | ProbeDestroy returns `kSuccess` normally. No further probing or retry. | H |

### Segmentation API

All cases use the common initialization/finalization fixture. `B` is a
resource-bounded positive buffer count at most `MaxNumBuffers()`; `S` is a
small positive segment capacity. Positive-capacity cases require at least one
supported buffer. If the reported maximum is zero, report positive-buffer cases as skipped
with that explicit capacity reason; zero-buffer context cases remain valid. Explicitly seed every record before reading it. `seg.` below means
`Database::segmentation`. Cases needing distinct buffers require `B >= 2`;
skip them with a capacity reason when the maximum is below two.

| Planned test name | Setup and action; minimal extra members for units | Required assertions | Basis |
| --- | --- | --- | --- |
| `UnitSegmentationMaxNumBuffersReportsCapacity` | Call optional MaxNumBuffers; use its reported capacity to select bounded create/use cases. | A nonnegative count; zero is valid. No expected implementation-specific number or copied adapter oracle. | H |
| `UnitSegmentationCreateBuffersSinglePosition` | Create one buffer with capacity one; explicitly set and read offset zero. Extra: segment setter/getters, free. | `kSuccess` with adequate resources; requested record is usable. No default contents assertion. | H |
| `UnitSegmentationCreateBuffersMultipleBuffers` | Create feasible `B` buffers of capacity `S`; seed/read first, interior, and last offsets of every buffer. Extra: segment setter/getters, free. | `kSuccess`; requested buffers and offsets are usable without aliasing. | H |
| `UnitSegmentationCreateBuffersAtMaximum` | If reported capacity is small enough for the fixture budget, request that count with a small segment size; otherwise report resource-bound omission. At maximum zero, only create/free the context. | Supported count usable under adequate resources; no record access when count is zero, no induced OOM or allocation instrumentation. | H |
| `UnitSegmentationCreateBuffersZeroCountOrSize` | Separate no-context lifetimes for zero buffers and zero-sized buffers; count never above reported capacity; do not access nonexistent offsets. | Normal `kSuccess`; context can be freed. | H |
| `UnitSegmentationCreateBuffersNegativeCountOrSize` | Separate cases: negative buffer count with valid size; negative size with valid count, including signed minima. | Exactly `kIllegalArgumentError`. | H |
| `UnitSegmentationCreateBuffersAboveCapacity` | Query reported maximum M; request M+1 buffers with otherwise valid small sizes, including zero. Only form M+1 when representable as int and within the fixture's resource budget; cover M=0 with a request for one buffer. | Exactly `kIllegalArgumentError`. Report an explicit omission if the maximum cannot be exceeded safely; do not overflow INT_MAX or force OOM. | H |
| `UnitSegmentationGetValueAllLegalValues` | Seed active records with all `V × R` pairs; read each. Extra: create, setter, free. | Exact seeded values for every legal `Value` regardless of remoteness. | H |
| `UnitSegmentationGetValueSelectsBufferAndOffset` | Seed distinguishable values across feasible buffers and first/interior/last valid offsets. Extra: create, setter, free. | Returns the requested buffer's requested record. | H |
| `UnitSegmentationGetRemotenessLegalDomain` | Seed all `V × R` pairs, including zero and the declared maximum, then read each remoteness. Extra: create, setter, free. | Exact seeded remoteness; the project-wide minimum range is confirmed; no storage-width assumption. | H |
| `UnitSegmentationGetRemotenessSelectsBufferAndOffset` | Distinct remotenesses across feasible buffers and first/interior/last valid offsets. Extra: create, setter, free. | Returns the requested buffer's requested record. | H |
| `UnitSegmentationSetValueRemotenessLegalPairs` | Write every legal value with each confirmed supported remoteness at valid offsets. Extra: create, both segment getters, free. | Both fields equal the supplied pair; setter has no return value. | H |
| `UnitSegmentationSetValueRemotenessOverwritesPair` | Explicitly seed pair A, then write distinct pair B. Extra: create, both segment getters, free. | Both fields contain B. | H |
| `UnitSegmentationSetValueRemotenessPreservesOtherRecords` | Seed neighbors and other buffers; change one target. Extra: create, both segment getters, free. | Target changes; explicitly initialized other records retain their values and remotenesses. | H |
| `UnitSegmentationLoadRestoresSavedSegment` | Public setup: seed/flush an in-range segment; overwrite destination memory; load into an active buffer; read all logical records. | `kSuccess`; exact saved pairs restored, replacing previous contents. | H |
| `UnitSegmentationLoadSelectsDestinationBuffer` | Keep multiple buffers with different explicit data; load a valid saved segment into one. Extra: create, setter, flush, getters, free. | Destination receives saved contents; other buffers retain their data. | H |
| `UnitSegmentationLoadRejectsInactiveBuffer` | Keep source segment valid; try inactive destination indices such as `-1` and `B`, separately. Extra: create, setter, flush, free. | Exactly `kIllegalArgumentError`. Avoid combining inactive destination with missing segment because error precedence is unspecified. | H |
| `UnitSegmentationLoadMissingSegment` | Active buffer; request a valid segment index in the intended range 0..n-1 that has not yet been saved. | Exactly `kFileSystemError`; do not use an out-of-range index to create a missing-file test. | H |
| `UnitSegmentationFlushSavesBufferContents` | Fill an active buffer; flush an in-range segment; overwrite memory; reload using public API. | `kSuccess`; reloaded pairs match the reference data; no format-inspection adapter. | H |
| `UnitSegmentationFlushSelectsSourceBuffer` | Distinct explicit buffer patterns; flush selected sources to distinct segment indices, then reload. Extra: create, setter, load, getters, free. | Saved content comes from the selected buffer, independent of buffer index equaling segment index. | H |
| `UnitSegmentationFlushRejectsInactiveBuffer` | Flush from a negative or inactive buffer index; segment index otherwise valid. Separate cases avoid error-precedence assumptions. | Exactly `kIllegalArgumentError`. | H |
| `UnitSegmentationNegativeDiskIndices` | In separate valid-context cases call Load and Flush with an active buffer and seg_idx -1 or INT_MIN. | Exactly `kIllegalArgumentError`. | H |
| `UnitSegmentationFlushOverwritesExistingIndex` | Save A, write B, successfully flush again to the same valid disk segment index; reload using public methods. | kSuccess; loaded records equal B. Replacing the same index does not add a segment. No rollback/retry assertion. | H |
| `UnitSegmentationFreeBuffersSucceeds` | Create buffers using public methods; free once; finalization completes cleanup. | `kSuccess` normally; generic leak checks may supplement without backend allocation assertions. | H |
| `UnitSegmentationFreeBuffersMakesIndicesInactive` | Save a valid segment, free all buffers, then try loading it into a formerly active index. Extra: create, setter, flush, load. | Free succeeds; load returns `kIllegalArgumentError` because destination is no longer active. | H |
| `UnitSegmentationFreeBuffersWithoutContext` | Initialized database, no segment context created; call FreeBuffers. | `kSuccess`; successful no-op. | H |
| `UnitSegmentationFreeBuffersRepeated` | Create and free successfully; free again. | Second call returns `kSuccess`. | H |
| `UnitSegmentationConsolidateSingleSegment` | Flush one fully initialized segment with matching `tier_size`; consolidate; observe normal archive through initialized probe. Extra: create, setter, flush, probe lifecycle/readers, free. | `kSuccess`; archive contains exactly the logical pairs. | H |
| `UnitSegmentationConsolidateMultipleSegments` | Publicly save full segments at consecutive indices; consolidate before freeing; probe all positions. | `kSuccess`; exact ordered concatenation: global position = seg_idx × segment_size + offset. | H |
| `UnitSegmentationConsolidateEmptyTier` | Create a zero-buffer, zero-sized context; check optional TierStatus before consolidation; call Consolidate(0,0) without preceding segment flushes, then free. | kSuccess; a fresh tier reports missing before and solved after consolidation when TierStatus exists. No file inspection and no empty-tier probe call. | H |
| `UnitSegmentationConsolidateEmptyTierShapes` | Cross segment counts 0/1/several with capacities 0/positive at tier_size=0. Create a zero-buffer context, perform no segment flushes, consolidate, and free in separate lifetimes. | kSuccess and solved TierStatus when supported for every shape. No dependency on positive buffer capacity, physical segment-file materialization, or valid positions to probe. | H |
| `UnitSegmentationConsolidateNegativeArguments` | Separate valid-context cases with negative tier_size or segment count; other argument valid. | Exactly `kIllegalArgumentError`. | H |

The segment getters and setter expressly assume an active buffer and valid offset. Calls violating those assumptions do not belong in portable checked-error tests.

## Group 2: component tests

Each component owns its full lifetime and temporary storage. Exercise small
method subsets first; mixed stress cases supplement their coverage. For
permanent data, compare against the test's reference records, even when two
database access paths are also compared with each other.

### Lifecycle, naming, and persistent status

| Planned test name | Setup and action | Required assertions | Basis |
| --- | --- | --- | --- |
| `ComponentBasicLifecycle` | Init valid ordinary inputs with null aux; optionally inspect GetPath; Finalize. | Successful lifecycle; if supported, GetPath identifies the sandbox while initialized and is null after finalization. | H+U |
| `ComponentLifecycleRestartsWithFreshResidency` | Write/free an archive, finalize, then initialize the same game/variant/path. | On-disk data/status survive; previous in-memory tiers are not resident; a newly initialized probe reads the reference records. | H+U |
| `ComponentFallbackTierNamesRoundTrip` | Initialize with null namer; write and read two distinct tiers in one sandbox. | Fallback naming supports correct independent round trips; no assumed filename spelling. | H+U |
| `ComponentCustomTierNamesRoundTrip` | Initialize with a valid deterministic callback producing distinct legal names; write, restart with the same callback, and read. | Correct records for each tier; callback-driven storage remains accessible. No assumptions about call count or path layout. | H+U |
| `ComponentTierSolvedStatusLifecycle` | Fresh tier: create, explicitly populate, check status, flush, check status, free, check status; repeat for each creation mode. | Missing before final archive is flushed; solved after successful flush and after freeing memory. In-memory residency is separate from disk solved status. | H+U |
| `ComponentGameSolvedDeclarationPersists` | Fresh game: check incomplete, call `SetGameSolved`, check solved, finalize/reinitialize same identity, check again. | Declaration changes status to solved and remains readable in the later session. | H+U |
| `ComponentSandboxIsolation` | In sequential backend sessions, write different records for the same game/variant/tier in separate private sandboxes; reopen each. | Each sandbox reads its own records and status. Do not overlap module-global initialization. | H+U |

### Serial and externally synchronized record operations

| Planned test name | Setup and action | Required assertions | Basis |
| --- | --- | --- | --- |
| `ComponentSerialSetterFamilies` | Separate scenarios for value-only after remoteness seeding, remoteness-only after value seeding, and combined writes. | Every position matches expected fields; untouched fields retained. | H+U |
| `ComponentSerialSetterInteroperability` | Combined write -> value-only -> remoteness-only -> combined write; inspect after each. | Exact per-operation state; neighboring records unaffected. | H+U |
| `ComponentSerialMaximizeReferenceFold` | Ascending/descending/shuffled/duplicate candidates under multiple comparators. | Reference maximum after each call; boolean matches strict improvement. | H+U |
| `ComponentCounterCountdownAndClear` | Countdown through zero; separately decrement a few times, clear, decrement, clear again. | Exact preceding-count returns; stable zero; no underflow. | H+U |
| `ComponentCounterValueStateTransitions` | Undecided count -> decided value through each setter family -> counter methods -> explicitly return to undecided. | Decided operations return zero without changing remoteness; restored undecided interpretation uses retained or newly written remoteness. | H+U |
| `ComponentCounterMultiplePositions` | Interleave different counts and clear/decrement/query sequences across first/interior/last records. | Independent per-position reference models hold. | H+U |
| `ComponentSerialRecordsPersistAfterFlush` | Explicitly initialize every record; mix setters/maximize/counters; flush, free, probe. | Exact final records persist; no default-initialization assumption. | H+U |
| `ComponentExternallySynchronizedWorkers` | Ordinary tier; protect all operations sharing a position with an external lock; join before flush/free. | Reference state and persisted records match serialized operations. | H+U |

### Concurrent record operations

| Planned test name | Setup and action | Required assertions | Basis |
| --- | --- | --- | --- |
| `ComponentConcurrentSequentialParity` | Run deterministic serial operation script in both creation modes with distinct sandboxes. | Identical logical records and public readback; no file-byte equality requirement. | H+U |
| `ComponentConcurrentDisjointPositions` | Workers operate on disjoint subsets; exercise each method family before mixing them. | Independent reference models hold; no cross-position interference. | H+U |
| `ComponentConcurrentSamePositionReadersAndWriters` | Overlap supported solving setters/getters on a seeded position; include loaded getters on that solving tier when present; join before final readback. | Successful setter calls; individually permitted field results, atomic per-call behavior, and clean race-sanitizer runs. Two getter calls are not one pair snapshot. | H+U |
| `ComponentConcurrentMaximizeSafety` | Pure thread-safe comparator; seeded position; maximize and read concurrently. | Legal individual results and no races; exact maximum assertions are covered separately. | H+U |
| `ComponentConcurrentUndecidedCounterSafety` | Seed n; overlap decrement, clear, and count reads without reseeding. | No races; returns/counts in 0..n; exact multiset/conservation assertions in dedicated cases. | H+U |
| `ComponentConcurrentPairAndIndependentFieldContention` | Separate cases for combined-pair writers and independent value-only/remoteness-only writers on a seeded position; join. | Final pair is one complete combined write; independent setters preserve the other field. Validate a legal serial order respecting completed calls, without fixing overlapping-thread order. | H+U |
| `ComponentConcurrentMaximizeSameCandidate` | Initial (kLose,0); comparator orders by value then remoteness; two threads offer (kWin,7). | Exactly one true and one false; final (kWin,7). Both calls are atomic. | H+U |
| `ComponentConcurrentMaximizeDistinctCandidates` | Initial (kLose,0); threads offer (kTie,3) and (kWin,7) under the same comparator. | Final (kWin,7). Returns may be (true,true) if Tie wins first or (false,true) if Win wins first; do not require a fixed number of true results. | H+U |
| `ComponentConcurrentDecrementExactReturns` | Seed count `n`; more than `n` concurrent decrements. | positive returns are exactly 1..n once each, remaining returns zero; final zero. | H+U |
| `ComponentConcurrentClearExactReturns` | Seed positive count; multiple concurrent clears. | one original count, remaining returns zero; final zero. | H+U |
| `ComponentConcurrentDecrementClearConservation` | Seed `n`; mix decrements and clears without reseeding; ensure final draining operation. | positive decrement count plus sum of clear returns equals n; final zero; no negative count. | H+U |
| `ComponentConcurrentDecidedCountersAndReadOnlyRecords` | Separate fixed-decided counter contention and fully seeded read-only multireader scenarios. | Counter calls zero and records unchanged; read-only answers exact. | H+U |
| `ComponentConcurrentFinalPersistenceAndStatus` | Join all workers; inspect state; flush, free, probe/status. | Exact quiescent state persists; solved tier status. | H+U |
| `ComponentConcurrentDeterministicStress` | Fixed reported seeds; bounded workers and disjoint/contended positions; atomic-operation model checks; lifecycle/I/O after joins. | Reference results and permitted histories, no races, exact quiescent persistence. No timing-dependent thread-order expectations. | H+U |

### Checkpoint workflows

These workflows use regular solving tiers. Concurrent solving and checkpoint
operations are exercised in separate contexts.

| Planned test name | Setup and action | Required assertions | Basis |
| --- | --- | --- | --- |
| `ComponentCheckpointOrdinaryRoundTrip` | Create ordinary solving tier; write explicit records/status; save; free; check exists; load. | Exact records/status restored; solving and loaded getters agree; `IsTierLoaded` true. | H+U |
| `ComponentCheckpointRestoresSavedSnapshot` | Save A; mutate records/status without saving; free; restore. | Saved A restored, including decided records and undecided counters. | H+U |
| `ComponentCheckpointLatestSaveWins` | Save A; change records/status; save B; free and restore. | Complete B restored. | H+U |
| `ComponentCheckpointSurvivesReinitialization` | Save; finalize; initialize same game/variant/path; check exists; restore. | Checkpoint survives lifecycle restart; exact progress restored. | H+U |
| `ComponentCheckpointIndependentTiers` | Save checkpoints for multiple tiers; restore/remove them in varied order. | Each tier/status stays distinct; removing one preserves others. | H+U |
| `ComponentCheckpointResumeAndFinish` | Restore mixed decided/counter state; continue setters/maximize/counters; flush; free; probe. | Explicit reference model matches final permanent records; tier reports solved. | H+U |
| `ComponentCheckpointRemoveThenLoad` | Save; remove; check exists; attempt load into sentinel status buffer. | Removal succeeds; exists false; load nonzero and changes neither status nor residency. | H+U |
| `ComponentCheckpointDoesNotMarkSolved` | Separate fresh and already-solved archive cases; save checkpoint with writes paused; compare tier status before/after. | Checkpoint leaves tier archive status unchanged; checkpoint-only tier reports missing. | H+U |

### Loaded-tier and probe workflows

| Planned test name | Setup and action | Required assertions | Basis |
| --- | --- | --- | --- |
| `ComponentLoadReadUnload` | Load solved tier; read; unload; query residency and loaded getters. | Correct resident reads; after unload, residency false and getters return documented errors. | H+U |
| `ComponentLoadedTiersAndSolvingTier` | Load solved tiers; create a distinct solving tier; interleave reads/mutations; free solving tier and unload others. | No tier aliasing; each release uses its proper method. Rejection errors are isolated in unit tests instead of continuing the component after failure. | H+U |
| `ComponentUnloadPreservesOtherTiers` | Load distinct tiers; unload one; continue queries on others. | Remaining tiers stay resident with exact records. | H+U |
| `ComponentFlushLoadAndProbeAgree` | Solve; flush; free; load; read; unload; probe. | Solving, loaded, and permanent views match the same explicit reference data. | H+U |
| `ComponentProbeReadOrders` | Probe initialized archive in forward, reverse, repeated, and fixed-seed shuffled order, including all positions. | Correct results independently of request order/internal buffer geometry. | H+U |
| `ComponentProbeTierAndGetterSwitching` | One probe alternates tiers and value/remoteness methods at same/different positions. | No stale tier/cache state; each read matches fixture. | H+U |
| `ComponentIndependentProbes` | Initialize multiple probes; interleave their reads sequentially; destroy one; continue using others. | Each works independently; destroying one preserves others. No thread-safety requirement is inferred. | H+U |
| `ComponentProbeAfterMemoryRelease` | Write/flush archive; free solving tier; initialize/read/destroy probes in bounded cycles. | Permanent reads work independently of residency; generic leak diagnostics may supplement. | H+U |

### Segmentation and consolidation

| Planned test name | Setup and action | Required assertions | Basis |
| --- | --- | --- | --- |
| `ComponentSegmentationMinimalLifecycle` | Query capacity; create one buffer; explicitly fill; flush; consolidate; free; probe final archive. | Logical records survive; resulting normal tier archive reports `kDbTierStatusSolved`. | H+U |
| `ComponentSegmentationReusesOneBufferForManySegments` | Reuse one buffer for n consecutive segment indices 0..n-1; consolidate before freeing. | Each saved segment keeps its explicit data; global records follow index × segment_size + offset. | H+U |
| `ComponentSegmentationKeepsMultipleBuffersIndependent` | Alternate updates, reads, and flushes across feasible buffers with distinct deterministic patterns. | No cross-buffer or cross-offset contamination; every saved segment has its selected source data. | H+U |
| `ComponentSegmentationLoadModifyAndResave` | Flush segment A; overwrite memory; load A; modify selected records; flush to another segment index; reload both. | Load restores A; subsequent writes work; independently saved indices remain distinguishable. | H+U |
| `ComponentSegmentationOverwriteThenConsolidate` | Save two complete segments; replace the first at the same index with new records; consolidate with segment count still two, then free/probe. | Final archive contains the replacement first segment and original second segment, in index order. | H+U |
| `ComponentSegmentationLoadsIntoDifferentBuffers` | Save from buffer zero; load the same segment into another active buffer; alternate loads of different saved segments. | Segment identity is independent of original and destination buffer indices; correct records after each load. | H+U |
| `ComponentSegmentationConsolidatesOutOfOrderFlushes` | Flush the complete index set 0..n-1 in shuffled order, without repeated indices; consolidate before freeing. | Same ordered logical tier regardless of flush order. | H+U |
| `ComponentSegmentationHandlesPartialFinalSegment` | Example segment size 4 and tier size 10: initialize only logical records 0..9; never read or write the final two padding offsets; flush/consolidate. | Exactly the ten logical records survive; padding is ignored without being initialized or accessed through public getters/setter. | H+U |
| `ComponentSegmentationMatchesNormalTierArchive` | Write the same reference tier normally and through consecutive segments in separate sandboxes; consolidate before freeing; load/probe each supported view. | Both have solved status and identical logical records, including allowed undecided positions. No byte comparison; normal flush uses nullptr aux. | H+U |
| `ComponentSegmentationPersistsAcrossDatabaseSessions` | Consolidate successfully; finalize; reinitialize the same sandbox; read using probes and loaded-tier getters. | Complete logical data persists and tier remains solved. | H+U |
| `ComponentSegmentationTracksTierStatus` | Start with no archive; create/fill/flush segments; consolidate; query tier status before any output and after successful consolidation. | Initially missing; solved after successful consolidation. | H+U |
| `ComponentSegmentationIntermediateStatus` | For a fresh nonempty tier, flush first an incomplete set of logical segments, then the complete set; query TierStatus before Consolidate. | kDbTierStatusMissing after both segment-flush stages; kDbTierStatusSolved only after successful Consolidate. No inspection of segment files. | H+U |
| `ComponentSegmentationStreamingStress` | Many consecutive saved segments, few buffers, fixed-seed nonrepeating flush order, reloads, and final consolidation before free. | Every logical record matches the independent model; no tail access or time threshold. | H+U |

### Combined workflows

| Planned test name | Setup and action | Required assertions | Basis |
| --- | --- | --- | --- |
| `ComponentRecoveredSolveFeedsAllReaders` | Load child tiers; create a regular parent solving tier; mutate records/counters; checkpoint/free/restore; continue serially; flush/free and compare supported reader paths. | Exact restored status/records, serial continuation, correct residency, and final reference archive/solved status. | H+U |
| `ComponentBoundedMixedStress` | Bounded fixed-seed scripts over supported method subsets; atomic concurrent record phases followed by joins and release; regular-solving checkpoint workflows in separate contexts; lifecycle/I/O. | Independent model after quiescent phases; valid atomic histories during contention; no cross-tier/path contamination or unsupported retries. | H+U |

## Explanations requested in review

| Topic | Explanation |
| --- | --- |
| Why 25 transitions? | V contains five legal values. Choosing any old value and any new value gives 5 × 5 = 25 ordered pairs, including five unchanged-value writes. This is not a count of distinct enum values. |
| What was workflow 3.a? | It referred to item 3.a of the original request: estimate concurrent memory before creation. Its mention of CreateSolvingTier was a typo for CreateConcurrentSolvingTier. The plan now names the correct method. |
| What does index-to-position mapping mean? | With segment capacity S=4, segment 0 covers positions 0..3, segment 1 covers 4..7, and segment 2 covers 8..11. Global position is seg_idx × S + offset. If tier size is 10, the last two offsets are inaccessible tail and consolidation ignores them. |
| Missing or duplicate segment indices? | For n=3, required disk indices are 0,1,2. For a nonempty tier, if index 1 was never saved, consolidation's complete-input precondition is unmet; do not deliberately consolidate that shape. Successfully flushing index 1 twice replaces that segment, rather than creating a fourth segment. Load of a missing but in-range segment has its explicit FileSystemError case. |
| Why one true for duplicate maximize candidates? | Start at (kLose,0), order by value then remoteness, and let two calls both offer (kWin,7). One atomic call replaces the old pair and returns true; the other sees an equal pair and returns false. Without atomic read/compare/replace, both could incorrectly report true. The confirmed atomic contract rules that out. |
| Why no fixed true count for distinct maximize candidates? | Starting at (kLose,0), offers (kTie,3) and (kWin,7) finish at Win. If Tie acts first, both calls return true; if Win acts first, Tie returns false. Both interleavings are valid. Losing the Win update would violate atomic maximize. |
| Counter race examples | Starting at count 1, two decrements must return 1 and 0, never both 1. Starting at count 3, two clears must return 3 and 0. A predicate check and update must be one atomic call; the confirmed contract already requires this, so these are resolved examples rather than new questions. |
| Finalize success | Finalize is void. Its spec now promises successful completion/no-op in the approved states; no ABI or return-type change was made. |

## Coverage deliberately left elsewhere

| Coverage | Disposition |
| --- | --- |
| Private allocation/I/O fault hooks, recognized corrupt-file construction, exact byte accounting and implementation-specific aux configurations. | Concrete implementation tests; no adapters or new runtime testing hooks added here. Generic error tests use only deterministic public checked-input/missing-file conditions. |
| OOM codes, TierStatusCorrupted/CheckError, GameStatusCheckError, and unexpected getter failures. | Return contracts remain in the header, but there is no portable way to force these conditions for every implementation. No claim of complete branch coverage. |
| Unknown initial fields, invalid/oversized counters, invalid record inputs, corrupted probe reads, tail access, and lifecycle overlap. | No default-content assertions or intentional undefined-behavior/death tests. Unwritten contents remain unspecified; invalid accesses explicitly declared UB stay excluded. |
| Retry, rollback, continued solving after failure, power-loss durability, consolidation after FreeBuffers/reinit. | Unsupported workflows excluded. Successful repeat operations explicitly approved by review remain tested. |
| Stronger corruption checks and exact GetPath spelling/lifetime/ownership policy. | Accepted limitations; no new guarantees or questions imposed. |

## Review record

The following preserves substantive comments from the reviewed version. Active
test tables above incorporate the decisions; comments here are historical
review input, not additional active test cases. Unresolved matters are only in
the open-question table. Dispositions reflect the latest decisions; the
original Q7 migration proposal and earlier INT64_MIN-only identifier rule are
superseded by Q12. All original review-comment cells are preserved.

| Original source | Review comment (preserved) | Disposition |
| --- | --- | --- |
| Review of `UnitDescriptorProvidesEveryRequiredMember` | I forgot to mention that not all member functions are required. For simplicity, assume only Init() and Finalize() always exist. All other methods should only be tested after verifying that their corresponding function pointers are not null. | Only Init and Finalize are required. Optional operations and their setup/readback/cleanup dependencies are guarded separately. |
| Review of `UnitInitSucceedsWithTierNamer` | I've modified the spec to always allow `NULL` to be passed to the `aux` parameter to allow writing generic tests in our task. It will be a separate out-of-scope task to write instance-specific tests. | Generic initialization now always uses nullptr aux; no backend configuration adapter. |
| Review of `UnitInitReportsOperationalFailure` | I don't think this is possible without a concret implementation spec. Unless you can provide a plan on how to test this we should remove this test case. | Removed from the generic suite: needs implementation-specific fault hooks, resource accounting, or file-format knowledge. Coverage belongs with the concrete implementation. |
| Review of `UnitInitInputAndRepeatPolicies` | We should remove these tests: the API is intentionally vague and all possible inputs may be allowed by an instance. Repeated initialization is undefined behavior. | Removed as requested: outside the generic contract, undefined behavior, or unsupported retry/free-before-consolidation workflow. Defined boundary cases are retained separately. |
| Review of `UnitFinalizeEdgeStatePolicies` | Finalize before init, twice, and after failed init (not easily testable) should all be defined as legal operations and should always return success. Help me modify the spec here. Outstanding probe at finalize is undefined behavior. | Documented successful Finalize before Init, after failed Init, and after successful Finalize. Failed-Init cleanup has no portable induced-failure test. Live probes at Finalize are UB. The signature remains void. |
| Review of `UnitFlushAndSolvedRepeatPolicies` | Flush without a solving tier is undefined. Flush and game-solved declaration are idempotent. Modify the spec. | Idempotence is documented. Flush without a solving tier is UB and excluded. |
| Review of `UnitCreateTierSizeAndStateBoundaries` | Zero sizes are allowed; negative size should return `kIllegalArgumentError`; overflowing is impossible unless you mean OOM, in which case it should return `kMallocFailureError`. Calling the create function twice is undefined behavior and we should note it in the spec. The intended usage is to only have one solving tier alive at a time and calling a second time without freeing the current one first almost always leads to memory leak. Mode switching is undefined behavior. | Added accepted-zero and rejected-negative cases. Allocation failure is documented as kMallocFailureError but its injected test is out of scope; repeated creation and switching active modes are UB. |
| Review of `UnitFreeSolvingTierRepeated` | This test is valid: the spec should also explicitly say that a second call to free after a successful first call should always succeed. | Applied to the header and revised scenarios. |
| Review of `UnitSetValueEveryLegalValue` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitSetValueEveryValueTransition` | Where did the number 25 come from? | 25 = five legal old values times five legal new values, including the five same-value transitions. |
| Review of `UnitSetRemotenessEveryLegalRemoteness` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitSetValueRemotenessEveryLegalPair` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitGetValueEveryLegalResult` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitGetRemotenessEveryLegalResult` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitRecordInvalidInputsAndMissingTier` | These are all undefined behaviors and should not be tested. | Removed as requested: outside the generic contract, undefined behavior, or unsupported retry/free-before-consolidation workflow. Defined boundary cases are retained separately. |
| Review of `UnitMaximizeValueRemotenessDomainBoundaries` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitMaximizeValueRemotenessEquivalentRanks` | The spec should say that no operation is performed in this case. | Applied to the header and revised scenarios. |
| Review of `UnitMaximizeValueRemotenessInvalidArguments` | These are undefined behaviors and should not be tested. | Removed as requested: outside the generic contract, undefined behavior, or unsupported retry/free-before-consolidation workflow. Defined boundary cases are retained separately. |
| Review of `UnitDecrementNumUndecidedChildrenPositive` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitDecrementNumUndecidedChildrenDecidedValues` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitClearNumUndecidedChildrenPositive` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitClearNumUndecidedChildrenDecidedValues` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitGetNumUndecidedChildrenUndecidedCounts` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitGetNumUndecidedChildrenDecidedValues` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitCounterMethodsInvalidInputs` | Missing tier, invalid position, negative are undefined and should not be tested. Oversized counter is also undefined, skipped in the test, but we should modify the spec to require the user to check the implementation's spec for maximum allowed counter size. | Removed as requested: outside the generic contract, undefined behavior, or unsupported retry/free-before-consolidation workflow. Defined boundary cases are retained separately. |
| Review of `UnitTierMemUsageLoadUpperBound` | This is intentionally vague and we can keep the test simple. | Kept simple portable estimator checks. Exact measured byte-bound tests require implementation-specific instrumentation and were removed. |
| Review of `UnitConcurrentTierMemUsageCreationUpperBound` | This is intentionally vague and we can keep the test simple. | Kept portable valid-request/error-sentinel checks; peak semantics are documented. |
| Review of `UnitMemoryEstimateInputAndOverflowBoundaries` | We should modify the spec and define `SIZE_MAX` to be the error sentinel for all the above cases except for zero-sized tiers, which is allowed. | SIZE_MAX is documented for negative size, invalid tier, overflow, and other errors. Zero is accepted. No backend-specific allocation formula is inferred to manufacture an overflow input. |
| Review of `UnitTierMemUsageOrdinaryCreationBound` | Yes, it also bounds `CreateSolvingTier`. | TierMemUsage now explicitly bounds both LoadTier and CreateSolvingTier peak usage. |
| Review of `UnitCheckpointUnspecifiedInputs` | Zero status size/NULL status/save with no solving tier: allowed, but not guaranteed success. Zero status size but non-null status: `kIllegalArgumentError`. Load while solving tier exists: undefined behavior. Tier-size mismatch: `kRuntimeError`. Restore concurrency mode: currently not supported. Modify the spec accordingly. | The latest checkpoint corrections supersede the earlier save nonnull+zero rejection and tier-size mismatch error: Save permits nonnull+zero, and mismatched tier sizes are UB. Null+positive and Load nonnull+zero remain checked errors. No-tier saves accept any normal return; active-context loads remain UB and restoration remains serial. |
| Review of `UnitGetValueFromLoadedMatrix` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitGetRemotenessFromLoadedMatrix` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitLoadedTierUnspecifiedCases` | Missing/corrupt file: `kRuntimeError`. Repeated load is undefined. Loading solving tier, unloading non-resident tier: `kIllegalArgumentError`. Wrong size/invalid position: undefined. | Added portable missing-file and residency errors. Corrupt-file construction remains implementation-specific. Repeated load, wrong size, and invalid positions are UB and excluded. |
| Review of `UnitProbeValueMatrix` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitProbeRemotenessMatrix` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitProbeUnspecifiedCases` | Repeated init: equivalent to first calling destroy and then calling init again on the same target. Repeated destroy: second invocation always succeeds if first one succeeds. Corrupt archive and invalid position are undefined. | Repeated lifecycle operations are defined. The latest initialization correction rejects ProbeInit(nullptr) with kIllegalArgumentError; ProbeDestroy(nullptr) remains a successful no-op. Corrupt reads/invalid positions remain UB; valid read errors return sentinels and still require manual probe destruction. |
| Review of `UnitSegmentationCreateBuffersInputContractPending` | Zero size/count are valid whereas negative ones should return `kIllegalArgumentError`. Duplicate creation and creation while another tier is active are undefined. | Zero count/size accepted; negative arguments and count above maximum return IllegalArgumentError. Duplicate or conflicting creation is UB. |
| Review of `UnitSegmentationGetValueAllLegalValues` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitSegmentationGetRemotenessLegalDomain` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitSegmentationSetValueRemotenessLegalPairs` | This is correct. | Confirmed; revised plan follows the shared minimum domain and current header. |
| Review of `UnitSegmentationFlushInactiveBufferContractPending` | Spec should be corrected. | The error-constant typo is fixed; inactive-buffer checks remain generic. |
| Review of `UnitSegmentationDiskIndexContractPending` | Negative indices returns `kIllegalArgumentError` for Load() and Flush(), but undefined for other methods that require maximum efficiency. Loading into active buffer overwrites existing contents (notice how there's no method to "clear" contents or set buffer inactive). | Negative Load/Flush indices are checked; active-buffer loading overwrites; a successful Flush replaces the existing disk segment at that index. |
| Review of `UnitSegmentationFreeBuffersIdleContractPending` | Both are valid and should succeed if previous condition is clean. | Both clean idle and repeated-free cases are now explicitly successful. |
| Review of `UnitSegmentationConsolidateMultipleSegments` | Not sure what the question means here: what is "index-to-position mapping"? | Index-to-position mapping means where a segment offset lands in the whole tier. With size 4, segment 2 holds positions 8..11. |
| Review of `UnitSegmentationConsolidateShapeContractPending` | Zero tier size and segment count are valid but negative should return `kIllegalArgumentError`. Missmatched capacity/count/size are undefined. Short tail contents are undefined. What are missing/duplicate indices? Invocation after free is undefined. | All nonnegative empty-tier shapes are settled. Consolidate requires no preceding segment flushes for an empty tier and publishes a zero-position archive with solved status. Tests inspect public status, not physical storage. Negative checked errors and nonempty shape constraints remain unchanged. |
| Review of `ComponentIdentityAndNameEdgePolicies` | Remove this test. | Removed as requested: outside the generic contract, undefined behavior, or unsupported retry/free-before-consolidation workflow. Defined boundary cases are retained separately. |
| Review of `ComponentConcurrentPairAndIndependentFieldContention` | All of these are well-defined behaviors: the guarantee is that each function call listed in the docstring of CreateSolvingTier() is atomic but without a defined ordering. | Per-call atomicity is documented, so these assertions are now required; no fixed interleaving is imposed. |
| Review of `ComponentConcurrentMaximizeDuplicateAndDistinctCandidates` | I don't understand the questions. Please give concrete examples. | Added concrete duplicate/distinct-candidate examples. Exact outcomes follow from the confirmed atomic-call contract; thread order remains unspecified. |
| Review of `ComponentConcurrentDecrementExactReturns` | This is correct. | Confirmed and promoted to required assertions under atomic per-call semantics. |
| Review of `ComponentConcurrentClearExactReturns` | This is correct. | Confirmed and promoted to required assertions under atomic per-call semantics. |
| Review of `ComponentConcurrentDecrementClearConservation` | This is correct. | Confirmed and promoted to required assertions under atomic per-call semantics. |
| Review of `ComponentConcurrentDeterministicStress` | This is correct. | Applied to the header and revised scenarios. |
| Review of `ComponentCheckpointDoesNotMarkSolved` | Saving checkpoint does not affect tier solved status; they are separate systems. | Applied to the header and revised scenarios. |
| Review of `ComponentSegmentationReusesOneBufferForManySegments` | Segment indices are zero-indexed consecutive integers from 0 to n-1, if the total number of segments is n. The behavior is otherwise undefined. | Applied to the header and revised scenarios. |
| Review of `ComponentSegmentationConsolidatesOutOfOrderFlushes` | See comment above. | Applied to the header and revised scenarios. |
| Review of `ComponentSegmentationHandlesPartialFinalSegment` | Tail is not initialized and accessing those indices is undefined behavior. | Applied to the header and revised scenarios. |
| Review of `ComponentSegmentationMatchesNormalTierArchive` | See comment above. | Applied to the header and revised scenarios. |
| Review of `ComponentSegmentationConsolidatesAfterFreeBuffers` | Tier remains the one initialized when CreateBuffers() is called and it is not cleared until FreeBuffers() is called. Spec should state this explicitly. | Removed as requested: outside the generic contract, undefined behavior, or unsupported retry/free-before-consolidation workflow. Defined boundary cases are retained separately. |
| Review of `ComponentSegmentationIntermediateStatusPending` | `kDbTierStatusMissing` should be returned when only saved segment files exist, regardless of their completeness. | Applied to the header and revised scenarios. |
| Review of `ComponentSegmentationStreamingStress` | See comment above on segment mapping. | Applied to the header and revised scenarios. |
| Review of `ComponentSegmentationRetriesAfterOperationalFailure` | Remove this test case. | Removed as requested: outside the generic contract, undefined behavior, or unsupported retry/free-before-consolidation workflow. Defined boundary cases are retained separately. |
| Review of `ComponentBoundedMixedStress` | I believe all questions have been resolved. Leave a question if anything else remains unresolved for this test case. | Atomicity, serial checkpoint continuation, and default flush aux are resolved. The scenario has no additional open question; optional methods and their dependencies are checked. |
| Reviewed issue: `segmentation.Flush` names `kIllegalArgument`, but the canonical status is `kIllegalArgumentError`. | Agreed. | Already corrected in the reviewed header; retained kIllegalArgumentError. |
| Reviewed issue: `CheckpointSave` labels its `const void *status` as `@param[out]`. | Please help me correct this. | Corrected to input status, explicitly unchanged. |
| Reviewed issue: `TierMemUsage` promises a bound for loading, but the solver workflow uses it before ordinary solving-tier creation. | Extend the contract to both: agreed. What is workflow 3.a? | Extended TierMemUsage to loading and ordinary creation; workflow 3.a is explained above. |
| Reviewed issue: Memory upper-bound accounting is undefined. | All memory upper-bounds returned are peak usages. | Both estimators now explicitly bound peak usage, including temporary memory; no exact accounting tests. |
| Reviewed issue: Scalar input domains and validation are incomplete. | For all signed `int64_t` values, only `INT64_MIN` is invalid; all other values are not explicitly rejected throughout the system. | Superseded by latest Q12: valid Tier, Position, and remoteness data are nonnegative. All negative values are invalid data; existing named negative error sentinels and constants remain unchanged. The INT64_MIN migration and analysis-conversion tasks are withdrawn. |
| Reviewed issue: Database does not state its remoteness limit or a distinct child-counter capacity. | Any valid Database implementation must at least support `kRemotenessMax`. It may optionally support larger ranges but we cannot test them here. | Minimum global remoteness range is explicit; optional larger backend domains excluded from this suite; counter limit documented. |
| Reviewed issue: Initial solving-tier and segment-buffer contents are unspecified. | We should explicitly define them as unknown in the specs. | Unknown fields until explicitly written is explicit for solving tiers and buffers, with field-specific writes. |
| Reviewed issue: Multi-reader/multi-writer safety lacks an explicit operation-atomicity contract. | I don't understand this question. Please give concrete examples of possible race conditions so I can specify the expected behavior. | Atomic calls and comparator requirements are now documented. The maximize/counter examples explain the permitted outcomes; there is no open concurrency question here. |
| Reviewed issue: Separate record getters cannot return a pair snapshot. | This is a limitation. | Limitation explicitly documented; no combined pair-snapshot assertion during writers. |
| Reviewed issue: Concurrent scope beyond the nine record operations is unclear. | Loaded getters are equivalent to solving getters. Checkpoints during writes are undefined; they must be called when all writes are paused. Probes are for disk storage only and does not interfere with solving or loaded tiers. Overlapping with lifecycle operations is also undefined. | The latest checkpoint correction additionally excludes concurrent solving tiers entirely. Loaded getter parity and quiescent regular-tier checkpoint/lifecycle requirements are documented. Probes remain disk readers; no shared-probe/archive-replacement guarantee added. |
| Reviewed issue: `CheckpointLoad` does not choose or state the restored concurrency mode. | It corresponds to serial mode. Please update the specs. | CheckpointLoad explicitly restores ordinary externally synchronized mode. |
| Reviewed issue: Zero-size checkpoint status and invalid load parameters are unclear. | Resolved in comments for individual test cases. | Updated by the latest checkpoint corrections: Save permits either pointer form with zero size and any normal API return; Load's pointer/size checked errors remain. A checkpoint tier-size mismatch is UB and is excluded from tests. |
| Reviewed issue: Failure state and transactional guarantees are mostly absent. | No retries are intended to be carried out throughout the whole solving process. System fails and stops immediately upon any failure. | No retries or rollback/continuation guarantee added; stop-on-failure expectation documented. |
| Reviewed issue: Release/repeat-state policies differ. | Resolved in comments for individual test cases. | Applied per-method release and repeat rules; prohibited repeats excluded. |
| Reviewed issue: Several return types cannot distinguish errors. | Resolved in comments for individual test cases. | SIZE_MAX estimator errors added; invalid record inputs are caller precondition violations. No generic private-error injection. |
| Reviewed issue: Callback and aux contracts are missing. | Resolved in comments for individual test cases. | Both Init and Flush accept default null aux. Comparator requirements are explicit. Configuration/name-edge tests remain excluded. |
| Reviewed issue: `GetPath` lifetime and spelling rules are unstated. | These are less important details and I don't care how them perform. | No spelling, identity, or ownership additions; tests compare designated sandbox directory only. |
| Reviewed issue: Segmentation conflates buffer count and segment count. | Yes it should be corrected to `num_buffers`. Capacity can be zero. | Creation parameter is num_buffers; zero maximum/count/size allowed; over-capacity requests return IllegalArgumentError. |
| Reviewed issue: Segment addressing, overwrites, and final tails are underspecified. | Agreed. | Consecutive logical mapping, disk-segment replacement, and inaccessible outside-tier offsets are documented. Empty consolidation needs no prior flushes and is checked through public tier status; physical files remain opaque. |
| Reviewed issue: `Consolidate` has no tier argument and `FreeBuffers` may erase its implicit tier context. | Tier metadata does not survive freeing buffers. Consolidation cannot be resumed after reinitialization. | Tier context ends at FreeBuffers; consolidate before free, within the same Init lifetime; no consolidation restart. |
| Reviewed issue: Permanent archives and solved status have no explicit completeness rule. | `kUndecided` is a valid value for unreachable positions in a solved tier. The spec permits any value/remoteness combination for solved tiers. | All legal value/remoteness pairs allowed in final archives, including undecided unreachable positions. |
| Reviewed issue: Game completion and identity are partly implicit. | Game solved status is intentionally left as user responsibility. | SetGameSolved is an idempotent caller declaration; no tier enumeration or same-path identity-edge tests. |
| Reviewed issue: Corruption detection strength is unspecified. | This is less of a concern for now. Leave it as a limitation. | Retained limitation; corrupt archive preparation/detection-strength tests excluded. |
| Reviewed issue: Global state limits instance isolation. | It is currently a limitation that only one Database implementation can be initialized exactly once at a time. | Only one initialized Database at a time within a process; fixture lifetimes serialize across all backends. |
