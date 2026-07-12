#include "web_server.h"
#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>
#include "config_store.h"
#include "espnow_receiver.h"
#include "spi_vtx.h"

WebServer server(80);

// CSS and HTML for Web UI
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ELRS Backpack VRX Emulator</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #121212;
            color: #e0e0e0;
            margin: 0;
            padding: 10px;
        }
        .container {
            max-width: 800px;
            margin: auto;
            background-color: #1e1e1e;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.3);
        }
        h1, h2, h3 {
            color: #ff5722;
            text-align: center;
        }
        h1 { margin-bottom: 5px; }
        .subtitle {
            text-align: center;
            color: #888;
            margin-top: 0;
            margin-bottom: 25px;
            font-size: 0.9em;
        }
        .card {
            background-color: #262626;
            padding: 15px;
            margin-bottom: 20px;
            border-radius: 6px;
            border-left: 4px solid #ff5722;
        }
        .grid-2 {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
        }
        @media(max-width: 600px) {
            .grid-2 { grid-template-columns: 1fr; }
        }
        .form-group {
            margin-bottom: 12px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
            font-size: 0.9em;
        }
        input[type="text"], input[type="number"], select {
            width: 100%;
            padding: 8px;
            border: 1px solid #444;
            background-color: #333;
            color: #fff;
            border-radius: 4px;
            box-sizing: border-box;
        }
        input:focus, select:focus {
            outline: none;
            border-color: #ff5722;
        }
        .btn {
            background-color: #ff5722;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-weight: bold;
            display: block;
            width: 100%;
            text-align: center;
            margin-top: 15px;
        }
        .btn:hover {
            background-color: #e64a19;
        }
        .status-container {
            display: flex;
            justify-content: space-around;
            text-align: center;
            background-color: #2d2d2d;
            padding: 10px;
            border-radius: 6px;
            margin-bottom: 15px;
        }
        .status-val {
            font-size: 1.2em;
            font-weight: bold;
            color: #ff5722;
        }
        .badge {
            background-color: #d32f2f;
            color: white;
            padding: 3px 8px;
            border-radius: 12px;
            font-size: 0.8em;
            display: inline-block;
        }
        .badge.connected {
            background-color: #388e3c;
        }
        @keyframes blinker {
            50% { opacity: 0; }
        }
        .channel-bar-container {
            margin-top: 10px;
        }
        .channel-bar {
            background-color: #444;
            height: 18px;
            border-radius: 9px;
            overflow: hidden;
            position: relative;
        }
        .channel-fill {
            background-color: #ff5722;
            height: 100%;
            width: 50%;
            transition: width 0.1s ease;
        }
        .channel-text {
            position: absolute;
            width: 100%;
            text-align: center;
            font-size: 0.8em;
            font-weight: bold;
            color: #fff;
            line-height: 18px;
            top: 0;
        }
        .vtx-table-container {
            margin-top: 20px;
            overflow-x: auto;
        }
        .vtx-table {
            width: 100%;
            border-collapse: collapse;
            font-size: 0.9em;
            text-align: center;
        }
        .vtx-table th, .vtx-table td {
            padding: 8px 4px;
            border: 1px solid #333;
        }
        .vtx-table th {
            background-color: #262626;
            color: #ff5722;
            font-weight: bold;
        }
        .vtx-table td.band-label {
            background-color: #262626;
            color: #fff;
            font-weight: bold;
        }
        .vtx-table td.freq-cell {
            background-color: #1e1e1e;
            cursor: pointer;
            transition: background-color 0.2s, color 0.2s;
        }
        .vtx-table td.freq-cell:hover {
            background-color: #ff5722;
            color: #fff;
        }
        .vtx-table td.active-freq {
            background-color: #ff5722 !important;
            color: #fff !important;
            font-weight: bold;
            box-shadow: inset 0 0 5px rgba(0,0,0,0.5);
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ELRS Backpack VRX Emulator</h1>
        <div class="subtitle">ESP32-S3 Super Mini VRX Channel Controller</div>

        <!-- Live Status Card -->
        <div class="card">
            <h2>Live Receiver Status</h2>
            <div class="status-container">
                <div>
                    <div>Link Status</div>
                    <div id="link-status"><span class="badge">DISCONNECTED</span></div>
                </div>
                <div>
                    <div>Tuned Frequency</div>
                    <div id="tuned-freq" class="status-val">5658 MHz</div>
                </div>
                <div>
                    <div>Tuned Channel</div>
                    <div id="tuned-channel" class="status-val">R1</div>
                </div>
            </div>

            <div class="grid-2">
                <div>
                    <label>6POS Switch (Channel <span id="6pos-ch-num">11</span>)</label>
                    <div class="channel-bar-container">
                        <div class="channel-bar">
                            <div id="6pos-bar" class="channel-fill"></div>
                            <div id="6pos-text" class="channel-text">1500us</div>
                        </div>
                    </div>
                </div>
                <div>
                    <label>S2 continuous (Channel <span id="s2-ch-num">12</span>)</label>
                    <div class="channel-bar-container">
                        <div class="channel-bar">
                            <div id="s2-bar" class="channel-fill"></div>
                            <div id="s2-text" class="channel-text">1500us</div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Interactive VTX Frequency Table -->
            <div class="vtx-table-container">
                <h3>VTX Frequency Table</h3>
                <table class="vtx-table">
                    <thead>
                        <tr>
                            <th>Band</th>
                            <th>CH 1</th>
                            <th>CH 2</th>
                            <th>CH 3</th>
                            <th>CH 4</th>
                            <th>CH 5</th>
                            <th>CH 6</th>
                            <th>CH 7</th>
                            <th>CH 8</th>
                        </tr>
                    </thead>
                    <tbody id="vtx-table-body">
                        <!-- Dynamically populated via JS -->
                    </tbody>
                </table>
            </div>
        </div>

        <form action="/save" method="POST">
            <!-- Binding Phrase Card -->
            <div class="card">
                <h2>Binding Phrase & WiFi Configuration</h2>
                <div class="form-group" style="display: flex; gap: 10px; align-items: flex-end;">
                    <div style="flex-grow: 1;">
                        <label for="phrase">Binding Phrase</label>
                        <input type="text" id="phrase" name="phrase" value="PLACEHOLDER_PHRASE" oninput="updateUID()">
                    </div>
                    <button type="button" id="bind-btn" class="btn" style="margin-top: 0; width: auto; height: 38px; padding: 0 20px; white-space: nowrap;" onclick="startBinding()">Start Bind</button>
                </div>
                <div class="form-group">
                    <label>Computed UID (6 bytes)</label>
                    <input type="text" id="uid-display" value="PLACEHOLDER_UID" readonly style="font-family: monospace; background-color: #222; color: #ff5722;">
                </div>
                <div class="form-group">
                    <label for="tx_power">WiFi TX Power Limit</label>
                    <select id="tx_power" name="tx_power">
                        <option value="8" PLACEHOLDER_TP_8>-1 dBm (Lowest power)</option>
                        <option value="20" PLACEHOLDER_TP_20>5 dBm</option>
                        <option value="34" PLACEHOLDER_TP_34>8.5 dBm</option>
                        <option value="44" PLACEHOLDER_TP_44>11 dBm (~50% Power)</option>
                        <option value="60" PLACEHOLDER_TP_60>15 dBm (~75% Power - Max recommended)</option>
                        <option value="78" PLACEHOLDER_TP_78>19.5 dBm (Max Power - NOT recommended)</option>
                    </select>
                </div>
            </div>

            <!-- RC Channel Mapping Card -->
            <div class="card">
                <h2>RC Channels & Mode Settings</h2>
                <div class="grid-2">
                    <div class="form-group">
                        <label for="ch_6pos">6POS switch Channel (1-16)</label>
                        <input type="number" id="ch_6pos" name="ch_6pos" min="1" max="16" value="PLACEHOLDER_6POS">
                    </div>
                    <div class="form-group">
                        <label for="ch_s2">S2 Knob Channel (1-16)</label>
                        <input type="number" id="ch_s2" name="ch_s2" min="1" max="16" value="PLACEHOLDER_S2">
                    </div>
                </div>
                <div class="form-group">
                    <label for="ctrl_mode">Control Mode</label>
                    <select id="ctrl_mode" name="ctrl_mode" onchange="togglePresets()">
                        <option value="0" PLACEHOLDER_MODE_0>Mode 0: 6pos = Band selection, S2 = Channel selection</option>
                        <option value="1" PLACEHOLDER_MODE_1>Mode 1: 6pos = 6 favorite presets (configured below)</option>
                    </select>
                </div>
            </div>

            <!-- Favorite Presets Card -->
            <div id="presets-card" class="card">
                <h2>Mode 1: Favorite Presets Mapping</h2>
                <div class="grid-2">
                    <!-- Preset 1-6 dropdowns -->
                    PLACEHOLDER_PRESETS_FORM
                </div>
            </div>

            <!-- SPI Hardware Pin Configuration Card -->
            <div class="card">
                <h2>SPI Pin Connections (RTC6715 bit-bang)</h2>
                <div class="grid-2">
                    <div class="form-group">
                        <label for="pin_clk">SPI CLK Pin</label>
                        <input type="number" id="pin_clk" name="pin_clk" value="PLACEHOLDER_PIN_CLK">
                    </div>
                    <div class="form-group">
                        <label for="pin_data">SPI DATA Pin</label>
                        <input type="number" id="pin_data" name="pin_data" value="PLACEHOLDER_PIN_DATA">
                    </div>
                </div>
                <div class="form-group">
                    <label for="pin_cs">SPI CS (Slave Select) Pin</label>
                    <input type="number" id="pin_cs" name="pin_cs" value="PLACEHOLDER_PIN_CS">
                </div>
            </div>

            <button type="submit" class="btn">Save & Apply Changes</button>
        </form>
    </div>

    <!-- JS script for computing MD5 live on the page and refreshing stats -->
    <script src="https://cdnjs.cloudflare.com/ajax/libs/crypto-js/4.1.1/crypto-js.min.js"></script>
    <script>
        function updateUID() {
            var phrase = document.getElementById("phrase").value;
            if (phrase === "") {
                document.getElementById("uid-display").value = "00 00 00 00 00 00";
                return;
            }
            // MD5 computation
            var hash = CryptoJS.MD5(phrase).toString();
            var uid_bytes = [];
            for (var i = 0; i < 6; i++) {
                var b = parseInt(hash.substr(i*2, 2), 16);
                uid_bytes.push(b.toString(16).toUpperCase().padStart(2, '0'));
            }
            document.getElementById("uid-display").value = uid_bytes.join(" ");
        }

        function togglePresets() {
            var mode = document.getElementById("ctrl_mode").value;
            var presetsCard = document.getElementById("presets-card");
            if (mode == "1") {
                presetsCard.style.display = "block";
            } else {
                presetsCard.style.display = "none";
            }
        }

        // VTX Frequencies Table Data
        const bandNames = ["A", "B", "E", "F", "R", "L", "D", "U", "O", "H"];
        const frequencies = [
            [5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725], // Band A
            [5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866], // Band B
            [5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945], // Band E
            [5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880], // Band F
            [5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917], // Band R
            [5333, 5373, 5413, 5453, 5493, 5533, 5573, 5613], // Band L
            [5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621], // Band D
            [5325, 5348, 5366, 5384, 5402, 5420, 5438, 5456], // Band U
            [5474, 5492, 5510, 5528, 5546, 5564, 5582, 5600], // Band O
            [5653, 5693, 5733, 5773, 5813, 5853, 5893, 5933]  // Band H
        ];

        function startBinding() {
            fetch('/bind')
                .then(response => response.json())
                .then(data => {
                    if(data.status === "ok") {
                        updateStatus();
                    }
                });
        }

        function selectChannel(bandIdx, chIdx) {
            fetch(`/select?band=${bandIdx}&channel=${chIdx}`)
                .then(response => response.json())
                .then(data => {
                    if(data.status === "ok") {
                        updateStatus();
                    }
                });
        }

        function renderVTXTable() {
            const tbody = document.getElementById("vtx-table-body");
            tbody.innerHTML = "";
            for (let b = 0; b < frequencies.length; b++) {
                const tr = document.createElement("tr");
                const tdLabel = document.createElement("td");
                tdLabel.className = "band-label";
                tdLabel.innerText = "Band " + bandNames[b];
                tr.appendChild(tdLabel);

                for (let c = 0; c < frequencies[b].length; c++) {
                    const tdFreq = document.createElement("td");
                    tdFreq.className = "freq-cell";
                    tdFreq.id = `cell-${b}-${c}`;
                    tdFreq.innerText = frequencies[b][c];
                    tdFreq.onclick = () => selectChannel(b, c);
                    tr.appendChild(tdFreq);
                }
                tbody.appendChild(tr);
            }
        }

        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById("tuned-freq").innerText = data.active_freq + " MHz";
                    document.getElementById("tuned-channel").innerText = data.active_channel_name;

                    if (data.is_binding_mode) {
                        document.getElementById("link-status").innerHTML = `<span class="badge" style="background-color: #ffb300; animation: blinker 1s linear infinite;">BINDING (${data.binding_remaining}s)</span>`;
                        document.getElementById("bind-btn").innerText = "Binding...";
                        document.getElementById("bind-btn").disabled = true;
                    } else {
                        document.getElementById("bind-btn").innerText = "Start Bind";
                        document.getElementById("bind-btn").disabled = false;
                        if (data.espnow_receiving) {
                            document.getElementById("link-status").innerHTML = '<span class="badge connected">CONNECTED</span>';
                        } else {
                            document.getElementById("link-status").innerHTML = '<span class="badge">DISCONNECTED</span>';
                        }
                    }

                    // Update channels
                    var pos6_val = data.val_6pos;
                    var s2_val = data.val_s2;

                    document.getElementById("6pos-bar").style.width = ((pos6_val - 172) * 100 / 1640) + "%";
                    document.getElementById("6pos-text").innerText = pos6_val + " us";

                    document.getElementById("s2-bar").style.width = ((s2_val - 172) * 100 / 1640) + "%";
                    document.getElementById("s2-text").innerText = s2_val + " us";

                    document.getElementById("6pos-ch-num").innerText = data.ch_6pos_num;
                    document.getElementById("s2-ch-num").innerText = data.ch_s2_num;

                    // Highlight active cell in VTX Table
                    document.querySelectorAll('.freq-cell').forEach(el => el.classList.remove('active-freq'));
                    const activeCell = document.getElementById(`cell-${data.active_band}-${data.active_channel}`);
                    if (activeCell) {
                        activeCell.classList.add('active-freq');
                    }
                });
        }

        // Live stats update loop
        setInterval(updateStatus, 300);

        // Initial calls
        renderVTXTable();
        updateUID();
        togglePresets();
    </script>
