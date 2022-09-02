#ifndef VISTLE_UTIL_CRYPTO_H
#define VISTLE_UTIL_CRYPTO_H

#include "export.h"

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>

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

} // namespace crypto
} // namespace vistle
#endif
