#include "simple-rsyncd/security/ssh_key.hpp"

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include <openssl/sha.h>

#include <cstring>

namespace simple_rsyncd {

namespace {

constexpr char kMagicV0[] = "SSH SIG";
constexpr char kMagicV1[] = "SSHSIG";

void appendWireString(std::string& out, const std::string& value) {
    const std::uint32_t len = static_cast<std::uint32_t>(value.size());
    out.push_back(static_cast<char>((len >> 24) & 0xFF));
    out.push_back(static_cast<char>((len >> 16) & 0xFF));
    out.push_back(static_cast<char>((len >> 8) & 0xFF));
    out.push_back(static_cast<char>(len & 0xFF));
    out.append(value);
}

std::uint32_t readBe32(const std::uint8_t* p) {
    return (static_cast<std::uint32_t>(p[0]) << 24) |
           (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) |
           static_cast<std::uint32_t>(p[3]);
}

int ecNidFromSshType(const std::string& key_type) {
    if (key_type == "ecdsa-sha2-nistp256") {
        return NID_X9_62_prime256v1;
    }
    if (key_type == "ecdsa-sha2-nistp384") {
        return NID_secp384r1;
    }
    if (key_type == "ecdsa-sha2-nistp521") {
        return NID_secp521r1;
    }
    return NID_undef;
}

const EVP_MD* mdFromSshSigType(const std::string& sig_type) {
    if (sig_type == "rsa-sha2-256") {
        return EVP_sha256();
    }
    if (sig_type == "rsa-sha2-512") {
        return EVP_sha512();
    }
    if (sig_type == "ssh-rsa") {
        return EVP_sha1();
    }
    return nullptr;
}

EVP_PKEY* pkeyFromRsa(BIGNUM* n, BIGNUM* e) {
    RSA* rsa = RSA_new();
    if (!rsa) {
        return nullptr;
    }
    if (RSA_set0_key(rsa, n, e, nullptr) != 1) {
        RSA_free(rsa);
        return nullptr;
    }

    EVP_PKEY* pkey = EVP_PKEY_new();
    if (!pkey) {
        RSA_free(rsa);
        return nullptr;
    }
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    if (EVP_PKEY_assign(pkey, EVP_PKEY_RSA, rsa) != 1) {
#else
    if (EVP_PKEY_set1_RSA(pkey, rsa) != 1) {
#endif
        EVP_PKEY_free(pkey);
        RSA_free(rsa);
        return nullptr;
    }
    RSA_free(rsa);
    return pkey;
}

EVP_PKEY* pkeyFromEd25519(const std::string& raw32) {
    if (raw32.size() != 32) {
        return nullptr;
    }

#if OPENSSL_VERSION_NUMBER >= 0x10200000L
    return EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr,
                                       reinterpret_cast<const unsigned char*>(raw32.data()),
                                       raw32.size());
#else
    (void)raw32;
    return nullptr;
#endif
}

EVP_PKEY* pkeyFromEc(const std::string& key_type, const std::string& curve_id, const std::string& q_bytes) {
    const int nid = ecNidFromSshType(key_type);
    if (nid == NID_undef) {
        return nullptr;
    }

    EC_KEY* ec = EC_KEY_new_by_curve_name(nid);
    if (!ec) {
        return nullptr;
    }

    const EC_GROUP* group = EC_KEY_get0_group(ec);
    EC_POINT* point = EC_POINT_new(group);
    if (!point) {
        EC_KEY_free(ec);
        return nullptr;
    }

    if (EC_POINT_oct2point(group, point,
                           reinterpret_cast<const unsigned char*>(q_bytes.data()),
                           q_bytes.size(), nullptr) != 1) {
        EC_POINT_free(point);
        EC_KEY_free(ec);
        return nullptr;
    }

    if (EC_KEY_set_public_key(ec, point) != 1) {
        EC_POINT_free(point);
        EC_KEY_free(ec);
        return nullptr;
    }
    EC_POINT_free(point);

    EVP_PKEY* pkey = EVP_PKEY_new();
    if (!pkey) {
        EC_KEY_free(ec);
        return nullptr;
    }
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    if (EVP_PKEY_assign(pkey, EVP_PKEY_EC, ec) != 1) {
#else
    if (EVP_PKEY_set1_EC_KEY(pkey, ec) != 1) {
#endif
        EVP_PKEY_free(pkey);
        EC_KEY_free(ec);
        return nullptr;
    }
    EC_KEY_free(ec);
    return pkey;
}

bool digestVerify(EVP_PKEY* pkey, const EVP_MD* md, const std::string& data, const std::string& sig_raw) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return false;
    }

