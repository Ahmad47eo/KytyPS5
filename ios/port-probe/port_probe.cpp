#include <cstdint>

#if !defined(__aarch64__) && !defined(__arm64__)
#error "KytyPS5 iOS port probe must compile for ARM64"
#endif

static_assert(sizeof(void*) == 8, "KytyPS5 iOS port requires a 64-bit process");

extern "C" {

struct KytyIosPortProbe {
    std::uint32_t abi_version;
    std::uint32_t pointer_bits;
    std::uint32_t arm64_target;
    std::uint32_t cxx_standard;
};

const KytyIosPortProbe* kyty_ios_port_probe() {
    static constexpr KytyIosPortProbe result {
        1,
        static_cast<std::uint32_t>(sizeof(void*) * 8),
        1,
        20,
    };
    return &result;
}

}
