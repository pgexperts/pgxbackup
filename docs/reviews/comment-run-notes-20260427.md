# Comment-run memorialization notes
Date: 2026-04-27T19:38:15Z
Pass: documentation pass via 8 parallel agents
Branch / commit: eol @ a8e52a015622835bb058ba0ecce73d41db09c52e

This file collects the findings from the documentation pass that were *memorialized but not fixed*, per the user's instruction. Findings are grouped by the area each agent covered. Cross-reference: `docs/reviews/review-20260427-193815.md` (the prior fresh-eyes review).

---

## Confirmed bugs (do not fix without the user's call)

1. **`src/common/error/error.c:391`** — Wrong-buffer null-terminator. `messageBuffer[sizeof(stackTraceBuffer) - 1] = '\0';` should target `stackTraceBuffer`. Buffers are equal-sized today, so the write does not overflow, but `stackTraceBuffer` is left without its guaranteed final NUL when the supplied stack trace ≥ 8191 bytes. Already in the prior code review. *Reported by agent 1; consistent with prior review.*

2. **`src/common/error/error.c:432`** (similar pattern, currently benign): `messageBufferTemp[sizeof(messageBuffer) - 1] = 0;` indexes `messageBufferTemp` using `sizeof(messageBuffer)`. Both are `ERROR_MESSAGE_BUFFER_SIZE` so equivalent today; if the buffer sizes ever diverge the bug surfaces. *Agent 1.*

3. **`src/common/memContext.c:766`** (apparent typo, currently dead): chained assignment to `contextAlloc->listSize` *inside* a struct initializer that overwrites `*contextAlloc`. Pre-overwrite write is dead. Looks accidental but is harmless today. *Agent 1.*

4. **`src/common/memContext.c:1209`** — `FUNCTION_TEST_PARAM(BOOL, false)` logs the literal `false` instead of the parameter `fatal`. Trace value will be wrong; no functional impact. *Agent 1.*

5. **`src/common/log.c:393`** — `snprintf(..., DRY_RUN_PREFIX)` uses a string literal as the format argument. Safe today (the literal contains no `%`), but `-Wformat-security` flags this; should be `"%s", DRY_RUN_PREFIX`. *Agent 1.*

---

## Security-relevant items (intentional / by design / out of scope)

6. **`src/common/crypto/cipherBlock.c:187`** — `EVP_BytesToKey(..., 1, ...)` with iteration count 1. Required for on-disk format and `openssl enc` wire-compatibility. The API is deprecated in OpenSSL 3+ and weak vs. PBKDF2/scrypt/Argon2; cannot be bumped without an on-disk format break. Already in prior code review. *Agent 5.*

7. **`src/common/crypto/common.c:84`** — `RAND_bytes(...)` return value not checked. A 0/-1 return on insufficient entropy would silently produce a non-random salt for cipherBlock encryption. Already in prior code review. *Agent 5.*

8. **`src/common/crypto/common.c` (cryptoInit)** — `OPENSSL_init_ssl()` return value not checked. *Agent 5.*

9. **`src/common/crypto/cipherBlock.c`** — Derived `key` and `initVector` stack buffers are not zeroed (`OPENSSL_cleanse`/`memset_s`) before frame teardown. Key material may persist in stack memory until overwritten by later activations. *Agent 5.*

10. **`src/common/crypto/hash.c`** (MD5 path) — Vendored MD5 used to sidestep FIPS-mode bans. Justified for S3 `Content-MD5` (required by AWS) but means MD5 is silently available wherever `hashTypeMd5` is requested. *Agent 5.*

11. **`src/common/io/tls/common.c:239-250`** — `SSL_CTX_set_min_proto_version(TLS1_2_VERSION)` and the configured TLS 1.2 / TLS 1.3 cipher allow-lists are only applied when `verifyPeer=true`. With `verifyPeer=false` (insecure mode) the connection runs against OpenSSL's compile-time defaults. Already in prior code review; agent added an inline `SECURITY NOTE` and matching extern doc. *Agent 4.*

12. **`src/common/io/tls/client.c:446`** — `SSL_CTX_set_options(this->context, SSL_OP_ALL)` enables OpenSSL's grab-bag of bug-compatibility workarounds. Set is not curated. Practical impact bounded by the verifyPeer-gated TLS-1.2 floor. Already in prior code review; agent added an inline warning. *Agent 4.*

13. **`src/common/io/tls/server.c:352`** — `SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE` is set *without* `SSL_VERIFY_FAIL_IF_NO_PEER_CERT`, by design. Handshake completes even without a client cert; authentication is decided post-handshake via `tlsServerAuth()` / `ioSessionAuthenticatedSet()`. Anything reading from the session must check `ioSessionAuthenticated()` before trusting the peer. *Agent 4 — flagged inline as an invariant.*

14. **`src/common/io/http/response.c:91-101`** — Unexpected-TLS-EOF tolerance is content-type-driven (XML/JSON only). Adding a non-self-validating content type to the allow-list would erode this truncation defense. *Agent 4 — flagged inline.*

15. **`src/common/type/xml.c:346`** — `xmlReadMemory(..., 0)` uses default flags. No explicit `XML_PARSE_NONET` or `XML_PARSE_NOENT`. Modern libxml2 defaults are safe, but explicit-is-better-than-implicit. Already in prior code review. *Agent 2.*

---

## Correctness & design concerns (subtle, mostly low impact)

16. **`src/common/type/json.c`** — `\uXXXX` escape validation diverges between `jsonReadSkipStr` (uses `isdigit`, accepts `香` and rejects `¯`) and `jsonReadStr` (requires `\u00XX` ASCII, accepts `¯`). `jsonValidate()` (which uses skip) and `jsonReadStr()` therefore disagree on the same input. Already in prior code review. *Agent 2.*

