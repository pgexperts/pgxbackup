# pgxbackup packaging design

**Status:** Draft for discussion. No spec or workflow files written yet.
**Audience:** Maintainers and downstream users planning a migration from upstream pgBackRest.
**Last updated:** 2026-04-30

## Context

Upstream pgBackRest never owned its own packaging — distribution went through the PostgreSQL Global Development Group (PGDG) at `apt.postgresql.org` and `yum.postgresql.org`. PGDG packages upstream pgBackRest, not this fork; the fork is starting packaging from scratch.

The fork's `main` branch ships:
- A clean rename from `pgbackrest` to `pgxbackup` for the binary, default paths (`/etc/pgxbackup`, `/var/lib/pgxbackup`, `/tmp/pgxbackup`, etc.), config file (`pgxbackup.conf`), and project metadata.
- A legacy probe at `/etc/pgbackrest.conf` so existing pgBackRest installations continue to be read out of the box (`src/config/parse.c:60`).
- On-disk format identifiers preserved (`backrest-version`, `backrest-checksum`, `INFO_SECTION_BACKREST`) so backups produced by upstream remain restorable by pgxbackup, and vice versa.

These choices set the constraints for packaging: **the package coexists with `pgbackrest`, does not replace it, and reads existing pgBackRest configs.**

## Goals

1. **One-command install** on the supported distros: `apt install pgxbackup`, `dnf install pgxbackup`.
2. **Coexistence** with existing `pgbackrest` packages from PGDG. Both can be installed; users migrate when ready.
3. **Reproducible** builds (already enforced by the `reproducibility.yml` CI gate).
4. **Signed** packages and repository metadata. No `--allow-insecure-repositories`.
5. **Fail-loud** publishing: a release does not go live until install-and-smoke against the target distro succeeds.
6. **One source of truth**: the same git tag produces every distro's package, with no per-distro source patches.

## Non-goals

- Container images (Docker, OCI). Separate concern; defer.
- Windows / macOS packaging. The tool runs on Linux only.
- 32-bit packages. The CI matrix tests 32-bit builds for ABI hygiene; we don't ship them.
- A `systemd` service unit. pgBackRest is invoked from `cron`, PostgreSQL's `archive_command`, or user-defined scripts — there is no daemon to manage.
- Backports for distros past their vendor EOL.

## Distribution targets

Initial matrix (matching the test.yml platform coverage so the same containers can be reused):

| Family | Version | Codename | Arch | Notes |
|---|---|---|---|---|
| Debian | 12 | bookworm | x86_64, aarch64 | Current stable |
| Debian | 13 | trixie | x86_64, aarch64 | Released 2025; current testing → stable |
| Ubuntu | 22.04 | jammy | x86_64, aarch64 | LTS, supported until 2027 |
| Ubuntu | 24.04 | noble | x86_64, aarch64 | LTS, supported until 2029 |
| Ubuntu | 26.04 | (TBD) | x86_64, aarch64 | LTS, when GA |
| RHEL family | 8 | n/a | x86_64, aarch64 | Maintenance support extends through 2029 |
| RHEL family | 9 | n/a | x86_64, aarch64 | Rocky/Alma/CentOS Stream pick up via the same RPM |
| RHEL family | 10 | n/a | x86_64, aarch64 | When GA |

Total at v1: 5 deb × 2 arch + 3 rpm × 2 arch = **16 package artifacts per release**.

**Explicitly out of v1:** Ubuntu 20.04 (focal — past LTS support window), Debian 11 (bullseye — also past primary support).

## Package contents and layout

### Common to all distros

```
/usr/bin/pgxbackup
/etc/pgxbackup/                         # owned, mode 750, root:root
/etc/pgxbackup/conf.d/                  # owned, mode 750, root:root
/var/lib/pgxbackup/                     # owned, mode 750, postgres:postgres
/var/log/pgxbackup/                     # owned, mode 750, postgres:postgres
/var/spool/pgxbackup/                   # owned, mode 750, postgres:postgres
/usr/share/doc/pgxbackup/copyright      # MIT license text
/usr/share/doc/pgxbackup/changelog.gz
/usr/share/man/man1/pgxbackup.1.gz      # generated from help.xml
```

### Debian/Ubuntu specifics

