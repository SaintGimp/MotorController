#ifndef tmc4671_H
#define tmc4671_H

#include <stdint.h>
#include "tmc4671_registers.h"

namespace servo_controller
{
    class Tmc4671
    {
        public:
        Tmc4671(struct spi_m_sync_descriptor* spi, uint8_t csn_gpio);
        void Reset();

        uint32_t GetRegisterValue(uint8_t registerAddress);
        void SetRegisterValue(uint8_t registerAddress, uint32_t value);

        void GetHardwareType(char* buffer);
        // TODO: this interface should be at the register level and we should have another
        // high-level controller to represent a logical motor
        void StartMotor();
        void StopMotor();
        void SetTargetVelocity(uint32_t rpm);

        private:
        uint32_t SendReceiveDatagram(uint8_t command, uint32_t data);

        struct spi_m_sync_descriptor* spi;
        uint8_t csn_gpio;
        uint8_t transmit_buffer[5];
        uint8_t receive_buffer[5];
        spi_xfer xfer;
    };
}

#endif