#include "crsf_parser.h"
#include "config_store.h"
#include "spi_vtx.h"

CRSFParser crsf;

uint8_t current_selected_band = 4;    // Default: Band R (Raceband)
uint8_t current_selected_channel = 0; // Default: Channel 1
uint16_t current_selected_freq = 5658;  // Default: R1 (5658 MHz)

uint32_t last_crsf_packet_time = 0;

// 10 bands x 8 channels frequency table
const uint16_t b_frequencies[10][8] = {
    {5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725}, // Band 1 (A)
    {5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866}, // Band 2 (B)
    {5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945}, // Band 3 (E)
    {5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880}, // Band 4 (F)
    {5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917}, // Band 5 (R)
    {5333, 5373, 5413, 5453, 5493, 5533, 5573, 5613}, // Band 6 (Foxeer L)
    {5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621}, // Band 7 (Std L)
    {5325, 5348, 5366, 5384, 5402, 5420, 5438, 5456}, // Band 8 (U)
    {5474, 5492, 5510, 5528, 5546, 5564, 5582, 5600}, // Band 9 (O)
    {5653, 5693, 5733, 5773, 5813, 5853, 5893, 5933}  // Band 10 (H)
};

void CRSFParser::begin(HardwareSerial &serial, long baud, int rx_pin) {
    _serial = &serial;
    _serial->begin(baud, SERIAL_8N1, rx_pin, -1);

    // Initialize channels to standard mid value (1500us)
    for (int i = 0; i < CRSF_MAX_CHANNELS; i++) {
        channels[i] = 1500;
    }
}

uint8_t CRSFParser::crsf_crc8(const uint8_t *ptr, uint8_t len) {
    static const uint8_t table[256] = {
        0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83,
        0xD7, 0x02, 0xA8, 0x7D, 0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06,
        0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F, 0xA4, 0x71, 0xDB, 0x0E,
        0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
        0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75,
        0x21, 0xF4, 0x5E, 0x8B, 0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9,
        0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0, 0xCF, 0x1A, 0xB0, 0x65,
        0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
        0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA,
        0xEE, 0x3B, 0x91, 0x44, 0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F,
        0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16, 0xEF, 0x3A, 0x90, 0x45,
        0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
        0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E,
        0x6A, 0xBF, 0x15, 0xC0, 0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F,
        0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36, 0x19, 0xCC, 0x66, 0xB3,
        0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
        0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1,
        0xA5, 0x70, 0xDA, 0x0F, 0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74,
        0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D, 0xD6, 0x03, 0xA9, 0x7C,
        0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
        0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07,
        0x53, 0x86, 0x2C, 0xF9
    };
    uint8_t crc = 0;
    while (len--) crc = table[crc ^ *ptr++];
    return crc;
}

void CRSFParser::parseChannels(const uint8_t *p) {
    auto toUs = [](uint16_t r) { return (uint16_t)((r * 0.625f) + 880); };
    channels[0] = toUs((p[0] | p[1] << 8) & 0x07FF);
    channels[1] = toUs((p[1] >> 3 | p[2] << 5) & 0x07FF);
    channels[2] = toUs((p[2] >> 6 | p[3] << 2 | p[4] << 10) & 0x07FF);
    channels[3] = toUs((p[4] >> 1 | p[5] << 7) & 0x07FF);
    channels[4] = toUs((p[5] >> 4 | p[6] << 4) & 0x07FF);
    channels[5] = toUs((p[6] >> 7 | p[7] << 1 | p[8] << 9) & 0x07FF);
    channels[6] = toUs((p[8] >> 2 | p[9] << 6) & 0x07FF);
    channels[7] = toUs((p[9] >> 5 | p[10] << 3) & 0x07FF);
    channels[8] = toUs((p[11] | p[12] << 8) & 0x07FF);
    channels[9] = toUs((p[12] >> 3 | p[13] << 5) & 0x07FF);
    channels[10] = toUs((p[13] >> 6 | p[14] << 2 | p[15] << 10) & 0x07FF);
    channels[11] = toUs((p[15] >> 1 | p[16] << 7) & 0x07FF);
    channels[12] = toUs((p[16] >> 4 | p[17] << 4) & 0x07FF);
    channels[13] = toUs((p[17] >> 7 | p[18] << 1 | p[19] << 9) & 0x07FF);
    channels[14] = toUs((p[19] >> 2 | p[20] << 6) & 0x07FF);
    channels[15] = toUs((p[20] >> 5 | p[21] << 3) & 0x07FF);
    last_crsf_packet_time = millis();
    updated = true;
}

