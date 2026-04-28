# CI strategy

This document explains *why* each CI gate exists for `pgexperts/pgxbackup`. The premise: a green CI run should let an operator confidently deploy the resulting binary to back up production PostgreSQL data. Every gate maps to a specific class of production failure that the gate is designed to prevent.

## Branch model

- **`eol`** — historical. Frozen at the upstream `pgbackrest/pgbackrest` HEAD as of fork creation. Memorialized; CI does not run on it. Use it as a reference point for what we forked from.
- **`main`** — working branch. Every gate listed below fires on every push and every PR.
- **`*-ci`, `*-cig`** — convention for ad-hoc CI runs on feature branches without opening a PR. Inherited from upstream.

## Gates and what each catches

| Workflow | Production failure mode it prevents |
|---|---|
| `test.yml` (matrix: 9 distros + arches + libcs) | Platform-specific bugs: wrong checksum on big-endian s390x, glibc-vs-musl divergence, 32-bit pointer-arithmetic surprises, FreeBSD/macOS POSIX corners. |
| `test.yml` valgrind jobs (`u22`, `aarch64`) | Use of uninitialized memory; reads past heap allocations that ASan misses. |
| `test.yml` integration tests | End-to-end backup → restore round-trip against real PostgreSQL versions. The only gate that exercises libpq integration. |
| `test.yml` documentation builds | Doc XML / configuration table changes that break `pgbackrest help`. |
| `test.yml` `unity` job | Symbol leakage — anything inadvertently exported from the binary. The diff against `symbol.out` is the canary. |
| `test.yml` `code-format` job | Drift from the project uncrustify config. |
| `sanitizers.yml` (ASan + UBSan) | Heap buffer overflows, use-after-free, leaks, signed-overflow UB, alignment violations, NULL deref. ASan finds a much wider class than valgrind at lower overhead. |
| `static-analysis.yml::cppcheck` | Integer overflow, format-string mismatches, null-deref, dead code, leaked resources on error paths. Catches things `-Wall -Wextra` does not. |
| `static-analysis.yml::scan-build` | Path-sensitive bugs: double-free, division by zero, null deref reachable on a specific code path. |
| `static-analysis.yml::clang-tidy` | API misuse (e.g., `cert-err33-c`: ignoring the return of a function that can fail), bug-prone patterns, narrow-cast errors. |
| `coverage.yml::coverage-completeness` | A new file added to `src/` that no test exercises. The 100% per-file gate inside `test.pl` does not catch this — only files in `define.yaml` are checked. |
| `reproducibility.yml` | Build-time nondeterminism: a `__DATE__` macro sneaking in, build path leaking into debug info, library timestamp embedding. Forensic provability for "this binary wrote this backup". |
| `actionlint.yml` | Workflow YAML typos that would silently disable a gate. (Unparseable YAML produces no jobs and a green checkmark.) |
| `dependabot.yml` | GitHub Actions versions falling behind. Reduces supply-chain attack surface. |

## What "exemplar" means here

For a backup tool, **silent corruption is the existential failure**. A bug that crashes loudly is recoverable; a bug that emits a backup that won't restore is catastrophic. The gates above are weighted toward catching silent-corruption classes:

- **Memory safety** (ASan, valgrind, scan-build): a use-after-free in the manifest writer corrupts the manifest silently.
- **Integer / cast errors** (UBSan, cppcheck, clang-tidy `cert-*`): a truncated size in the block-incremental code corrupts a delta restore.
- **Coverage completeness**: an unexercised function in production binary is, by definition, untested behavior at scale.
- **Reproducibility**: when an operator says "the backup is corrupt", we need to know exactly what binary wrote it.
- **Cross-arch round-trips** (s390x in test.yml `arch:` matrix): a checksum that's correct on x86 and wrong on big-endian is the kind of bug that hits the 0.1% of users who run on z/Linux and is invisible to everyone else.

What this CI *can't* catch and we accept:
- **Filesystem-level races during a live backup** — these need integration tests against real concurrent workloads, beyond unit-test scope.
- **Behavior of versioned PostgreSQL features added after the fork's PG-version table** — would need test images for new PG releases.
- **Network-partition behavior of the protocol layer** — chaos testing is out of scope.

## Known gaps and follow-ons

These are recognized weaknesses in the current CI; they are *not* blockers for confidence, but they would meaningfully tighten the net:

1. **Sanitizers in the unit-test run.** `test.pl` hard-codes its `meson setup` invocation at `test/test.pl:506` and `test/test.pl:663`, so passing `-Db_sanitize=address,undefined` requires Perl modifications. The minimum patch is to read an env var or new `--sanitize=` flag. Once threaded through, the existing valgrind-on-tests path can run alongside sanitizers-on-tests, multiplying coverage.

2. **Fuzz harnesses.** The codebase parses several attacker-controllable formats: INI (`src/common/ini.c`), JSON (`src/common/type/json.c`), pack (`src/common/type/pack.c`), config option strings, manifest, S3/GCS/Azure XML and JSON responses. Each is a fuzz target candidate. libFuzzer entry points (`LLVMFuzzerTestOneInput`) for these would surface real bugs cheaply. OSS-Fuzz integration is the gold-standard goal.

