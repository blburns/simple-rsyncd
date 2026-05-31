# Production Gate Checklist

**Purpose:** Define objective criteria for when `simple-rsyncd` may be labeled *beta*, *security-ready*, or *production-ready*. This complements [ROADMAP_CHECKLIST.md](ROADMAP_CHECKLIST.md) with release gates tied to **tests, packaging, and honest documentation**.

**Baseline:** v0.3.1 (May 2026) — packaging complete; core tests green; **Beta** label.

---

## Release tier definitions

| Tier | Label | Intended use |
|------|--------|----------------|
| **Distribution** | Packaging release | CI artifacts, installers, cross-platform builds |
| **Beta** | Early adopters | Internal/staging; known clients; no SLA |
| **Security-ready** | Hardened beta | Internet-facing with TLS, auth, and audit |
| **Production** | General production | Replace rsyncd-like workloads with standard tooling |

**v0.3.1 today:** ✅ Distribution · ✅ Beta · ❌ Security-ready · ❌ Production

---

## Current test baseline (v0.3.1)

Run from build directory:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build . -j
ctest --output-on-failure
```

| CTest target | Status | Pass rate |
|--------------|--------|-----------|
| `ConfigTests` | ✅ Pass | 7/7 |
| `AuthTests` | ✅ Pass | 5/5 |
| `ModuleTests` | ✅ Pass | 12/12 |
| `ProtocolTests` | ✅ Pass | 11/11 |
| `IntegrationTests` | ✅ Pass | 16/16 |

**Overall:** 5/5 suites green · **100% CTest pass rate**

### Fixes in v0.3.1 (Gate 1 partial)

| Area | Fix |
|------|-----|
| Module FS | Path validation allows create/list/upload without pre-existing target; directory read permission |
| Protocol | `key=value` arguments parsed on direct command lines (no `@RSYNCD` header) |
| Upload | `transferFile` no longer blocked by existence-only path checks |

---

## Gate 1 — v0.3.1 · Stabilization (beta gate)

**Goal:** Trustworthy core file operations and green tests. Suitable for **controlled beta** deployments.

**Target label:** *Beta — not a rsyncd drop-in*

**Status:** ✅ **Complete** (v0.3.1)

### Required — code & tests

- [x] **All CTest suites pass** (`ctest` 5/5 green on Linux and macOS)
- [x] Fix module directory listing (`listDirectory` returns populated `FileListing`)
- [x] Fix module `createDirectory` / `deleteDirectory` (including nested + recursive)
- [x] Fix protocol argument parsing (`key=value` tokens, e.g. `recursive=true`, `max_depth=3`)
- [x] Fix upload/transfer path in `ProtocolHandler` (integration upload scenarios)
- [x] Add regression tests for the above (keep existing tests; do not weaken assertions)

### Required — documentation honesty

- [x] README **Features** section matches implemented code (remove or mark *planned* for OAuth2, Prometheus, YAML hot-reload, rate limiting if not done)
- [x] Document that protocol is **simplified** — not full native `rsync(1)` / `rsyncd` wire compatibility
- [ ] Update [PROJECT_STATUS.md](PROJECT_STATUS.md), [HONEST_ASSESSMENT.md](HONEST_ASSESSMENT.md), [FEATURE_AUDIT.md](FEATURE_AUDIT.md) to v0.3.x reality
- [ ] User docs: replace “works with `rsync rsync://…`” claims unless verified with a named client/version matrix

### Required — packaging (already largely done for v0.3.0)

- [x] `make static-package` produces DEB/RPM/FreeBSD PKG/macOS DMG+PKG
- [x] macOS PKG installs to `/usr/local/bin`, `/etc/simple-rsyncd/`, `/Library/LaunchDaemons`
- [x] CPack `Contents/` leak fixed via [packaging/macos/pkg/rebuild-from-cpack.sh](../packaging/macos/pkg/rebuild-from-cpack.sh)
- [ ] macOS DMG payload aligned with PKG paths (`/etc/simple-rsyncd`, not `/usr/local/etc/…`)
- [ ] Service smoke test documented: install package → start daemon → one LIST/GET/PUT cycle

### Verification commands

```bash
# Tests
cd build && ctest --output-on-failure

# Module/protocol spot checks
./src/tests/test_module
./src/tests/test_protocol
./src/tests/test_integration

# Package smoke (macOS example)
sudo installer -pkg dist/simple-rsyncd-0.3.0-macos-intel.pkg -target /
ls /etc/simple-rsyncd/examples/
/usr/local/bin/simple-rsyncd --help   # or test --config /etc/simple-rsyncd/rsyncd.conf.example
```

### Exit criteria

All **Required** boxes checked · CI runs `ctest` on push · GitHub release notes say **Beta**

---

## Gate 2 — v0.4.0 · Security-ready

**Goal:** Safe for **internet-facing** or multi-tenant use with encryption and hardened OS integration.

**Target label:** *Security-ready beta*

