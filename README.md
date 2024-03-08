**Experimental and incomplete. Do not use.**

Architecture unaware library that tries to program AVR devices by bit-banging ISP.

Used by: https://github.com/atoomnetmarc/lcd-network/tree/main/Firmware

This library has been programmed specifically to be used with the limited resources of the ATmega48. It is also used on the ESP32-S2.

Tested target devices:
| device |
| -- |
| ATmega48 |

You must implement `RiscvEmulatorImplementationSpecific.h` for your architecture in your own project:

```c
#include <stdint.h>

#ifndef AvrIspProgrammerImplementationSpecific_h
#define AvrIspProgrammerImplementationSpecific_h

/**
 * Initializes all the ISP I/O pins.
*/
inline void AvrIspProgrammerIoInit(void) {

}

/**
 * Set reset high.
*/
inline void AvrIspProgrammerIoResetHigh(void) {

}

/**
 * Set reset low.
*/
inline void AvrIspProgrammerIoResetLow(void) {

}

/**
 * Set SCK high.
*/
inline void AvrIspProgrammerIoSckHigh(void) {

}

/**
 * Set SCK low.
*/
inline void AvrIspProgrammerIoSckLow(void) {

}

/**
 * Set MOSI high.
*/
inline void AvrIspProgrammerIoMosiHigh(void) {

}

/**
 * Set MOSI low.
*/
inline void AvrIspProgrammerIoMosiLow(void) {

}

/**
 * Read MISO
*/
inline uint8_t AvrIspProgrammerIoMiso(void) {

}

/**
 * Delay certain amount of micro seconds.
*/
inline void AvrIspProgrammerDelayUs(double us) {
}
#endif
```



[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
