#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct bignum_st;
typedef struct bignum_st BIGNUM;

struct evp_pkey_st;
typedef struct evp_pkey_st EVP_PKEY;

namespace simple_rsyncd {

/**
 * @brief SSH wire encoding helpers (RFC 4251 style strings / mpint)
 */
class SshWireReader {
public:
    explicit SshWireReader(const std::string& blob);

    bool readString(std::string& out);
    bool readByte(std::uint8_t& out);
    bool readMpint(BIGNUM** out);
    bool empty() const;
    size_t remaining() const;

private:
    const std::uint8_t* data_;
    size_t size_;
    size_t offset_;
};

class SshPublicKeyImport {
public:
    /** Import SSH public key blob (base64-decoded .pub body) to OpenSSL EVP_PKEY. Caller frees with EVP_PKEY_free. */
    static EVP_PKEY* fromSshBlob(const std::string& decoded_blob);

    static std::string base64Decode(const std::string& encoded);
    static std::string base64Encode(const std::string& raw);

    /** Build base64 key_data field for ssh-ed25519 public keys (32-byte raw key). */
    static std::string buildEd25519KeyData(const std::string& raw32);

    /** Build SSH wire signature blob and return base64 encoding. */
    static std::string buildSignatureBlob(const std::string& sig_type, const std::string& sig_raw);
};

class SshSignatureVerify {
public:
    /** Default namespace for OpenSSH ssh-keygen -Y sign (-n flag). */
    static constexpr const char* kDefaultNamespace = "simple-rsyncd";

    /** Verify SSH-format signature (base64) over @p data using @p public_key blob (base64 key_data field). */
    static bool verify(const std::string& key_type,
                       const std::string& key_data_b64,
                       const std::string& data,
                       const std::string& signature_b64);

    /** Strip PEM armor from ssh-keygen -Y sign output; returns base64 body or input unchanged. */
    static std::string normalizeSignatureInput(const std::string& signature);
};

}  // namespace simple_rsyncd
