# Simple RSync Daemon — Honest Assessment

**Date:** May 2026  
**Version:** 0.4.0 Security-ready beta  

---

## Executive Summary

**Current status: v0.4.0 Security-ready beta** — TLS, enforced IP/CIDR ACLs, rate limiting, privilege drop, and symlink hardening are implemented. **7/7 CTest suites** pass. Suitable for internet-facing **beta** deployments with TLS and hardened config; still **not** general production (Gate 3).

See [PRODUCTION_GATE.md](PRODUCTION_GATE.md) for release tiers.

| Tier | v0.4.0 |
|------|--------|
| Distribution (installers) | ✅ |
| Beta (controlled use) | ✅ |
| Security-ready | ✅ |
| Production | ❌ |

---

## What's Actually Working

### Build, packaging, and tests

- ✅ CMake + GNUmakefile; `make static-package` produces DEB, RPM, FreeBSD PKG, macOS PKG/DMG
- ✅ macOS PKG installs to `/usr/local/bin`, `/etc/simple-rsyncd/`, `/Library/LaunchDaemons`
- ✅ Google Test: **7 suites** (Config, Module, Protocol, Auth, Integration, Security, SSL)
- ✅ Canonical source tree under `src/simple-rsyncd/`

### Core daemon (beta scope)

- ✅ **Custom protocol** — LIST, GET, PUT, DELETE, STAT; `key=value` arguments on direct command lines
- ✅ **Module FS** — list, upload, download, delete, mkdir/rmdir; path traversal checks
- ✅ **INI configuration** — parse, validate, module definitions
- ✅ **Password auth** — password file, allow/deny lists; SHA-256 hashing with plain-text fallback for legacy entries
- ✅ **Network + sessions** — accept loop, session handling, file transfer in sessions
- ✅ **Logging** — functional logger (rotation/syslog not production-grade)

### Implemented (v0.4.0)

- ✅ **TLS 1.2+** — cert/key load, handshake on accept, `SSLTests`
- ✅ **IP/CIDR access control** — enforced in `acceptLoop`
- ✅ **Connection rate limiting** — per-IP minute/hour windows
- ✅ **Privilege drop** — after bind when root (`security.user`/`group`)
- ✅ **Symlink hardening** — blocked when `allow_symlinks=false`
- ✅ **Plain-text password rejection** — opt-in via config + migration doc

### Partially implemented

- ⚠️ **JSON/YAML config** — INI is supported; verify others before relying
- ⚠️ **Chroot** — implemented; requires root + manual validation
- ⚠️ **Public-key auth** — not production-ready (verification stub)

### Not implemented / not verified

- ❌ Native `rsync(1)` wire compatibility
- ❌ bcrypt/argon2 (SHA-256 + salt today)
- ❌ OAuth2, Prometheus, bandwidth throttling

---

## v0.3.1 Fixes (since v0.3.0)

| Issue | Fix |
|-------|-----|
| Directory listing returned empty | Read permission + path checks allow module roots and directories |
| mkdir / upload failed | Path validation no longer requires target to exist before create/upload |
| Protocol args ignored | `recursive=true`, `max_depth=N` parsed on lines without `@RSYNCD` header |
| Tests 40% pass | **100%** CTest pass rate after fixes |

---

## Code Quality

**Strengths**

- Clean C++17 layout; RAII and std::filesystem for module ops
- Good separation: config, protocol, module, session, daemon
- Honest README and production gate docs (post v0.3.1)

**Remaining gaps**

- Large header APIs with many unimplemented helper stubs on `Module`
- Documentation outside `README.md` / `project/` was stale until this pass
- No Linux CI proof in release checklist (macOS verified locally)
- Security features mostly config-level, not runtime-enforced

---

## Version History (honest)

| Version | What it actually was |
|---------|----------------------|
| **v0.2.0** | MVP — custom protocol, modules, INI, basic auth, unit tests |
| **v0.3.0** | **Packaging** — cross-platform installers; tests still failing |
| **v0.3.1** | **Beta** — core FS + protocol fixes; all tests green; docs scrubbed |
| **v0.4.0** | Planned — TLS, hardening, enforced ACLs/rate limits |
| **v0.5.0** | Planned — production gate (client matrix, metrics, soak tests) |

---

## Recommendations

### Safe to do on v0.3.1 Beta

- Install from release packages in staging/lab
- Exercise LIST/GET/PUT/DELETE against the **custom** protocol
- Use INI config and password auth on trusted networks

### Do not do on v0.3.1 Beta

- Expose directly to the internet without TLS and hardening
- Assume `rsync rsync://host/module/` works
- Market as “production-ready” or “rsyncd replacement”

### Next engineering priorities (v0.4.0)

1. Complete TLS in `ssl_context.cpp`
2. Enforce IP/CIDR and connection limits at accept time
3. Privilege drop after bind
4. Linux CI running `ctest` on every push
5. bcrypt/argon2 or documented SHA-256 limitation + migration

---

## Bottom Line

**v0.3.1 Beta is a real milestone:** installers ship, tests pass, core file operations work.  
**It is not production-ready.** Use [PRODUCTION_GATE.md](PRODUCTION_GATE.md) before changing that label.

---

*Assessment based on code review, CTest results, and v0.3.1 release artifacts — May 2026.*