    bool ok = false;
    if (EVP_DigestVerifyInit(ctx, nullptr, md, nullptr, pkey) == 1 &&
        EVP_DigestVerify(ctx,
                         reinterpret_cast<const unsigned char*>(sig_raw.data()), sig_raw.size(),
                         reinterpret_cast<const unsigned char*>(data.data()), data.size()) == 1) {
        ok = true;
    }

    EVP_MD_CTX_free(ctx);
    return ok;
}

bool ed25519Verify(EVP_PKEY* pkey, const std::string& data, const std::string& sig_raw) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return false;
    }

    bool ok = false;
    if (EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey) == 1 &&
        EVP_DigestVerify(ctx,
                         reinterpret_cast<const unsigned char*>(sig_raw.data()), sig_raw.size(),
                         reinterpret_cast<const unsigned char*>(data.data()), data.size()) == 1) {
        ok = true;
    }

    EVP_MD_CTX_free(ctx);
    return ok;
}

bool verifyInnerSignature(EVP_PKEY* pkey,
                          const std::string& key_type,
                          const std::string& sig_type,
                          const std::string& sig_raw,
                          const std::string& signed_data) {
    if (key_type == "ssh-ed25519" || sig_type == "ssh-ed25519") {
        return ed25519Verify(pkey, signed_data, sig_raw);
    }
    if (key_type == "ssh-rsa" || sig_type == "ssh-rsa" ||
        sig_type == "rsa-sha2-256" || sig_type == "rsa-sha2-512") {
        const EVP_MD* md = mdFromSshSigType(sig_type);
        if (!md) {
            md = EVP_sha256();
        }
        if (digestVerify(pkey, md, signed_data, sig_raw)) {
            return true;
        }
        if (sig_type != "ssh-rsa") {
            return digestVerify(pkey, EVP_sha1(), signed_data, sig_raw);
        }
        return false;
    }
    if (key_type.rfind("ecdsa-sha2-", 0) == 0) {
        const EVP_MD* md = EVP_sha256();
        if (sig_type.find("nistp384") != std::string::npos) {
            md = EVP_sha384();
        } else if (sig_type.find("nistp521") != std::string::npos) {
            md = EVP_sha512();
        }
        return digestVerify(pkey, md, signed_data, sig_raw);
    }
    return false;
}

std::string hashMessage(const std::string& hash_alg, const std::string& message) {
    if (hash_alg == "sha512") {
        unsigned char digest[SHA512_DIGEST_LENGTH];
        SHA512(reinterpret_cast<const unsigned char*>(message.data()), message.size(), digest);
        return std::string(reinterpret_cast<char*>(digest), SHA512_DIGEST_LENGTH);
    }
    if (hash_alg == "sha256") {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(message.data()), message.size(), digest);
        return std::string(reinterpret_cast<char*>(digest), SHA256_DIGEST_LENGTH);
    }
    return message;
}

std::string buildOpenSshSignedDataV1(const std::string& namespace_name,
                                     const std::string& hash_alg,
                                     const std::string& message) {
    std::string out;
    out.append(kMagicV1, sizeof(kMagicV1) - 1);
    appendWireString(out, namespace_name);
    appendWireString(out, "");
    appendWireString(out, hash_alg);
    appendWireString(out, hashMessage(hash_alg, message));
    return out;
}

std::string buildOpenSshSignedDataV0(const std::string& namespace_name,
                                     const std::string& hash_alg,
                                     const std::string& message) {
    std::string out;
    out.append(kMagicV0, sizeof(kMagicV0) - 1);
    appendWireString(out, namespace_name);
    out.push_back('\0');
    appendWireString(out, hash_alg);
    appendWireString(out, hashMessage(hash_alg, message));
    return out;
}

