/*

Copyright 2024 Marc Ketel
SPDX-License-Identifier: Apache-2.0

*/

#ifndef AvrIspProgrammer_h
#define AvrIspProgrammer_h

#include <stdint.h>

#include <AvrIspProgrammerImplementationSpecific.h>

typedef struct {
    uint8_t low;
    uint8_t high;
    uint8_t extended;
} FuseBits_t;

/**
 * Delay for at least 2 cycles of the target CPU clock.
 */
static inline void AvrIspProgrammerDelay2CyclesTargetCPU(void) {
    AvrIspProgrammerDelayUs(5);
}

/**
 * Transfers 1 byte over SPI.
 *
 * @returns The answer of the SPI slave.
 */
uint8_t AvrIspProgrammerSpiTransfer(uint8_t value) {
    for (int8_t bit = 7; bit >= 0; bit--) {
        // Send bit
        if (value & _BV(bit)) {
            AvrIspProgrammerIoMosiHigh();
        } else {
            AvrIspProgrammerIoMosiLow();
        }

        // Receive bit
        value &= ~_BV(bit);
        if (AvrIspProgrammerIoMiso()) {
            value |= _BV(bit);
        }

        // Clock out the bit.
        AvrIspProgrammerIoSckHigh();
        AvrIspProgrammerDelay2CyclesTargetCPU();
        AvrIspProgrammerIoSckLow();
        AvrIspProgrammerDelay2CyclesTargetCPU();
    }

    return value;
}

/**
 * Generic send 4 bytes, receive 1 byte.
 *
 * @param byte1 The first instruction byte.
 * @param byte2 The second instruction byte.
 * @param byte3 The third instruction byte.
 * @param byte4 The fourth instruction byte.
 *
 * @returns Answer of the target at byte 4, if applicable.
 */
uint8_t AvrIspProgrammerSerialProgrammingInstruction(uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4) {
    AvrIspProgrammerSpiTransfer(byte1);
    AvrIspProgrammerSpiTransfer(byte2);
    AvrIspProgrammerSpiTransfer(byte3);
    return AvrIspProgrammerSpiTransfer(byte4);
}

/**
 * Disable serial programming.
 */
void AvrIspProgrammerDisableSerialProgramming(void) {
    AvrIspProgrammerIoResetHigh();
}

/**
 * Tries to enable serial programming.
 *
 * @returns 1 when target has been put into serial programming.
 */
uint8_t AvrIspProgrammerEnableSerialProgramming(void) {

    AvrIspProgrammerIoInit();

    // Retry a few times to enter programming mode.
    for (int8_t retries = 16; retries > 0; retries--) {
        AvrIspProgrammerIoSckLow();

        // Pulse reset.
        AvrIspProgrammerIoResetHigh();
        AvrIspProgrammerDelay2CyclesTargetCPU();
        AvrIspProgrammerIoResetLow();

        // Wait for at least 20ms and enable serial programming by sending the Programming Enable serial instruction to pin MOSI.
        AvrIspProgrammerDelayUs(20000);

        // Send programming enable
        AvrIspProgrammerSpiTransfer(0xAC);
        AvrIspProgrammerSpiTransfer(0x53);
        uint8_t echo = AvrIspProgrammerSpiTransfer(0x00);
        AvrIspProgrammerSpiTransfer(0x00);

        if (echo == 0x53) {
            return 1;
        }
    }

    AvrIspProgrammerDisableSerialProgramming();
    return 0;
}

/**
 * Poll RDY/~BSY flag
 *
 * @returns 0 when ready to accept new programming command.
 */
uint8_t AvrIspProgrammerPollRDY(void) {
    return AvrIspProgrammerSerialProgrammingInstruction(0xF0, 0x00, 0x00, 0x00) & 0b00000001;
}

/**
 * Waits forever on RDY flag to go to 0
 */
void AvrIspProgrammingWaitReady(void) {
    while (AvrIspProgrammerPollRDY() != 0) {
        AvrIspProgrammerDelayUs(1000);
    }
}

/**
 * Chip erase.
 */
void AvrIspProgrammerChipErase(void) {
    AvrIspProgrammerSerialProgrammingInstruction(0xAC, 0x80, 0x00, 0x00);
    AvrIspProgrammingWaitReady();
}

/**
 * Load extended address byte.
 *
 * @param extendedAddress The extended address byte.
 */
