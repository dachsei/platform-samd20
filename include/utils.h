#pragma once

#include <cstdint>
#include <sam.h>

namespace mcu {
namespace util {

constexpr int getSercomIndex(const Sercom* inst)
{
	const uintptr_t sercomSize = (reinterpret_cast<uintptr_t>(SERCOM1) - reinterpret_cast<uintptr_t>(SERCOM0));
	const uintptr_t sercomBase = reinterpret_cast<uintptr_t>(SERCOM0);
	return (reinterpret_cast<uintptr_t>(inst) - sercomBase) / sercomSize;
}

} // namespace util
} // namespace mcu
