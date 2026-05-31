# Password file migration (v0.4.0)

## Format

```
username:sha256:<salt_hex>:<hash_hex>
```

Legacy plain-text lines (`username:password`) still work when `reject_plaintext_passwords=false` (default for backward compatibility).

## Enable hashed-only passwords

In `rsyncd.conf`:

```ini
[auth]
enabled = true
method = password
password_file = /etc/simple-rsyncd/passwd
reject_plaintext_passwords = true
```

Plain-text entries in the password file are **skipped** at load time when this flag is true.

## Migrate existing entries

Generate a hash with the daemon linked against OpenSSL (or any tool producing the same format):

```bash
# Example: hash password "secret" for user "backup"
# Use simple-rsyncd test helper or C++ PasswordHasher in a one-off tool.
```

Recommended approach for operators:

1. Copy `passwd` to `passwd.bak`
2. For each user, replace `user:plain` with `user:sha256:...` using a small script that calls OpenSSL SHA-256 with random salt matching `PasswordHasher` format in `auth.cpp`
3. Set `reject_plaintext_passwords=true`
4. Run `simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf`
5. Verify login before removing `passwd.bak`

## Limitations

- Storage uses **SHA-256 + salt**, not bcrypt/argon2. Suitable for beta/staging; plan upgrade for high-threat environments (v0.5.0+).
- Passwords in environment variables or process listings remain an operator concern.