void CRSFParser::handle() {
    while (_serial->available()) {
        uint8_t b = _serial->read();
        switch (_state) {
            case 0:
                if (b == 0xC8 || b == 0xEE) { // Support standard sync bytes
                    _buf[0] = b;
                    _state = 1;
                }
                break;
            case 1:
                if (b >= 2 && b <= 62) {
                    _buf[1] = b;
                    _len = b;
                    _idx = 2;
                    _state = 2;
                } else {
                    _state = 0;
                }
                break;
            case 2:
                _buf[_idx++] = b;
                if (_idx >= _len + 2) {
                    if (crsf_crc8(&_buf[2], _len - 1) == _buf[_len + 1]) {
                        if (_buf[2] == 0x16) { // RC channels packed
                            parseChannels(&_buf[3]);
                        }
                    }
                    _state = 0;
                }
                break;
        }
    }
}

void process_crsf_channels() {
    uint8_t selected_band = current_selected_band;
    uint8_t selected_channel = current_selected_channel;

    int idx_6pos = global_config.ch_6pos - 1;
    int idx_s2 = global_config.ch_s2 - 1;

    if (idx_6pos < 0 || idx_6pos >= 16 || idx_s2 < 0 || idx_s2 >= 16) {
        return;
    }

    // Map the standard 1000-2000us values back to raw scale or compare directly
    uint16_t val_6pos = crsf.channels[idx_6pos];
    uint16_t val_s2 = crsf.channels[idx_s2];

    if (global_config.control_mode == 0) {
        // Mode 0: 6pos selects one of 6 configured bands, S2 selects Channel 1-8
        int pos_6pos = 0;
        if (val_6pos < 1150) {
            pos_6pos = 0;
        } else if (val_6pos < 1320) {
            pos_6pos = 1;
        } else if (val_6pos < 1490) {
            pos_6pos = 2;
        } else if (val_6pos < 1660) {
            pos_6pos = 3;
        } else if (val_6pos < 1830) {
            pos_6pos = 4;
        } else {
            pos_6pos = 5;
        }

        selected_band = global_config.pos6_bands[pos_6pos];

        // S2 continuous knob mapping to 8 channels
        if (val_s2 < 1125) {
            selected_channel = 0;
        } else if (val_s2 < 1250) {
            selected_channel = 1;
        } else if (val_s2 < 1375) {
            selected_channel = 2;
        } else if (val_s2 < 1500) {
            selected_channel = 3;
        } else if (val_s2 < 1625) {
            selected_channel = 4;
        } else if (val_s2 < 1750) {
            selected_channel = 5;
        } else if (val_s2 < 1875) {
            selected_channel = 6;
        } else {
            selected_channel = 7;
        }

    } else if (global_config.control_mode == 1) {
        // Mode 1: 6pos selects favorite preset
        int preset_idx = 0;
        if (val_6pos < 1150) {
            preset_idx = 0;
        } else if (val_6pos < 1320) {
            preset_idx = 1;
        } else if (val_6pos < 1490) {
            preset_idx = 2;
        } else if (val_6pos < 1660) {
            preset_idx = 3;
        } else if (val_6pos < 1830) {
            preset_idx = 4;
        } else {
            preset_idx = 5;
        }

        selected_band = global_config.preset_bands[preset_idx];
        selected_channel = global_config.preset_channels[preset_idx];
    }

    if (selected_band >= 10) selected_band = 4; // Default to R
    if (selected_channel >= 8) selected_channel = 0;

    uint16_t freq = b_frequencies[selected_band][selected_channel];

    if (selected_band != current_selected_band || selected_channel != current_selected_channel || freq != current_selected_freq) {
        current_selected_band = selected_band;
        current_selected_channel = selected_channel;
        current_selected_freq = freq;

        handle_vrx_change(current_selected_band, current_selected_channel, current_selected_freq);
    }
}
