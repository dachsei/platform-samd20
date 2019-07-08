#pragma once

#include <cstdint>
#include <sam.h>
#include "clocks.h"
#include "utils.h"

/*
 * Synchronous SPI Master
 */
class SPI {
public:
	/**
	 * @param sercom The Sercom interface to use
	 * @param baud Baud rate (bits per second)
	 * @param clkGen ClockGenerator for generating the Baud clock. Must be at least 2 * baud
	 * @param mode SPI Mode 0, 1, 2 or 3
	 * @param lsbFirst If true LSB is transmitted first
	 * @param pinLayout a combination of SERCOM_SPI_CTRLA_DOPO and SERCOM_SPI_CTRLA_DIPO to set the pinLayout
	*/
	SPI(
		Sercom* sercom, unsigned baud, const ClockGenerator& clkGen,
		unsigned mode = 0, bool lsbFirst = false,
		uint32_t pinLayout = SERCOM_SPI_CTRLA_DOPO(0) | SERCOM_SPI_CTRLA_DIPO(2)
	)
		: sercom{sercom}
	{
		unsigned sercomIndex = getSercomIndex(sercom);
		PM->APBCMASK.reg |= 1 << (PM_APBCMASK_SERCOM0_Pos + sercomIndex);
		clkGen.routeToPeripheral(GCLK_CLKCTRL_ID_SERCOM0_CORE_Val + sercomIndex);

		sercom->SPI.CTRLA.reg = SERCOM_SPI_CTRLA_MODE_SPI_MASTER
			| (pinLayout & (SERCOM_SPI_CTRLA_DOPO_Msk | SERCOM_SPI_CTRLA_DIPO_Msk))
			| SERCOM_SPI_CTRLA_FORM_SPI
			| (mode << SERCOM_SPI_CTRLA_CPHA_Pos)
			| (lsbFirst << SERCOM_SPI_CTRLA_DORD_Pos);
		sercom->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_RXEN | SERCOM_SPI_CTRLB_CHSIZE(0);
		sercom->SPI.BAUD.reg = clkGen.frequency / (2 * baud) - 1;
		while (sercom->SPI.STATUS.bit.SYNCBUSY);
		sercom->SPI.CTRLA.bit.ENABLE = true;
	}

	/**
	 * Synchronously sends and receives a byte
	 * @param data Data to send
	 * @return Received data
	 */
	uint8_t transfer(uint8_t data) const
	{
		sercom->SPI.DATA.reg = data;
		while (!sercom->SPI.INTFLAG.bit.RXC);
		return sercom->SPI.DATA.reg;
	}

private:
	Sercom* const sercom;
};
