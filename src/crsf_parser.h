#ifndef CRSF_PARSER_H
#define CRSF_PARSER_H

#include <Arduino.h>

#define CRSF_MAX_CHANNELS 16

class CRSFParser {
public:
    uint16_t channels[CRSF_MAX_CHANNELS];
    bool updated = false;

    void begin(HardwareSerial &serial, long baud, int rx_pin);
    void handle();

private:
    HardwareSerial *_serial;
    uint8_t _buf[64];
    uint8_t _state = 0;
    uint8_t _len = 0;
    uint8_t _idx = 0;
    uint8_t crsf_crc8(const uint8_t *ptr, uint8_t len);
    void parseChannels(const uint8_t *p);
};

extern CRSFParser crsf;

extern uint8_t current_selected_band;    // 0:A, 1:B, 2:E, 3:F, 4:R, 5:L (Foxeer L), 6:D (Std L)...
extern uint8_t current_selected_channel; // 0-7 (Channel 1-8)
extern uint16_t current_selected_freq;   // Frequency in MHz

extern uint32_t last_crsf_packet_time;

extern const uint16_t b_frequencies[10][8];

void process_crsf_channels();

#endif // CRSF_PARSER_H