bool parseSshSigEnvelope(const std::string& sig_blob,
                         std::uint32_t& version,
                         std::string& namespace_name,
                         std::string& hash_alg,
                         std::string& sig_type,
                         std::string& sig_raw) {
    static constexpr char kMagicV0[] = "SSH SIG";
    static constexpr char kMagicV1[] = "SSHSIG";

    std::size_t header_len = 0;
    if (sig_blob.size() >= sizeof(kMagicV1) - 1 &&
        sig_blob.compare(0, sizeof(kMagicV1) - 1, kMagicV1) == 0) {
        header_len = sizeof(kMagicV1) - 1;
    } else if (sig_blob.size() >= sizeof(kMagicV0) - 1 &&
               sig_blob.compare(0, sizeof(kMagicV0) - 1, kMagicV0) == 0) {
        header_len = sizeof(kMagicV0) - 1;
    } else {
        return false;
    }

    if (sig_blob.size() < header_len + 4) {
        return false;
    }

    version = readBe32(reinterpret_cast<const std::uint8_t*>(sig_blob.data() + header_len));
    const std::string envelope_body = sig_blob.substr(header_len + 4);
    SshWireReader reader(envelope_body);

    std::string embedded_pubkey;
    std::string reserved_string;
    if (!reader.readString(embedded_pubkey) || !reader.readString(namespace_name)) {
        return false;
    }

    if (version >= 1) {
        if (!reader.readString(reserved_string)) {
            return false;
        }
    } else {
        std::uint8_t reserved_byte = 0;
        if (!reader.readByte(reserved_byte) || reserved_byte != 0) {
            return false;
        }
    }

    std::string inner_sig_blob;
    if (!reader.readString(hash_alg) || !reader.readString(inner_sig_blob)) {
        return false;
    }

    SshWireReader inner(inner_sig_blob);
    return inner.readString(sig_type) && inner.readString(sig_raw);
}

}  // namespace

SshWireReader::SshWireReader(const std::string& blob)
    : data_(reinterpret_cast<const std::uint8_t*>(blob.data()))
    , size_(blob.size())
    , offset_(0) {}

bool SshWireReader::readString(std::string& out) {
    if (offset_ + 4 > size_) {
        return false;
    }
    const std::uint32_t len = readBe32(data_ + offset_);
    offset_ += 4;
    if (offset_ + len > size_) {
        return false;
    }
    out.assign(reinterpret_cast<const char*>(data_ + offset_), len);
    offset_ += len;
    return true;
}

bool SshWireReader::readByte(std::uint8_t& out) {
    if (offset_ >= size_) {
        return false;
    }
    out = data_[offset_++];
    return true;
}

bool SshWireReader::readMpint(BIGNUM** out) {
    std::string bytes;
    if (!readString(bytes)) {
        return false;
    }
    if (bytes.empty()) {
        *out = BN_new();
        return *out != nullptr;
    }
    *out = BN_bin2bn(reinterpret_cast<const unsigned char*>(bytes.data()), static_cast<int>(bytes.size()), nullptr);
    return *out != nullptr;
}

bool SshWireReader::empty() const {
    return offset_ >= size_;
}

size_t SshWireReader::remaining() const {
    return offset_ < size_ ? size_ - offset_ : 0;
}

std::string SshPublicKeyImport::base64Decode(const std::string& encoded) {
    BIO* bio = BIO_new_mem_buf(encoded.data(), static_cast<int>(encoded.size()));
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    char buffer[4096];
    std::string decoded;
    int length = 0;
    while ((length = BIO_read(bio, buffer, sizeof(buffer))) > 0) {
        decoded.append(buffer, length);
    }
    BIO_free_all(bio);
    return decoded;
}

std::string SshPublicKeyImport::base64Encode(const std::string& raw) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, raw.data(), static_cast<int>(raw.size()));
    BIO_flush(bio);

    BUF_MEM* mem = nullptr;
    BIO_get_mem_ptr(bio, &mem);
    std::string encoded;
    if (mem && mem->data && mem->length > 0) {
        encoded.assign(mem->data, mem->length);
    }
    BIO_free_all(bio);
    return encoded;
}

EVP_PKEY* SshPublicKeyImport::fromSshBlob(const std::string& decoded_blob) {
    SshWireReader reader(decoded_blob);
    std::string key_type;
    if (!reader.readString(key_type)) {
        return nullptr;
    }

    if (key_type == "ssh-rsa") {
        BIGNUM* e = nullptr;
        BIGNUM* n = nullptr;
        if (!reader.readMpint(&e) || !reader.readMpint(&n)) {
            BN_free(e);
            BN_free(n);
            return nullptr;
        }
        return pkeyFromRsa(n, e);
    }

    if (key_type == "ssh-ed25519") {
        std::string raw;
        if (!reader.readString(raw)) {
            return nullptr;
        }
        return pkeyFromEd25519(raw);
    }

    if (key_type.rfind("ecdsa-sha2-", 0) == 0) {
        std::string curve_id;
        std::string q_bytes;
        if (!reader.readString(curve_id) || !reader.readString(q_bytes)) {
            return nullptr;
        }
        (void)curve_id;
        return pkeyFromEc(key_type, curve_id, q_bytes);
    }

    return nullptr;
}

