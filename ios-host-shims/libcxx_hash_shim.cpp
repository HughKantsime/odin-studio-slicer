// libcxx_hash_shim.cpp — fills the `std::__1::__hash_memory(void const*, size_t)`
// symbol that libslic3r's hash-table paths reference. Apple's SDK 26.4
// libc++ calls this out-of-line; iOS 26.2 (and lower) runtime libc++ doesn't
// export it, so dyld aborts the process at launch with "Symbol not found".
//
// The function signature + semantics are stable across libc++ versions.
// Algorithm here is MurmurHash2 / FNV hybrid — any size_t-returning hash is
// correct for the uses (std::unordered_{map,set} bucket lookup); only
// stability-across-runs matters, not a specific distribution. Providing the
// symbol in the main app binary takes priority at dyld-resolution time so
// libc++ calls find *this* implementation.
//
// Remove when we bump deployment target past whatever iOS version first
// ships this symbol out-of-line in libc++.1.dylib.

#include <cstddef>
#include <cstdint>

namespace std {
inline namespace __1 {

extern "C++" std::size_t
__hash_memory(void const* ptr, std::size_t size) noexcept;

std::size_t __hash_memory(void const* ptr, std::size_t size) noexcept {
    // Murmur2-64A — fast, good distribution, deterministic.
    const std::uint64_t seed = 0xC70F6907ULL;
    const std::uint64_t m = 0xc6a4a7935bd1e995ULL;
    const int r = 47;

    std::uint64_t h = seed ^ (size * m);
    const std::uint8_t* data = static_cast<const std::uint8_t*>(ptr);
    const std::size_t nblocks = size / 8;

    for (std::size_t i = 0; i < nblocks; ++i) {
        std::uint64_t k;
        // Portable unaligned load.
        std::uint8_t buf[8];
        for (int b = 0; b < 8; ++b) buf[b] = data[i * 8 + b];
        k = (std::uint64_t)buf[0]
          | ((std::uint64_t)buf[1] << 8)
          | ((std::uint64_t)buf[2] << 16)
          | ((std::uint64_t)buf[3] << 24)
          | ((std::uint64_t)buf[4] << 32)
          | ((std::uint64_t)buf[5] << 40)
          | ((std::uint64_t)buf[6] << 48)
          | ((std::uint64_t)buf[7] << 56);
        k *= m;
        k ^= k >> r;
        k *= m;
        h ^= k;
        h *= m;
    }

    const std::uint8_t* tail = data + nblocks * 8;
    switch (size & 7) {
        case 7: h ^= std::uint64_t(tail[6]) << 48; [[fallthrough]];
        case 6: h ^= std::uint64_t(tail[5]) << 40; [[fallthrough]];
        case 5: h ^= std::uint64_t(tail[4]) << 32; [[fallthrough]];
        case 4: h ^= std::uint64_t(tail[3]) << 24; [[fallthrough]];
        case 3: h ^= std::uint64_t(tail[2]) << 16; [[fallthrough]];
        case 2: h ^= std::uint64_t(tail[1]) << 8;  [[fallthrough]];
        case 1: h ^= std::uint64_t(tail[0]);       h *= m;
    }

    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    return static_cast<std::size_t>(h);
}

}  // inline namespace __1
}  // namespace std
