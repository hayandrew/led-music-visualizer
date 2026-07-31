#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-C3 LED Visualizer Control Portal</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;800&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg: radial-gradient(circle at 50% 0%, #1a172e, #07060a);
            --panel-bg: rgba(20, 16, 36, 0.5);
            --panel-border: rgba(255, 255, 255, 0.08);
            --panel-shadow: 0 10px 40px rgba(0, 0, 0, 0.5);
            --text: #f3f0f7;
            --text-muted: #8d859d;
            --purple: #a855f7;
            --purple-glow: rgba(168, 85, 247, 0.4);
            --cyan: #06b6d4;
            --cyan-glow: rgba(6, 182, 212, 0.4);
            --green: #10b981;
            --rose: #f43f5e;
            --amber: #f59e0b;
        }

        body {
            background: var(--bg);
            color: var(--text);
            font-family: 'Outfit', sans-serif;
            margin: 0;
            padding: 24px;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            box-sizing: border-box;
            color-scheme: dark;
        }

        .container {
            width: 100%;
            max-width: 950px;
            display: grid;
            grid-template-columns: 1.1fr 1fr;
            gap: 28px;
            margin-top: 24px;
        }

        @media (max-width: 768px) {
            .container {
                grid-template-columns: 1fr;
            }
        }

        header {
            width: 100%;
            max-width: 950px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 0 8px;
        }

        h1 {
            font-weight: 800;
            font-size: 30px;
            margin: 0;
            letter-spacing: -0.5px;
            background: linear-gradient(135deg, var(--cyan), var(--purple));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        .status {
            display: flex;
            align-items: center;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 16px;
            border-radius: 20px;
            background: rgba(255, 255, 255, 0.03);
            border: 1px solid var(--panel-border);
            backdrop-filter: blur(12px);
            -webkit-backdrop-filter: blur(12px);
        }

        .status-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            margin-right: 8px;
            background: var(--rose);
            box-shadow: 0 0 10px var(--rose);
            transition: all 0.3s ease;
        }

        .status.connected .status-dot {
            background: var(--green);
            box-shadow: 0 0 10px var(--green);
        }

        .card {
            background: var(--panel-bg);
            border: 1px solid var(--panel-border);
            border-radius: 28px;
            padding: 28px;
            box-shadow: var(--panel-shadow);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            display: flex;
            flex-direction: column;
            gap: 22px;
        }

        .card-title {
            font-size: 22px;
            font-weight: 600;
            margin: 0 0 6px 0;
            letter-spacing: -0.2px;
            border-left: 3px solid var(--cyan);
            padding-left: 12px;
        }

        .control-group {
            display: flex;
            flex-direction: column;
            gap: 10px;
        }

        .control-header {
            display: flex;
            justify-content: space-between;
            font-size: 14px;
            color: var(--text-muted);
            font-weight: 600;
        }

        .val-display {
            color: var(--cyan);
            font-weight: 600;
            background: rgba(6, 182, 212, 0.1);
            padding: 2px 8px;
            border-radius: 6px;
            font-size: 13px;
        }

        /* Sliders */
        input[type="range"] {
            -webkit-appearance: none;
            width: 100%;
            height: 6px;
            border-radius: 3px;
            background: rgba(255, 255, 255, 0.08);
            outline: none;
        }

        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 18px;
            height: 18px;
            border-radius: 50%;
            background: var(--cyan);
            box-shadow: 0 0 10px var(--cyan-glow);
            cursor: pointer;
            transition: transform 0.1s, background 0.3s;
            border: none;
        }

        input[type="range"]::-webkit-slider-thumb:hover {
            transform: scale(1.25);
            background: #fff;
        }

        /* Switch */
        .switch-container {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 14px 20px;
            background: rgba(255, 255, 255, 0.02);
            border-radius: 18px;
            border: 1px solid rgba(255, 255, 255, 0.04);
        }

        .switch-label {
            font-size: 15px;
            font-weight: 600;
        }

        .switch {
            position: relative;
            display: inline-block;
            width: 48px;
            height: 24px;
        }

        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }

        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: rgba(255, 255, 255, 0.12);
            transition: .3s;
            border-radius: 24px;
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 18px;
            width: 18px;
            left: 3px;
            bottom: 3px;
            background-color: #fff;
            transition: .3s;
            border-radius: 50%;
        }

        input:checked + .slider {
            background-color: var(--purple);
            box-shadow: 0 0 10px var(--purple-glow);
        }

        input:checked + .slider:before {
            transform: translateX(24px);
        }

        /* Mode Selection List */
        .modes-grid {
            display: flex;
            flex-direction: column;
            gap: 10px;
            max-height: 280px;
            overflow-y: auto;
            padding-right: 4px;
        }

        /* Custom scrollbar for modes list */
        .modes-grid::-webkit-scrollbar {
            width: 6px;
        }
        .modes-grid::-webkit-scrollbar-thumb {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 3px;
        }

        .mode-btn {
            background: rgba(255, 255, 255, 0.02);
            border: 1px solid rgba(255, 255, 255, 0.04);
            color: var(--text-muted);
            padding: 14px 18px;
            border-radius: 16px;
            cursor: pointer;
            font-family: inherit;
            font-size: 15px;
            font-weight: 600;
            text-align: left;
            transition: all 0.2s ease;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .mode-btn:hover {
            background: rgba(255, 255, 255, 0.05);
            color: var(--text);
            transform: translateX(3px);
            border-color: rgba(255, 255, 255, 0.08);
        }

        .mode-btn.active {
            background: linear-gradient(135deg, rgba(6, 182, 212, 0.12), rgba(168, 85, 247, 0.12));
            border-color: var(--purple);
            color: var(--text);
            box-shadow: 0 4px 20px rgba(168, 85, 247, 0.15);
        }

        .mode-btn.active::after {
            content: "●";
            color: var(--cyan);
            font-size: 10px;
            text-shadow: 0 0 6px var(--cyan);
        }

        /* Canvas and Visuals */
        #visualizer-canvas {
            width: 100%;
            height: 220px;
            background: rgba(0, 0, 0, 0.2);
            border-radius: 18px;
            border: 1px solid rgba(255, 255, 255, 0.04);
        }

        .envelope-container {
            display: flex;
            flex-direction: column;
            gap: 8px;
        }

        .envelope-bar-bg {
            width: 100%;
            height: 12px;
            background: rgba(255, 255, 255, 0.04);
            border-radius: 6px;
            overflow: hidden;
            border: 1px solid rgba(255, 255, 255, 0.02);
        }

        .envelope-bar-fill {
            height: 100%;
            width: 0%;
            background: linear-gradient(90deg, var(--cyan), var(--purple));
            border-radius: 6px;
            transition: width 0.05s ease-out;
            box-shadow: 0 0 10px var(--cyan-glow);
        }

        .stats-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 16px;
        }

        .stat-card {
            background: rgba(255, 255, 255, 0.015);
            border: 1px solid rgba(255, 255, 255, 0.03);
            border-radius: 18px;
            padding: 16px;
            text-align: center;
        }

        .stat-value {
            font-size: 22px;
            font-weight: 800;
            color: var(--text);
            margin-top: 4px;
        }
        
        .stat-label {
            font-size: 12px;
            color: var(--text-muted);
            font-weight: 600;
        }

        footer {
            margin-top: 48px;
            font-size: 12px;
            color: var(--text-muted);
            text-align: center;
            letter-spacing: 0.5px;
        }
    </style>
