#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <openssl/ssl.h>

namespace simple_rsyncd {

/**
 * @brief TLS connection wrapper for a connected client socket
 */
class SSLConnection {
public:
    SSLConnection(SSL* ssl, int fd);
    ~SSLConnection();

    SSLConnection(const SSLConnection&) = delete;
    SSLConnection& operator=(const SSLConnection&) = delete;
    SSLConnection(SSLConnection&& other) noexcept;
    SSLConnection& operator=(SSLConnection&& other) noexcept;

    bool isOpen() const;
    ssize_t read(void* buffer, size_t length);
    ssize_t write(const void* buffer, size_t length);
    void shutdown();
    void close();

    int fd() const { return fd_; }

private:
    SSL* ssl_;
    int fd_;
    bool open_;
};

/**
 * @brief Server TLS context (OpenSSL)
 */
class SSLContext {
public:
    SSLContext();
    ~SSLContext();

    SSLContext(const SSLContext&) = delete;
    SSLContext& operator=(const SSLContext&) = delete;

    bool initialize(const std::string& cert_file,
                      const std::string& key_file,
                      const std::string& ca_file = "",
                      const std::string& min_tls_version = "1.2",
                      const std::string& cipher_suite = "");

    bool isInitialized() const;
    void cleanup();

    std::unique_ptr<SSLConnection> acceptConnection(int client_fd);

    const std::string& lastError() const { return last_error_; }

private:
    bool initialized_;
    SSL_CTX* ctx_;
    std::string last_error_;

    bool configureMinTlsVersion(const std::string& min_tls_version);
    bool loadCertificateChain(const std::string& cert_file, const std::string& key_file);
    bool loadCaFile(const std::string& ca_file);
};

}  // namespace simple_rsyncd
