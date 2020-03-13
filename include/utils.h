#pragma once

#include <cstdint>
#include <sam.h>

namespace mcu {
namespace util {

inline constexpr int getSercomIndex(const Sercom* inst)
{
	const uintptr_t sercomSize = (reinterpret_cast<uintptr_t>(SERCOM1) - reinterpret_cast<uintptr_t>(SERCOM0));
	const uintptr_t sercomBase = reinterpret_cast<uintptr_t>(SERCOM0);
	return (reinterpret_cast<uintptr_t>(inst) - sercomBase) / sercomSize;
}

inline constexpr unsigned getTimerIndex(const Tc* inst)
{
	const uintptr_t timerSize = (reinterpret_cast<uintptr_t>(TC1) - reinterpret_cast<uintptr_t>(TC0));
	const uintptr_t timerBase = reinterpret_cast<uintptr_t>(TC0);
	return (reinterpret_cast<uintptr_t>(inst) - timerBase) / timerSize;
}

inline void checkTimerGenerator(unsigned timerIndex, uint8_t requiredGenerator)
{
#ifndef NDEBUG
	static uint8_t generators[4] = {0xff, 0xff, 0xff, 0xff};

	unsigned pairIndex = timerIndex / 2;
	assert(pairIndex < 4);

	if (generators[pairIndex] == 0xff) { generators[pairIndex] = requiredGenerator; }
	else {
		assert(generators[pairIndex] == requiredGenerator);
	}
#endif
}

} // namespace util
} // namespace mcu
