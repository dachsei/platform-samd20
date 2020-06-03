#include "flash_eeprom.h"
#include <algorithm>

void mcu::FlashEEPROM::write(const std::byte* flashAddress, gsl::span<const std::byte> src) noexcept
{
	Row* row = reinterpret_cast<Row*>(reinterpret_cast<uintptr_t>(flashAddress) & ~RowOffsetMask);
	if (row != cacheTag) {
		cacheRow(row);
	}

	size_t offset = reinterpret_cast<uintptr_t>(flashAddress) & RowOffsetMask;
	if (offset + src.size() < cache.bytes.size()) {
		std::copy(src.begin(), src.end(), cache.bytes.begin() + offset);
		cacheDirty = true;
	} else {
		size_t chunkSize = RowSize - offset;
		std::copy(src.begin(), src.begin() + chunkSize, cache.bytes.begin() + offset);
		cacheDirty = true;
		write(flashAddress + chunkSize, src.subspan(chunkSize));
	}
}

void mcu::FlashEEPROM::read(const std::byte* flashAddress, gsl::span<std::byte> dst) noexcept
{
	Row* row = reinterpret_cast<Row*>(reinterpret_cast<uintptr_t>(flashAddress) & ~RowOffsetMask);
	if (row != cacheTag) {
		cacheRow(row);
	}

	size_t offset = reinterpret_cast<uintptr_t>(flashAddress) & RowOffsetMask;
	if (offset + dst.size() <= cache.bytes.size()) {
		std::copy(cache.bytes.begin() + offset, cache.bytes.begin() + offset + dst.size(), dst.begin());
	} else {
		size_t chunkSize = RowSize - offset;
		std::copy(cache.bytes.begin() + offset, cache.bytes.begin() + offset + chunkSize, dst.begin());
		read(flashAddress + chunkSize, dst.subspan(chunkSize));
	}
}

void mcu::FlashEEPROM::commit() noexcept
{
	if(cacheDirty) {
		NVMCTRL->ADDR.reg = reinterpret_cast<uintptr_t>(cacheTag) / 2;
		NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
		while(!NVMCTRL->INTFLAG.bit.READY);

		Row& dst = *cacheTag;
		for (size_t i = 0; i < cache.pages.size(); i++) {
			copyPage(cache.pages[i], dst[i]);
			NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
			while(!NVMCTRL->INTFLAG.bit.READY);
		}
		cacheDirty = false;
	}
}

mcu::FlashEEPROM::Cache mcu::FlashEEPROM::cache;
mcu::FlashEEPROM::Row* mcu::FlashEEPROM::cacheTag;
bool mcu::FlashEEPROM::cacheDirty = false;

void mcu::FlashEEPROM::cacheRow(Row* row) noexcept
{
	commit();
	cache.pages = *row;
	cacheTag = row;
}

void mcu::FlashEEPROM::copyPage(const Page& src, Page& dst) noexcept
{
	const unsigned* srcIt = src.data();
	const unsigned* srcEnd = src.data() + src.size();
	unsigned* dstIt = dst.data();
	do {
		*dstIt = *srcIt;
		dstIt++;
		srcIt++;
	} while (srcIt != srcEnd);
}
