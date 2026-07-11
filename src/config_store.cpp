#include "config_store.h"
#include <Preferences.h>
#include "uid_generator.h"

DeviceConfig global_config;
Preferences preferences;

void reset_config_defaults() {
    strcpy(global_config.binding_phrase, "expresslrs");
    generate_uid(global_config.binding_phrase, global_config.uid);
    global_config.wifi_tx_power = 44; // 11 dBm, stable and under 75%
    global_config.ch_6pos = 11;
    global_config.ch_s2 = 12;
    global_config.control_mode = 0; // Mode 0: 6pos = Band, S2 = Channel

    // Preset bands and channels (0:A, 1:B, 2:E, 3:F, 4:R, 5:L)
    for (int i = 0; i < 6; i++) {
        global_config.preset_bands[i] = 4; // R (Raceband)
        global_config.preset_channels[i] = i; // channels 1 to 6 (0-5)
    }

    // Default SPI pins for bit-bang
    global_config.pin_clk = 4;
    global_config.pin_data = 5;
    global_config.pin_cs = 6;
}

void init_config() {
    preferences.begin("elrs_backpack", false);
    load_config();
}

void load_config() {
    if (!preferences.isKey("init")) {
        // First run, set and save defaults
        reset_config_defaults();
        save_config();
        preferences.putBool("init", true);
        return;
    }

    String phrase = preferences.getString("phrase", "expresslrs");
    strcpy(global_config.binding_phrase, phrase.c_str());

    preferences.getBytes("uid", global_config.uid, 6);

    // Check if UID is all zeros, if so, generate from binding phrase
    bool all_zeros = true;
    for (int i = 0; i < 6; i++) {
        if (global_config.uid[i] != 0) {
            all_zeros = false;
            break;
        }
    }
    if (all_zeros) {
        generate_uid(global_config.binding_phrase, global_config.uid);
    }

    global_config.wifi_tx_power = preferences.getInt("tx_power", 44);
    global_config.ch_6pos = preferences.getInt("ch_6pos", 11);
    global_config.ch_s2 = preferences.getInt("ch_s2", 12);
    global_config.control_mode = preferences.getInt("ctrl_mode", 0);

    preferences.getBytes("preset_bands", global_config.preset_bands, 6);
    preferences.getBytes("preset_ch", global_config.preset_channels, 6);

    global_config.pin_clk = preferences.getInt("pin_clk", 4);
    global_config.pin_data = preferences.getInt("pin_data", 5);
    global_config.pin_cs = preferences.getInt("pin_cs", 6);
}

void save_config() {
    // Re-generate UID in case phrase changed
    generate_uid(global_config.binding_phrase, global_config.uid);

    preferences.putString("phrase", global_config.binding_phrase);
    preferences.putBytes("uid", global_config.uid, 6);
    preferences.putInt("tx_power", global_config.wifi_tx_power);
    preferences.putInt("ch_6pos", global_config.ch_6pos);
    preferences.putInt("ch_s2", global_config.ch_s2);
    preferences.putInt("ctrl_mode", global_config.control_mode);

    preferences.putBytes("preset_bands", global_config.preset_bands, 6);
    preferences.putBytes("preset_ch", global_config.preset_channels, 6);

    preferences.putInt("pin_clk", global_config.pin_clk);
    preferences.putInt("pin_data", global_config.pin_data);
    preferences.putInt("pin_cs", global_config.pin_cs);
}
