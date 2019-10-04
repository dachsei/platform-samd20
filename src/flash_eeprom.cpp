#include "flash_eeprom.h"
#include <algorithm>

void mcu::FlashEEPROM::write(const std::byte* flashAddress, gsl::span<const std::byte> src) noexcept
{
	uintptr_t row = reinterpret_cast<uintptr_t>(flashAddress) & ~RowOffsetMask;
	if (row != cacheTag) {
		cacheRow(row);
	}

	size_t offset = reinterpret_cast<uintptr_t>(flashAddress) & RowOffsetMask;
	if (offset + src.size() < RowSize) {
		std::copy(src.begin(), src.end(), cache.begin() + offset);
		cacheDirty = true;
	} else {
		size_t chunkSize = RowSize - offset;
		std::copy(src.begin(), src.begin() + chunkSize, cache.begin() + offset);
		cacheDirty = true;
		write(flashAddress + chunkSize, src.subspan(chunkSize));
	}
}

void mcu::FlashEEPROM::read(const std::byte* flashAddress, gsl::span<std::byte> dst) noexcept
{
	uintptr_t row = reinterpret_cast<uintptr_t>(flashAddress) & ~RowOffsetMask;
	if (row != cacheTag) {
		cacheRow(row);
	}

	size_t offset = reinterpret_cast<uintptr_t>(flashAddress) & RowOffsetMask;
	if (offset + dst.size() < RowSize) {
		std::copy(cache.begin() + offset, cache.begin() + offset + dst.size(), dst.begin());
	} else {
		size_t chunkSize = RowSize - offset;
		std::copy(cache.begin() + offset, cache.begin() + offset + chunkSize, dst.begin());
		read(flashAddress, dst.subspan(chunkSize));
	}
}

void mcu::FlashEEPROM::commit() noexcept
{
	if(cacheDirty) {
		NVMCTRL->ADDR.reg = cacheTag / 2;
		NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
		while(!NVMCTRL->INTFLAG.bit.READY);
		
		for(int i = 0; i < 4; i++) {
			unsigned* dst = reinterpret_cast<unsigned*>(cacheTag + i * FLASH_PAGE_SIZE);
			const unsigned* src = reinterpret_cast<unsigned*>(cache.data() + i * FLASH_PAGE_SIZE);

			std::copy(src, src + FLASH_PAGE_SIZE / sizeof(unsigned), dst);
			NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
			while(!NVMCTRL->INTFLAG.bit.READY);
		}
		cacheDirty = false;
	}
}

alignas(unsigned) std::array<std::byte, mcu::FlashEEPROM::RowSize> mcu::FlashEEPROM::cache;
uintptr_t mcu::FlashEEPROM::cacheTag = 0;
bool mcu::FlashEEPROM::cacheDirty = false;

void mcu::FlashEEPROM::cacheRow(uintptr_t rowStart) noexcept
{
	commit();
	unsigned* rowStartPtr = reinterpret_cast<unsigned*>(rowStart);
	unsigned* cachePtr = reinterpret_cast<unsigned*>(cache.data());
	std::copy(rowStartPtr, rowStartPtr + RowSize / sizeof(unsigned), cachePtr);
	cacheTag = rowStart;
}
