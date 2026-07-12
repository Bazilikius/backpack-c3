#include "espnow_receiver.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "config_store.h"

uint16_t last_crsf_channels[16] = {0};
bool crsf_channels_received = false;
uint32_t last_packet_time = 0;

uint8_t current_selected_band = 4;    // Default: Band R (Raceband)
uint8_t current_selected_channel = 0; // Default: Channel 1 (0)
uint16_t current_selected_freq = 5658;  // Default: R1 (5658 MHz)

bool is_binding_mode = false;
uint32_t binding_mode_start_time = 0;

// 10 bands x 8 channels frequency table
const uint16_t b_frequencies[10][8] = {
    {5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725}, // Band A (0)
    {5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866}, // Band B (1)
    {5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945}, // Band E (2)
    {5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880}, // Band F (3)
    {5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917}, // Band R (4)
    {5333, 5373, 5413, 5453, 5493, 5533, 5573, 5613}, // Band L (5)
    {5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621}, // Band D (6)
    {5325, 5348, 5366, 5384, 5402, 5420, 5438, 5456}, // Band U (7)
    {5474, 5492, 5510, 5528, 5546, 5564, 5582, 5600}, // Band O (8)
    {5653, 5693, 5733, 5773, 5813, 5853, 5893, 5933}  // Band H (9)
};

// Forward declaration of internal function to notify SPI of change
void handle_vrx_change(uint8_t band, uint8_t channel, uint16_t freq);

