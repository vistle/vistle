#include "crypto.h"

#include <botan/version.h>
#if BOTAN_VERSION_MAJOR < 2
#error "Botan 1.x is not supported"
#endif
#include <botan/hex.h>
#include <botan/system_rng.h>
#include <botan/mac.h>
#include <botan/hash.h>

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
static std::vector<uint8_t> s_key;
static std::vector<uint8_t> s_session_data;
static std::string s_temp_key;
#if BOTAN_VERSION_MAJOR >= 3
std::unique_ptr<Botan::HashFunction> s_hash_algo;
#endif

static std::unique_ptr<Botan::MessageAuthenticationCode> make_mac()
{
    static const char mac_algorithm[] = "HMAC(SHA-256)";
    std::unique_ptr<Botan::MessageAuthenticationCode> mac_algo(Botan::MessageAuthenticationCode::create(mac_algorithm));
    if (!mac_algo) {
        throw except::exception("failed to create message authentication code algorithm");
    }
    return mac_algo;
}

static std::unique_ptr<Botan::HashFunction> make_hash()
{
    static const char hash_algorithm[] = "SHA-256";
    std::unique_ptr<Botan::HashFunction> hash_algo(Botan::HashFunction::create(hash_algorithm));
    if (!hash_algo) {
        throw except::exception("failed to create hash algorithm");
    }
    return hash_algo;
}

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

    auto mac_algo = make_mac();

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
    std::unique_lock<std::recursive_mutex> guard(s_mutex);
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

    std::vector<uint8_t> data(length);
    auto rng = std::make_unique<Botan::System_RNG>();
    rng->randomize(data.data(), data.size());

    return data;
}

std::vector<uint8_t> compute_mac(const void *data, size_t length, const std::vector<uint8_t> &key)
{
    std::unique_lock<std::recursive_mutex> guard(s_mutex);
    assert(s_initialized);

    auto mac_algo = make_mac();
    if (!mac_algo)
        return std::vector<uint8_t>();
    mac_algo->set_key(key.data(), key.size());
    mac_algo->start();
    mac_algo->update(static_cast<const char *>(data));
    auto tag = mac_algo->final();
    return from_secure<uint8_t>(tag);
}

bool verify_mac(const void *data, size_t length, const std::vector<uint8_t> &key, const std::vector<uint8_t> &mac)
{
    std::unique_lock<std::recursive_mutex> guard(s_mutex);
    assert(s_initialized);

    auto mac_algo = make_mac();
    if (!mac_algo)
        return false;
    mac_algo->set_key(key.data(), key.size());
    mac_algo->start();
    mac_algo->update(static_cast<const char *>(data));
    return mac_algo->verify_mac(mac.data(), mac.size());
}

std::string hex_encode(const std::vector<uint8_t> &data)
{
    return Botan::hex_encode(data.data(), data.size());
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

HashFunction::HashFunction(std::unique_ptr<Botan::HashFunction> &&hash): hash(std::move(hash))
{}

HashFunction::HashFunction(HashFunction &&other): hash(std::move(other.hash))
{}

HashFunction::~HashFunction() = default;

HashFunction hash_new()
{
    std::unique_lock<std::recursive_mutex> guard(s_mutex);

#if BOTAN_VERSION_MAJOR == 2
    HashFunction hash(make_hash());
#else
    if (!s_hash_algo)
        s_hash_algo = make_hash();

    HashFunction hash(s_hash_algo->new_object());
#endif
    return hash;
}

void hash_update(HashFunction &hash, const void *data, size_t length)
{
    hash.hash->update(static_cast<const uint8_t *>(data), length);
}

std::vector<uint8_t> hash_final(HashFunction &hash)
{
    auto result = from_secure<uint8_t>(hash.hash->final());
    return result;
}

} // namespace crypto
} // namespace vistle
