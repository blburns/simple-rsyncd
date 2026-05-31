#include "simple-rsyncd/security/privileges.hpp"

#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <cerrno>
#include <cstring>

namespace simple_rsyncd {

bool dropProcessPrivileges(const std::string& user, const std::string& group) {
    if (user.empty() && group.empty()) {
        return true;
    }

    if (geteuid() != 0) {
        // Not running as root — skip silently (dev/test environments)
        return true;
    }

    if (!group.empty()) {
        struct group* gr = getgrnam(group.c_str());
        if (!gr) {
            return false;
        }
        if (setgid(gr->gr_gid) != 0) {
            return false;
        }
    }

    if (!user.empty()) {
        struct passwd* pw = getpwnam(user.c_str());
        if (!pw) {
            return false;
        }
        if (setgroups(0, nullptr) != 0) {
            return false;
        }
        if (setuid(pw->pw_uid) != 0) {
            return false;
        }
    }

    return true;
}

bool enterChroot(const std::string& path) {
    if (path.empty()) {
        return true;
    }

    if (geteuid() != 0) {
        return false;
    }

    return chroot(path.c_str()) == 0 && chdir("/") == 0;
}

}  // namespace simple_rsyncd
