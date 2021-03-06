#pragma once

#include <cstdint>
#include <sam.h>
#include "clocks.h"
#include "utils.h"

namespace mcu {

/**
 * @brief Simple I2cSlave
 * 
 * Call I2cSlave::interruptHandler in the SERCOMx interrupt Handler
 */
class I2cSlave {
public:
	/**
	 * @param sercom The Sercom interface to use
	 * @param generatorCore ClockGenerator that should clock this sercom
	 * @param generator32k A ClockGenerator with a frequency of around 32kHz
	 * @param addr The address to listen on. Right aligned (without Read/Write bit)
	 * @param lowTimeout If enabled and SCL is held low for 25ms-35ms, this device will release its clock hold
	 * @param sdaHold Defines how long the SDA line is held with respect to the negative SCL edge.
	 *                0: 0ns, 1: 50-100ns, 2: 300-600ns, 3: 400-800ns.
	 *                Use SERCOM_I2CS_CTRLA_SDAHOLD_*_Val Macros
	 */
	I2cSlave(
		Sercom* sercom, const ClockGenerator& generatorCore, const ClockGenerator& generator32k, uint8_t addr,
		bool lowTimeout = false, unsigned sdaHold = SERCOM_I2CS_CTRLA_SDAHOLD_450_Val
	)
		: sercom{sercom}
	{
		unsigned sercomIndex = util::getSercomIndex(sercom);
		generatorCore.routeToPeripheral(GCLK_CLKCTRL_ID_SERCOM0_CORE_Val + sercomIndex);
		generator32k.routeToPeripheral(GCLK_CLKCTRL_ID_SERCOMX_SLOW_Val);
		PM->APBCMASK.reg |= 1 << (PM_APBCMASK_SERCOM0_Pos + sercomIndex);

		sercom->I2CS.CTRLA.reg = SERCOM_I2CS_CTRLA_MODE_I2C_SLAVE
			| (lowTimeout << SERCOM_I2CS_CTRLA_LOWTOUT_Pos)
			| SERCOM_I2CS_CTRLA_SDAHOLD(sdaHold);
		sercom->I2CS.CTRLB.reg = SERCOM_I2CS_CTRLB_AMODE(0) | SERCOM_I2CS_CTRLB_SMEN;
		sercom->I2CS.ADDR.reg = SERCOM_I2CS_ADDR_ADDR(addr);
		sercom->I2CS.INTENSET.reg = SERCOM_I2CS_INTFLAG_DRDY | SERCOM_I2CS_INTFLAG_PREC | SERCOM_I2CS_INTFLAG_AMATCH;
		sercom->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_DRDY | SERCOM_I2CS_INTFLAG_PREC | SERCOM_I2CS_INTFLAG_AMATCH;
		auto irq = static_cast<IRQn_Type>(static_cast<unsigned>(SERCOM0_IRQn) + sercomIndex);
		NVIC_ClearPendingIRQ(irq);
		NVIC_EnableIRQ(irq);
		while (sercom->I2CS.STATUS.bit.SYNCBUSY);
		sercom->I2CS.CTRLA.bit.ENABLE = true;
	}

	/**
	 * Call in the Sercom Interrupt handler
	 * @param onStop Callable that gets called on a Stop condition. Signature: void onStop()
	 * @param onWrite Callable that gets called on each byte written from master. Signature: void onWrite(uint8_t data)
	 * @param onRead Callable that gets called on each byte read. Return the byte to send to master. Signature: uint8_t onRead()
	 */
	template <typename FnStop, typename FnWrite, typename FnRead>
	void interruptHandler(FnStop onStop, FnWrite onWrite, FnRead onRead) const
	{
		uint8_t intflags = sercom->I2CS.INTFLAG.reg;
		if (intflags & SERCOM_I2CS_INTFLAG_AMATCH) {
			sercom->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_AMATCH;
		}
		if (intflags & SERCOM_I2CS_INTFLAG_PREC) {
			onStop();
			sercom->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_PREC;
		}
		if (intflags & SERCOM_I2CS_INTFLAG_DRDY) {
			if (sercom->I2CS.STATUS.bit.DIR == 0) {
				onWrite(sercom->I2CS.DATA.reg);
			} else {
				sercom->I2CS.DATA.reg = onRead();
			}
		}
	}

	Sercom* const sercom;
};

} // namespace mcu
