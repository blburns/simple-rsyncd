#pragma once

#include <string>

namespace simple_rsyncd {

/**
 * @brief Drop process privileges after binding privileged ports
 * @return true on success or when no drop requested
 */
bool dropProcessPrivileges(const std::string& user, const std::string& group);

/**
 * @brief Enter chroot jail (requires root, call before setuid)
 */
bool enterChroot(const std::string& path);

}  // namespace simple_rsyncd
