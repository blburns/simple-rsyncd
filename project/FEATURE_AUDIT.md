# Simple-Rsyncd Feature Audit

**Date:** May 2026  
**Version:** 0.3.1 Beta  
**Purpose:** Implemented vs stubbed vs unverified — no marketing inflation.

---

## Executive Summary

| Area | Status | Notes |
|------|--------|-------|
| Packaging | ✅ Shipped | DEB, RPM, FreeBSD PKG, macOS PKG/DMG |
| Custom protocol | ✅ Working | Not native rsync wire format |
| Module file ops | ✅ Working | Fixed in v0.3.1 |
| Tests | ✅ 5/5 suites | Google Test + CTest |
| INI config | ✅ Working | Primary supported format |
| Password auth | ✅ Working | SHA-256 + plain-text legacy |
| TLS | ❌ Stub | v0.4.0 |
| Native rsync client | ❌ Unverified | v0.5.0 matrix |
| OAuth2 / Prometheus | ❌ None | Remove from sales language |

---

## 1. Protocol & Transfer

| Feature | Status | Evidence |
|---------|--------|----------|
| LIST / GET / PUT / DELETE / STAT | ✅ | `protocol.cpp`, `ProtocolHandler` |
| `key=value` arguments | ✅ | `parseArguments`, tests |
| `@RSYNCD` header lines | ✅ | Parser handles version prefix |
| Binary upload/download in session | ✅ | `session.cpp`, integration tests |
| Delta sync | ❌ | Not implemented |
| Checksums on wire | ❌ | Not implemented |
| Native rsync 3.x compatibility | ❌ | Not tested; custom format |

---

## 2. Module System

| Feature | Status | Evidence |
|---------|--------|----------|
| Path isolation / traversal block | ✅ | `resolvePath`, `isPathSafe` |
| listDirectory (files + dirs) | ✅ | `FileSystemModule`, ModuleTests |
| createDirectory / deleteDirectory | ✅ | Including nested paths |
| transferFile / receiveFile | ✅ | Integration tests |
| read-only / allow_delete / overwrite | ✅ | Config + enforcement |
| include/exclude patterns | ✅ | `ModuleConfig::isExcluded` |
| Script hooks (pre/post) | ⚠️ | Declared; lightly used |
| chroot per module | ❌ | Not gate-tested |

---

## 3. Configuration

| Feature | Status | Evidence |
|---------|--------|----------|
| INI parse + validate | ✅ | ConfigTests |
| JSON | ⚠️ | jsoncpp linked; verify before relying |
| YAML | ⚠️ | Optional dep; verify before relying |
| Env var substitution | ⚠️ | Claimed in docs; verify per deploy |
| Hot-reload | ⚠️ | Partial / experimental |

---

## 4. Security & Auth

| Feature | Status | Evidence |
|---------|--------|----------|
| Password file auth | ✅ | AuthTests |
| SHA-256 password hash | ✅ | `PasswordHasher` in auth.cpp |
| Plain-text password fallback | ⚠️ | Legacy compat — avoid in new deploys |
| Allow/deny user lists | ✅ | AuthTests |
| IP allow/deny in config | ⚠️ | Structures exist; live enforce → v0.4.0 |
| Rate limiting | ❌ | Config only |
| TLS / SSL | ❌ | `ssl_context.cpp` stub |
| OAuth2 | ❌ | |
| Public-key auth | ❌ | OpenSSL types present; flow incomplete |

---

## 5. Operations & Packaging

| Feature | Status | Evidence |
|---------|--------|----------|
| systemd unit (Linux) | ✅ | In package payloads |
| launchd plist (macOS) | ✅ | `/Library/LaunchDaemons` |
| `/etc/simple-rsyncd` config layout | ✅ | macOS PKG v0.3.0+ |
| Log rotation API | ⚠️ | Present; not soak-tested |
| Prometheus / metrics | ❌ | |
| Health check endpoint | ❌ | |

---

## 6. Testing

| Suite | Tests | Status |
|-------|-------|--------|
| ConfigTests | 7 | ✅ |
| ModuleTests | 12 | ✅ |
| ProtocolTests | 11 | ✅ |
| AuthTests | 5 | ✅ |
| IntegrationTests | 16 | ✅ |

**Gap:** Linux CI not recorded in release verification; network-level E2E against live daemon is manual.

---

## 7. Documentation Accuracy

| Document | v0.3.1 status |
|----------|---------------|
| `README.md` | ✅ Updated — Beta, implemented vs planned |
| `project/PRODUCTION_GATE.md` | ✅ Current |
| `project/HONEST_ASSESSMENT.md` | ✅ This pass |
| `project/FEATURE_AUDIT.md` | ✅ This pass |
| `docs/shared/user-guide/` | ⚠️ Header updated; full scrub ongoing |

---

## Summary Table

| Category | Completion | Beta-ready? |
|----------|------------|-------------|
| Infrastructure / packaging | ~95% | ✅ |
| Custom protocol + modules | ~85% | ✅ |
| Configuration (INI) | ~90% | ✅ |
| Authentication | ~70% | ✅ on trusted nets |
| Security (TLS, enforce ACLs) | ~25% | ❌ |
| Native rsync compatibility | 0% | ❌ |
| Observability | ~10% | ❌ |
| Documentation | ~80% | ✅ improving |

---

## Conclusion

**v0.3.1 Beta delivers what it claims:** packaged daemon, custom protocol, module file ops, passing tests.  
**It does not deliver:** production security, native rsync clients, or full feature parity with README claims from earlier versions.

Next gate: **v0.4.0** — see [PRODUCTION_GATE.md](PRODUCTION_GATE.md) and [ROADMAP_CHECKLIST.md](ROADMAP_CHECKLIST.md).

---

*Audit completed: May 2026*
