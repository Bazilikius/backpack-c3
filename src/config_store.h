#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <Arduino.h>

struct DeviceConfig {
    char binding_phrase[64];
    uint8_t uid[6];
    int wifi_tx_power; // 0.25 dBm units, e.g. 44 (11dBm, ~50%) or 60 (15dBm, 75%)
    int ch_6pos;       // 1-based channel number, e.g. 11
    int ch_s2;         // 1-based channel number, e.g. 12
    int control_mode;  // 0: 6pos selects Band, S2 selects Channel
                       // 1: 6pos selects one of 6 favorit presets (configured in web interface)

    // 6 presets for Favorite Mode (Mode 1)
    uint8_t preset_bands[6];    // 0:A, 1:B, 2:E, 3:F, 4:R, 5:L, 6:D, 7:U, 8:O, 9:H
    uint8_t preset_channels[6]; // 0-7 (Channel 1-8)

    // SPI Pin configuration
    int pin_clk;
    int pin_data;
    int pin_cs;
};

extern DeviceConfig global_config;

void init_config();
void save_config();
void load_config();
void reset_config_defaults();

#endif // CONFIG_STORE_H
