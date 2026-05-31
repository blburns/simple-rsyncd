#include "simple-rsyncd/security/ssl_context.hpp"

#include <openssl/err.h>
#include <openssl/x509.h>
#include <unistd.h>

#include <cstring>

namespace simple_rsyncd {

namespace {

std::string sslErrorString() {
    const unsigned long err = ERR_get_error();
    if (err == 0) {
        return "unknown OpenSSL error";
    }
    char buffer[256];
    ERR_error_string_n(err, buffer, sizeof(buffer));
    return std::string(buffer);
}

}  // namespace

SSLConnection::SSLConnection(SSL* ssl, int fd)
    : ssl_(ssl), fd_(fd), open_(ssl != nullptr && fd >= 0) {}

SSLConnection::~SSLConnection() {
    close();
}

SSLConnection::SSLConnection(SSLConnection&& other) noexcept
    : ssl_(other.ssl_), fd_(other.fd_), open_(other.open_) {
    other.ssl_ = nullptr;
    other.fd_ = -1;
    other.open_ = false;
}

SSLConnection& SSLConnection::operator=(SSLConnection&& other) noexcept {
    if (this != &other) {
        close();
        ssl_ = other.ssl_;
        fd_ = other.fd_;
        open_ = other.open_;
        other.ssl_ = nullptr;
        other.fd_ = -1;
        other.open_ = false;
    }
    return *this;
}

bool SSLConnection::isOpen() const {
    return open_;
}

ssize_t SSLConnection::read(void* buffer, size_t length) {
    if (!open_ || ssl_ == nullptr) {
        return -1;
    }
    const int result = SSL_read(ssl_, buffer, static_cast<int>(length));
    if (result <= 0) {
        const int err = SSL_get_error(ssl_, result);
        if (err == SSL_ERROR_ZERO_RETURN) {
            open_ = false;
            return 0;
        }
        return -1;
    }
    return result;
}

ssize_t SSLConnection::write(const void* buffer, size_t length) {
    if (!open_ || ssl_ == nullptr) {
        return -1;
    }
    const int result = SSL_write(ssl_, buffer, static_cast<int>(length));
    if (result <= 0) {
        return -1;
    }
    return result;
}

void SSLConnection::shutdown() {
    if (ssl_ != nullptr) {
        SSL_shutdown(ssl_);
    }
}

void SSLConnection::close() {
    if (ssl_ != nullptr) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    open_ = false;
}

SSLContext::SSLContext() : initialized_(false), ctx_(nullptr) {
    static bool openssl_initialized = false;
    if (!openssl_initialized) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        openssl_initialized = true;
    }
}

SSLContext::~SSLContext() {
    cleanup();
}

bool SSLContext::configureMinTlsVersion(const std::string& min_tls_version) {
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    if (min_tls_version == "1.3") {
        return SSL_CTX_set_min_proto_version(ctx_, TLS1_3_VERSION) == 1;
    }
    if (min_tls_version == "1.2") {
        return SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION) == 1;
    }
#endif
    // OpenSSL 1.1.1 fallback — disable SSLv3/TLS1.0/TLS1.1
    SSL_CTX_set_options(ctx_, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    return true;
}

bool SSLContext::loadCertificateChain(const std::string& cert_file, const std::string& key_file) {
    if (SSL_CTX_use_certificate_chain_file(ctx_, cert_file.c_str()) != 1) {
        last_error_ = "failed to load certificate: " + sslErrorString();
        return false;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx_, key_file.c_str(), SSL_FILETYPE_PEM) != 1) {
        last_error_ = "failed to load private key: " + sslErrorString();
        return false;
    }
    if (SSL_CTX_check_private_key(ctx_) != 1) {
        last_error_ = "private key does not match certificate";
        return false;
    }
    return true;
}

bool SSLContext::loadCaFile(const std::string& ca_file) {
    if (ca_file.empty()) {
        return true;
    }
    if (SSL_CTX_load_verify_locations(ctx_, ca_file.c_str(), nullptr) != 1) {
        last_error_ = "failed to load CA file: " + sslErrorString();
        return false;
    }
    return true;
}

bool SSLContext::initialize(const std::string& cert_file,
                            const std::string& key_file,
                            const std::string& ca_file,
                            const std::string& min_tls_version,
                            const std::string& cipher_suite) {
    cleanup();
    last_error_.clear();

    ctx_ = SSL_CTX_new(TLS_server_method());
    if (ctx_ == nullptr) {
        last_error_ = "SSL_CTX_new failed: " + sslErrorString();
        return false;
    }

    SSL_CTX_set_options(ctx_, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

    if (!configureMinTlsVersion(min_tls_version)) {
        last_error_ = "unsupported TLS version: " + min_tls_version;
        cleanup();
        return false;
    }

    if (!cipher_suite.empty()) {
        if (SSL_CTX_set_cipher_list(ctx_, cipher_suite.c_str()) != 1) {
            last_error_ = "invalid cipher suite: " + cipher_suite;
            cleanup();
            return false;
        }
    } else {
        if (SSL_CTX_set_cipher_list(ctx_, "HIGH:!aNULL:!MD5:!RC4") != 1) {
            last_error_ = "failed to set default cipher list";
            cleanup();
            return false;
        }
    }

    if (!loadCertificateChain(cert_file, key_file)) {
        cleanup();
        return false;
    }

    if (!loadCaFile(ca_file)) {
        cleanup();
        return false;
    }

    initialized_ = true;
    return true;
}

bool SSLContext::isInitialized() const {
    return initialized_;
}

void SSLContext::cleanup() {
    if (ctx_ != nullptr) {
        SSL_CTX_free(ctx_);
        ctx_ = nullptr;
    }
    initialized_ = false;
}

std::unique_ptr<SSLConnection> SSLContext::acceptConnection(int client_fd) {
    if (!initialized_ || ctx_ == nullptr || client_fd < 0) {
        last_error_ = "SSL context not initialized";
        return nullptr;
    }

    SSL* ssl = SSL_new(ctx_);
    if (ssl == nullptr) {
        last_error_ = "SSL_new failed: " + sslErrorString();
        return nullptr;
    }

    if (SSL_set_fd(ssl, client_fd) != 1) {
        last_error_ = "SSL_set_fd failed: " + sslErrorString();
        SSL_free(ssl);
        return nullptr;
    }

    if (SSL_accept(ssl) != 1) {
        last_error_ = "TLS handshake failed: " + sslErrorString();
        SSL_free(ssl);
        return nullptr;
    }

    return std::make_unique<SSLConnection>(ssl, client_fd);
}

}  // namespace simple_rsyncd
