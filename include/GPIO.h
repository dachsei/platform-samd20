#pragma once

#include <cstdint>
#include <sam.h>

namespace mcu {

#if defined(__SAMD20E14__) || defined(__SAMD20E15__) || defined(__SAMD20E16__) || defined(__SAMD20E17__) || defined(__SAMD20E18__)
#define __SAMD20E__
#endif

class GPIO {
public:
#ifdef __SAMD20E__
	/**
	 * @param pin Pin number starting at 0. PA00 is 0
	 */
	explicit constexpr GPIO(uint8_t pin) : pin{pin} {}
#elif
	/**
	 * @param port Port number. PA is 0, PB is 1
	 * @param pin Pin number starting at 0. PA00 is 0, PB00 is also 0
	 */
	constexpr GPIO(uint8_t port, uint8_t pin) : port{port}, pin{pin} {};

	/**
	 * @param pin Pin number in a sequential space. PA00 is 0, PB00 is 32
	 */
	explicit constexpr GPIO(uint16_t pin) : GPIO(pin >> 5, pin & 0x1f);
#endif

	enum Mode {
		Output,
		Input,
		InputPullup,
		InputPulldown
	};

	static constexpr unsigned InterruptPeripheral = 0;

	void setMode(Mode mode)
	{
		switch (mode) {
			case Output:
				PORT->Group[port].DIRSET.reg = 1 << pin;
				PORT->Group[port].PINCFG[pin].reg = PORT_PINCFG_DRVSTR;
				PORT->Group[port].OUTCLR.reg = 1 << pin;
				break;
			case Input:
				PORT->Group[port].DIRCLR.reg = 1 << pin;
				PORT->Group[port].PINCFG[pin].reg = PORT_PINCFG_INEN;
				break;
			case InputPullup:
				PORT->Group[port].DIRCLR.reg = 1 << pin;
				PORT->Group[port].OUTSET.reg = 1 << pin;
				PORT->Group[port].PINCFG[pin].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
				break;
			case InputPulldown:
				PORT->Group[port].DIRCLR.reg = 1 << pin;
				PORT->Group[port].OUTCLR.reg = 1 << pin;
				PORT->Group[port].PINCFG[pin].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
				break;
		}
	}

	void setHigh() { PORT_IOBUS->Group[port].OUTSET.reg = 1 << pin; }
	void setLow() { PORT_IOBUS->Group[port].OUTCLR.reg = 1 << pin; }
	void toggle() { PORT_IOBUS->Group[port].OUTTGL.reg = 1 << pin; }
	void write(bool value) { if (value) setHigh(); else setLow(); }
	bool read() const { return (PORT->Group[port].IN.reg & (1 << pin)) != 0; }

	/**
	 * Enables Pinmux and selects the specified function
	 * @param function Peripheral function to select. Best practice is to use one of the MUX_* Macros
	 */
	void enablePeripheral(unsigned function)
	{
		if (pin & 1) {
			// Odd numbered pin
			PORT->Group[port].PMUX[pin >> 1].bit.PMUXO = function;
		} else {
			// Even numbered pin
			PORT->Group[port].PMUX[pin >> 1].bit.PMUXE = function;
		}
		PORT->Group[port].PINCFG[pin].bit.PMUXEN = true;
	}

	/**
	 * Disables the Pinmux
	 */
	void disablePeripheral() { PORT->Group[port].PINCFG[pin].bit.PMUXEN = false; }

#ifdef __SAMD20E__
	static constexpr uint8_t port = 0;
#elif
	const uint8_t port;
#endif
	const uint8_t pin;
};

} // namespace mcu
