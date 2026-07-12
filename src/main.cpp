#include <Arduino.h>
#include "config_store.h"
#include "crsf_parser.h"
#include "spi_vtx.h"
#include "web_server.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n[SYSTEM] Starting direct CRSF-to-VRX Controller...");

    // 1. Initialize Configuration
    init_config();

    // 2. Initialize VTX SPI Interface
    init_spi_vtx();

    // 3. Initialize Web UI Server (WiFi softAP)
    init_web_server();

    // 4. Initialize Hardware UART for direct CRSF listening
    // We use Serial1 (Hardware Serial 1)
    Serial.printf("[SYSTEM] Listening for direct CRSF Serial RX on Pin %d at %ld Baud\n",
                  global_config.pin_crsf_rx, global_config.crsf_baud);
    crsf.begin(Serial1, global_config.crsf_baud, global_config.pin_crsf_rx);

    Serial.println("[SYSTEM] Ready! Listening for CRSF Serial & Web UI Clients.");
}

void loop() {
    // Process incoming CRSF Serial Bytes
    crsf.handle();

    // If new channels packet received, process them
    if (crsf.updated) {
        process_crsf_channels();
        crsf.updated = false;
    }

    // Handle Web Server Clients
    handle_web_server();

    // Tiny delay to keep background tasks happy
    delay(1);
}
