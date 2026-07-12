#include <Arduino.h>
#include "config_store.h"
#include "espnow_receiver.h"
#include "spi_vtx.h"
#include "web_server.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n[SYSTEM] Starting ELRS Backpack VRX Emulator...");

    // 1. Initialize Configuration
    init_config();
    Serial.printf("[SYSTEM] Binding Phrase: %s\n", global_config.binding_phrase);
    Serial.printf("[SYSTEM] UID: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  global_config.uid[0], global_config.uid[1], global_config.uid[2],
                  global_config.uid[3], global_config.uid[4], global_config.uid[5]);

    // 2. Initialize VTX SPI Interface
    init_spi_vtx();

    // 3. Initialize ESP-NOW with configured parameters
    init_espnow();

    // 4. Initialize Web UI Server
    init_web_server();

    Serial.println("[SYSTEM] Ready! Listening for ESP-NOW and Web UI Clients.");
}

void loop() {
    // Handle Web Server Clients
    handle_web_server();

    // Check if the ESP-NOW binding has timed out
    check_binding_timeout();

    // Tiny delay to keep background tasks happy
    delay(1);
}
