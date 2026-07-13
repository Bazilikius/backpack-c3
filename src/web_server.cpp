#include "web_server.h"
#include <WiFi.h>
#include <WebServer.h>
#include "config_store.h"
#include "crsf_parser.h"
#include "spi_vtx.h"

WebServer server(80);

// CSS and HTML for Web UI
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CRSF Direct VRX Controller</title>
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
        <h1>CRSF Direct VRX Controller</h1>
        <div class="subtitle">Direct CRSF Serial Listener & RTC6715 SPI Controller</div>

        <!-- Live Status Card -->
        <div class="card">
            <h2>Live Receiver Status</h2>
            <div class="status-container">
                <div>
                    <div>CRSF Connection</div>
                    <div id="link-status"><span class="badge">DISCONNECTED</span></div>
                </div>
                <div>
                    <div>Tuned Frequency</div>
                    <div id="tuned-freq" class="status-val">5658 MHz</div>
                </div>
                <div>
                    <div>Tuned Channel</div>
                    <div id="tuned-channel" class="status-val">Band 5 (R) - Ch 1</div>
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
            <!-- CRSF Interface Config Card -->
            <div class="card">
                <h2>CRSF Serial Configuration</h2>
                <div class="grid-2">
                    <div class="form-group">
                        <label for="pin_crsf_rx">CRSF RX Pin</label>
                        <input type="number" id="pin_crsf_rx" name="pin_crsf_rx" value="PLACEHOLDER_PIN_CRSF_RX">
                    </div>
                    <div class="form-group">
                        <label for="crsf_baud">CRSF Baud Rate</label>
                        <select id="crsf_baud" name="crsf_baud">
                            <option value="115200" PLACEHOLDER_BAUD_115200>115200</option>
                            <option value="416700" PLACEHOLDER_BAUD_416700>416700 (Standard)</option>
                            <option value="921600" PLACEHOLDER_BAUD_921600>921600</option>
                        </select>
                    </div>
                </div>
            </div>

            <!-- RC Channel Mapping Card -->
            <div class="card">
                <h2>RC Channels & Mode Settings</h2>
                <div class="grid-2">
                    <div class="form-group">
                        <label for="ch_6pos">6POS Switch Channel (1-16)</label>
                        <input type="number" id="ch_6pos" name="ch_6pos" min="1" max="16" value="PLACEHOLDER_6POS">
                    </div>
                    <div class="form-group">
                        <label for="ch_s2">S2 Knob Channel (1-16)</label>
                        <input type="number" id="ch_s2" name="ch_s2" min="1" max="16" value="PLACEHOLDER_S2">
                    </div>
                </div>
                <div class="form-group">
                    <label for="ctrl_mode">Control Mode</label>
                    <select id="ctrl_mode" name="ctrl_mode" onchange="toggleModes()">
                        <option value="0" PLACEHOLDER_MODE_0>Mode 0: 6pos = Band selection, S2 = Channel selection</option>
                        <option value="1" PLACEHOLDER_MODE_1>Mode 1: 6pos = 6 favorite presets (configured below)</option>
                    </select>
                </div>
            </div>

            <!-- Mode 0: 6POS Bands Configuration Card -->
            <div id="mode0-card" class="card">
                <h2>Mode 0: 6POS Positions Bands Mapping</h2>
                <p style="font-size: 0.85em; color: #aaa; margin-top:0;">Map each of the 6 positions of your 6pos switch to ANY band in the grid.</p>
                <div class="grid-2">
                    PLACEHOLDER_POS6_BANDS_FORM
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

            <!-- SPI Hardware Pin Configuration Card / VRX Control Protocol -->
            <div class="card">
                <h2>VRX Control Protocol & SPI/Legacy Pins</h2>
                <div class="form-group">
                    <label for="legacy_mode">Control Interface Protocol</label>
                    <select id="legacy_mode" name="legacy_mode">
                        <option value="0" PLACEHOLDER_LEGACY_0>SPI Register Control (Rapidfire, Steadyview, custom SPI mods)</option>
                        <option value="1" PLACEHOLDER_LEGACY_1>Legacy 3-bit Parallel standard (Foxeer Wildfire, TBS Fusion, stock modules)</option>
                        <option value="2" PLACEHOLDER_LEGACY_2>Legacy 3-bit Parallel inverted</option>
                    </select>
                </div>
                <div class="grid-2">
                    <div class="form-group">
                        <label for="pin_clk">SPI CLK / CH3 Pin</label>
                        <input type="number" id="pin_clk" name="pin_clk" value="PLACEHOLDER_PIN_CLK">
                    </div>
                    <div class="form-group">
                        <label for="pin_data">SPI DATA / CH1 Pin</label>
                        <input type="number" id="pin_data" name="pin_data" value="PLACEHOLDER_PIN_DATA">
                    </div>
                </div>
                <div class="form-group">
                    <label for="pin_cs">SPI CS / CH2 Pin</label>
                    <input type="number" id="pin_cs" name="pin_cs" value="PLACEHOLDER_PIN_CS">
                </div>
            </div>

            <button type="submit" class="btn">Save & Apply Changes</button>
        </form>
    </div>

    <script>
        function toggleModes() {
            var mode = document.getElementById("ctrl_mode").value;
            var mode0Card = document.getElementById("mode0-card");
            var presetsCard = document.getElementById("presets-card");
            if (mode == "0") {
                mode0Card.style.display = "block";
                presetsCard.style.display = "none";
            } else {
                mode0Card.style.display = "none";
                presetsCard.style.display = "block";
            }
        }

        // VTX Frequencies Table Data (10 bands)
        const bandNames = ["1 (A)", "2 (B)", "3 (E)", "4 (F)", "5 (R)", "6 (L Foxeer)", "7 (L Std)", "8 (U)", "9 (O)", "10 (H)"];
        const frequencies = [
            [5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725], // Band A
            [5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866], // Band B
            [5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945], // Band E
            [5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880], // Band F
            [5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917], // Band R
            [5333, 5373, 5413, 5453, 5493, 5533, 5573, 5613], // Band L Foxeer
            [5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621], // Band L Std
            [5325, 5348, 5366, 5384, 5402, 5420, 5438, 5456], // Band U
            [5474, 5492, 5510, 5528, 5546, 5564, 5582, 5600], // Band O
            [5653, 5693, 5733, 5773, 5813, 5853, 5893, 5933]  // Band H
        ];

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
                tdLabel.innerText = bandNames[b];
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

                    if (data.crsf_connected) {
                        document.getElementById("link-status").innerHTML = '<span class="badge connected">CONNECTED</span>';
                    } else {
                        document.getElementById("link-status").innerHTML = '<span class="badge">DISCONNECTED</span>';
                    }

                    // Update channels
                    var pos6_val = data.val_6pos;
                    var s2_val = data.val_s2;

                    document.getElementById("6pos-bar").style.width = ((pos6_val - 1000) * 100 / 1000) + "%";
                    document.getElementById("6pos-text").innerText = pos6_val + " us";

                    document.getElementById("s2-bar").style.width = ((s2_val - 1000) * 100 / 1000) + "%";
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
        toggleModes();
    </script>
