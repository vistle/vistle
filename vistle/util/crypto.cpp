#include "crypto.h"

#include <botan/botan.h>
#include <botan/rng.h>
#include <botan/hex.h>
#include <botan/hmac.h>
#include <botan/sha2_32.h>

#include <memory>
#include <cassert>
#include <cstdlib>
#include <mutex>
#include <iostream>
#include "sysdep.h"
#include "exception.h"

namespace vistle {
namespace crypto {

static std::recursive_mutex s_mutex;

static bool s_initialized = false;
#if BOTAN_VERSION_MAJOR < 2
static std::unique_ptr<Botan::LibraryInitializer> s_botan_lib;
#endif
static std::vector<uint8_t> s_key;
static std::vector<uint8_t> s_session_data;
static const char mac_algorithm[] = "HMAC(SHA-256)";
static std::string s_temp_key;

template<typename C, class Container>
static std::vector<C> from_secure(const Container &secure)
{
    std::vector<C> result;
    std::copy(secure.begin(), secure.end(), std::back_inserter(result));
    return result;
}


bool initialize(size_t secret_size)
{
    std::unique_lock<std::recursive_mutex> guard(s_mutex);
    if (s_initialized)
        return true;

#if BOTAN_VERSION_MAJOR < 2
    s_botan_lib.reset(new Botan::LibraryInitializer);
    if (!s_botan_lib) {
        throw except::exception("failed to initialize Botan library");
        return false;
    }

    std::unique_ptr<Botan::MessageAuthenticationCode> mac_algo(new Botan::HMAC(new Botan::SHA_256));
    if (!mac_algo) {
        throw except::exception("failed to create message authentication code algorithm");
        return false;
    }
#else
    std::unique_ptr<Botan::MessageAuthenticationCode> mac_algo(Botan::MessageAuthenticationCode::create(mac_algorithm));
    if (!mac_algo) {
        throw except::exception("failed to create message authentication code algorithm");
        return false;
    }
#endif

    s_initialized = true;

    s_session_data = random_data(secret_size);

    if (const char *key = getenv("VISTLE_KEY")) {
        try {
            auto sec_key = Botan::hex_decode(key);
            s_key = from_secure<uint8_t>(sec_key);
            return true;
        } catch (...) {
            std::cerr << "Could not decode VISTLE_KEY environment variable" << std::endl;
        }
    }

    std::cerr << "VISTLE_KEY environment variable not set, using random session key" << std::endl;
    s_key = random_data(secret_size);
    std::string hex_key = Botan::hex_encode(s_key.data(), s_key.size());
    setenv("VISTLE_KEY", hex_key.c_str(), 1);

    return true;
}

bool set_session_key(const std::string &hex_key)
{
    assert(s_initialized);

    try {
        auto sec_key = Botan::hex_decode(hex_key);
        s_key = from_secure<uint8_t>(sec_key);
    } catch (...) {
        return false;
    }

    setenv("VISTLE_KEY", hex_key.c_str(), 1);
    return true;
}

std::string get_session_key()
{
    auto key = session_key();
    std::string hex_key = Botan::hex_encode(key.data(), key.size());
    return hex_key;
}

std::vector<uint8_t> random_data(size_t length)
{
    std::unique_lock<std::recursive_mutex> guard(s_mutex);
    assert(s_initialized);

    auto rng = Botan::RandomNumberGenerator::make_rng();
    std::vector<uint8_t> data;
    auto sec = rng->random_vec(length);
    std::copy(sec.begin(), sec.end(), std::back_inserter(data));
    return data;
}

std::vector<uint8_t> compute_mac(const void *data, size_t length, const std::vector<uint8_t> &key)
{
    std::unique_lock<std::recursive_mutex> guard(s_mutex);
    assert(s_initialized);

#if BOTAN_VERSION_MAJOR < 2
    std::unique_ptr<Botan::MessageAuthenticationCode> mac_algo(new Botan::HMAC(new Botan::SHA_256));
#else
    std::unique_ptr<Botan::MessageAuthenticationCode> mac_algo(Botan::MessageAuthenticationCode::create(mac_algorithm));
#endif
    if (!mac_algo)
        return std::vector<uint8_t>();
    mac_algo->set_key(key.data(), key.size());
#if BOTAN_VERSION_MAJOR >= 2
    mac_algo->start();
#endif
    mac_algo->update(static_cast<const char *>(data));
    auto tag = mac_algo->final();
    return from_secure<uint8_t>(tag);
}

bool verify_mac(const void *data, size_t length, const std::vector<uint8_t> &key, const std::vector<uint8_t> &mac)
{
    std::unique_lock<std::recursive_mutex> guard(s_mutex);
    assert(s_initialized);

#if BOTAN_VERSION_MAJOR < 2
    std::unique_ptr<Botan::MessageAuthenticationCode> mac_algo(new Botan::HMAC(new Botan::SHA_256));
#else
    std::unique_ptr<Botan::MessageAuthenticationCode> mac_algo(Botan::MessageAuthenticationCode::create(mac_algorithm));
#endif
    if (!mac_algo)
        return false;
    mac_algo->set_key(key.data(), key.size());
#if BOTAN_VERSION_MAJOR >= 2
    mac_algo->start();
#endif
    mac_algo->update(static_cast<const char *>(data));
    return mac_algo->verify_mac(mac.data(), mac.size());
}

const std::vector<uint8_t> &session_data()
{
    std::unique_lock<std::recursive_mutex> guard(s_mutex);
    assert(s_initialized);

    if (s_session_data.empty()) {
        throw except::exception("cryptographic session data not initialized");
    }
    return s_session_data;
}

const std::vector<uint8_t> &session_key()
{
    std::unique_lock<std::recursive_mutex> guard(s_mutex);
    assert(s_initialized);

    return s_key;
}

} // namespace crypto
} // namespace vistle
