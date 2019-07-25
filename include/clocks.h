#pragma once

#include <cassert>
#include <samd20.h>

class ClockSource {
public:
	/// @return value to use inf GCLK_GENCTRL_SRC
	virtual int id() const = 0;
	/// @return frequency in Hz
	virtual unsigned frequency() const = 0;
};

class ClockGenerator {
public:
	/**
	 * @param id 0-7
	 * @param source Clock to use as input for this generator
	 * @param div Division factor for the clock
	 * @param divToPow2 if true than the real division factor is 2 ^ (\p div + 1)
	 */
	ClockGenerator(uint8_t id, const ClockSource& source, uint16_t div = 1, bool divToPow2 = false)
		: id{id}, frequency{source.frequency() / (divToPow2 ? 1 << (div + 1) : div)}
	{
		GCLK->GENDIV.reg = GCLK_GENDIV_ID(id) | GCLK_GENDIV_DIV(div);
		GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(id)
			| GCLK_GENCTRL_SRC(source.id())
			| (divToPow2 << GCLK_GENCTRL_DIVSEL_Pos)
			| GCLK_GENCTRL_GENEN;
		while (GCLK->STATUS.bit.SYNCBUSY);
	}

	const unsigned frequency;
	const uint8_t id;

	/// Uses this generator as input to the specified peripheral clock
	void routeToPeripheral(uint8_t peripheralId) const
	{
		GCLK->CLKCTRL.reg = GCLK_CLKCTRL_GEN(id) | GCLK_CLKCTRL_ID(peripheralId) | GCLK_CLKCTRL_CLKEN;
		while (GCLK->STATUS.bit.SYNCBUSY);
	}
};


//class ExternalOscillator final : public ClockSource {};

//class External32KOscillator final : public ClockSource {};

//class Internal32KOscillator final : public ClockSource {};

class InternalLowPower32KOscillator final : public ClockSource {
public:
	InternalLowPower32KOscillator() = default;

	int id() const final { return GCLK_GENCTRL_SRC_OSCULP32K_Val; }
	unsigned frequency() const final { return 32768; }
};

class Internal8MegOscillator final : public ClockSource {
public:
	/**
	 * @param prescaler Prescaler value 0 - 3. Frequency is divided by 2 ^ \p prescaler
	 */
	explicit Internal8MegOscillator(uint8_t prescaler = 0)
		: prescaler{prescaler}
	{
		const uint32_t calib = SYSCTRL->OSC8M.reg & (SYSCTRL_OSC8M_CALIB_Msk | SYSCTRL_OSC8M_FRANGE_Msk);
		SYSCTRL->OSC8M.reg = calib | SYSCTRL_OSC8M_PRESC(prescaler) | SYSCTRL_OSC8M_ENABLE;
		while (!SYSCTRL->PCLKSR.bit.OSC8MRDY);
	}

	int id() const final { return GCLK_GENCTRL_SRC_OSC8M_Val; }
	unsigned frequency() const final { return 8000000 / (1 << prescaler); };

private:
	const uint8_t prescaler;
};

class DigitalFrequencyLockedLoop final : public ClockSource {
public:
	DigitalFrequencyLockedLoop(const ClockGenerator& reference, uint32_t multiplier)
		: frequency_{reference.frequency * multiplier}
	{
		assert(reference.frequency < 35100);
		reference.routeToPeripheral(GCLK_CLKCTRL_ID_DFLL48M_Val);
		SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;
		while (!SYSCTRL->PCLKSR.bit.DFLLRDY);

		const uint8_t coarseValue = (*reinterpret_cast<uint32_t*>(FUSES_DFLL48M_COARSE_CAL_ADDR) & FUSES_DFLL48M_COARSE_CAL_Msk) >> FUSES_DFLL48M_COARSE_CAL_Pos;
		SYSCTRL->DFLLVAL.reg = SYSCTRL_DFLLVAL_COARSE(coarseValue);
		SYSCTRL->DFLLMUL.reg = SYSCTRL_DFLLMUL_CSTEP(0x1f / 4) | SYSCTRL_DFLLMUL_FSTEP(0xff / 4) | SYSCTRL_DFLLMUL_MUL(multiplier);
		SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_MODE | SYSCTRL_DFLLCTRL_ENABLE;
		constexpr uint32_t readyBits = SYSCTRL_PCLKSR_DFLLRDY | SYSCTRL_PCLKSR_DFLLLCKC | SYSCTRL_PCLKSR_DFLLLCKF;
		while ((SYSCTRL->PCLKSR.reg & readyBits) != readyBits);
	}

	int id() const final { return GCLK_GENCTRL_SRC_DFLL48M_Val; }
	unsigned frequency() const final { return frequency_; }

private:
	const unsigned frequency_;
};

class ReadWaitStateInit final {
public:
	ReadWaitStateInit(unsigned waitStates)
	{
		NVMCTRL->CTRLB.bit.RWS = waitStates;
	}
};