17. **`src/common/type/list.c`** — `comparatorDescList` uses a file-scope global to flip a comparator. Concurrent descending sort/find on different lists in the same *thread* would corrupt results. Not currently triggered because pgBackRest uses parallel workers via `fork()`, not threads. *Agent 2.*

18. **`src/common/type/stringList.c`** — `strLstMergeAnti` requires sorted inputs but produces wrong results silently if either input is unsorted (no assertion). Documented in the header but not enforced. *Agent 2.*

19. **`src/common/type/convert.c`** — `cvtZToTime` is TZ-sensitive when no offset is supplied. Documented inline but a comprehensive caller audit might find non-portable epoch values. *Agent 2.*

20. **`src/common/regExp.c`** (`regExpError`) — Calls `regerror()` with a NULL `regex_t *`. Permitted by glibc/musl in practice; POSIX leaves it undefined when given a non-format error code. Works for the codes seen here but is technically UB. *Agent 5.*

21. **`src/common/lock.c`** — `lockRead` with `param.remove=true` silently ignores `unlink` failure ("do not report failures" comment in code). In degraded-fs conditions, stale lock files can accumulate. *Agent 5.*

22. **`src/common/exec.c`** — `execOpen` partial-init: a `fork()` failure between pipe creation and child `execvp` leaves up to six pipe fds open in the parent until `execFreeResource` runs. No compensating cleanup on the partial-init path. *Agent 5.*

23. **`src/common/fork.c`** — `forkDetach` second-fork survivor does not check `chdir`/`close` errors. In an environment where root `/` is unreadable the daemonization succeeds but the daemon fails late. *Agent 5.*

24. **`src/command/backup/backup.c:1507-1574`** — Page-checksum failures only `LOG_WARN`, never fail the backup. README documents this as intentional ("detect early, before backups expire"). Agent 7 added an inline comment marking the policy. Already in prior code review. *Agent 7.*

25. **`src/command/backup/blockIncr.c`** — xxHash variant/bit-width and collision-rate analysis are not formally documented. Block filter uses `XXH3_128bits` truncated to a `checksumSize`-byte prefix (5..11 bytes per `backup.c::checksumSizeMap`). Whole-file SHA1 is the backstop for truncated-prefix collisions, but no statistical bound (vs. file/block count) is computed or documented. Already in prior code review. *Agent 7.*

26. **`src/command/backup/pageChecksum.c`** — All-zero-page detection uses `pd_upper==0` OR `pd_checksum==0` depending on header-check mode. Either heuristic could be defeated by a corrupted page that happens to have those bytes zero, but SHA1 file checksums catch any further drift. Treated as acceptable. *Agent 7.*

27. **`src/command/restore/restore.c`** — `pg_control` rename-last is the *only* crash-safety mechanism for partial restores; the manifest is intentionally written into PGDATA before any data files so a delta retry can pick up where it left off. *Agent 7 — flagged inline as an invariant.*

---

## Infrastructure / build / CI

28. **`.github/workflows/test.yml:5-13`** — Trigger filters (`integration`, `**-ci`, `**-cig`) do not include this fork's only branch (`eol`). No CI runs on push or PR to `eol`. Already in prior code review. *Agent 8 confirmed.*

---

## Items NOT covered by this pass (scope)

- `src/build/` (the codegen executable) — intentionally skipped; build-time only, no runtime risk.
- `doc/` — Perl documentation engine; orthogonal to the C codebase.
- `*.auto.c.inc` / `*.auto.h` — generated; agents were instructed never to edit.
- `test/` — agents touched no test code.
- A handful of small `.h` files (mostly in `command/` and `common/`) where existing one-line headers were judged adequate — listed file-by-file in each agent's individual report.

---

## Cross-cutting invariants documented during the pass

These are now in the code as comments (where inserted by an agent), but worth listing here as a quick index:

- **Memory contexts integrate with `errorHandlerSet`.** `memContextClean` runs from the registered handler list and unwinds long-lived contexts when an error escapes; `memContextKeep` survives that unwind.
- **The `*Pub` / `THIS_PUB` idiom is universal.** Public struct must be the first member; getters are inline; setters get a `Set` suffix.
- **Filter pipeline byte direction:** Read goes driver → filter[0] → ... → caller. Write goes caller → filter[0] → ... → driver. Place size/checksum filters at the end you want to measure.
- **NULL input = end-of-stream signal.** Every InOut filter must accept NULL to flush trailing state (gzip footer, AEAD tag, etc.).
- **`ioFilterResult` requires `ioFilterGroupClose`.** Calling for a result before close returns NULL.
- **`ioWriteFlush` and `ioWriteSeek` forbid filters.** Asserted via the `filterGroupSet` snapshot taken at open time.
- **Storage features form a partial order.** `storageFeaturePathSync`/`HardLink`/`SymLink` all require `storageFeaturePath`; the constructor enforces this.
- **`storage/iterator.c` recursion uses an explicit stack** so `expire` and `verify` can walk repos with millions of files without blowing the C stack.
- **Object-store backends do not implement `move`.** Callers must avoid it on S3/GCS/Azure; the command layer already does, via `storageFeaturePath` checks.
- **Storage backend feature set is wire-discovered for `remote`.** No client-side hard-coding — the remote backend transparently adopts whatever its peer's underlying driver advertises.
- **Server-side TLS authentication is post-handshake.** `tlsServerAuth()` / `ioSessionAuthenticated()` must be checked before trusting the peer (the handshake itself does not enforce client-cert presence).