</body>
</html>
)rawliteral";

String get_presets_form_html() {
    String html = "";
    const char* band_names[] = {"A", "B", "E", "F", "R", "L", "D", "U", "O", "H"};

    for (int i = 0; i < 6; i++) {
        html += "<div class=\"form-group\" style=\"border-bottom: 1px solid #333; padding-bottom: 10px;\">";
        html += "<label>Preset Position " + String(i + 1) + "</label>";
        html += "<div style=\"display: flex; gap: 5px;\">";

        // Band dropdown
        html += "<select name=\"preset_band_" + String(i) + "\">";
        for (int b = 0; b < 10; b++) {
            String selected = (global_config.preset_bands[i] == b) ? "selected" : "";
            html += "<option value=\"" + String(b) + "\" " + selected + ">Band " + String(band_names[b]) + "</option>";
        }
        html += "</select>";

        // Channel dropdown
        html += "<select name=\"preset_ch_" + String(i) + "\">";
        for (int c = 0; c < 8; c++) {
            String selected = (global_config.preset_channels[i] == c) ? "selected" : "";
            html += "<option value=\"" + String(c) + "\" " + selected + ">Ch " + String(c + 1) + "</option>";
        }
        html += "</select>";

        html += "</div></div>";
    }
    return html;
}

void handle_root() {
    String page = String(index_html);

    page.replace("PLACEHOLDER_PHRASE", String(global_config.binding_phrase));

    char uid_str[32];
    snprintf(uid_str, sizeof(uid_str), "%02X %02X %02X %02X %02X %02X",
             global_config.uid[0], global_config.uid[1], global_config.uid[2],
             global_config.uid[3], global_config.uid[4], global_config.uid[5]);
    page.replace("PLACEHOLDER_UID", String(uid_str));

    // Select placeholder replacement
    page.replace("PLACEHOLDER_TP_8", (global_config.wifi_tx_power == 8) ? "selected" : "");
    page.replace("PLACEHOLDER_TP_20", (global_config.wifi_tx_power == 20) ? "selected" : "");
    page.replace("PLACEHOLDER_TP_34", (global_config.wifi_tx_power == 34) ? "selected" : "");
    page.replace("PLACEHOLDER_TP_44", (global_config.wifi_tx_power == 44) ? "selected" : "");
    page.replace("PLACEHOLDER_TP_60", (global_config.wifi_tx_power == 60) ? "selected" : "");
    page.replace("PLACEHOLDER_TP_78", (global_config.wifi_tx_power == 78) ? "selected" : "");

    page.replace("PLACEHOLDER_6POS", String(global_config.ch_6pos));
    page.replace("PLACEHOLDER_S2", String(global_config.ch_s2));

    page.replace("PLACEHOLDER_MODE_0", (global_config.control_mode == 0) ? "selected" : "");
    page.replace("PLACEHOLDER_MODE_1", (global_config.control_mode == 1) ? "selected" : "");

    page.replace("PLACEHOLDER_PRESETS_FORM", get_presets_form_html());

    page.replace("PLACEHOLDER_PIN_CLK", String(global_config.pin_clk));
    page.replace("PLACEHOLDER_PIN_DATA", String(global_config.pin_data));
    page.replace("PLACEHOLDER_PIN_CS", String(global_config.pin_cs));

    server.send(200, "text/html", page);
}

