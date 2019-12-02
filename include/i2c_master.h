#pragma once

#include <optional>
#include <sam.h>
#include <gsl/span>
#include "clocks.h"
#include "utils.h"

namespace mcu {

class I2cMaster {
public:
	I2cMaster(Sercom* sercom, const mcu::ClockGenerator& clock, const mcu::ClockGenerator& slowClock, unsigned frequency = 100000)
		:port{sercom->I2CM}
	{
		int sercomIndex = mcu::util::getSercomIndex(sercom);
		PM->APBCMASK.reg |= 1 << (PM_APBCMASK_SERCOM0_Pos + sercomIndex);
		clock.routeToPeripheral(GCLK_CLKCTRL_ID_SERCOM0_CORE_Val + sercomIndex);
		slowClock.routeToPeripheral(GCLK_CLKCTRL_ID_SERCOMX_SLOW_Val);

		port.CTRLA.reg = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER;
		(void)port.CTRLB.reg;
		volatile uint32_t ctrla = port.CTRLA.reg;
		port.CTRLB.reg = 0;
		port.BAUD.reg = computeBaud(frequency, clock.frequency);
		while (port.STATUS.bit.SYNCBUSY);
		port.CTRLA.bit.ENABLE = true;
		while (port.STATUS.bit.SYNCBUSY);
		port.STATUS.bit.BUSSTATE = BusstateIdle;
	}

	std::optional<std::byte> readReg(uint8_t slaveAddress, uint8_t reg)
	{
		port.ADDR.reg = (slaveAddress << 1) | 0;
		while (port.INTFLAG.reg == 0);
		port.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB | SERCOM_I2CM_INTFLAG_SB;
		if (port.STATUS.reg & (StatusErrorMask | SERCOM_I2CM_STATUS_RXNACK) != 0) {
			if (port.STATUS.bit.BUSSTATE == BusstateOwner)
				port.CTRLB.reg = SERCOM_I2CM_CTRLB_CMD(CommandStop);
			return std::nullopt;
		}
		
		port.DATA.reg = reg;
		while (port.INTFLAG.reg == 0);
		port.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB | SERCOM_I2CM_INTFLAG_SB;
		if (port.STATUS.reg & StatusErrorMask) {
			if (port.STATUS.bit.BUSSTATE == BusstateOwner)
				port.CTRLB.reg = SERCOM_I2CM_CTRLB_CMD(CommandStop);
			return std::nullopt;
		}

		port.ADDR.reg = (slaveAddress << 1) | 1;
		while (port.INTFLAG.reg == 0);
		port.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB | SERCOM_I2CM_INTFLAG_SB;
		if (port.STATUS.reg & (StatusErrorMask | SERCOM_I2CM_STATUS_RXNACK) != 0) {
			if (port.STATUS.bit.BUSSTATE == BusstateOwner)
				port.CTRLB.reg = SERCOM_I2CM_CTRLB_CMD(CommandStop);
			return std::nullopt;
		}

		std::byte result = static_cast<std::byte>(port.DATA.reg);
		port.CTRLB.reg = SERCOM_I2CM_CTRLB_ACKACT | SERCOM_I2CM_CTRLB_CMD(CommandStop);
		return result;
	}

	bool writeReg(uint8_t slaveAddress, uint8_t reg, std::byte data)
	{
		port.ADDR.reg = (slaveAddress << 1) | 0;
		while (port.INTFLAG.reg == 0);
		port.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB | SERCOM_I2CM_INTFLAG_SB;
		if (port.STATUS.reg & (StatusErrorMask | SERCOM_I2CM_STATUS_RXNACK) != 0) {
			if (port.STATUS.bit.BUSSTATE == BusstateOwner)
				port.CTRLB.reg = SERCOM_I2CM_CTRLB_CMD(CommandStop);
			return false;
		}
		
		port.DATA.reg = reg;
		while (port.INTFLAG.reg == 0);
		port.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB | SERCOM_I2CM_INTFLAG_SB;
		if (port.STATUS.reg & (StatusErrorMask | SERCOM_I2CM_STATUS_RXNACK) != 0) {
			if (port.STATUS.bit.BUSSTATE == BusstateOwner)
				port.CTRLB.reg = SERCOM_I2CM_CTRLB_CMD(CommandStop);
			return false;
		}

		port.DATA.reg = static_cast<uint8_t>(data);
		while (port.INTFLAG.reg == 0);
		port.INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB | SERCOM_I2CM_INTFLAG_SB;
		if (port.STATUS.reg & (StatusErrorMask) != 0) {
			if (port.STATUS.bit.BUSSTATE == BusstateOwner)
				port.CTRLB.reg = SERCOM_I2CM_CTRLB_CMD(CommandStop);
			return false;
		}
		
		port.CTRLB.reg = SERCOM_I2CM_CTRLB_CMD(CommandStop);
		return true;
	}

private:
	SercomI2cm& port;
	
	static constexpr unsigned CommandRead = 0x2;
	static constexpr unsigned CommandStop = 0x3;
	static constexpr unsigned BusstateIdle = 0x1;
	static constexpr unsigned BusstateOwner = 0x2;
	static constexpr unsigned StatusErrorMask = SERCOM_I2CM_STATUS_ARBLOST;
	
	static constexpr unsigned i2cRise = 215;
	static constexpr uint16_t computeBaud(unsigned desiredFrequency, unsigned coreFrequency)
	{
		//                  coreFreq - (desiredFreq * 10) - (coreFreq * desiredFreq * i2cRise)
		// BAUD + BAUDLOW = ------------------------------------------------------------------
		//                  desiredFreq
		int baudSum = (((coreFrequency - (desiredFrequency * 10) - (i2cRise * (desiredFrequency / 100) * (coreFrequency / 1000) / 10000)) * 10 + 5) / (desiredFrequency * 10));
		assert(2 <= baudSum && baudSum <= 0xff*2);

		if (baudSum % 2 == 1)
			return (baudSum / 2) | ((baudSum / 2 + 1) << 8);
		else
			return baudSum / 2;
	}
};

}
