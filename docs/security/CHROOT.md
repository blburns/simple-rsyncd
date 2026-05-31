# Optional chroot (v0.4.0)

## Configuration

```ini
[security]
chroot_enabled = true
chroot_directory = /var/lib/simple-rsyncd/chroot
drop_privileges = true
user = rsync
group = rsync
```

## Requirements

- **Root** required at daemon start to call `chroot(2)` and bind privileged ports
- Chroot directory must contain minimal runtime deps if modules live inside jail (policy depends on layout)
- Module paths in config must be valid **inside** the chroot after jailing

## Order of operations (daemon start)

1. Load configuration and validate modules
2. Bind listen socket
3. Enter chroot (if enabled)
4. Drop privileges to configured user/group
5. Accept connections

## Testing

Automated tests skip live chroot when not root. Validate manually:

```bash
sudo simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf
sudo simple-rsyncd start --config /etc/simple-rsyncd/rsyncd.conf
```

Confirm process runs as configured user (`ps`) and module paths resolve inside the jail.

## Risks

- Misconfigured chroot can prevent module access or daemon start
- OpenSSL and dynamic libraries may require files copied into jail on some platforms — test on target OS before production use
