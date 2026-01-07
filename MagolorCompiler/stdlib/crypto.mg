// Std.Crypto - AES-256-GCM encryption using OpenSSL
// Provides secure encryption/decryption with password-based key derivation

@link { -lssl -lcrypto }
@include { <openssl/evp.h> <openssl/rand.h> }
@cimport { openssl/evp.h openssl/rand.h }

using Std.Array;

// Encryption result containing ciphertext + metadata
pub class CryptoResult {
    pub success: bool;
    pub data: Array<int>;      // Encrypted/decrypted bytes
    pub error: string;
    pub iv: Array<int>;        // Initialization vector (12 bytes for GCM)
    pub tag: Array<int>;       // Auth tag (16 bytes for GCM)
    pub salt: Array<int>;      // Salt for key derivation (16 bytes)
    
    pub fn create() {
        this.success = false;
        this.error = "";
    }
}

// Key derivation config
pub class KeyConfig {
    pub iterations: int;       // PBKDF2 iterations (default 100000)
    pub keyLength: int;        // Key length in bytes (32 for AES-256)
    
    pub fn create() {
        this.iterations = 100000;
        this.keyLength = 32;
    }
    
    pub static fn standard() -> KeyConfig {
        return new KeyConfig();
    }
    
    pub static fn fast() -> KeyConfig {
        let cfg = new KeyConfig();
        cfg.iterations = 10000;
        return cfg;
    }
    
    pub static fn paranoid() -> KeyConfig {
        let cfg = new KeyConfig();
        cfg.iterations = 500000;
        return cfg;
    }
}

// Derive key from password using PBKDF2-SHA256
pub fn deriveKey(password: string, salt: Array<int>, config: KeyConfig) -> Array<int> {
    @cpp {
        #include <openssl/evp.h>
        #include <openssl/rand.h>
        
        std::vector<int64_t> key(config.keyLength);
        std::vector<unsigned char> saltBytes(salt.size());
        for (size_t i = 0; i < salt.size(); i++) {
            saltBytes[i] = static_cast<unsigned char>(salt[i]);
        }
        
        std::vector<unsigned char> keyBytes(config.keyLength);
        PKCS5_PBKDF2_HMAC(
            password.c_str(), password.length(),
            saltBytes.data(), saltBytes.size(),
            config.iterations,
            EVP_sha256(),
            config.keyLength, keyBytes.data()
        );
        
        for (int i = 0; i < config.keyLength; i++) {
            key[i] = keyBytes[i];
        }
        return key;
    }
}

// Generate cryptographically secure random bytes
pub fn randomBytes(count: int) -> Array<int> {
    @cpp {
        #include <openssl/rand.h>
        
        std::vector<unsigned char> bytes(count);
        RAND_bytes(bytes.data(), count);
        
        std::vector<int64_t> result(count);
        for (int i = 0; i < count; i++) {
            result[i] = bytes[i];
        }
        return result;
    }
}

// Generate random salt (16 bytes)
pub fn generateSalt() -> Array<int> {
    return randomBytes(16);
}

// Generate random IV for AES-GCM (12 bytes)
pub fn generateIV() -> Array<int> {
    return randomBytes(12);
}

// Encrypt data using AES-256-GCM
pub fn encrypt(plaintext: Array<int>, password: string) -> CryptoResult {
    @cpp {
        #include <openssl/evp.h>
        #include <openssl/rand.h>
        
        CryptoResult result;
        result.success = false;
        
        // Generate salt and IV
        std::vector<unsigned char> salt(16);
        std::vector<unsigned char> iv(12);
        RAND_bytes(salt.data(), 16);
        RAND_bytes(iv.data(), 12);
        
        // Derive key
        std::vector<unsigned char> key(32);
        PKCS5_PBKDF2_HMAC(
            password.c_str(), password.length(),
            salt.data(), salt.size(),
            100000, EVP_sha256(),
            32, key.data()
        );
        
        // Setup encryption
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            result.error = "Failed to create cipher context";
            return result;
        }
        
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.error = "Failed to init encryption";
            return result;
        }
        
        // Convert input
        std::vector<unsigned char> plain(plaintext.size());
        for (size_t i = 0; i < plaintext.size(); i++) {
            plain[i] = static_cast<unsigned char>(plaintext[i]);
        }
        
        // Encrypt
        std::vector<unsigned char> cipher(plain.size() + 16);
        int len = 0, cipherLen = 0;
        
        if (EVP_EncryptUpdate(ctx, cipher.data(), &len, plain.data(), plain.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.error = "Encryption failed";
            return result;
        }
        cipherLen = len;
        
        if (EVP_EncryptFinal_ex(ctx, cipher.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.error = "Encryption finalize failed";
            return result;
        }
        cipherLen += len;
        
        // Get auth tag
        std::vector<unsigned char> tag(16);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data());
        EVP_CIPHER_CTX_free(ctx);
        
        // Store results
        result.data.resize(cipherLen);
        for (int i = 0; i < cipherLen; i++) {
            result.data[i] = cipher[i];
        }
        
        result.salt.resize(16);
        result.iv.resize(12);
        result.tag.resize(16);
        
        for (int i = 0; i < 16; i++) result.salt[i] = salt[i];
        for (int i = 0; i < 12; i++) result.iv[i] = iv[i];
        for (int i = 0; i < 16; i++) result.tag[i] = tag[i];
        
        result.success = true;
        return result;
    }
}