void handle_status() {
    String json = "{";
    json += "\"active_freq\":" + String(current_selected_freq) + ",";
    json += "\"active_band\":" + String(current_selected_band) + ",";
    json += "\"active_channel\":" + String(current_selected_channel) + ",";

    const char* band_names[] = {"A", "B", "E", "F", "R", "L", "D", "U", "O", "H"};
    String ch_name = String(band_names[current_selected_band]) + String(current_selected_channel + 1);
    json += "\"active_channel_name\":\"" + ch_name + "\",";

    bool receiving = (millis() - last_packet_time < 3000);
    json += "\"espnow_receiving\":" + String(receiving ? "true" : "false") + ",";

    json += "\"is_binding_mode\":" + String(is_binding_mode ? "true" : "false") + ",";
    int remaining = 0;
    if (is_binding_mode) {
        remaining = 30 - (millis() - binding_mode_start_time) / 1000;
        if (remaining < 0) remaining = 0;
    }
    json += "\"binding_remaining\":" + String(remaining) + ",";

    json += "\"ch_6pos_num\":" + String(global_config.ch_6pos) + ",";
    json += "\"ch_s2_num\":" + String(global_config.ch_s2) + ",";

    int val_6pos = 1500;
    int val_s2 = 1500;
    if (global_config.ch_6pos >= 1 && global_config.ch_6pos <= 16) {
        val_6pos = last_crsf_channels[global_config.ch_6pos - 1];
    }
    if (global_config.ch_s2 >= 1 && global_config.ch_s2 <= 16) {
        val_s2 = last_crsf_channels[global_config.ch_s2 - 1];
    }

    // In case no packet received yet, default to center (992 in CRSF is ~1500us scale)
    if (val_6pos == 0) val_6pos = 992;
    if (val_s2 == 0) val_s2 = 992;

    // Map CRSF 11-bit range (172-1811) to standard 1000-2000us representation for display
    int us_6pos = map(val_6pos, 172, 1811, 1000, 2000);
    int us_s2 = map(val_s2, 172, 1811, 1000, 2000);

    json += "\"val_6pos\":" + String(us_6pos) + ",";
    json += "\"val_s2\":" + String(us_s2);
    json += "}";

    server.send(200, "application/json", json);
}

