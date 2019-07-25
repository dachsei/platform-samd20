#include "flash_eeprom.h"
#include <algorithm>

inline void mcu::FlashEEPROM::writeByte(const uint8_t* flashAddress, uint8_t data) noexcept
{
	uint32_t* row = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(flashAddress) & ~RowOffsetMask);
	if(row != cacheTag){
		cacheRow(row);
	}
	
	size_t offset = reinterpret_cast<uintptr_t>(flashAddress) & RowOffsetMask;
	cache[offset] = data;
	cacheDirty = true;
}

inline uint8_t mcu::FlashEEPROM::readByte(const uint8_t * flashAddress) noexcept
{
	uint32_t* row = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(flashAddress) & ~RowOffsetMask);
	if(row != cacheTag){
		cacheRow(row);
	}
	
	size_t offset = reinterpret_cast<uintptr_t>(flashAddress) & RowOffsetMask;
	return cache[offset];
}

void mcu::FlashEEPROM::writeData(const void* flashAddress, const void* data, size_t length) noexcept
{
	const uint8_t* flashIt = static_cast<const uint8_t*>(flashAddress);
	const uint8_t* dataIt = static_cast<const uint8_t*>(data);
	for(size_t i = 0; i < length; i++) {
		writeByte(flashIt, *dataIt);
		flashIt++;
		dataIt++;
	}
}

void mcu::FlashEEPROM::readData(const void* flashAddress, void* destination, size_t length) noexcept
{
	const uint8_t* flashIt = static_cast<const uint8_t*>(flashAddress);
	uint8_t* destinationIt = static_cast<uint8_t*>(destination);
	for(size_t i = 0; i < length; i++) {
		*destinationIt = readByte(flashIt);
		destinationIt++;
		flashIt++;
	}
}

void mcu::FlashEEPROM::commit() noexcept
{
	if(cacheDirty) {
		NVMCTRL->ADDR.reg = reinterpret_cast<uintptr_t>(cacheTag) / 2;
		NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
		while(!NVMCTRL->INTFLAG.bit.READY);
		
		for(int i = 0; i < 4; i++) {
			const size_t pageSizeInWords = FLASH_PAGE_SIZE / sizeof(*cache);
			uint32_t* dst = const_cast<uint32_t*>(cacheTag + i * pageSizeInWords);
			const uint32_t* src = cache + i * pageSizeInWords;

			std::copy(src, src + pageSizeInWords, dst);
			NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
			while(!NVMCTRL->INTFLAG.bit.READY);
		}
		cacheDirty = false;
	}
}

uint32_t mcu::FlashEEPROM::cache[RowSize / 4];
const uint32_t* mcu::FlashEEPROM::cacheTag = nullptr;
bool mcu::FlashEEPROM::cacheDirty = false;

void mcu::FlashEEPROM::cacheRow(const uint32_t* rowStart) noexcept
{
	commit();
	std::copy(rowStart, rowStart + RowSize / sizeof(*rowStart), cache);
	cacheTag = rowStart;
}
