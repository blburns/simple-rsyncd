# Public key authentication

simple-rsyncd supports SSH public key authentication when `auth.method = public_key`.

## Key file format

Keys are loaded from `auth.public_key_file` in OpenSSH `authorized_keys` format:

```
deploy: ssh-ed25519 AAAA... comment@host
operator: ssh-rsa AAAA... ops@host
```

You may also use standard `authorized_keys` lines without a username prefix; the username is taken from the comment before `@`:

```
ssh-ed25519 AAAA... operator@host
```

Supported key types: `ssh-ed25519`, `ssh-rsa`, `rsa-sha2-256`, `rsa-sha2-512`, and `ecdsa-sha2-nistp256/384/521`.

## Session handshake

When public key auth is enabled, the client must authenticate before other commands:

```
AUTH <username> <challenge> signature=<signature>
```

- **username** — must have a key in the public key file
- **challenge** — nonce or token the client signs (same bytes on both sides)
- **signature** — base64-encoded SSH signature blob, or PEM output from `ssh-keygen -Y sign`

On success the server responds with `@RSYNCD: OK` and allows subsequent `LIST`, `GET`, etc.

## Signing the challenge

### Option A: OpenSSL / programmatic (raw challenge)

Sign the challenge bytes directly with Ed25519 (or RSA/ECDSA), then send the SSH wire signature blob (type + raw signature), base64-encoded.

### Option B: OpenSSH ssh-keygen (recommended for operators)

```bash
CHALLENGE="your-nonce-from-server-or-client"
printf '%s' "$CHALLENGE" | ssh-keygen -Y sign -f ~/.ssh/id_ed25519 -n simple-rsyncd > /tmp/sig.pem
```

Send the PEM block (or its base64 body) as the `signature=` value. The namespace **must** be `simple-rsyncd` (see `SshSignatureVerify::kDefaultNamespace`).

Verify locally before connecting:

```bash
printf '%s' "$CHALLENGE" | ssh-keygen -Y verify -f ~/.ssh/id_ed25519.pub \
  -I simple-rsyncd -n simple-rsyncd -s /tmp/sig.pem
```

## Configuration example

```json
{
  "auth": {
    "enabled": true,
    "method": "public_key",
    "public_key_file": "/etc/simple-rsyncd/authorized_keys"
  }
}
```

## Password auth (same AUTH command)

When `auth.method = password`:

```
AUTH <username> _ password=<secret>
```
