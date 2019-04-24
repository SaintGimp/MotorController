#include <stdio.h>
#include <atmel_start.h>
#include "tmc4671.h"

// TMC4671 datasheet:
// https://www.trinamic.com/products/integrated-circuits/details/tmc4671-es/

namespace servo_controller
{
    Tmc4671::Tmc4671(struct spi_m_sync_descriptor* spi, uint8_t csn_gpio)
    {
        this->spi = spi;
        this->csn_gpio = csn_gpio;

       	xfer.txbuf = transmit_buffer;
    	xfer.rxbuf = receive_buffer;
        xfer.size = 5;
    }

    void Tmc4671::Reset()
    {
        SetRegisterValue(TMC4671_dsADC_MCFG_B_MCFG_A, 0x00100010);
        SetRegisterValue(TMC4671_dsADC_MCLK_A, 0x20000000);
        SetRegisterValue(TMC4671_dsADC_MCLK_B, 0x00000000);
        SetRegisterValue(TMC4671_dsADC_MDEC_B_MDEC_A, 0x014E014E);
        SetRegisterValue(TMC4671_ADC_I1_SCALE_OFFSET, 0x000D5CC6);
        SetRegisterValue(TMC4671_ADC_I0_SCALE_OFFSET, 0x000D5D96);
        SetRegisterValue(TMC4671_ADC_I_SELECT, 0x09000001);
        SetRegisterValue(TMC4671_AENC_0_SCALE_OFFSET, 0x01000000);
        SetRegisterValue(TMC4671_AENC_1_SCALE_OFFSET, 0x01000000);
        SetRegisterValue(TMC4671_AENC_2_SCALE_OFFSET, 0x01000000);
        SetRegisterValue(TMC4671_AENC_SELECT, 0x03020100);
        SetRegisterValue(TMC4671_PWM_POLARITIES, 0x00000000);
        SetRegisterValue(TMC4671_PWM_MAXCNT, 0x00000F9F);
        SetRegisterValue(TMC4671_PWM_BBM_H_BBM_L, 0x00000505);
        SetRegisterValue(TMC4671_PWM_SV_CHOP, 0x00000007);
        SetRegisterValue(TMC4671_MOTOR_TYPE_N_POLE_PAIRS, 0x00030004);
        SetRegisterValue(TMC4671_PHI_E_EXT, 0x00000000);
        SetRegisterValue(TMC4671_UQ_UD_EXT, 0x00000FA0);
        SetRegisterValue(TMC4671_ABN_DECODER_MODE, 0x00000000);
        SetRegisterValue(TMC4671_ABN_DECODER_PPR, 0x00010000);
        SetRegisterValue(TMC4671_ABN_DECODER_COUNT, 0x00000000);
        SetRegisterValue(TMC4671_ABN_DECODER_COUNT_N, 0x00000000);
        SetRegisterValue(TMC4671_ABN_DECODER_PHI_E_PHI_M_OFFSET, 0x00000000);
        SetRegisterValue(TMC4671_HALL_MODE, 0x00000001);
        SetRegisterValue(TMC4671_HALL_PHI_E_PHI_M_OFFSET, 0x00000000);
        SetRegisterValue(TMC4671_AENC_DECODER_MODE, 0x00000000);
        SetRegisterValue(TMC4671_AENC_DECODER_PHI_A_OFFSET, 0x00000000);
        SetRegisterValue(TMC4671_AENC_DECODER_PPR, 0x00000001);
        SetRegisterValue(TMC4671_VELOCITY_SELECTION, 0x0000000C);
        SetRegisterValue(TMC4671_PHI_E_SELECTION, 0x00000005);
        SetRegisterValue(TMC4671_PID_FLUX_P_FLUX_I, 0x01000100);
        SetRegisterValue(TMC4671_PID_TORQUE_P_TORQUE_I, 0x027603E8);
        SetRegisterValue(TMC4671_PID_VELOCITY_P_VELOCITY_I, 0x00780064);
        SetRegisterValue(TMC4671_PID_TORQUE_FLUX_LIMITS, 0x00000320);
        SetRegisterValue(TMC4671_MODE_RAMP_MODE_MOTION, 0x00000000);
        SetRegisterValue(TMC4671_PID_TORQUE_FLUX_TARGET, 0x00000000);
        //SetRegisterValue(TMC4671_PID_POSITION_ACTUAL, 0x2129FB50);
    }

    void Tmc4671::StartMotor()
    {
        SetRegisterValue(TMC4671_MODE_RAMP_MODE_MOTION, 0x00000002);
    }

    void Tmc4671::StopMotor()
    {
        SetRegisterValue(TMC4671_MODE_RAMP_MODE_MOTION, 0x00000000);
    }

    void Tmc4671::SetTargetVelocity(uint32_t rpm)
    {
        SetRegisterValue(TMC4671_PID_VELOCITY_TARGET, rpm);
    }

    void Tmc4671::GetHardwareType(char* buffer)
    {
        SetRegisterValue(TMC4671_CHIPINFO_ADDR, 0);
        uint32_t value = GetRegisterValue(TMC4671_CHIPINFO_DATA);

        buffer[3] = value & 0xFF;
        value = value >> 8;
        buffer[2] = value & 0xFF;
        value = value >> 8;
        buffer[1] = value & 0xFF;
        value = value >> 8;
        buffer[0] = value & 0xFF;

        buffer[4] = '\0';
    }

    uint32_t Tmc4671::GetRegisterValue(uint8_t registerAddress)
    {
        // Clear the high bit of the address to indicate a read
        uint8_t command = registerAddress & 0x7F;
        return SendReceiveDatagram(command, 0);
    }

    void Tmc4671::SetRegisterValue(uint8_t registerAddress, uint32_t value)
    {
        // Set the high bit of the address to indicate a write
        uint8_t command = registerAddress | 0x80;
        SendReceiveDatagram(command, value);
    }

    uint32_t Tmc4671::SendReceiveDatagram(uint8_t command, uint32_t data)
    {
        // The datagram has 5 bytes, where byte 0 is the command
        // and the subsequent 4 represent the big-endian 32-bit value.
        // The command sets the high bit for a write operation or clears
        // it for a read operation.

        transmit_buffer[0] = command;
        // Put the data into the buffer in big-endian order
        for (int i = 3; i >= 0; i--)
        {
            transmit_buffer[i + 1] = data & 0xFF;
            data >>= 8;
        }

       	gpio_set_pin_level(csn_gpio, 0);
        spi_m_sync_transfer(&SPI_0, &xfer);
        gpio_set_pin_level(csn_gpio, 1);

        // The response is the same size as the transmitted
        // datagram, where the first byte is always 0 and the
        // rest are the big-endian 32-bit value.

        uint32_t response = receive_buffer[1];
        response = response << 8;
        response |= receive_buffer[2];
        response = response << 8;
        response |= receive_buffer[3];
        response = response << 8;
        response |= receive_buffer[4];

        return response;
    }
}