Maps to [ROADMAP_CHECKLIST.md § v0.4.0](ROADMAP_CHECKLIST.md#-v040---security-release-checklist).

### Required — TLS & crypto

- [ ] `SSLContext` loads cert/key/CA (no stub `(void)cert_file`)
- [ ] TLS handshake on accept when `ssl_enabled` in config
- [ ] TLS version and cipher suite configurable; insecure defaults disabled
- [ ] Integration test: TLS connection succeeds with test cert

### Required — authentication & secrets

- [ ] Password file: **no plain-text** passwords in new installs (migration path documented)
- [ ] Prefer bcrypt or argon2 over salted SHA-256 for password storage (or document SHA-256 limitation)
- [ ] Public-key auth verified with real SSH key fixtures (if advertised)
- [ ] OAuth2 **removed from README** or fully implemented — no middle ground

### Required — access control & hardening

- [ ] IP/CIDR allow/deny enforced on live connections (not config-only)
- [ ] Rate limiting: connection limits enforced (config exists today; logic missing)
- [ ] Privilege drop: daemon runs as non-root after bind (Linux + macOS)
- [ ] Optional chroot documented and tested per module
- [ ] Path security: symlink/hardlink traversal cases covered by tests

### Required — quality

- [ ] Gate 1 (v0.3.1) still green
- [ ] Security scan in CI (cppcheck minimum; optional valgrind/ASan build)
- [ ] Threat model doc (1–2 pages): assets, trust boundaries, known gaps

### Exit criteria

Gate 1 + all **Required** v0.4.0 items · Release notes list TLS and hardening · Still not claimed as full rsyncd replacement unless Gate 3 client matrix passes

---

## Gate 3 — v0.5.0 · Production-ready

**Goal:** **General production** use with observability, performance evidence, and client compatibility.

Maps to [ROADMAP_CHECKLIST.md § v0.5.0](ROADMAP_CHECKLIST.md#-v050---production-ready-release-checklist).

### Required — compatibility

- [ ] **Client compatibility matrix** published and tested:

  | Client | Version | LIST | GET | PUT | DELETE | Notes |
  |--------|---------|------|-----|-----|--------|-------|
  | simple-rsyncd test client | — | | | | | |
  | rsync | 3.x | | | | | Only if truly supported |
  | … | | | | | | |

- [ ] Decision documented: **custom protocol** vs **native rsyncd** — README aligned with decision
- [ ] Delta sync / rolling checksum: implemented **or** explicitly out of scope for v1.0

### Required — operations

- [ ] systemd / launchd units pass `--config` and restart on failure
- [ ] Log rotation works under load (not just API present)
- [ ] Health/metrics: Prometheus endpoint **or** documented alternative
- [ ] Upgrade path documented (package upgrade preserves `/etc/simple-rsyncd/`)

### Required — quality & performance

- [ ] Unit test coverage ≥ 80% on core paths (module, protocol, session, auth)
- [ ] Integration tests cover: upload, download, delete, list, auth failure, TLS (if Gate 2)
- [ ] Benchmark baseline published (throughput MB/s, concurrent sessions)
- [ ] 24h soak test script or CI nightly (no leaks, no handle growth)

### Required — release process

- [ ] CI builds + tests on: Debian, RHEL-family, FreeBSD, macOS (match package matrix)
- [ ] Signed/tag reproducible release artifacts
- [ ] SECURITY.md + supported versions policy

### Exit criteria

Gates 1–2 green · Gate 3 **Required** complete · GitHub release labeled **Production** · No known P0/P1 open issues

---

## Documentation alignment checklist

Use before any release marketed beyond *packaging*:

| Claim in README/docs | Verify in code | v0.3.0 actual |
|----------------------|----------------|---------------|
| SSL/TLS encryption | `ssl_context.cpp` real TLS | ❌ Stub |
| OAuth2 | auth flow exists | ❌ Not implemented |
| Prometheus / metrics | HTTP/metrics endpoint | ❌ Not implemented |
| YAML config + hot-reload | parser + watcher | ⚠️ Partial / verify |
| JSON config | JSON parser wired | ⚠️ Verify end-to-end |
| Rate limiting | enforced in daemon loop | ❌ Config only |
| `rsync rsync://…` client | tested matrix | ❌ Not verified |
| Password hashing | `PasswordHasher` | ✅ SHA-256 + salt |
| Cross-platform packages | `make static-package` | ✅ |

Action: either **implement**, **remove**, or **mark planned** for every ❌/⚠️ row before beta/production labels.

---

## Recommended version labeling

| Version | When to ship | GitHub pre-release? |
|---------|--------------|---------------------|
| **v0.3.0** | Packaging milestone | Optional |
| **v0.3.1** | Gate 1 complete (current) | Yes — *Beta* |
| **v0.4.0** | Gate 2 complete | Yes — *Security preview* |
| **v0.5.0** | Gate 3 complete | No — *Production* |

---

## Suggested work order (next 2–4 weeks)

1. **Fix module FS** (unblocks 3 module + 3 integration tests)
2. **Fix protocol argument parsing** (unblocks 2 tests)
3. **Fix upload transfer path** (unblocks 2 integration tests)
4. **Run full `ctest` on Linux CI** — macOS + Debian VMs from Ansible inventory
5. **README/doc scrub** — same PR or immediately after tests green
6. **Rebuild + re-release v0.3.1** with *Beta* in release notes

---

## Quick reference links

- [ROADMAP_CHECKLIST.md](ROADMAP_CHECKLIST.md) — full task trees
- [TECHNICAL_DEBT.md](TECHNICAL_DEBT.md) — known debt items
- [HONEST_ASSESSMENT.md](HONEST_ASSESSMENT.md) — implementation vs marketing
- Failing tests: `src/tests/test_module.cpp`, `test_protocol.cpp`, `test_integration.cpp`

---

*Last updated: May 2026 · v0.3.1 Beta — all CTest suites passing*
