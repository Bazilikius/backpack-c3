#include "config_store.h"
#include <Preferences.h>

DeviceConfig global_config;
Preferences preferences;

void reset_config_defaults() {
    global_config.ch_6pos = 11;
    global_config.ch_s2 = 12;
    global_config.control_mode = 0; // Mode 0: 6pos = Band, S2 = Channel

    // Default 6pos bands (0:A, 1:B, 2:E, 3:F, 4:R, 5:L (Foxeer L))
    for (int i = 0; i < 6; i++) {
        global_config.pos6_bands[i] = i;
    }

    // Favorite presets
    for (int i = 0; i < 6; i++) {
        global_config.preset_bands[i] = 4; // R (Raceband)
        global_config.preset_channels[i] = i; // channels 1 to 6 (0-5)
    }

    // Default SPI pins for bit-bang
    global_config.pin_clk = 4;
    global_config.pin_data = 5;
    global_config.pin_cs = 6;

#if defined(CONFIG_IDF_TARGET_ESP32C3)
    global_config.pin_crsf_rx = 10; // Default RX pin on ESP32-C3
#else
    global_config.pin_crsf_rx = 16; // Default RX pin on ESP32-S3
#endif

    global_config.crsf_baud = 416700; // Standard CRSF Baud rate
}

void init_config() {
    preferences.begin("elrs_backpack", false);
    load_config();
}

void load_config() {
    if (!preferences.isKey("init")) {
        reset_config_defaults();
        save_config();
        preferences.putBool("init", true);
        return;
    }

    global_config.ch_6pos = preferences.getInt("ch_6pos", 11);
    global_config.ch_s2 = preferences.getInt("ch_s2", 12);
    global_config.control_mode = preferences.getInt("ctrl_mode", 0);

    preferences.getBytes("pos6_bands", global_config.pos6_bands, 6);
    preferences.getBytes("preset_bands", global_config.preset_bands, 6);
    preferences.getBytes("preset_ch", global_config.preset_channels, 6);

    global_config.pin_clk = preferences.getInt("pin_clk", 4);
    global_config.pin_data = preferences.getInt("pin_data", 5);
    global_config.pin_cs = preferences.getInt("pin_cs", 6);
    global_config.pin_crsf_rx = preferences.getInt("pin_crsf_rx",
#if defined(CONFIG_IDF_TARGET_ESP32C3)
        10
#else
        16
#endif
    );
    global_config.crsf_baud = preferences.getLong("crsf_baud", 416700);
}

void save_config() {
    preferences.putInt("ch_6pos", global_config.ch_6pos);
    preferences.putInt("ch_s2", global_config.ch_s2);
    preferences.putInt("ctrl_mode", global_config.control_mode);

    preferences.putBytes("pos6_bands", global_config.pos6_bands, 6);
    preferences.putBytes("preset_bands", global_config.preset_bands, 6);
    preferences.putBytes("preset_ch", global_config.preset_channels, 6);

    preferences.putInt("pin_clk", global_config.pin_clk);
    preferences.putInt("pin_data", global_config.pin_data);
    preferences.putInt("pin_cs", global_config.pin_cs);
    preferences.putInt("pin_crsf_rx", global_config.pin_crsf_rx);
    preferences.putLong("crsf_baud", global_config.crsf_baud);
}