void AvrIspProgrammerLoadExtendedAddress(uint8_t extendedAddress) {
    AvrIspProgrammerSerialProgrammingInstruction(0x4D, 0x00, extendedAddress, 0x00);
}

/**
 * Load program memory page.
 *
 * @param address Byte address.
 * @param data The byte to load into page memory.
 */
void AvrIspProgrammerLoadProgramMemoryPage(uint16_t address, uint8_t data) {
    uint8_t wordAddress = address >> 1;

    // Serial.printf("Writing value 0x%.2X to page address %i\r\n", data, address);

    if (address & 0x01) {
        AvrIspProgrammerSerialProgrammingInstruction(0x48, 0x00, wordAddress, data); // High byte
    } else {
        AvrIspProgrammerSerialProgrammingInstruction(0x40, 0x00, wordAddress, data); // Low byte
    }
}

/**
 * Read program memory.
 *
 * @param address Byte address.
 *
 * @returns The byte read from memory.
 */
uint8_t AvrIspProgrammerReadProgramMemory(uint16_t address) {

    uint16_t wordAddress = address >> 1;
    uint8_t addressWordLSB = wordAddress & 0xFF;
    uint8_t addressWordMSB = wordAddress >> 8;

    if (address & 0x01) {
        // High byte
        return AvrIspProgrammerSerialProgrammingInstruction(0x28, addressWordMSB, addressWordLSB, 0x00);
    } else {
        // Low byte
        return AvrIspProgrammerSerialProgrammingInstruction(0x20, addressWordMSB, addressWordLSB, 0x00);
    }
}

/**
 * Read lock bits.
 */
uint8_t AvrIspProgrammerReadLockBits(void) {
    return AvrIspProgrammerSerialProgrammingInstruction(
        0x58,
        0x00,
        0x00,
        0x00);
}

/**
 * Read signature bytes.
 *
 * @param signature The 3 byte signature.
 */
void AvrIspProgrammerReadSignature(uint8_t signature[3]) {
    for (uint8_t i = 0; i < 3; i++) {
        signature[i] = AvrIspProgrammerSerialProgrammingInstruction(
            0x30,
            0x00,
            i,
            0x00);
    }
}

/**
 * Read all fuse bits.
 *
 * @param fusebits Fuse bits.
 */
void AvrIspProgrammerReadFuseBits(FuseBits_t *fusebits) {
    fusebits->low = AvrIspProgrammerSerialProgrammingInstruction(
        0x50,
        0x00,
        0x00,
        0x00);

    fusebits->high = AvrIspProgrammerSerialProgrammingInstruction(
        0x58,
        0x08,
        0x00,
        0x00);

    fusebits->extended = AvrIspProgrammerSerialProgrammingInstruction(
        0x50,
        0x08,
        0x00,
        0x00);
}

/**
 * Read calibration byte.
 */
uint8_t AvrIspProgrammerReadCalibrationByte(void) {
    return AvrIspProgrammerSerialProgrammingInstruction(
        0x38,
        0x00,
        0x00,
        0x00);
}

/**
 * Write memory page.
 *
 * @param address write page at byte address.
 */
void AvrIspProgrammerWriteMemoryPage(uint16_t address) {
    uint16_t wordAddress = address >> 1;
    uint8_t addressWordLSB = wordAddress & 0xFF;
    uint8_t addressWordMSB = wordAddress >> 8;

    AvrIspProgrammerSerialProgrammingInstruction(0x4C, addressWordMSB, addressWordLSB, 0x00);
    AvrIspProgrammingWaitReady();
}

/**
 * Write lock bits.
 */
void AvrIspProgrammerWriteLockBits(uint8_t lockbits) {
    AvrIspProgrammerSerialProgrammingInstruction(0xAC, 0xE0, 0x00, lockbits);
    AvrIspProgrammingWaitReady();
}

/**
 * Write all fuse bits.
 *
 * @param fusebits Fuse bits.
 */
void AvrIspProgrammerWriteFuseBits(FuseBits_t *fusebits) {
    AvrIspProgrammerSerialProgrammingInstruction(0xAC, 0xA0, 0x00, fusebits->low);
    AvrIspProgrammingWaitReady();
    AvrIspProgrammerSerialProgrammingInstruction(0xAC, 0xA8, 0x00, fusebits->high);
    AvrIspProgrammingWaitReady();
    AvrIspProgrammerSerialProgrammingInstruction(0xAC, 0xA4, 0x00, fusebits->extended);
    AvrIspProgrammingWaitReady();
}

#endif