- `debian/control` declares `Depends: postgresql-common, libssl3, libxml2, libpq5, libyaml-0-2, liblz4-1, libzstd1, libbz2-1.0, libssh2-1, zlib1g` (versions floated from each distro's policy).
- `debian/postinst` creates the `postgres` group/user if absent (it almost always exists from `postgresql-common`), then chowns the `/var/{lib,log,spool}/pgxbackup` dirs.
- `debian/changelog` populated by a release script from the `Co-Authored-By`-stripped git log.
- **No `Conflicts:` against `pgbackrest`.** They coexist; users running both during a migration is the supported path.

### RHEL specifics

- `pgxbackup.spec` `BuildRequires: gcc, meson, ninja-build, openssl-devel, libxml2-devel, postgresql-devel, libyaml-devel, lz4-devel, libzstd-devel, bzip2-devel, libssh2-devel, zlib-devel`.
- `Requires:` matches the runtime split.
- `%post` does the same group/user / chown as Debian's postinst.
- **No `Conflicts:` against `pgbackrest`.** Same coexistence policy.

## Naming and conflict resolution

The package name is **`pgxbackup`**. It does **not** Provide/Replace/Conflict against the existing `pgbackrest` package from PGDG.

Rationale:
- The binary name is `pgxbackup`, the config dir is `/etc/pgxbackup`, the lock dir is `/tmp/pgxbackup` — there are no file-path collisions.
- Users in mid-migration may legitimately want both installed. `pgbackrest backup` continues to work via PGDG's package; `pgxbackup backup` runs the fork. Same backup repository can be read by both (on-disk format identifiers preserved).
- A `Conflicts:` would force users to commit to the migration before they could even test the fork. That's the wrong shape for a continuity-fork's adoption story.

When a user is fully migrated, `apt remove pgbackrest` or `dnf remove pgbackrest` is the manual cleanup step. A future `pgxbackup-pgbackrest-compat` metapackage that does Replaces+Conflicts is possible if there's demand, but is not in v1.

## Build pipeline

GitHub Actions, one job per (distro, arch) pair, driven by a matrix:

```yaml
name: packages
on:
  push:
    tags: ['v*']
  workflow_dispatch:    # for manual builds before a tag

jobs:
  build:
    strategy:
      matrix:
        target:
          - { distro: debian-12,  arch: amd64,    image: 'debian:12'         }
          - { distro: debian-12,  arch: arm64,    image: 'debian:12'         }
          - { distro: ubuntu-22,  arch: amd64,    image: 'ubuntu:22.04'      }
          # ... etc
          - { distro: rhel-9,     arch: amd64,    image: 'rockylinux:9'      }
          - { distro: rhel-9,     arch: arm64,    image: 'rockylinux:9'      }
    # ... build steps per family ...
```

Each job:
1. Checks out the tagged source.
2. Spins up a clean Docker container for the target distro.
3. Installs build deps and runs `meson setup --buildtype=release && ninja`.
4. Runs `dpkg-buildpackage -b` (deb) or `rpmbuild -bb` (rpm).
5. Runs install-and-smoke against a SECOND clean container of the same distro (catches packaging-vs-build environment skew).
6. Uploads the artifact as a workflow artifact (always) and attaches to the GitHub Release (on tags).

Build environments use the same Docker images that `test.yml` already exercises, so we get free reuse and no separate "is the build container right" question.

## Distribution channel

Three options were considered:

### Option A: GitHub Releases only
**Pros:** Zero infrastructure. Simple to set up.
**Cons:** Users `wget` and `dpkg -i` manually. No automatic updates. Hard to communicate security fixes.
**Verdict:** Insufficient for a continuity fork that needs to support production users.

### Option B: Hosted apt/yum repo (Cloudsmith, packagecloud)
**Pros:** Standard `apt update` / `dnf update` flow. Cloudsmith free tier offers 50 GB storage and 10 GB/mo bandwidth for OSS projects — sufficient for a tool with low download volume.
**Cons:** External dependency. Subject to provider's service availability and policy changes.
**Verdict:** **Recommended for v1.** Best UX/cost ratio.

### Option C: Self-hosted (S3 + aptly + createrepo_c)
**Pros:** Maximum control. No third-party dependency. Can be migrated from B without users noticing.
**Cons:** Operational ownership: GPG infra, repo metadata signing, monitoring.
**Verdict:** Phase 4+. Migration from B → C is straightforward (the package files are the same; only the metadata location changes; signing key continues to be the same project key).

**Decision:** Cloudsmith free tier for Phase 3 (initial public availability). Self-hosted (S3 + aptly + createrepo_c) is the planned end state once operational scale or policy changes warrant it. The migration is a non-event for users — the only change is the URL in their `sources.list` / `.repo` file.

## Signing

GPG-sign every package and every repo metadata file. Without signing, `apt update` and `dnf update` either fail or require `--allow-insecure-repositories` (which we should never instruct users to set).

### Key management

- One **project-owned release-signing master key**, GPG, RSA 4096 bits, **2-year expiration** with documented renewal procedure (next rotation date tracked in `docs/packaging-design.md` and surfaced as a calendar reminder ~3 months before expiry).
- Project-owned, not personal: the master key is the project's identity, not the maintainer's. If maintainership transitions, the key transitions with the project. Procedure documented in a separate `docs/packaging-key-rotation.md` (deliverable in Phase 3).
- Private key stored in:
  1. A project-controlled HSM-equivalent (hardware token in the project's lockbox; YubiKey-class device with PIN). Not the maintainer's personal hardware.
  2. An encrypted backup in a sealed envelope (printed paperkey) held in physical project storage.
  3. **NOT** in a GitHub Actions secret as the only copy. GH secrets are recoverable by GitHub Inc. and represent a single point of compromise.
- The signing key's **public** part: published on the project website, in the GitHub repo (`docs/pgxbackup-signing-key.asc`), and via keyserver upload.
- For CI signing: the master key is **never** in CI. A short-lived (1-year) **signing subkey** is exported with its passphrase to a GitHub Actions secret. Revoking the subkey does not invalidate previously signed releases and does not require master-key rotation.

### Rotation procedure (2-year cycle)

1. ~3 months before master expiry, generate a new master key.
2. Sign the new key with the old key (cross-signature).
3. Publish the new public key alongside the old; users see both as valid via the cross-signature for the overlap period.
4. New signing subkey, exported to CI; old subkey revoked.
5. After ~1 month overlap, the old master is allowed to expire.
6. The full procedure is rehearsed once before a real rotation is needed (Phase 4 deliverable).

### What gets signed

- Each `.deb` via `debsigs --sign=origin` (the `origin` role is what apt verifies on `apt-get install`).
- Each `.rpm` via `rpmsign --addsign`.
- The apt repo's `Release` file via `gpg --clearsign`.
- The yum repo's `repodata/repomd.xml` via `gpg --detach-sign`.

### Installation instructions

The README's "Install" section will direct users to:

```sh
# Debian/Ubuntu
curl -fsSL https://pgxbackup.example/pgxbackup-signing-key.asc | sudo tee /etc/apt/keyrings/pgxbackup.asc > /dev/null
echo 'deb [signed-by=/etc/apt/keyrings/pgxbackup.asc] https://pgxbackup.example/deb bookworm main' | sudo tee /etc/apt/sources.list.d/pgxbackup.list
sudo apt update && sudo apt install pgxbackup
```

(or yum equivalent)

## Versioning

The git tag is authoritative: tag `v2.59.0` produces packages versioned `2.59.0-1` (deb) and `2.59.0-1.el9` (rpm). The trailing `-1` (deb) or `-1.el9` (rpm) is the **package revision** — it bumps when the same source needs to be repackaged (typo in `debian/control`, missing dependency, etc.) without changing source.

Pre-release tags like `v2.59.0-rc1` produce `2.59.0~rc1-1` (deb) — note the `~` which sorts before `2.59.0` per Debian policy — and `2.59.0-0.1.rc1.el9` (rpm).

The current `meson.build` says `2.59.0dev`. The first real release will be `2.59.0` (drop the `dev` suffix, tag `v2.59.0`).

## Migration story for existing pgBackRest users

The migration is documented step-by-step (target file: `docs/migration-from-pgbackrest.md`, written when packaging is real):

1. **Verify backups still restore with current pgBackRest.** Don't migrate over a known-broken setup.
2. **Add the pgxbackup repo** (apt or yum) per the install instructions.
3. **Install pgxbackup** alongside the existing pgbackrest. Both binaries coexist.
4. **Test pgxbackup against the existing repo:**
   - `pgxbackup info` (reads `/etc/pgbackrest.conf` automatically via the legacy probe — confirm output matches `pgbackrest info`).
   - `pgxbackup verify` (read-only check of the existing repo).
5. **Update invocations** in:
   - PostgreSQL's `archive_command` and `restore_command`.
   - Any `cron` jobs.
   - Operations runbooks.
6. **Trial backup-and-restore cycle** with pgxbackup against a non-production target.
7. **Cut over** production. Remove the old pgbackrest package: `apt remove pgbackrest` / `dnf remove pgbackrest`.
8. **Optionally migrate config:** `mv /etc/pgbackrest.conf /etc/pgxbackup/pgxbackup.conf`. (The legacy probe makes this optional — pgxbackup reads the old path.)

Pin point: **on-disk format identifiers were intentionally preserved** so the existing repository is fully readable by pgxbackup with no migration step. This is the most critical property of the design — without it, users would face a much heavier cutover.

## Testing strategy

Per release, before publishing:

1. **Build smoke** (already in the build job): `pgxbackup version` works inside the build container.
2. **Install smoke** (separate job, fresh container of the same distro):
   - `dpkg -i` or `rpm -i` succeeds.
   - `pgxbackup version` returns the expected version string.
   - `pgxbackup help backup` renders.
3. **Repo install smoke** (after publishing to staging repo): `apt update && apt install -y pgxbackup` from the staging URL succeeds, then the binary smoke runs.
4. **Round-trip smoke** (most valuable, longest): in a container with PostgreSQL, run a full backup, then restore to a different cluster, then `pg_dumpall` both and diff.

The first three are mandatory gates. The fourth is recommended but not blocking (it's covered by the integration test matrix anyway).

## Phasing

**Phase 1 (v0.1, build-only):** Two-week effort.
- `debian/` and `rhel/` packaging directories with control / spec / rules / changelog templates.
- One workflow `package.yml` that builds Debian 12 amd64 only, on every push to `main`.
- Artifact uploaded to GitHub workflow run; not yet published anywhere.
- Validates the source can be packaged at all.

**Phase 2 (v0.2, matrix):** Two-week effort.
- Expand to the full matrix from the table above.
- Install-smoke job per artifact.
- Tag-driven attach to GitHub Release.

**Phase 3 (v0.3, signed repos):** Three-week effort.
- Generate signing key, store securely.
- Set up Cloudsmith account for the project.
- Add publishing step to the workflow on tagged releases.
- Add install instructions to README.

**Phase 4 (post-v1):** Documentation, migration guide, key rotation procedure documented.

## Decisions

The 10 design questions, locked in:

1. **Distribution channel:** Cloudsmith free tier for Phase 3, transitioning to self-hosted (S3 + aptly + createrepo_c) when operational scale or policy warrants. Same signing key throughout, so the migration is a sources-list URL change for users.
2. **Architectures at v1:** x86_64 + aarch64 from day one. Matrix doubles in size; CI cost is justified by aarch64's growing presence in PostgreSQL deployments (Graviton, Ampere).
3. **Distros at v1:** RHEL 8/9/10 + Debian 12/13 + Ubuntu 22.04/24.04/26.04(when GA). Ubuntu 20.04 and Debian 11 not in v1.
4. **Signing key:** project-owned (not personal). Held in project-controlled hardware token; backed up by printed paperkey in physical project storage.
5. **Signing key expiration:** 2-year rotation cycle with cross-signed handover. CI uses 1-year signing subkey; master never touches CI.
6. **Release cadence:** tag-driven. A `v*` tag triggers the build-and-publish pipeline. No time-based releases planned.
7. **Coexistence:** `pgxbackup` does NOT Conflict/Replace `pgbackrest`. Both coexist; users migrate when ready.
8. **systemd unit:** none. pgBackRest is not a daemon.
9. **Source tarballs:** yes. `git archive` on tag, attached to GitHub Release as `pgxbackup-<version>.tar.gz`.
10. **Per-build reproducibility check on packaged artifacts:** Phase 4 deliverable. The binary-level reproducibility gate (`reproducibility.yml`) already protects against most non-determinism; package-level adds confidence around `dh_*` and `rpmbuild` macro stability.

## Decisions to defer

- **Universe of distros**: more obscure distros (Alpine, Arch, FreeBSD ports) are a separate effort. Source builds will continue to work on those for now.
- **Auto-update integration**: hooks into unattended-upgrades, dnf-automatic, etc. — defer until users ask.
- **Per-package telemetry**: knowing how many users `apt install` the package — defer; not a goal.

---

**Next step after this design is reviewed:** turn the open questions into explicit decisions, then start phase 1 (Debian 12 amd64 build-only artifact). Phase 1's deliverable is small — a working `dpkg-buildpackage` invocation in CI — but proves out the entire pipeline shape.
