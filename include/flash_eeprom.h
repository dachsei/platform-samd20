#pragma once
#include <array>
#include <gsl/span>
#include <sam.h>

namespace mcu {

#define EEPROM [[gnu::section(".eeprom")]]

class FlashEEPROM
{
public:
	FlashEEPROM() = delete;

	template <typename T>
	static void write(const T* flashAddress, const T& src) noexcept
	{
		write(flashAddress, gsl::make_span(&src, 1));
	}
	template <typename T>
	static T read(const T* flashAddress) noexcept
	{
		T result;
		read(flashAddress, gsl::make_span(&result, 1));
		return result;
	}

	template <typename T>
	static void write(const T* flashAddress, gsl::span<const T> src) noexcept
	{
		write(reinterpret_cast<const std::byte*>(flashAddress), gsl::as_bytes(src));
	}
	template <typename T>
	static void read(const T* flashAddress, gsl::span<T> dst) noexcept
	{
		read(reinterpret_cast<const std::byte*>(flashAddress), gsl::as_bytes(dst));
	}

	static void write(const std::byte* flashAddress, gsl::span<const std::byte> src) noexcept;
	static void read(const std::byte* flashAddress, gsl::span<std::byte> dst) noexcept;
	
	static void commit() noexcept;
	static bool needsCommit() noexcept { return cacheDirty; }

	static constexpr size_t RowSize = FLASH_PAGE_SIZE * 4;
	static constexpr uintptr_t RowOffsetMask = RowSize - 1;
	
private:
	alignas(unsigned) static std::array<std::byte, RowSize> cache;
	static uintptr_t cacheTag;
	static bool cacheDirty;
	
	static void cacheRow(uintptr_t rowStart) noexcept;
};

} // namespace mcu