// Decrypt data using AES-256-GCM
pub fn decrypt(encrypted: CryptoResult, password: string) -> CryptoResult {
    @cpp {
        #include <openssl/evp.h>
        
        CryptoResult result;
        result.success = false;
        
        if (!encrypted.success || encrypted.data.empty()) {
            result.error = "Invalid encrypted data";
            return result;
        }
        
        // Convert arrays
        std::vector<unsigned char> salt(encrypted.salt.size());
        std::vector<unsigned char> iv(encrypted.iv.size());
        std::vector<unsigned char> tag(encrypted.tag.size());
        std::vector<unsigned char> cipher(encrypted.data.size());
        
        for (size_t i = 0; i < encrypted.salt.size(); i++) salt[i] = encrypted.salt[i];
        for (size_t i = 0; i < encrypted.iv.size(); i++) iv[i] = encrypted.iv[i];
        for (size_t i = 0; i < encrypted.tag.size(); i++) tag[i] = encrypted.tag[i];
        for (size_t i = 0; i < encrypted.data.size(); i++) cipher[i] = encrypted.data[i];
        
        // Derive key
        std::vector<unsigned char> key(32);
        PKCS5_PBKDF2_HMAC(
            password.c_str(), password.length(),
            salt.data(), salt.size(),
            100000, EVP_sha256(),
            32, key.data()
        );
        
        // Setup decryption
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            result.error = "Failed to create cipher context";
            return result;
        }
        
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.error = "Failed to init decryption";
            return result;
        }
        
        // Decrypt
        std::vector<unsigned char> plain(cipher.size());
        int len = 0, plainLen = 0;
        
        if (EVP_DecryptUpdate(ctx, plain.data(), &len, cipher.data(), cipher.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.error = "Decryption failed";
            return result;
        }
        plainLen = len;
        
        // Set expected tag
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag.data());
        
        // Verify and finalize
        if (EVP_DecryptFinal_ex(ctx, plain.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.error = "Authentication failed - wrong password or corrupted data";
            return result;
        }
        plainLen += len;
        EVP_CIPHER_CTX_free(ctx);
        
        // Store results
        result.data.resize(plainLen);
        for (int i = 0; i < plainLen; i++) {
            result.data[i] = plain[i];
        }
        
        result.success = true;
        return result;
    }
}

// Encrypt string to binary
pub fn encryptString(text: string, password: string) -> CryptoResult {
    @cpp {
        std::vector<int64_t> bytes(text.size());
        for (size_t i = 0; i < text.size(); i++) {
            bytes[i] = static_cast<unsigned char>(text[i]);
        }
    }
    return encrypt(bytes, password);
}

// Decrypt binary to string
pub fn decryptToString(encrypted: CryptoResult, password: string) -> string {
    let result = decrypt(encrypted, password);
    if (!result.success) {
        return "";
    }
    @cpp {
        std::string text;
        text.reserve(result.data.size());
        for (auto b : result.data) {
            text += static_cast<char>(b);
        }
        return text;
    }
}

// Hash password (for storage, not encryption)
pub fn hashPassword(password: string) -> string {
    @cpp {
        #include <openssl/evp.h>
        #include <openssl/rand.h>
        #include <sstream>
        #include <iomanip>
        
        // Generate salt
        std::vector<unsigned char> salt(16);
        RAND_bytes(salt.data(), 16);
        
        // Derive hash
        std::vector<unsigned char> hash(32);
        PKCS5_PBKDF2_HMAC(
            password.c_str(), password.length(),
            salt.data(), salt.size(),
            100000, EVP_sha256(),
            32, hash.data()
        );
        
        // Format: salt$hash (hex encoded)
        std::stringstream ss;
        for (auto b : salt) ss << std::hex << std::setfill('0') << std::setw(2) << (int)b;
        ss << "$";
        for (auto b : hash) ss << std::hex << std::setfill('0') << std::setw(2) << (int)b;
        return ss.str();
    }
}

// Verify password against hash
pub fn verifyPassword(password: string, hash: string) -> bool {
    @cpp {
        #include <openssl/evp.h>
        
        // Parse hash format: salt$hash
        size_t delim = hash.find('$');
        if (delim == std::string::npos || delim != 32) return false;
        
        std::string saltHex = hash.substr(0, 32);
        std::string hashHex = hash.substr(33);
        
        // Decode salt
        std::vector<unsigned char> salt(16);
        for (int i = 0; i < 16; i++) {
            salt[i] = std::stoi(saltHex.substr(i*2, 2), nullptr, 16);
        }
        
        // Derive hash
        std::vector<unsigned char> computed(32);
        PKCS5_PBKDF2_HMAC(
            password.c_str(), password.length(),
            salt.data(), salt.size(),
            100000, EVP_sha256(),
            32, computed.data()
        );
        
        // Compare (constant time)
        std::stringstream ss;
        for (auto b : computed) ss << std::hex << std::setfill('0') << std::setw(2) << (int)b;
        
        return CRYPTO_memcmp(ss.str().c_str(), hashHex.c_str(), 64) == 0;
    }
}
