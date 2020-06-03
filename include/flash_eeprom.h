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
		write(flashAddress, gsl::span(&src, 1));
	}
	template <typename T>
	static T read(const T* flashAddress) noexcept
	{
		T result;
		read(flashAddress, gsl::span(&result, 1));
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
		read(reinterpret_cast<const std::byte*>(flashAddress), gsl::as_writable_bytes(dst));
	}

	static void write(const std::byte* flashAddress, gsl::span<const std::byte> src) noexcept;
	static void read(const std::byte* flashAddress, gsl::span<std::byte> dst) noexcept;
	
	static void commit() noexcept;
	static bool needsCommit() noexcept { return cacheDirty; }
	
private:
	using Page = std::array<unsigned, FLASH_PAGE_SIZE / sizeof(unsigned)>;
	using Row = std::array<Page, 4>;

	static constexpr size_t RowSize = sizeof(Row);
	static constexpr uintptr_t RowOffsetMask = sizeof(Row) - 1;

	union Cache {
		std::array<std::byte, sizeof(Row)> bytes;
		Row pages;
	};
	static Cache cache;
	static Row* cacheTag;
	static bool cacheDirty;
	
	static void cacheRow(Row* row) noexcept;
	static void copyPage(const Page& src, Page& dst) noexcept;
};

} // namespace mcu