std::string SshSignatureVerify::normalizeSignatureInput(const std::string& signature) {
    const auto begin = signature.find("-----BEGIN SSH SIGNATURE-----");
    if (begin == std::string::npos) {
        return signature;
    }

    const auto end = signature.find("-----END SSH SIGNATURE-----");
    if (end == std::string::npos) {
        return signature;
    }

    std::string body;
    const auto lines_start = signature.find('\n', begin);
    if (lines_start == std::string::npos) {
        return signature;
    }

    size_t pos = lines_start + 1;
    while (pos < end) {
        const size_t line_end = signature.find('\n', pos);
        const size_t chunk_end = (line_end == std::string::npos || line_end > end) ? end : line_end;
        body.append(signature.substr(pos, chunk_end - pos));
        if (line_end == std::string::npos || line_end >= end) {
            break;
        }
        pos = line_end + 1;
    }
    return body;
}

bool SshSignatureVerify::verify(const std::string& key_type,
                                const std::string& key_data_b64,
                                const std::string& data,
                                const std::string& signature_b64) {
    const std::string normalized_sig = normalizeSignatureInput(signature_b64);
    const std::string key_blob = SshPublicKeyImport::base64Decode(key_data_b64);
    const std::string sig_blob = SshPublicKeyImport::base64Decode(normalized_sig);
    if (key_blob.empty() || sig_blob.empty()) {
        return false;
    }

    EVP_PKEY* pkey = SshPublicKeyImport::fromSshBlob(key_blob);
    if (!pkey) {
        return false;
    }

    bool verified = false;

    const bool is_v0 = sig_blob.size() >= sizeof(kMagicV0) - 1 &&
                       sig_blob.compare(0, sizeof(kMagicV0) - 1, kMagicV0) == 0;
    const bool is_v1 = sig_blob.size() >= sizeof(kMagicV1) - 1 &&
                       sig_blob.compare(0, sizeof(kMagicV1) - 1, kMagicV1) == 0;

    if (is_v0 || is_v1) {
        std::uint32_t version = 0;
        std::string namespace_name;
        std::string hash_alg;
        std::string sig_type;
        std::string sig_raw;
        const bool parsed = parseSshSigEnvelope(sig_blob, version, namespace_name, hash_alg, sig_type, sig_raw);
        if (parsed) {
            const std::string sign_message = version >= 1
                ? buildOpenSshSignedDataV1(namespace_name, hash_alg, data)
                : buildOpenSshSignedDataV0(namespace_name, hash_alg, data);
            verified = verifyInnerSignature(pkey, key_type, sig_type, sig_raw, sign_message);
        }
    } else {
        SshWireReader sig_reader(sig_blob);
        std::string sig_type;
        std::string sig_raw;
        if (sig_reader.readString(sig_type) && sig_reader.readString(sig_raw)) {
            verified = verifyInnerSignature(pkey, key_type, sig_type, sig_raw, data);
            if (!verified && key_type == "ssh-ed25519") {
                const std::string sign_message_v1 =
                    buildOpenSshSignedDataV1(kDefaultNamespace, "sha512", data);
                verified = verifyInnerSignature(pkey, key_type, sig_type, sig_raw, sign_message_v1);
                if (!verified) {
                    const std::string sign_message_v0 =
                        buildOpenSshSignedDataV0(kDefaultNamespace, "sha512", data);
                    verified = verifyInnerSignature(pkey, key_type, sig_type, sig_raw, sign_message_v0);
                }
            }
        }
    }

    EVP_PKEY_free(pkey);
    return verified;
}

std::string SshPublicKeyImport::buildEd25519KeyData(const std::string& raw32) {
    if (raw32.size() != 32) {
        return "";
    }
    std::string blob;
    appendWireString(blob, "ssh-ed25519");
    appendWireString(blob, raw32);
    return base64Encode(blob);
}

std::string SshPublicKeyImport::buildSignatureBlob(const std::string& sig_type,
                                                   const std::string& sig_raw) {
    std::string blob;
    appendWireString(blob, sig_type);
    appendWireString(blob, sig_raw);
    return base64Encode(blob);
}

}  // namespace simple_rsyncd
