#pragma once

#include <cstdint>
#include <cstddef>
#include <sam.h>
#include "clocks.h"
#include "utils.h"

namespace mcu {

class UART {
public:
	UART(Sercom* port, const ClockGenerator& clkGen, unsigned baudRate, uint32_t pinLayout) noexcept
		:sercom(port->USART)
	{
		unsigned sercomIndex = util::getSercomIndex(port);
		PM->APBCMASK.reg |= 1 << (PM_APBCMASK_SERCOM0_Pos + sercomIndex);
		clkGen.routeToPeripheral(GCLK_CLKCTRL_ID_SERCOM0_CORE_Val + sercomIndex);

		while (sercom.STATUS.bit.SYNCBUSY);
		sercom.CTRLA.reg = SERCOM_USART_CTRLA_MODE_USART_INT_CLK | SERCOM_USART_CTRLA_FORM_0 | SERCOM_USART_CTRLA_DORD | pinLayout;
		sercom.CTRLB.reg = SERCOM_USART_CTRLB_CHSIZE(0) | SERCOM_USART_CTRLB_TXEN | SERCOM_USART_CTRLB_RXEN;
		sercom.BAUD.reg = 65536 - (65536ULL * 16 * baudRate) / clkGen.frequency;
		sercom.INTENSET.reg = SERCOM_USART_INTENSET_RXC;
		while (sercom.STATUS.bit.SYNCBUSY);
		sercom.CTRLA.bit.ENABLE = 1;
	}

	void transmit(std::byte data) noexcept
	{
		while (!sercom.INTFLAG.bit.DRE);
		sercom.DATA.reg = std::to_integer<uint8_t>(data);
	}

	template <typename Fun>
	void interrupt(Fun receiveCallback)
	{
		if (sercom.INTFLAG.bit.RXC) {
			receiveCallback(static_cast<std::byte>(sercom.DATA.reg));
		}
	}

private:
	SercomUsart& sercom;
};

};
