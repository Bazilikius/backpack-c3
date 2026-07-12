#ifndef ESPNOW_RECEIVER_H
#define ESPNOW_RECEIVER_H

#include <Arduino.h>

extern uint16_t last_crsf_channels[16];
extern bool crsf_channels_received;
extern uint32_t last_packet_time;

extern uint8_t current_selected_band;    // 0:A, 1:B, 2:E, 3:F, 4:R, 5:L
extern uint8_t current_selected_channel; // 0-7 (Channel 1-8)
extern uint16_t current_selected_freq;   // Frequency in MHz

extern const uint16_t b_frequencies[10][8];

extern bool is_binding_mode;
extern uint32_t binding_mode_start_time;

void init_espnow();
void stop_espnow();
void start_binding_mode();
void check_binding_timeout();
void process_espnow_channels(uint16_t* channels);
void update_wifi_tx_power(int power_val);

// Decodes raw packed CRSF channels (22 bytes) into 16 channels
void unpack_crsf_channels(const uint8_t* payload, uint16_t* channels);

#endif // ESPNOW_RECEIVER_H
