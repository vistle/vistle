#ifndef VISTLE_UTIL_CRYPTO_H
#define VISTLE_UTIL_CRYPTO_H

#include "export.h"

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <memory>

namespace Botan {
class HashFunction;
}

namespace vistle {
namespace crypto {

V_UTILEXPORT bool initialize(size_t secret_size = 64);
V_UTILEXPORT bool set_session_key(const std::string &hex_key);
V_UTILEXPORT std::string get_session_key();

V_UTILEXPORT const std::vector<uint8_t> &session_data();
V_UTILEXPORT const std::vector<uint8_t> &session_key();

V_UTILEXPORT std::vector<uint8_t> random_data(size_t length);
V_UTILEXPORT std::vector<uint8_t> compute_mac(const void *data, size_t length, const std::vector<uint8_t> &key);
V_UTILEXPORT bool verify_mac(const void *data, size_t length, const std::vector<uint8_t> &key,
                             const std::vector<uint8_t> &mac);
V_UTILEXPORT std::string hex_encode(const std::vector<uint8_t> &data);

class HashFunction;
V_UTILEXPORT HashFunction hash_new();
V_UTILEXPORT void hash_update(HashFunction &hash, const void *data, size_t length);
V_UTILEXPORT std::vector<uint8_t> hash_final(HashFunction &hash);

class V_UTILEXPORT HashFunction {
    friend HashFunction hash_new();
    friend void hash_update(HashFunction &hash, const void *data, size_t length);
    friend std::vector<uint8_t> hash_final(HashFunction &hash);

public:
    HashFunction(std::unique_ptr<Botan::HashFunction> &&hash);
    HashFunction(HashFunction &&other);
    ~HashFunction();

private:
    std::unique_ptr<Botan::HashFunction> hash;
};

} // namespace crypto
} // namespace vistle
#endif
