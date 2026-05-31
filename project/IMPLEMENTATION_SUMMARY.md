# Implementation Summary

**Last updated:** May 2026  
**Current version:** 0.3.1 Beta

---

## v0.3.1 Beta (May 2026)

### Code fixes

- **Module path validation** — `isPathSafe` checks module-root containment without requiring the target path to exist (unblocks create, upload, list root).
- **Read/write permissions** — directories and module roots readable; write checks use nearest existing parent.
- **Protocol arguments** — `parseModule`, `parsePath`, `parseArguments` on direct command lines (no `@RSYNCD` header).
- **Empty path semantics** — `fileExists("")` / `directoryExists("")` return false.

### Quality

- CTest: **5/5 suites, 100% pass** (macOS build verified).
- GitHub release with five platform packages.

### Documentation

- README: Beta banner, implemented vs planned, custom protocol disclaimer.
- Internal project docs aligned to v0.3.1 (this pass).

---

## v0.3.0 (May 2026) — Packaging milestone

- Cross-platform `make static-package` (DEB, RPM, FreeBSD PKG, macOS PKG/DMG).
- macOS PKG: config under `/etc/simple-rsyncd`, CPack payload rebuild script.
- Consolidated source tree; duplicate `src/core/` removed.
- **Tests were still failing** — fixed in v0.3.1.

---

## v0.2.0 MVP (2025) — Core implementation

Completed once; still the foundation:

| Component | Status |
|-----------|--------|
| Custom protocol (LIST/GET/PUT/DELETE/STAT) | ✅ |
| File transfer in sessions | ✅ |
| INI configuration | ✅ |
| FileSystemModule | ✅ |
| Password authentication | ✅ |
| Network accept + sessions | ✅ |
| Google Test unit tests | ✅ |

Not part of MVP but added later: integration tests, SHA-256 hashing, packaging.

---

## Not yet implemented (by design — later gates)

| Feature | Target |
|---------|--------|
| TLS handshake | v0.4.0 |
| Enforced rate limits / IP ACLs | v0.4.0 |
| Privilege drop, chroot hardening | v0.4.0 |
| Native rsync client matrix | v0.5.0 |
| Prometheus / soak tests | v0.5.0 |

---

## Next steps

See [PRODUCTION_GATE.md](PRODUCTION_GATE.md) Gate 2 (v0.4.0).

---

*For historical v0.2.0 detail, see git history prior to v0.3.0.*
