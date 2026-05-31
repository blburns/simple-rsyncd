/*
 * Copyright 2024 SimpleDaemons
 * Licensed under the Apache License, Version 2.0
 */

#include <gtest/gtest.h>
#include "simple-rsyncd/security/ssl_context.hpp"

#include <openssl/ssl.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

namespace simple_rsyncd {

class SSLContextTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        cert_dir_ = std::filesystem::temp_directory_path() / "simple_rsyncd_tls_test";
        std::filesystem::create_directories(cert_dir_);
        cert_file_ = (cert_dir_ / "server.crt").string();
        key_file_ = (cert_dir_ / "server.key").string();

        const std::string cmd = "openssl req -x509 -newkey rsa:2048 -keyout '" + key_file_ +
                                "' -out '" + cert_file_ +
                                "' -days 1 -nodes -subj '/CN=localhost' 2>/dev/null";
        ASSERT_EQ(std::system(cmd.c_str()), 0);
        ASSERT_TRUE(std::filesystem::exists(cert_file_));
        ASSERT_TRUE(std::filesystem::exists(key_file_));
    }

    static void TearDownTestSuite() {
        std::error_code ec;
        std::filesystem::remove_all(cert_dir_, ec);
    }

    static std::filesystem::path cert_dir_;
    static std::string cert_file_;
    static std::string key_file_;
};

std::filesystem::path SSLContextTest::cert_dir_;
std::string SSLContextTest::cert_file_;
std::string SSLContextTest::key_file_;

TEST_F(SSLContextTest, LoadsCertificateAndKey) {
    SSLContext ctx;
    EXPECT_TRUE(ctx.initialize(cert_file_, key_file_, "", "1.2", "HIGH:!aNULL:!MD5:!RC4"));
    EXPECT_TRUE(ctx.isInitialized());
    EXPECT_FALSE(ctx.initialize("", key_file_));
}

TEST_F(SSLContextTest, TlsHandshakeSucceeds) {
    SSLContext server_ctx;
    ASSERT_TRUE(server_ctx.initialize(cert_file_, key_file_));

    const int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(listen_fd, 0);

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    ASSERT_EQ(bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), 0);
    ASSERT_EQ(listen(listen_fd, 1), 0);

    socklen_t len = sizeof(addr);
    ASSERT_EQ(getsockname(listen_fd, reinterpret_cast<sockaddr*>(&addr), &len), 0);

    std::thread server_thread([&]() {
        const int accepted = accept(listen_fd, nullptr, nullptr);
        if (accepted < 0) {
            return;
        }
        auto conn = server_ctx.acceptConnection(accepted);
        if (conn && conn->isOpen()) {
            conn->write("OK", 2);
            conn->close();
        } else {
            close(accepted);
        }
    });

    const int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(client_fd, 0);
    ASSERT_EQ(connect(client_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), 0);

    SSL_CTX* client_ctx = SSL_CTX_new(TLS_client_method());
    ASSERT_NE(client_ctx, nullptr);
    SSL* ssl = SSL_new(client_ctx);
    ASSERT_NE(ssl, nullptr);
    SSL_set_fd(ssl, client_fd);
    ASSERT_EQ(SSL_connect(ssl), 1);

    char buffer[8] = {};
    const int n = SSL_read(ssl, buffer, sizeof(buffer));
    EXPECT_GT(n, 0);
    EXPECT_EQ(std::string(buffer, static_cast<size_t>(n)), "OK");

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(client_ctx);
    close(client_fd);

    server_thread.join();
    close(listen_fd);
}

}  // namespace simple_rsyncd
