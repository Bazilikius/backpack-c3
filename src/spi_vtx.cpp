#include "spi_vtx.h"
#include "config_store.h"

// Helper functions for bit-banging
static void spi_write_bit(bool bit) {
    digitalWrite(global_config.pin_clk, LOW);
    delayMicroseconds(1);
    digitalWrite(global_config.pin_data, bit ? HIGH : LOW);
    delayMicroseconds(1);
    digitalWrite(global_config.pin_clk, HIGH);
    delayMicroseconds(1);
    digitalWrite(global_config.pin_clk, LOW);
    delayMicroseconds(1);
}

void init_spi_vtx() {
    pinMode(global_config.pin_clk, OUTPUT);
    pinMode(global_config.pin_data, OUTPUT);
    pinMode(global_config.pin_cs, OUTPUT);

    // Initial state
    digitalWrite(global_config.pin_cs, HIGH);
    digitalWrite(global_config.pin_clk, LOW);
    digitalWrite(global_config.pin_data, LOW);
}

static uint32_t calcFrequencyData(uint16_t frequency) {
    uint32_t N;
    uint32_t A;
    // RTC6715 synthesizer formula
    uint16_t freq_val = (frequency - 479) / 2;
    N = freq_val / 32;
    A = freq_val % 32;
    return (N << 7) | A;
}

void set_vrx_frequency(uint16_t frequency) {
    uint32_t reg_data = calcFrequencyData(frequency);

    // Enable SPI transmission (CS Low)
    digitalWrite(global_config.pin_cs, HIGH);
    delayMicroseconds(5);
    digitalWrite(global_config.pin_cs, LOW);
    delayMicroseconds(5);

    // Register Address (0x01) LSB first: 1, 0, 0, 0
    spi_write_bit(true);
    spi_write_bit(false);
    spi_write_bit(false);
    spi_write_bit(false);

    // Read/Write bit: 1 = Write
    spi_write_bit(true);

    // Data: 16 LSB bits of reg_data
    for (int i = 0; i < 16; i++) {
        spi_write_bit(reg_data & 1);
        reg_data >>= 1;
    }

    // 4 bits zero padding (total 20 bits of data)
    spi_write_bit(false);
    spi_write_bit(false);
    spi_write_bit(false);
    spi_write_bit(false);

    // Disable SPI transmission (CS High)
    delayMicroseconds(5);
    digitalWrite(global_config.pin_cs, HIGH);

    // Pull CLK and DATA low for safety
    digitalWrite(global_config.pin_clk, LOW);
    digitalWrite(global_config.pin_data, LOW);

    Serial.printf("[SPI_VTX] Frequency tuned to: %d MHz via SPI\n", frequency);
}

// Handler called when channel is changed
void handle_vrx_change(uint8_t band, uint8_t channel, uint16_t freq) {
    char band_name = 'A';
    switch (band) {
        case 0: band_name = 'A'; break;
        case 1: band_name = 'B'; break;
        case 2: band_name = 'E'; break;
        case 3: band_name = 'F'; break;
        case 4: band_name = 'R'; break;
        case 5: band_name = 'L'; break; // Foxeer L
        case 6: band_name = 'D'; break; // Std L
        case 7: band_name = 'U'; break;
        case 8: band_name = 'O'; break;
        case 9: band_name = 'H'; break;
    }
    Serial.printf("[VRX] Channel changed to Band %c Channel %d (%d MHz)\n", band_name, channel + 1, freq);

    // Always attempt RTC6715 SPI transmission first
    set_vrx_frequency(freq);

    // Handle legacy 3-bit binary parallel channel modes (CH1-CH3)
    if (global_config.legacy_mode == 1 || global_config.legacy_mode == 2) {
        // Ensure pins are OUTPUT
        pinMode(global_config.pin_data, OUTPUT); // CH1
        pinMode(global_config.pin_cs, OUTPUT);   // CH2
        pinMode(global_config.pin_clk, OUTPUT);  // CH3

        bool bit0 = (channel & 1);
        bool bit1 = ((channel >> 1) & 1);
        bool bit2 = ((channel >> 2) & 1);

        if (global_config.legacy_mode == 2) {
            // Inverted logic levels
            bit0 = !bit0;
            bit1 = !bit1;
            bit2 = !bit2;
        }

        digitalWrite(global_config.pin_data, bit0 ? HIGH : LOW);
        digitalWrite(global_config.pin_cs, bit1 ? HIGH : LOW);
        digitalWrite(global_config.pin_clk, bit2 ? HIGH : LOW);

        Serial.printf("[VRX] Legacy 3-bit parallel levels set: CH1(DATA)=%d, CH2(CS)=%d, CH3(CLK)=%d\n",
                      bit0, bit1, bit2);
    }
}