</body>
</html>
)rawliteral";

String get_presets_form_html() {
    String html = "";
    const char* band_names[] = {
        "1 (A)", "2 (B)", "3 (E)", "4 (F)", "5 (R)",
        "6 (L Foxeer)", "7 (L Std)", "8 (U)", "9 (O)", "10 (H)"
    };

    for (int i = 0; i < 6; i++) {
        html += "<div class=\"form-group\" style=\"border-bottom: 1px solid #333; padding-bottom: 10px;\">";
        html += "<label>Preset Position " + String(i + 1) + "</label>";
        html += "<div style=\"display: flex; gap: 5px;\">";

        // Band dropdown
        html += "<select name=\"preset_band_" + String(i) + "\">";
        for (int b = 0; b < 10; b++) {
            String selected = (global_config.preset_bands[i] == b) ? "selected" : "";
            html += "<option value=\"" + String(b) + "\" " + selected + ">" + String(band_names[b]) + "</option>";
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

String get_pos6_bands_form_html() {
    String html = "";
    const char* band_names[] = {
        "1 (A)", "2 (B)", "3 (E)", "4 (F)", "5 (R)",
        "6 (L Foxeer)", "7 (L Std)", "8 (U)", "9 (O)", "10 (H)"
    };

    for (int i = 0; i < 6; i++) {
        html += "<div class=\"form-group\" style=\"border-bottom: 1px solid #333; padding-bottom: 10px;\">";
        html += "<label>6POS Position " + String(i + 1) + " Band</label>";
        html += "<select name=\"pos6_band_" + String(i) + "\">";
        for (int b = 0; b < 10; b++) {
            String selected = (global_config.pos6_bands[i] == b) ? "selected" : "";
            html += "<option value=\"" + String(b) + "\" " + selected + ">" + String(band_names[b]) + "</option>";
        }
        html += "</select>";
        html += "</div>";
    }
    return html;
}

void handle_root() {
    String page = String(index_html);

    page.replace("PLACEHOLDER_PIN_CRSF_RX", String(global_config.pin_crsf_rx));

    page.replace("PLACEHOLDER_BAUD_115200", (global_config.crsf_baud == 115200) ? "selected" : "");
    page.replace("PLACEHOLDER_BAUD_416700", (global_config.crsf_baud == 416700) ? "selected" : "");
    page.replace("PLACEHOLDER_BAUD_921600", (global_config.crsf_baud == 921600) ? "selected" : "");

    page.replace("PLACEHOLDER_6POS", String(global_config.ch_6pos));
    page.replace("PLACEHOLDER_S2", String(global_config.ch_s2));

    page.replace("PLACEHOLDER_MODE_0", (global_config.control_mode == 0) ? "selected" : "");
    page.replace("PLACEHOLDER_MODE_1", (global_config.control_mode == 1) ? "selected" : "");

    page.replace("PLACEHOLDER_POS6_BANDS_FORM", get_pos6_bands_form_html());
    page.replace("PLACEHOLDER_PRESETS_FORM", get_presets_form_html());

    page.replace("PLACEHOLDER_PIN_CLK", String(global_config.pin_clk));
    page.replace("PLACEHOLDER_PIN_DATA", String(global_config.pin_data));
    page.replace("PLACEHOLDER_PIN_CS", String(global_config.pin_cs));

    page.replace("PLACEHOLDER_LEGACY_0", (global_config.legacy_mode == 0) ? "selected" : "");
    page.replace("PLACEHOLDER_LEGACY_1", (global_config.legacy_mode == 1) ? "selected" : "");
    page.replace("PLACEHOLDER_LEGACY_2", (global_config.legacy_mode == 2) ? "selected" : "");

    server.send(200, "text/html", page);
}

void handle_status() {
    String json = "{";
    json += "\"active_freq\":" + String(current_selected_freq) + ",";
    json += "\"active_band\":" + String(current_selected_band) + ",";
    json += "\"active_channel\":" + String(current_selected_channel) + ",";

    const char* band_names[] = {
        "1 (A)", "2 (B)", "3 (E)", "4 (F)", "5 (R)",
        "6 (L Foxeer)", "7 (L Std)", "8 (U)", "9 (O)", "10 (H)"
    };
    String ch_name = "Band " + String(band_names[current_selected_band]) + " - Ch " + String(current_selected_channel + 1);
    json += "\"active_channel_name\":\"" + ch_name + "\",";

    bool connected = (millis() - last_crsf_packet_time < 2000);
    json += "\"crsf_connected\":" + String(connected ? "true" : "false") + ",";

    json += "\"ch_6pos_num\":" + String(global_config.ch_6pos) + ",";
    json += "\"ch_s2_num\":" + String(global_config.ch_s2) + ",";

    int val_6pos = 1500;
    int val_s2 = 1500;
    if (global_config.ch_6pos >= 1 && global_config.ch_6pos <= 16) {
        val_6pos = crsf.channels[global_config.ch_6pos - 1];
    }
    if (global_config.ch_s2 >= 1 && global_config.ch_s2 <= 16) {
        val_s2 = crsf.channels[global_config.ch_s2 - 1];
    }

    json += "\"val_6pos\":" + String(val_6pos) + ",";
    json += "\"val_s2\":" + String(val_s2);
    json += "}";

    server.send(200, "application/json", json);
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
    if (server.hasArg("pin_crsf_rx")) {
        global_config.pin_crsf_rx = server.arg("pin_crsf_rx").toInt();
    }
    if (server.hasArg("crsf_baud")) {
        global_config.crsf_baud = server.arg("crsf_baud").toInt();
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
    if (server.hasArg("legacy_mode")) {
        global_config.legacy_mode = server.arg("legacy_mode").toInt();
    }

    // 6pos position bands
    for (int i = 0; i < 6; i++) {
        String pos_arg = "pos6_band_" + String(i);
        if (server.hasArg(pos_arg)) {
            global_config.pos6_bands[i] = server.arg(pos_arg).toInt();
        }
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

    // Redirect back to root page, restart ESP to apply pin changes
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "Updated. Restarting...");
    delay(500);
    ESP.restart();
}

void init_web_server() {
    // Start local Access Point for configuration
    WiFi.softAP("CRSF_VRX_Controller", ""); // open SSID

    server.on("/", handle_root);
    server.on("/status", handle_status);
    server.on("/select", HTTP_GET, handle_select);
    server.on("/save", HTTP_POST, handle_save);

    server.begin();
    Serial.println("[WebUI] Web server running on IP: 192.168.4.1");
}

void handle_web_server() {
    server.handleClient();
}
