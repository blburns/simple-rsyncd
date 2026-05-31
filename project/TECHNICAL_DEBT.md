# Simple RSync Daemon — Technical Debt

**Date:** May 2026  
**Current version:** 0.3.1 Beta

---

## Overview

v0.2.0 MVP and v0.3.x packaging/beta work cleared **blocking** debt for controlled beta use. Remaining items are **security, compatibility, and ops** — tracked by gate in [PRODUCTION_GATE.md](PRODUCTION_GATE.md).

| Priority | Items | Target |
|----------|-------|--------|
| 🔴 High | TLS, enforced ACLs, privilege drop | v0.4.0 |
| 🟡 Medium | bcrypt/argon2, JSON/YAML hardening, Linux CI | v0.4.0 |
| 🟢 Lower | Delta sync, Prometheus, soak tests | v0.5.0 |

---

## 🔴 High — v0.4.0 (security gate)

### TLS / SSLContext

**Status:** Stub — `(void)` cert paths; no live handshake on accept.  
**Effort:** ~40–60 h  
**Actions:** Load cert/key/CA; TLS accept; cipher config; integration test with test cert.

### Runtime access control & rate limits

**Status:** Config parsed; not consistently enforced on live connections.  
**Effort:** ~20–30 h  
**Actions:** IP/CIDR check at accept; connection cap in daemon loop; tests.

### Privilege drop / chroot

**Status:** Documented; not gate-tested.  
**Effort:** ~15–25 h  
**Actions:** Drop root after bind (Linux/macOS); optional chroot per module with tests.

---

## 🟡 Medium — v0.4.0 / ongoing

### Password hashing upgrade

**Status:** SHA-256 + salt via OpenSSL; plain-text fallback for legacy entries.  
**Effort:** ~10–15 h  
**Actions:** bcrypt or argon2; migration tool; document SHA-256 limitation if kept.

### JSON / YAML configuration

**Status:** INI is supported; JSON/YAML need end-to-end verification.  
**Effort:** ~15–20 h each  
**Actions:** Parser tests; example configs in CI; error messages.

### Linux CI

**Status:** macOS `ctest` verified for v0.3.1; Linux not in release checklist.  
**Effort:** ~5–10 h  
**Actions:** GitHub Actions or Ansible VM matrix running `ctest` on push.

### Documentation drift

**Status:** README + `project/` updated May 2026; user guide partially stale.  
**Effort:** ~5–10 h  
**Actions:** Scrub `docs/shared/user-guide/`; remove native rsync examples until v0.5.0 matrix.

### Module header bloat

**Status:** `module.hpp` declares many helpers without implementations.  
**Effort:** ~20 h (incremental)  
**Actions:** Trim or implement; reduce dead API surface.

---

## 🟢 Lower — v0.5.0 (production gate)

### Native rsync compatibility

**Status:** Custom protocol only; no client matrix.  
**Effort:** 50–100+ h (depends on scope decision)  
**Actions:** Publish matrix or formally scope custom protocol only.

### Delta sync / checksums

**Status:** Not implemented.  
**Effort:** ~30–50 h  

### Observability

**Status:** No Prometheus endpoint; log rotation not soak-tested.  
**Effort:** ~20–40 h  

### Performance / soak

**Status:** No benchmark baseline or 24h soak script.  
**Effort:** ~15–25 h  

---

## ✅ Resolved (do not re-open without regression)

| Item | Resolved in |
|------|-------------|
| Module list/create/upload path bugs | v0.3.1 |
| Protocol `key=value` parsing | v0.3.1 |
| Integration test suite | v0.3.1 |
| Cross-platform packaging | v0.3.0 |
| macOS PKG `/etc/simple-rsyncd` layout | v0.3.0 |
| Duplicate source trees | v0.3.0 |
| CTest failing (40% → 100%) | v0.3.1 |

---

## Recommendations

1. **Do not** market v0.3.1 as production — debt is security and compatibility, not packaging.
2. **Prioritize v0.4.0** before any internet-facing deployment.
3. **Keep docs aligned** — update user guide when adding features, not after release.

---

*Last updated: May 2026*