</head>
<body>
    <header>
        <h1>ESP32-C3 Visualizer</h1>
        <div id="connection-status" class="status">
            <span class="status-dot"></span>
            <span id="connection-text">Connecting...</span>
        </div>
    </header>

    <div class="container">
        <!-- Controls Column -->
        <div class="card">
            <div class="card-title">Settings</div>
            
            <div class="control-group">
                <div class="control-header">
                    <span>Brightness</span>
                    <span id="brightness-val" class="val-display">17%</span>
                </div>
                <input type="range" id="brightness" min="0" max="255" value="45">
            </div>

            <div class="control-group">
                <div class="control-header">
                    <span>Microphone Gain</span>
                    <span id="gain-val" class="val-display">1.0x</span>
                </div>
                <input type="range" id="gain" min="0.5" max="5.0" step="0.1" value="1.0">
            </div>

            <div class="switch-container">
                <span class="switch-label">Auto-Cycle Modes (15s)</span>
                <label class="switch">
                    <input type="checkbox" id="auto-cycle" checked>
                    <span class="slider"></span>
                </label>
            </div>

            <div class="control-group">
                <div class="control-header">Visualization Mode</div>
                <div class="modes-grid" id="modes-list">
                    <!-- Dynamic Buttons -->
                </div>
            </div>
        </div>

        <!-- Live Diagnostics Column -->
        <div class="card">
            <div class="card-title">Live Diagnostics</div>
            
            <canvas id="visualizer-canvas" width="400" height="220"></canvas>

            <div class="envelope-container">
                <div class="control-header">Volume Envelope</div>
                <div class="envelope-bar-bg">
                    <div id="envelope-bar-fill" class="envelope-bar-fill"></div>
                </div>
            </div>

            <div class="stats-grid">
                <div class="stat-card">
                    <div class="stat-label">Envelope Value</div>
                    <div id="env-numeric" class="stat-value">0</div>
                </div>
                <div class="stat-card">
                    <div class="stat-label">Peak Amplitude</div>
                    <div id="peak-numeric" class="stat-value">0</div>
                </div>
            </div>
        </div>
    </div>

    <footer>
        ESP32-C3 LED Visualizer Portal &bull; Antigravity Agentic Design
    </footer>

    <script>
        let ws;
        let modeNames = [
            "Diagnostic Heart",
            "Linear Spectrum",
            "Symmetric Spectrum",
            "Stereo VU Meter",
            "Bass Pulse",
            "Sound Ripples",
            "Ambient Noise"
        ];
        
        // Generate mode buttons
        const modesList = document.getElementById("modes-list");
        modeNames.forEach((name, index) => {
            const btn = document.createElement("button");
            btn.className = "mode-btn";
            btn.id = "mode-" + index;
            btn.innerText = name;
            btn.addEventListener("click", () => selectMode(index));
            modesList.appendChild(btn);
        });

        function connect() {
            const host = window.location.host;
            const wsUri = "ws://" + (host ? host : "192.168.4.1") + "/ws";
            
            console.log("Connecting to WebSocket:", wsUri);
            ws = new WebSocket(wsUri);
            
            ws.onopen = () => {
                console.log("WebSocket connected.");
                const status = document.getElementById("connection-status");
                status.className = "status connected";
                document.getElementById("connection-text").innerText = "Connected";
            };
            
            ws.onclose = () => {
                console.log("WebSocket disconnected. Retrying in 2 seconds...");
                const status = document.getElementById("connection-status");
                status.className = "status";
                document.getElementById("connection-text").innerText = "Disconnected, reconnecting...";
                setTimeout(connect, 2000);
            };
            
            ws.onmessage = (event) => {
                try {
                    const data = JSON.parse(event.data);
                    
                    if (data.type === "config") {
                        // Update settings widgets
                        document.getElementById("brightness").value = data.brightness;
                        document.getElementById("brightness-val").innerText = Math.round((data.brightness / 255) * 100) + "%";
                        
                        document.getElementById("gain").value = data.gain;
                        document.getElementById("gain-val").innerText = parseFloat(data.gain).toFixed(1) + "x";
                        
                        document.getElementById("auto-cycle").checked = data.autoCycle;
                        
                        setActiveModeButton(data.mode);
                    } else if (data.type === "stats") {
                        updateRealtimeGraph(data.bands, data.envelope, data.peak);
                    }
                } catch (e) {
                    console.error("Error parsing WS message:", e);
                }
            };
        }

        function sendConfig(configObj) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify(configObj));
            }
        }

        function selectMode(modeIndex) {
            sendConfig({ mode: modeIndex });
            setActiveModeButton(modeIndex);
        }

        function setActiveModeButton(activeIndex) {
            for (let i = 0; i < modeNames.length; i++) {
                const btn = document.getElementById("mode-" + i);
                if (btn) {
                    if (i === activeIndex) {
                        btn.classList.add("active");
                    } else {
                        btn.classList.remove("active");
                    }
                }
            }
        }

        // Sliders input events
        const brightnessSlider = document.getElementById("brightness");
        brightnessSlider.addEventListener("input", (e) => {
            const val = e.target.value;
            document.getElementById("brightness-val").innerText = Math.round((val / 255) * 100) + "%";
            sendConfig({ brightness: parseInt(val) });
        });

        const gainSlider = document.getElementById("gain");
        gainSlider.addEventListener("input", (e) => {
            const val = parseFloat(e.target.value);
            document.getElementById("gain-val").innerText = val.toFixed(1) + "x";
            sendConfig({ gain: val });
        });

        const autoCycleToggle = document.getElementById("auto-cycle");
        autoCycleToggle.addEventListener("change", (e) => {
            sendConfig({ autoCycle: e.target.checked });
        });

        // 7-Band Spectrum Rendering
        function updateRealtimeGraph(bands, envelope, peak) {
            const canvas = document.getElementById("visualizer-canvas");
            const ctx = canvas.getContext("2d");
            const W = canvas.width;
            const H = canvas.height;
            ctx.clearRect(0, 0, W, H);
            
            // Draw grid lines
            ctx.strokeStyle = "rgba(255, 255, 255, 0.03)";
            ctx.lineWidth = 1;
            for (let i = 1; i < 4; i++) {
                let y = (H / 4) * i;
                ctx.beginPath();
                ctx.moveTo(0, y);
                ctx.lineTo(W, y);
                ctx.stroke();
            }
            
            const barCount = 7;
            const padding = 16;
            const barWidth = (W - (padding * (barCount + 1))) / barCount;
            
            const colors = [
                "#f43f5e", // Rose (Sub-bass)
                "#ec4899", // Pink (Bass)
                "#a855f7", // Purple (Mid-bass)
                "#6366f1", // Indigo (Mids)
                "#3b82f6", // Blue (High-mids)
                "#06b6d4", // Cyan (Presence)
                "#10b981"  // Emerald (Brilliance)
            ];
            
            for (let i = 0; i < barCount; i++) {
                const val = bands[i];
                const barHeight = val * (H - 35);
                const x = padding + i * (barWidth + padding);
                const y = H - 20 - barHeight;
                
                // Draw gradient fill
                const gradient = ctx.createLinearGradient(x, y, x, H - 20);
                gradient.addColorStop(0, colors[i]);
                gradient.addColorStop(1, colors[i] + "11");
                
                ctx.fillStyle = gradient;
                
                // Round top corners
                ctx.beginPath();
                if (typeof ctx.roundRect === "function") {
                    ctx.roundRect(x, y, barWidth, Math.max(2, barHeight), [5, 5, 0, 0]);
                } else {
                    ctx.rect(x, y, barWidth, Math.max(2, barHeight));
                }
                ctx.fill();
            }
            
            // Labels
            const labels = ["Sub", "Bass", "M-Bass", "Mids", "H-Mids", "Pres", "Brill"];
            ctx.fillStyle = "var(--text-muted)";
            ctx.font = "bold 9px 'Outfit', sans-serif";
            ctx.textAlign = "center";
            for (let i = 0; i < barCount; i++) {
                const x = padding + i * (barWidth + padding) + barWidth / 2;
                ctx.fillText(labels[i], x, H - 6);
            }

            // Envelope bar fill
            const envFill = document.getElementById("envelope-bar-fill");
            const percent = Math.min(100, (envelope / 40000) * 100);
            envFill.style.width = percent + "%";

            // Numeric displays
            document.getElementById("env-numeric").innerText = Math.round(envelope);
            document.getElementById("peak-numeric").innerText = Math.round(peak);
        }

        // Initialize connection
        connect();
    </script>
</body>
</html>
)rawhtml";

#endif // WEB_UI_H
