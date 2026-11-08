#!/usr/bin/env python3
"""
AEZEL VCU Live Simulation & Test Backend
Runs a local HTTP & SSE server at http://localhost:8080 to simulate the AEZEL cockpit on PC.
Supports REST telemetry injection (/api/telemetry) and automated test scenarios (/api/scenario).
"""

import http.server
import socketserver
import json
import time
import threading
import os
import sys

PORT = 8080
DIRECTORY = os.path.dirname(os.path.abspath(__file__))

# Canonical vehicle presets
PRESETS = [
    {"name": "Bike (Direct/Non-OBD)", "type": "bike", "max_rpm": 12000, "max_speed": 180, "transmission": "manual"},
    {"name": "Bike (OBD Hybrid)", "type": "bike", "max_rpm": 14000, "max_speed": 200, "transmission": "manual"},
    {"name": "Scooter (Direct/CVT)", "type": "scooter", "max_rpm": 9000, "max_speed": 120, "transmission": "cvt"},
    {"name": "Scooter (OBD Hybrid)", "type": "scooter", "max_rpm": 10000, "max_speed": 120, "transmission": "cvt"},
    {"name": "Car Manual (OBD-II)", "type": "car", "max_rpm": 7000, "max_speed": 220, "transmission": "manual"},
    {"name": "Car Auto (OBD-II P-R-N-D)", "type": "car", "max_rpm": 7000, "max_speed": 240, "transmission": "auto"}
]

# Shared live vehicle state
state_lock = threading.Lock()
state = {
    "ignition": True,
    "speed": 0,
    "rpm": 1200,
    "gear": "N",
    "gear_idx": 0,
    "fuel_pct": 78,
    "batt_v": 13.8,
    "engine_temp": 82,
    "ambient_temp": 29,
    "coolant_temp": 84,
    "throttle_pct": 0,
    "engine_load_pct": 0,
    "lean_angle": 0.0,
    "trip_a": 12.4,
    "odometer": 14250.0,
    "profile_idx": 0,
    "profile_name": "Bike (Direct/Non-OBD)",
    "vehicle_type": "bike",
    "transmission": "manual",
    "tacho_max_rpm": 12000,
    "left_ind": False,
    "right_ind": False,
    "hazard": False,
    "neutral": True,
    "high_beam": False,
    "warning_banner": "",
    "neopixel_anim": "none",
    "last_update": time.time()
}

active_scenario_thread = None
stop_scenario_event = threading.Event()

def run_scenario(name):
    global stop_scenario_event
    stop_scenario_event.set()
    time.sleep(0.05)
    stop_scenario_event.clear()

    def worker():
        print(f"[SIM] Starting scenario: {name}")
        if name == "cruise_highway":
            # Accelerate through gears 1 -> 5 to 80 km/h
            with state_lock:
                state["ignition"] = True
                state["profile_idx"] = 0
                state["profile_name"] = "Bike (Direct/Non-OBD)"
                state["warning_banner"] = ""
            for spd in range(0, 85, 2):
                if stop_scenario_event.is_set(): return
                with state_lock:
                    state["speed"] = spd
                    if spd == 0:
                        state["gear"] = "N"; state["neutral"] = True; state["rpm"] = 1200
                    elif spd < 20:
                        state["gear"] = "1"; state["neutral"] = False; state["rpm"] = 2500 + spd * 100
                    elif spd < 38:
                        state["gear"] = "2"; state["gear_idx"] = 2; state["rpm"] = 3000 + (spd - 20) * 120
                    elif spd < 58:
                        state["gear"] = "3"; state["gear_idx"] = 3; state["rpm"] = 3800 + (spd - 38) * 110
                    else:
                        state["gear"] = "4"; state["gear_idx"] = 4; state["rpm"] = 4500 + (spd - 58) * 90
                    state["throttle_pct"] = 45.0
                    state["engine_load_pct"] = 52.0
                    state["odometer"] += 0.02
                    state["trip_a"] += 0.02
                time.sleep(0.06)

        elif name == "cornering_lean":
            # Lean bike 35 degrees through corner
            for ang in range(0, 36, 2):
                if stop_scenario_event.is_set(): return
                with state_lock:
                    state["lean_angle"] = float(ang)
                    state["speed"] = 45
                    state["gear"] = "3"
                    state["rpm"] = 4800
                time.sleep(0.05)
            time.sleep(0.5)
            for ang in range(35, -1, -2):
                if stop_scenario_event.is_set(): return
                with state_lock:
                    state["lean_angle"] = float(ang)
                time.sleep(0.05)

        elif name == "overheat_warning":
            with state_lock:
                state["engine_temp"] = 125
                state["coolant_temp"] = 118
                state["warning_banner"] = "OVERTEMP ALERT: 125°C - STOP ENGINE"
                state["neopixel_anim"] = "warning"

        elif name == "scooter_commute":
            with state_lock:
                state["profile_idx"] = 2
                state["profile_name"] = "Scooter (Direct/CVT)"
                state["vehicle_type"] = "scooter"
                state["transmission"] = "cvt"
                state["gear"] = "CVT"
                state["tacho_max_rpm"] = 9000
                state["warning_banner"] = ""
            for spd in range(0, 65, 3):
                if stop_scenario_event.is_set(): return
                with state_lock:
                    state["speed"] = spd
                    state["rpm"] = 2000 + spd * 75
                    state["throttle_pct"] = 60.0
                time.sleep(0.07)

        elif name == "car_obd_drive":
            with state_lock:
                state["profile_idx"] = 5
                state["profile_name"] = "Car Auto (OBD-II P-R-N-D)"
                state["vehicle_type"] = "car"
                state["transmission"] = "auto"
                state["gear"] = "D"
                state["tacho_max_rpm"] = 7000
                state["warning_banner"] = ""
            for spd in range(0, 110, 4):
                if stop_scenario_event.is_set(): return
                with state_lock:
                    state["speed"] = spd
                    state["rpm"] = 1500 + spd * 35
                    state["coolant_temp"] = 90
                    state["throttle_pct"] = 35.0
                    state["engine_load_pct"] = 42.0
                time.sleep(0.06)

        elif name == "reset":
            with state_lock:
                state["speed"] = 0
                state["rpm"] = 1200
                state["gear"] = "N"
                state["neutral"] = True
                state["lean_angle"] = 0.0
                state["throttle_pct"] = 0.0
                state["engine_load_pct"] = 0.0
                state["warning_banner"] = ""
                state["neopixel_anim"] = "none"

        print(f"[SIM] Scenario completed: {name}")

    t = threading.Thread(target=worker, daemon=True)
    t.start()
    return t

class SimHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def do_GET(self):
        if self.path == "/api/state":
            with state_lock:
                payload = json.dumps(state).encode('utf-8')
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(payload)

        elif self.path == "/favicon.ico":
            self.send_response(204)
            self.end_headers()

        elif self.path == "/api/stream":
            self.send_response(200)
            self.send_header("Content-Type", "text/event-stream")
            self.send_header("Cache-Control", "no-cache")
            self.send_header("Connection", "keep-alive")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()

            while True:
                try:
                    with state_lock:
                        data = json.dumps(state)
                    msg = f"data: {data}\n\n".encode('utf-8')
                    self.wfile.write(msg)
                    self.wfile.flush()
                    time.sleep(0.04)  # ~25 Hz live update
                except (BrokenPipeError, ConnectionResetError):
                    break

        elif self.path in ("/", "/index.html"):
            index_path = os.path.join(DIRECTORY, "index.html")
            if not os.path.exists(index_path):
                # Fallback to docs/preview/index.html
                index_path = os.path.join(DIRECTORY, "..", "docs", "preview", "index.html")
            try:
                with open(index_path, "rb") as f:
                    content = f.read()
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
                self.send_header("Pragma", "no-cache")
                self.send_header("Expires", "0")
                self.end_headers()
                self.wfile.write(content)
            except Exception as e:
                self.send_error(500, str(e))
        else:
            super().do_GET()

    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length).decode('utf-8') if content_length > 0 else "{}"
        try:
            req_data = json.loads(body)
        except Exception:
            req_data = {}

        if self.path == "/api/telemetry":
            with state_lock:
                for k, v in req_data.items():
                    if k in state:
                        state[k] = v
                state["last_update"] = time.time()
                resp = json.dumps({"status": "ok", "state": state}).encode('utf-8')

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(resp)

        elif self.path == "/api/scenario":
            sc_name = req_data.get("scenario", "reset")
            run_scenario(sc_name)
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(json.dumps({"status": "started", "scenario": sc_name}).encode('utf-8'))

        else:
            self.send_error(404, "Endpoint not found")

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

class ThreadedHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True

def main():
    with ThreadedHTTPServer(("127.0.0.1", PORT), SimHandler) as httpd:
        print(f"[AEZEL SIM] Server listening on http://127.0.0.1:{PORT}")
        print(f"[AEZEL SIM] Web Cockpit UI: http://127.0.0.1:{PORT}/")
        print(f"[AEZEL SIM] REST Telemetry Endpoint: POST http://127.0.0.1:{PORT}/api/telemetry")
        print(f"[AEZEL SIM] Scenario Trigger Endpoint: POST http://127.0.0.1:{PORT}/api/scenario")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[AEZEL SIM] Server stopped.")

if __name__ == "__main__":
    main()
