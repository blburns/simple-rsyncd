# Threat Model — simple-rsyncd v0.4.0

**Status:** Security-ready beta (Gate 2)  
**Last updated:** May 2026

## Assets

- Module filesystem data (files and directories under configured module roots)
- Credentials (password file hashes, optional TLS private keys)
- Daemon configuration (`/etc/simple-rsyncd/`)
- Network service availability (TCP bind port, default 873)

## Trust boundaries

| Boundary | Trust level |
|----------|-------------|
| Client → daemon (network) | **Untrusted** unless TLS + auth configured |
| Module path → host filesystem | **Semi-trusted** — constrained by module root, symlink policy |
| Operator → config files | **Trusted** |
| Package install → host | **Trusted** (DEB/RPM/PKG supply chain) |

## Security controls (v0.4.0)

- **TLS 1.2+** optional via `[ssl]` / `ssl.enabled` with cert/key loading
- **Password auth** with SHA-256 hashes; `reject_plaintext_passwords` skips legacy plain entries
- **IP/CIDR allow/deny** enforced at connection accept
- **Connection rate limiting** per client IP (minute/hour windows)
- **Privilege drop** after bind when running as root (`security.user` / `security.group`)
- **Optional chroot** before privilege drop (requires root)
- **Path hardening** — symlink escape blocked when `allow_symlinks=false`

## Known gaps (not v0.4.0)

- Native `rsync(1)` wire protocol not verified
- Public-key auth parsing exists; signature verification incomplete
- OAuth2 not implemented
- bcrypt/argon2 not used (SHA-256 + salt today)
- Bandwidth throttling not enforced
- No Prometheus / centralized audit pipeline

## Recommended deployment

1. Enable TLS for any non-localhost exposure
2. Set `reject_plaintext_passwords=true` and migrate password file (see [PASSWORD_MIGRATION.md](PASSWORD_MIGRATION.md))
3. Configure `access.allowed_networks` to minimum required CIDRs
4. Enable `rate_limit.enabled`
5. Run daemon as root only to bind port <1024, with immediate privilege drop to dedicated user

## Out of scope

- Protection against compromised host OS
- Side-channel attacks on TLS or hashing
- Multi-tenant isolation beyond module path separation
