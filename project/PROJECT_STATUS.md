# Simple RSync Daemon — Project Status

**Last updated:** May 2026  
**Current version:** v0.3.1 Beta

---

## Overview

Lightweight C++ daemon with a **custom** file-sync protocol, module-based paths, and cross-platform packaging. **Beta** quality for controlled deployments — not general production.

| Release tier | Status |
|--------------|--------|
| Distribution | ✅ v0.3.0+ |
| Beta | ✅ v0.3.1 |
| Security-ready | ❌ v0.4.0 |
| Production | ❌ v0.5.0 |

Details: [PRODUCTION_GATE.md](PRODUCTION_GATE.md) · [HONEST_ASSESSMENT.md](HONEST_ASSESSMENT.md)

---

## Completed

### Build & packaging (~95%)

- CMake + GNUmakefile; `make static-package`
- DEB, RPM, FreeBSD PKG, macOS PKG/DMG on GitHub releases
- macOS: `/usr/local/bin`, `/etc/simple-rsyncd/`, LaunchDaemon

### Core daemon (beta scope)

- Custom protocol: LIST, GET, PUT, DELETE, STAT + `key=value` args
- FileSystemModule: list, transfer, delete, mkdir/rmdir, path checks
- INI configuration and validation
- Password auth (SHA-256 + legacy plain-text entries)
- Sessions and network accept loop
- **CTest 5/5** — Config, Module, Protocol, Auth, Integration

### Documentation (May 2026)

- README, `project/*` internal docs, user guide intro aligned to v0.3.1 Beta

---

## Not ready (by design)

- TLS handshake on the wire
- Native `rsync(1)` client compatibility
- Runtime rate limiting and IP enforcement
- OAuth2, Prometheus, production observability
- bcrypt/argon2 (SHA-256 today)

---

## Metrics

| Metric | Value |
|--------|-------|
| Status | v0.3.1 Beta |
| Platforms shipped | Linux, FreeBSD, macOS |
| CTest | 5/5 suites pass (macOS verified) |
| Release | https://github.com/SimpleDaemons/simple-rsyncd/releases/tag/v0.3.1 |

---

## Next: v0.4.0 (security gate)

1. Complete TLS in `ssl_context.cpp`
2. Enforce IP/CIDR and connection limits at accept
3. Privilege drop after bind
4. Linux CI running `ctest`
5. Threat model + security scan in CI

See [ROADMAP_CHECKLIST.md](ROADMAP_CHECKLIST.md) and [TECHNICAL_DEBT.md](TECHNICAL_DEBT.md).

---

## Version history (short)

| Version | Milestone |
|---------|-----------|
| v0.2.0 | MVP — core code |
| v0.3.0 | Packaging; tests failing |
| v0.3.1 | Beta — tests green, docs scrub, packages released |

---

## Product editions (roadmap)

Future commercial tiers (Production / Enterprise / Datacenter) remain **roadmap** — not shipped in v0.3.1. See `docs/production/`, `docs/enterprise/`, `docs/datacenter/` for planning docs only.

---

*For day-to-day accuracy, prefer [HONEST_ASSESSMENT.md](HONEST_ASSESSMENT.md) over this summary.*
