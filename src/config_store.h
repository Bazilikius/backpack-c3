#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <Arduino.h>

struct DeviceConfig {
    int ch_6pos;       // 1-based channel number, e.g. 11
    int ch_s2;         // 1-based channel number, e.g. 12
    int control_mode;  // 0: 6pos selects Band, S2 selects Channel
                       // 1: 6pos selects one of 6 favorite presets (configured in web interface)

    // Mode 0: 6pos positions mapped bands (customizable)
    uint8_t pos6_bands[6]; // default: 0:A, 1:B, 2:E, 3:F, 4:R, 5:L (Foxeer L)

    // 6 presets for Favorite Mode (Mode 1)
    uint8_t preset_bands[6];    // 0:A, 1:B, 2:E, 3:F, 4:R, 5:L (Foxeer L), 6:D (Std L) etc.
    uint8_t preset_channels[6]; // 0-7 (Channel 1-8)

    // Pin configuration
    int pin_clk;
    int pin_data;
    int pin_cs;
    int pin_crsf_rx; // UART CRSF RX pin
    long crsf_baud;  // UART CRSF Baud rate (default 416700)

    // Legacy Mode configuration
    // 0: RTC6715 SPI Mode only (for Rapidfire, Steadyview, SPI mods)
    // 1: Legacy 3-bit Parallel standard mode (for Foxeer Wildfire, TBS Fusion, stock modules)
    // 2: Legacy 3-bit Parallel inverted mode
    int legacy_mode;
};

extern DeviceConfig global_config;

void init_config();
void save_config();
void load_config();
void reset_config_defaults();

#endif // CONFIG_STORE_H