3. **Sanitizer-instrumented integration tests.** The integration-test docker images would need to be rebuilt with sanitizer-instrumented `pgbackrest` to surface bugs in the protocol layer, parallel job dispatcher, and storage backends. High value, large lift.

4. **MSan (memory sanitizer).** Detects use-of-uninitialized-memory more aggressively than valgrind. Requires every linked library (libpq, OpenSSL, libxml2, ...) to also be MSan-instrumented; in practice this means a custom build of those libs. Realistic only if we accept the maintenance cost.

5. **Crypto known-answer tests.** Wycheproof or NIST CAVS test vectors run against `cipherBlock` would catch regressions in encryption that round-trip-tests miss.

6. **On-disk format stability.** A test that takes a corpus of backups produced by older versions and verifies the current code can restore them. Catches accidental format-version breakage (a class of bug that is invisible until a customer tries to restore from an old backup).

7. **Performance regression detection.** A microbenchmark suite that runs on every PR and flags >10% regressions. Currently `test.pl` has performance modules but no CI gate on the numbers.

8. **Action pinning to SHAs.** Workflows currently pin to major-version tags (`actions/checkout@v6`). For supply-chain hardening, pin to SHAs and let Dependabot float the SHA forward. Defers a decision rather than makes one — but worth considering. *(Done as of commit 564fb55af.)*

9. **clang-analyzer-* checks (path-sensitive).** Temporarily disabled in `.clang-tidy` pending triage of 20 findings surfaced on the first run that compiled cleanly. The findings, by category:
   - 7 × `core.NullDereference` in `src/command/verify/verify.c` (lines 1804, 1812, 1820, 1825, 1846, 1851, 1858) — possibly real bugs (missing null guards) or analyzer can't see that the variable is non-null on the relevant branch.
   - 4 × `deadcode.DeadStores` in `src/command/verify/verify.c:404, 426`, `src/db/db.c:656`, `src/storage/sftp/storage.c:1182`. Likely defensive-init redundancy from refactoring; verify each before removing.
   - 3 × `core.UndefinedBinaryOperatorResult` (uninitialized garbage) in `src/command/archive/push/file.c:200`, `src/command/backup/backup.c:380`, `src/command/restore/timeline.c:230`.
   - 2 × `core.uninitialized.Assign` in `src/command/restore/timeline.c:123`, `src/common/exec.c:252`.
   - 3 × `security.insecureAPI.strcpy` in `src/common/stackTrace.c:195`, `src/common/type/string.c:474`, `src/info/manifest/manifest.c:288` — bounds-checked strcpy usages flagged by CWE-119; replace with memcpy or suppress with rationale.
   - 1 × `core.NonNullParamChecker` in `src/common/type/buffer.c:76`.
   The `scan-build` job (same analyzer family) currently passes — the discrepancy means clang-tidy invokes the analyzer with broader defaults than scan-build's. Re-enable the `clang-analyzer-*` block in `.clang-tidy` after the 20 findings are resolved.

10. **cppcheck `style` category.** Currently disabled (only `warning,performance,portability` are enabled). The `style` category produced ~250 findings on the first run that compiled, dominated by `constParameterPointer`/`constVariablePointer`/`unknownEvaluationOrder` (false-positive on C99 compound literals). Re-engage `style` after a cleanup pass; meanwhile the `warning` category still catches null-deref, leak, format-string, and uninitialized-variable bugs.

11. **clang-tidy `cert-err33-c` (return-value-not-checked).** Temporarily disabled in `.clang-tidy`. Surfaced ~10 findings on calls to `printf`/`fprintf`/`close`/`dup2` etc. where the return value is intentionally ignored:
    - `src/command/exit.c:79-84` — exit-time IO cleanup (no recourse if these fail)
    - `src/common/error/error.c:354,355,432,451,482` — error-reporting paths (failing here means we cannot even tell the user what went wrong)
    - `src/command/info/info.c:1008` — likely similar
    The right fix per the rule is to annotate each call site with `(void)func(...)` to make the intent explicit. Re-enable `cert-err33-c` after that cleanup pass.

## How to add a new gate

1. Identify the production failure mode the gate prevents. State it in one sentence.
2. Pick the cheapest tool that catches that class of bug.
3. Make the gate fail-loudly: nonzero exit on finding, no silent skipping.
4. Document the gate in this file under "Gates and what each catches".
5. Where the gate produces output worth keeping (HTML reports, diffs), upload it as a workflow artifact on failure.
6. Add the workflow's branch filters: `main`, `**-ci`, `**-cig`. Do not add `eol`.

## How to suppress a finding

In order of preference:

1. Fix the finding.
2. If the finding is intentional and the tool offers an inline annotation (`// cppcheck-suppress`, `// NOLINT`), use that. Inline annotations stay close to the code, which means future readers will see them.
3. If the finding is a tool false positive that recurs, add an entry to the tool's project-level suppression file (e.g., `.cppcheck-suppress`) WITH A COMMENT explaining the suppression.
4. Never relax the workflow itself. Lowering `--error-exitcode`, removing a check from `.clang-tidy`, or weakening `WarningsAsErrors` defeats the gate's purpose. If a check is producing more noise than signal, that's a check-set decision, not a per-finding one — discuss it explicitly.