// CRSF CRC8 calculation using DVB-S2 poly
uint8_t crsf_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// MSP v2 CRC8-DVB-S2 calculation using polynomial 0xD5
uint8_t crc8_dvb_s2_one_byte(uint8_t crc, uint8_t a) {
    crc ^= a;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ 0xD5;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

uint8_t calculate_msp_v2_crc8(const uint8_t *packet, uint16_t len) {
    uint8_t crc = 0;
    // CRC is calculated over bytes from index 2 up to len - 2 (type, flags, function, size, payload)
    for (uint16_t i = 2; i < len - 1; i++) {
        crc = crc8_dvb_s2_one_byte(crc, packet[i]);
    }
    return crc;
}

// Unpack CRSF channels data from 22-byte packed representation
void unpack_crsf_channels(const uint8_t* payload, uint16_t* channels) {
    channels[0]  = (payload[0]  | (payload[1]  << 8))                    & 0x07FF;
    channels[1]  = ((payload[1]  >> 3) | (payload[2]  << 5))             & 0x07FF;
    channels[2]  = ((payload[2]  >> 6) | (payload[3]  << 2) | (payload[4] << 10)) & 0x07FF;
    channels[3]  = ((payload[4]  >> 1) | (payload[5]  << 7))             & 0x07FF;
    channels[4]  = ((payload[5]  >> 4) | (payload[6]  << 4))             & 0x07FF;
    channels[5]  = ((payload[6]  >> 7) | (payload[7]  << 1) | (payload[8] << 9))  & 0x07FF;
    channels[6]  = ((payload[8]  >> 2) | (payload[9]  << 6))             & 0x07FF;
    channels[7]  = ((payload[9]  >> 5) | (payload[10] << 3))             & 0x07FF;
    channels[8]  = (payload[11] | (payload[12] << 8))                    & 0x07FF;
    channels[9]  = ((payload[12] >> 3) | (payload[13] << 5))             & 0x07FF;
    channels[10] = ((payload[13] >> 6) | (payload[14] << 2) | (payload[15] << 10)) & 0x07FF;
    channels[11] = ((payload[15] >> 1) | (payload[16] << 7))             & 0x07FF;
    channels[12] = ((payload[16] >> 4) | (payload[17] << 4))             & 0x07FF;
    channels[13] = ((payload[17] >> 7) | (payload[18] << 1) | (payload[19] << 9))  & 0x07FF;
    channels[14] = ((payload[19] >> 2) | (payload[20] << 6))             & 0x07FF;
    channels[15] = ((payload[20] >> 5) | (payload[21] << 3))             & 0x07FF;
}

// Maps 11-bit RC switch positions and kontinuier continuous pots
void process_espnow_channels(uint16_t* channels) {
    uint8_t selected_band = current_selected_band;
    uint8_t selected_channel = current_selected_channel;

    // Safety check on channel configuration
    int idx_6pos = global_config.ch_6pos - 1;
    int idx_s2 = global_config.ch_s2 - 1;

    if (idx_6pos < 0 || idx_6pos >= 16 || idx_s2 < 0 || idx_s2 >= 16) {
        return;
    }

    uint16_t val_6pos = channels[idx_6pos];
    uint16_t val_s2 = channels[idx_s2];

    if (global_config.control_mode == 0) {
        // Mode 0: 6pos selects Band A-L (Bands 0-5), S2 selects Channel 1-8 (Channels 0-7)

        // 6pos mapping (6 regions)
        // CRSF Range 172 to 1811 (center around 992)
        if (val_6pos < 440) {
            selected_band = 0; // Band A
        } else if (val_6pos < 710) {
            selected_band = 1; // Band B
        } else if (val_6pos < 990) {
            selected_band = 2; // Band E
        } else if (val_6pos < 1260) {
            selected_band = 3; // Band F
        } else if (val_6pos < 1530) {
            selected_band = 4; // Band R
        } else {
            selected_band = 5; // Band L
        }

        // S2 continuous knob mapping (8 regions of ~205 width across 172-1811 range)
        if (val_s2 < 377) {
            selected_channel = 0; // Channel 1
        } else if (val_s2 < 582) {
            selected_channel = 1; // Channel 2
        } else if (val_s2 < 787) {
            selected_channel = 2; // Channel 3
        } else if (val_s2 < 992) {
            selected_channel = 3; // Channel 4
        } else if (val_s2 < 1197) {
            selected_channel = 4; // Channel 5
        } else if (val_s2 < 1402) {
            selected_channel = 5; // Channel 6
        } else if (val_s2 < 1607) {
            selected_channel = 6; // Channel 7
        } else {
            selected_channel = 7; // Channel 8
        }

    } else if (global_config.control_mode == 1) {
        // Mode 1: 6pos selects one of 6 favorite presets (defined in web interface)
        int preset_idx = 0;
        if (val_6pos < 440) {
            preset_idx = 0;
        } else if (val_6pos < 710) {
            preset_idx = 1;
        } else if (val_6pos < 990) {
            preset_idx = 2;
        } else if (val_6pos < 1260) {
            preset_idx = 3;
        } else if (val_6pos < 1530) {
            preset_idx = 4;
        } else {
            preset_idx = 5;
        }

        selected_band = global_config.preset_bands[preset_idx];
        selected_channel = global_config.preset_channels[preset_idx];
    }

    // Bounds checking
    if (selected_band >= 10) selected_band = 4; // default R
    if (selected_channel >= 8) selected_channel = 0;

    uint16_t freq = b_frequencies[selected_band][selected_channel];

    if (selected_band != current_selected_band || selected_channel != current_selected_channel || freq != current_selected_freq) {
        current_selected_band = selected_band;
        current_selected_channel = selected_channel;
        current_selected_freq = freq;

        handle_vrx_change(current_selected_band, current_selected_channel, current_selected_freq);
    }
}

// Internal callback for processing MSP packages (e.g. MSP_SET_VTX_CONFIG = 89)
void process_msp_packet(const uint8_t* payload, uint8_t size) {
    if (size < 4) return;
    uint8_t msp_cmd = payload[0];

    // Command 89 (0x59) is MSP_SET_VTX_CONFIG
    if (msp_cmd == 89) {
        uint8_t msp_band = payload[1];    // 1-indexed (1:A, 2:B, 3:E, 4:F, 5:R, 6:L)
        uint8_t msp_channel = payload[2]; // 1-indexed (1-8)

        if (msp_band >= 1 && msp_band <= 10 && msp_channel >= 1 && msp_channel <= 8) {
            uint8_t new_band = msp_band - 1;
            uint8_t new_channel = msp_channel - 1;
            uint16_t freq = b_frequencies[new_band][new_channel];

            current_selected_band = new_band;
            current_selected_channel = new_channel;
            current_selected_freq = freq;

            handle_vrx_change(current_selected_band, current_selected_channel, current_selected_freq);
        }
    }
}

// ESP-NOW Data Received Callback
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void on_data_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    const uint8_t* mac = recv_info->src_addr;
#else
void on_data_recv_cb(const uint8_t *mac, const uint8_t *data, int len) {
#endif
    last_packet_time = millis();

    if (len < 4) return;

    // Raw MSP v2 Over ESP-NOW Parsing & Binding logic
    if (len >= 9 && data[0] == '$' && data[1] == 'X') {
        uint8_t type = data[2]; // '<' or '>'
        uint16_t function = data[4] | (data[5] << 8);
        uint16_t size = data[6] | (data[7] << 8);

        if (size + 9 <= len) {
            uint8_t expected_crc = data[size + 8];
            uint8_t calculated_crc = calculate_msp_v2_crc8(data, size + 9);

            if (expected_crc == calculated_crc) {
                const uint8_t* payload = &data[8];

                // 1. Handle official MSP_ELRS_BIND (0x0009) packet in binding mode
                if (is_binding_mode && function == 0x0009) {
                    if (size >= 6) {
                        is_binding_mode = false;

                        // Extract official binding UID (first 6 bytes of bind payload)
                        memcpy(global_config.uid, payload, 6);
                        strcpy(global_config.binding_phrase, "bound via button");
                        save_config();

                        Serial.printf("[BIND] MSP_ELRS_BIND successful! UID: %02X:%02X:%02X:%02X:%02X:%02X\n",
                                      global_config.uid[0], global_config.uid[1], global_config.uid[2],
                                      global_config.uid[3], global_config.uid[4], global_config.uid[5]);

                        stop_espnow();
                        init_espnow();
                        return;
                    }
                }

                // 2. Handle official MSP_SET_VTX_CONFIG (0x0059)
                if (function == 0x0059) {
                    if (size >= 1) {
                        uint8_t msp_channel_idx = payload[0]; // 0-47
                        uint8_t msp_band = msp_channel_idx / 8;
                        uint8_t msp_channel = msp_channel_idx % 8;

                        if (msp_band < 10 && msp_channel < 8) {
                            uint16_t freq = b_frequencies[msp_band][msp_channel];
                            current_selected_band = msp_band;
                            current_selected_channel = msp_channel;
                            current_selected_freq = freq;

                            handle_vrx_change(current_selected_band, current_selected_channel, current_selected_freq);
                        }
                    }
                }
            }
        }
    }

    // Fallback simple binding mode if no MSP_ELRS_BIND packet arrives but standard packets are received
    if (is_binding_mode) {
        bool fallback_valid = false;

        uint8_t sync = data[0];
        if (sync == 0xEE || sync == 0xC8 || sync == 0xEA || sync == 0xC4) {
            uint8_t crsf_len = data[1];
            if (crsf_len + 2 <= len) {
                uint8_t expected_crc = data[crsf_len + 1];
                uint8_t calculated_crc = crsf_crc8(&data[2], crsf_len - 1);
                if (expected_crc == calculated_crc) {
                    fallback_valid = true;
                }
            }
        }
        else if (data[0] == '$' && data[1] == 'M' && data[2] == '<') {
            fallback_valid = true;
        }

        if (fallback_valid) {
            is_binding_mode = false;

            // Set learned MAC as UID, save it and clear/update binding phrase
            memcpy(global_config.uid, mac, 6);
            strcpy(global_config.binding_phrase, "bound via button");
            save_config();

            Serial.printf("[BIND] Fallback bound to TX with MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

            // Restart normal ESP-NOW with newly saved MAC/UID
            stop_espnow();
            init_espnow();
            return;
        }
    }

    // Try to parse CRSF frame
    // Sync byte is normally 0xEE (RC), 0xC8 (Telem to FC), 0xEA (Telem from handset)
    uint8_t sync = data[0];
    if (sync == 0xEE || sync == 0xC8 || sync == 0xEA || sync == 0xC4) {
        uint8_t crsf_len = data[1];
        // The length is the number of bytes after the length field (type, payload, CRC)
        if (crsf_len + 2 <= len) {
            // Verify CRC8
            uint8_t expected_crc = data[crsf_len + 1];
            uint8_t calculated_crc = crsf_crc8(&data[2], crsf_len - 1);
            if (expected_crc == calculated_crc) {
                uint8_t type = data[2];
                if (type == 0x16) { // CRSF_FRAMETYPE_RC_CHANNELS_PACKED
                    unpack_crsf_channels(&data[3], last_crsf_channels);
                    crsf_channels_received = true;
                    process_espnow_channels(last_crsf_channels);
                } else if (type == 0x32 || type == 0x3A) { // Command/MSP
                    // Inside CRSF carrying MSP, the payload starts after header
                    // Let's check for MSP frame signature '$M<'
                    if (crsf_len >= 8 && data[3] == '$' && data[4] == 'M' && data[5] == '<') {
                        uint8_t msp_size = data[6];
                        if (msp_size + 8 <= crsf_len) {
                            process_msp_packet(&data[7], msp_size);
                        }
                    }
                }
            }
        }
    } else if (data[0] == '$' && data[1] == 'M' && data[2] == '<') {
        // Raw MSP frame
        uint8_t msp_size = data[3];
        if (msp_size + 6 <= len) {
            process_msp_packet(&data[4], msp_size);
        }
    }
}

void update_wifi_tx_power(int power_val) {
    // Valid values are 8 to 78 (or 80)
    // 44 is 11 dBm, 60 is 15 dBm
    if (power_val < 8) power_val = 8;
    if (power_val > 78) power_val = 78;

    esp_wifi_set_max_tx_power(power_val);
}

void init_espnow() {
    // Initialize WiFi STA mode
    WiFi.mode(WIFI_AP_STA);

    // Stop WiFi driver temporarily to allow setting custom MAC
    esp_wifi_stop();

    // Set Station MAC address to UID matching the official ExpressLRS Backpack protocol
    uint8_t mac_addr[6];
    memcpy(mac_addr, global_config.uid, 6);
    mac_addr[0] &= 0xFE; // Force unicast (clear multicast bit)

    esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, mac_addr);
    if (err != ESP_OK) {
        Serial.printf("[SYSTEM] Failed to set STA MAC: %s\n", esp_err_to_name(err));
    } else {
        Serial.printf("[SYSTEM] STA MAC successfully set to: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    }

    // Set AP MAC address explicitly to be distinct and valid (forcing locally-administered 0x02 bit)
    // to prevent the ESP WiFi driver from failing to start the SoftAP interface.
    uint8_t ap_mac[6];
    memcpy(ap_mac, mac_addr, 6);
    ap_mac[0] |= 0x02; // Force locally administered on AP MAC
    ap_mac[5] ^= 1;    // Ensure last byte is different from STA MAC to avoid collisions

    err = esp_wifi_set_mac(WIFI_IF_AP, ap_mac);
    if (err != ESP_OK) {
        Serial.printf("[SYSTEM] Failed to set AP MAC: %s\n", esp_err_to_name(err));
    } else {
        Serial.printf("[SYSTEM] AP MAC successfully set to: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      ap_mac[0], ap_mac[1], ap_mac[2], ap_mac[3], ap_mac[4], ap_mac[5]);
    }

    // Restart WiFi driver
    esp_wifi_start();

    // Set WiFi channel to 1 for ESP-NOW Backpack communication
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    // Initialize ESP-NOW
    if (esp_now_init() == ESP_OK) {
        esp_now_register_recv_cb(on_data_recv_cb);
    }

    // Limit WiFi transmitter power to avoid S3 Super Mini brownout / noise issues
    update_wifi_tx_power(global_config.wifi_tx_power);
}

void stop_espnow() {
    esp_now_unregister_recv_cb();
    esp_now_deinit();
}

void start_binding_mode() {
    is_binding_mode = true;
    binding_mode_start_time = millis();
    Serial.println("[BIND] Starting bind mode: listening for valid packets...");
}

void check_binding_timeout() {
    if (is_binding_mode && (millis() - binding_mode_start_time > 30000)) {
        is_binding_mode = false;
        Serial.println("[BIND] Timeout. Exiting bind mode...");
    }
}