void handle_bind() {
    start_binding_mode();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handle_select() {
    if (server.hasArg("band") && server.hasArg("channel")) {
        int band = server.arg("band").toInt();
        int channel = server.arg("channel").toInt();

        if (band >= 0 && band < 10 && channel >= 0 && channel < 8) {
            current_selected_band = band;
            current_selected_channel = channel;
            current_selected_freq = b_frequencies[band][channel];

            handle_vrx_change(current_selected_band, current_selected_channel, current_selected_freq);

            server.send(200, "application/json", "{\"status\":\"ok\"}");
            return;
        }
    }
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"invalid band or channel\"}");
}

void handle_save() {
    if (server.hasArg("phrase")) {
        strncpy(global_config.binding_phrase, server.arg("phrase").c_str(), sizeof(global_config.binding_phrase) - 1);
    }
    if (server.hasArg("tx_power")) {
        global_config.wifi_tx_power = server.arg("tx_power").toInt();
    }
    if (server.hasArg("ch_6pos")) {
        global_config.ch_6pos = server.arg("ch_6pos").toInt();
    }
    if (server.hasArg("ch_s2")) {
        global_config.ch_s2 = server.arg("ch_s2").toInt();
    }
    if (server.hasArg("ctrl_mode")) {
        global_config.control_mode = server.arg("ctrl_mode").toInt();
    }
    if (server.hasArg("pin_clk")) {
        global_config.pin_clk = server.arg("pin_clk").toInt();
    }
    if (server.hasArg("pin_data")) {
        global_config.pin_data = server.arg("pin_data").toInt();
    }
    if (server.hasArg("pin_cs")) {
        global_config.pin_cs = server.arg("pin_cs").toInt();
    }

    // Preset dropdowns
    for (int i = 0; i < 6; i++) {
        String band_arg = "preset_band_" + String(i);
        String ch_arg = "preset_ch_" + String(i);
        if (server.hasArg(band_arg)) {
            global_config.preset_bands[i] = server.arg(band_arg).toInt();
        }
        if (server.hasArg(ch_arg)) {
            global_config.preset_channels[i] = server.arg(ch_arg).toInt();
        }
    }

    // Save to preferences
    save_config();

    // Apply changes
    stop_espnow();
    init_spi_vtx();
    init_espnow();

    // Redirect back to root page
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "Updated.");
}

void init_web_server() {
    // Start local Access Point for configuration
    WiFi.softAP("ELRS_Backpack_VRX_S3", ""); // open SSID

    server.on("/", handle_root);
    server.on("/status", handle_status);
    server.on("/bind", HTTP_GET, handle_bind);
    server.on("/select", HTTP_GET, handle_select);
    server.on("/save", HTTP_POST, handle_save);

    server.begin();
    Serial.println("[WebUI] Web server running on IP: 192.168.4.1");
}

void handle_web_server() {
    server.handleClient();
}
