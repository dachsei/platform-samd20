#pragma once
#include <cstddef>
#include <gsl/span>
#include <sam.h>

namespace mcu {

#define EEPROM [[gnu::section(".eeprom")]]

class FlashEEPROM
{
public:
	FlashEEPROM() = delete;

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

	static void write(const std::byte* flashAddress, gsl::span<const std::byte> src) noexcept
	{
		writeData(flashAddress, src.data(), src.size());
	}
	static void read(const std::byte* flashAddress, gsl::span<std::byte> dst) noexcept
	{
		readData(flashAddress, dst.data(), dst.size());
	}
	
	static void writeData(const void* flashAddress, const void* data, size_t length) noexcept;
	static void readData(const void* flashAddress, void* destination, size_t length) noexcept;
	
	static void writeByte(const uint8_t* flashAddress, uint8_t data) noexcept;
	static uint8_t readByte(const uint8_t* flashAddress) noexcept;
	
	static void commit() noexcept;
	static bool needsCommit() noexcept { return cacheDirty; }

	static constexpr size_t RowSize = FLASH_PAGE_SIZE * 4;
	static constexpr uintptr_t RowOffsetMask = RowSize - 1;
	
private:
	static uint32_t cache[RowSize / 4];
	static const uint32_t* cacheTag;
	static bool cacheDirty;
	
	static void cacheRow(const uint32_t* rowStart) noexcept;
};

} // namespace mcu
