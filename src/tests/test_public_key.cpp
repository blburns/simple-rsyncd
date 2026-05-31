/*
 * Copyright 2024 SimpleDaemons
 * Licensed under the Apache License, Version 2.0
 */

#include <gtest/gtest.h>
#include "simple-rsyncd/core/auth.hpp"
#include "simple-rsyncd/core/protocol.hpp"
#include "simple-rsyncd/core/session.hpp"
#include "simple-rsyncd/security/ssh_key.hpp"

#include <openssl/evp.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace simple_rsyncd {

namespace {

EVP_PKEY* generateEd25519Key() {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
    if (!ctx) {
        return nullptr;
    }
    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen_init(ctx) <= 0 ||
        EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return nullptr;
    }
    EVP_PKEY_CTX_free(ctx);
    return pkey;
}

std::string signEd25519(EVP_PKEY* pkey, const std::string& data) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return "";
    }

    std::string sig_raw;
    size_t sig_len = 0;
    if (EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey) != 1 ||
        EVP_DigestSign(ctx, nullptr, &sig_len,
                       reinterpret_cast<const unsigned char*>(data.data()), data.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    sig_raw.resize(sig_len);
    if (EVP_DigestSign(ctx,
                       reinterpret_cast<unsigned char*>(&sig_raw[0]), &sig_len,
                       reinterpret_cast<const unsigned char*>(data.data()), data.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    sig_raw.resize(sig_len);
    EVP_MD_CTX_free(ctx);
    return SshPublicKeyImport::buildSignatureBlob("ssh-ed25519", sig_raw);
}

PublicKeyInfo makeEd25519KeyInfo(EVP_PKEY* pkey) {
    unsigned char pub[64] = {};
    size_t pub_len = sizeof(pub);
    if (EVP_PKEY_get_raw_public_key(pkey, pub, &pub_len) != 1 || pub_len != 32) {
        return {};
    }

    PublicKeyInfo info;
    info.key_type = "ssh-ed25519";
    info.key_data = SshPublicKeyImport::buildEd25519KeyData(std::string(reinterpret_cast<char*>(pub), 32));
    info.comment = "test@localhost";
    return info;
}

}  // namespace

TEST(PublicKeyAuthTest, Ed25519VerifySignature) {
    EVP_PKEY* pkey = generateEd25519Key();
    ASSERT_NE(pkey, nullptr);

    PublicKeyInfo info = makeEd25519KeyInfo(pkey);
    ASSERT_FALSE(info.key_data.empty());

    const std::string challenge = "simple-rsyncd-auth-challenge-v1";
    const std::string signature = signEd25519(pkey, challenge);
    ASSERT_FALSE(signature.empty());

    EXPECT_TRUE(PublicKey::verifySignature(info, challenge, signature));
    EXPECT_FALSE(PublicKey::verifySignature(info, "wrong-challenge", signature));

    EVP_PKEY_free(pkey);
}

TEST(PublicKeyAuthTest, PublicKeyDatabaseAndManager) {
    EVP_PKEY* pkey = generateEd25519Key();
    ASSERT_NE(pkey, nullptr);

    PublicKeyInfo info = makeEd25519KeyInfo(pkey);
    const std::string challenge = "deploy-token-42";
    const std::string signature = signEd25519(pkey, challenge);

    const auto key_path = std::filesystem::temp_directory_path() / "simple_rsyncd_test_keys";
    {
        std::ofstream out(key_path);
        out << "deploy: ssh-ed25519 " << info.key_data << " deploy@host\n";
    }

    PublicKeyDatabase db(key_path.string());
    EXPECT_TRUE(db.userHasKeys("deploy"));
    EXPECT_TRUE(db.verifySignature("deploy", challenge, signature));
    EXPECT_FALSE(db.verifySignature("deploy", "tampered", signature));

    AuthConfig auth;
    auth.enabled = true;
    auth.method = "public_key";
    auth.public_key_file = key_path.string();

    AuthenticationManager mgr(auth);
    EXPECT_TRUE(mgr.authenticateUserWithKey("deploy", challenge, signature));
    EXPECT_FALSE(mgr.authenticateUserWithKey("deploy", challenge, "invalid"));

    EVP_PKEY_free(pkey);
    std::filesystem::remove(key_path);
}

TEST(PublicKeyAuthTest, StandardAuthorizedKeysFormat) {
    EVP_PKEY* pkey = generateEd25519Key();
    ASSERT_NE(pkey, nullptr);
    PublicKeyInfo info = makeEd25519KeyInfo(pkey);
    const std::string challenge = "nonce-abc";
    const std::string signature = signEd25519(pkey, challenge);

    const auto key_path = std::filesystem::temp_directory_path() / "simple_rsyncd_authkeys";
    {
        std::ofstream out(key_path);
        out << "ssh-ed25519 " << info.key_data << " operator@host\n";
    }

    PublicKeyDatabase db(key_path.string());
    EXPECT_TRUE(db.userHasKeys("operator"));
    EXPECT_TRUE(db.verifySignature("operator", challenge, signature));

    EVP_PKEY_free(pkey);
    std::filesystem::remove(key_path);
}

TEST(PublicKeyAuthTest, SshKeygenEd25519Fixture) {
    const auto dir = std::filesystem::temp_directory_path() / "simple_rsyncd_sshkeygen";
    std::filesystem::create_directories(dir);
    const std::string key_base = (dir / "id_test").string();

    const std::string gen_cmd = "ssh-keygen -t ed25519 -f '" + key_base + "' -N '' -q 2>/dev/null";
    if (std::system(gen_cmd.c_str()) != 0) {
        GTEST_SKIP() << "ssh-keygen not available";
    }

    std::ifstream pub_in(key_base + ".pub");
    ASSERT_TRUE(pub_in.is_open());
    std::string pub_line;
    ASSERT_TRUE(static_cast<bool>(std::getline(pub_in, pub_line)));

    auto key_info = PublicKey::parse(pub_line);
    ASSERT_TRUE(key_info.has_value());

    const std::string challenge = "ssh-keygen-fixture-challenge";
    const std::string sign_cmd = "printf '%s' '" + challenge +
                                 "' | ssh-keygen -Y sign -f '" + key_base +
                                 "' -n simple-rsyncd 2>/dev/null";
    FILE* pipe = popen(sign_cmd.c_str(), "r");
    if (!pipe) {
        GTEST_SKIP() << "ssh-keygen -Y sign not available";
    }

    std::string pem_sig;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        pem_sig += buffer;
    }
    pclose(pipe);

    if (pem_sig.find("BEGIN SSH SIGNATURE") == std::string::npos) {
        GTEST_SKIP() << "Could not produce signature with ssh-keygen -Y sign";
    }

    PublicKeyDatabase db;
    db.addKey("fixture", pub_line);
    EXPECT_TRUE(db.verifySignature("fixture", challenge, pem_sig));

    std::filesystem::remove_all(dir);
}

TEST(PublicKeyAuthTest, AuthProtocolParsing) {
    ProtocolParser parser;
    const std::string line = "AUTH deploy nonce-123 signature=AAAAbase64sig\n";
    ProtocolMessage msg = parser.parse(line);
    EXPECT_TRUE(msg.valid);
    EXPECT_EQ(msg.command, ProtocolCommand::AUTH);
    EXPECT_EQ(msg.module, "deploy");
    EXPECT_EQ(msg.path, "nonce-123");
    ASSERT_NE(msg.arguments.find("signature"), msg.arguments.end());
}

}  // namespace simple_rsyncd
