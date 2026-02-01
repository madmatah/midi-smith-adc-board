#!/usr/bin/env python3
"""
RTT Scope - Visualizes telemetry data sent over RTT Channel.
Required dependencies: vispy, pyglfw, numpy  (and libglfw3 installed on the computer)

To install dependencies on ubuntu :
apt install python3-vispy libglfw3 python3-pyglfw python3-numpy
"""

import argparse
import socket
import struct
import sys
import time
import glfw
from datetime import datetime
from typing import Optional, Tuple

import numpy as np
import vispy
from vispy import app, scene
from vispy.scene import visuals

# Use GLFW for window management
vispy.use(app='glfw')

class RttClient:
    """Manages the TCP connection to the Segger RTT Server."""

    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.sock: Optional[socket.socket] = None
        self.is_connected = False
        self.total_bytes_received = 0
        self.last_reconnect_attempt = 0
        self.reconnect_interval_s = 2.0

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def connect(self) -> bool:
        """Establishes connection to the RTT server."""
        self.last_reconnect_attempt = time.time()
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            self.sock.setblocking(False)
            self.is_connected = True
            print(f"Connected to RTT server at {self.host}:{self.port}")
            return True
        except Exception as e:
            self.is_connected = False
            return False

    def receive_data(self, buffer_size: int = 8192) -> Optional[bytes]:
        """Receives raw data from the socket in a non-blocking way."""
        if not self.is_connected:
            self._try_reconnect()
            return None

        try:
            data = self.sock.recv(buffer_size)
            if not data:  # Connection closed by peer
                self.close()
                return None
            self.total_bytes_received += len(data)
            return data
        except BlockingIOError:
            return None
        except Exception as e:
            print(f"Connection lost: {e}")
            self.close()
            return None

    def _try_reconnect(self) -> None:
        """Attempts to reconnect if the interval has passed."""
        if time.time() - self.last_reconnect_attempt > self.reconnect_interval_s:
            self.connect()

    def close(self) -> None:
        """Closes the socket connection."""
        self.is_connected = False
        if self.sock:
            try:
                self.sock.close()
            except:
                pass
            self.sock = None


class RttScope:
    """Visualizes RTT data using Vispy."""

    def __init__(self, n_points: int, y_max_initial: float, auto_scale: bool = True):
        self.n_points = n_points

        # Data buffer for visualization
        self.pos = np.zeros((n_points, 2), dtype=np.float32)
        self.pos[:, 0] = np.arange(n_points)

        # UI Setup
        self.canvas = scene.SceneCanvas(
            keys='interactive',
            show=True,
            title="RTT Channel Monitor",
            bgcolor='black'
        )

        self.grid = self.canvas.central_widget.add_grid(spacing=0)

        # Y Axis Setup
        self.y_axis = scene.AxisWidget(orientation='left', text_color='white', axis_color='white')
        self.y_axis.width_max = 60
        self.grid.add_widget(self.y_axis, row=0, col=0)

        # Plot View Setup
        self.view = self.grid.add_view(row=0, col=1, border_color='white')
        self.view.camera = 'panzoom'
        self.view.camera.set_range(x=(0, n_points), y=(0, y_max_initial))

        self.y_axis.link_view(self.view)

        # Plot Line
        self.line = visuals.Line(
            pos=self.pos,
            color='cyan',
            parent=self.view.scene,
            antialias=True
        )

        # Status Bar Setup
        # We add a widget at the bottom for status
        self.status_box = self.grid.add_widget(row=1, col=0, col_span=2)
        self.status_box.height_max = 30
        self.status_text = scene.Text(
            "Initializing...",
            color='white',
            anchor_x='left',
            parent=self.status_box,
            pos=(10, 15),
            font_size=10
        )

        # Hover Visuals
        self.hover_v_line = visuals.InfiniteLine(
            color=(1.0, 1.0, 1.0, 1.0),
            width=1,
            vertical=True,
            parent=self.view.scene,
            visible=False
        )
        self.hover_marker = visuals.Markers(
            parent=self.view.scene
        )
        # Fix: Initialize with a dummy point to avoid ValueError in Vispy's bounds calculation
        # visually hidden by visible=False
        self.hover_marker.set_data(pos=np.zeros((1, 2), dtype=np.float32))
        self.hover_marker.visible = False

        # Performance tracking
        self.last_update_time = time.time()
        self.last_bytes_count = 0
        self.kbps = 0.0

        # Interaction state
        self.is_paused = False
        self.auto_scale_enabled = auto_scale
        self.hover_data = None  # (index, value)

        # Snapshot feedback
        self.snapshot_message = ""
        self.snapshot_message_expiry = 0

        # Window geometry for fullscreen toggle
        self._window_geom = None
        self._is_fullscreen = False

        self.canvas.events.mouse_move.connect(self.on_mouse_move)
        self.canvas.events.mouse_wheel.connect(self.on_mouse_wheel)
        self.canvas.events.mouse_press.connect(self.on_mouse_press)
        self.canvas.events.key_press.connect(self.on_key_press)
        self.canvas.events.resize.connect(self.on_resize)

        # Help hint at the bottom right
        self.help_hint = scene.Text(
            "Press H for help",
            color='gray',
            anchor_x='right',
            parent=self.status_box,
            pos=(self.canvas.size[0] - 10, 15),
            font_size=10
        )

        # Help window overlay
        self.help_window = scene.Widget(
            parent=self.canvas.central_widget,
            bgcolor=(0.1, 0.1, 0.1, 0.9),
            border_color='white',
            border_width=1,
        )
        self.help_window.visible = False

        self.help_title = scene.Text(
            "Keyboard Shortcuts",
            color='white',
            parent=self.help_window,
            font_size=12,
            bold=True,
            anchor_x='center',
            anchor_y='top'
        )

        self.help_keys = scene.Text(
            "Space\nA\nD\nF\nQ\nH / ?",
            color='yellow',
            parent=self.help_window,
            font_size=10,
            anchor_x='right',
            anchor_y='center'
        )

        self.help_desc = scene.Text(
            ": Pause / Resume\n"
            ": Toggle Auto-scale\n"
            ": Save Snapshot (in Pause)\n"
            ": Toggle Fullscreen\n"
            ": Quit\n"
            ": Show / Hide Help",
            color='white',
            parent=self.help_window,
            font_size=10,
            anchor_x='left',
            anchor_y='center'
        )
        self._update_help_layout()

    def update_plot(self, new_raw_values: Tuple[int, ...]) -> None:
        """Updates the plot data and performs auto-scaling."""
        if self.is_paused:
            return

        shift = len(new_raw_values)
        if shift == 0:
            return

        if shift >= self.n_points:
            self.pos[:, 1] = new_raw_values[-self.n_points:]
        else:
            # Shift existing data left and append new data
            self.pos[:-shift, 1] = self.pos[shift:, 1]
            self.pos[-shift:, 1] = new_raw_values

        self.line.set_data(pos=self.pos)

        if self.auto_scale_enabled:
            self._auto_scale()

    def _auto_scale(self) -> None:
        """Adjusts the Y axis scale to center the signal and fit it in the view."""
        y_min = np.min(self.pos[:, 1])
        y_max = np.max(self.pos[:, 1])

        center = (y_min + y_max) / 2.0
        signal_span = y_max - y_min

        # Ensure we don't have a zero span
        if signal_span < 1.0:
            signal_span = 100.0

        view_span = self.view.camera.rect.height

        # We want the signal to occupy about 66% of the view (span * 1.5)
        # We add a minimum span of 100 units
        target_span = max(signal_span * 1.5, 100.0)

        # Update if current view is too tight or too loose
        if target_span > view_span * 1.1 or target_span < view_span * 0.5:
            self.view.camera.set_range(y=(center - target_span/2.0, center + target_span/2.0))

    def on_mouse_move(self, event) -> None:
        """Handles mouse movement to display values under the cursor (Pause mode only)."""
        if not self.is_paused or event.pos is None:
            self.hover_v_line.visible = False
            self.hover_marker.visible = False
            self.hover_data = None
            return

        # Map mouse screen coordinates to data coordinates
        tr = self.canvas.scene.node_transform(self.view.scene)
        pos = tr.map(event.pos)

        x_data = pos[0]

        if 0 <= x_data < self.n_points:
            idx = int(round(x_data))
            idx = max(0, min(self.n_points - 1, idx))
            val = self.pos[idx, 1]

            self.hover_data = (idx, val)

            # Update markers position
            self.hover_v_line.set_data(pos=idx)
            self.hover_v_line.visible = True

            self.hover_marker.set_data(
                pos=np.array([[idx, val]], dtype=np.float32),
                face_color='yellow',
                size=8
            )
            self.hover_marker.visible = True
        else:
            self.hover_v_line.visible = False
            self.hover_marker.visible = False
            self.hover_data = None

    def on_mouse_wheel(self, event) -> None:
        """Disables auto-scale when user zooms (only if not paused)."""
        if not self.is_paused and self.auto_scale_enabled:
            self.auto_scale_enabled = False
            print("Manual interaction detected: Auto-scale disabled.")

    def on_mouse_press(self, event) -> None:
        """Disables auto-scale when user interacts with mouse buttons (only if not paused)."""
        # Right button and middle button are often used for pan/zoom in Vispy
        if not self.is_paused and event.button in [2, 3] and self.auto_scale_enabled:
            self.auto_scale_enabled = False
            print("Manual interaction detected: Auto-scale disabled.")

    def on_key_press(self, event) -> None:
        """Handles key presses for Pause, Auto-scale toggle and Snapshot."""
        # Special keys like Space are handled via event.key
        if event.key == ' ':
            self.is_paused = not self.is_paused
            if not self.is_paused:
                # Clear hover visuals when resuming
                self.hover_v_line.visible = False
                self.hover_marker.visible = False
                self.hover_data = None

        text = event.text.lower()
        if text == 'a':
            self.auto_scale_enabled = True
            print("Auto-scale re-enabled.")
        elif text == 'd':
            if self.is_paused:
                self.export_snapshot()
            else:
                print("Snapshot is only available in PAUSE mode.")
        elif text == 'f':
            # Manual fullscreen toggle for GLFW backend
            window = self.canvas.native._id
            monitor = glfw.get_primary_monitor()
            vmode = glfw.get_video_mode(monitor)

            if not self._is_fullscreen:
                # Store current geometry
                self._window_geom = (glfw.get_window_pos(window), glfw.get_window_size(window))
                # Switch to fullscreen
                glfw.set_window_monitor(window, monitor, 0, 0, vmode.size.width, vmode.size.height, vmode.refresh_rate)
                self._is_fullscreen = True
            else:
                # Restore windowed mode
                if self._window_geom:
                    pos, size = self._window_geom
                    glfw.set_window_monitor(window, None, pos[0], pos[1], size[0], size[1], 0)
                else:
                    glfw.set_window_monitor(window, None, 100, 100, 800, 600, 0)
                self._is_fullscreen = False
        elif text == 'q':
            app.quit()
        elif text == 'h' or text == '?':
            self.help_window.visible = not self.help_window.visible

    def on_resize(self, event) -> None:
        """Updates UI components layout on window resize."""
        self._update_help_layout()

    def _update_help_layout(self) -> None:
        """Positions the help window and hint text."""
        w, h = self.canvas.size

        # Position help hint relative to its parent status_box
        if hasattr(self, 'help_hint') and hasattr(self, 'status_box'):
            # Use status_box width if available
            sb_w = self.status_box.size[0]
            if sb_w <= 1: # Not yet laid out
                sb_w = w
            self.help_hint.pos = (sb_w - 10, 15)

        # Position help window
        if hasattr(self, 'help_window'):
            # The help window is a child of central_widget, but we position it relative to the canvas
            hw_w, hw_h = 400, 200
            self.help_window.size = (hw_w, hw_h)
            self.help_window.pos = ((w - hw_w) // 2, (h - hw_h) // 2)

            # Y-down logic (0 is top)
            # Title at the top
            self.help_title.pos = (hw_w // 2, 30)

            # Shortcut block centered vertically in the box
            cols_y = 110 # Y position for the center of the block
            split_x = hw_w // 2 - 80
            cols_gap = 5
            self.help_keys.pos = (split_x - cols_gap, cols_y)
            self.help_desc.pos = (split_x + cols_gap, cols_y)

    def export_snapshot(self) -> None:
        """Exports the visible data points to a text file."""
        # Determine visible range
        rect = self.view.camera.rect
        x_min, x_max = rect.left, rect.right

        # Filter data points
        visible_indices = (self.pos[:, 0] >= x_min) & (self.pos[:, 0] <= x_max)
        visible_values = self.pos[visible_indices, 1]

        if len(visible_values) == 0:
            print("No data points visible in the current view.")
            return

        # Generate filename
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"rtt_snapshot_{timestamp}.txt"

        try:
            with open(filename, "w") as f:
                for val in visible_values:
                    f.write(f"{val:.0f}\n")

            msg = f"Snapshot saved to {filename}"
            print(msg)
            self.snapshot_message = msg
            self.snapshot_message_expiry = time.time() + 3.0 # Show for 3 seconds
        except Exception as e:
            print(f"Failed to save snapshot: {e}")

    def update_status(self, is_connected: bool, total_bytes: int) -> None:
        """Updates the status bar with connection state and throughput."""
        now = time.time()
        dt = now - self.last_update_time

        # Update throughput calculation every second
        if dt >= 1.0:
            bytes_diff = total_bytes - self.last_bytes_count
            self.kbps = (bytes_diff / 1024.0) / dt
            self.last_update_time = now
            self.last_bytes_count = total_bytes

        # Update UI text at every call (60Hz) for reactivity
        if not is_connected:
            status, color = "DISCONNECTED (Retrying...)", "#FF0000"
        elif self.is_paused:
            status, color = "PAUSED", "#FFA500"  # Orange
        else:
            status, color = "CONNECTED", "#00FF00"

        scale_mode = "[AUTO]" if self.auto_scale_enabled else "[MANUAL]"
        text = f"Status: {status} | Scale: {scale_mode} | Throughput: {self.kbps:.1f} kB/s"

        # Override with snapshot feedback if active
        if self.snapshot_message and now < self.snapshot_message_expiry:
            text = self.snapshot_message
            color = "#00FFFF" # Cyan
        elif self.hover_data:
            _, val = self.hover_data
            text += f" | Value: {val:.0f}"

        self.status_text.text = text
        self.status_text.color = color

    def run(self) -> None:
        """Starts the Vispy application loop."""
        print("Starting visualization. Close the window to exit.")
        app.run()


def main():
    parser = argparse.ArgumentParser(description=" RTT Scope Visualizer")
    parser.add_argument("--host", default="127.0.0.1", help="RTT server IP (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=60001, help="RTT server port (default: 60001)")
    parser.add_argument("--points", type=int, default=10000, help="Number of points to display")
    parser.add_argument("--y-max", type=float, default=16384, help="Initial Y axis maximum")
    parser.add_argument("--no-auto-scale", action="store_true", help="Disable auto-scaling on startup")

    args = parser.parse_args()

    scope = RttScope(
        n_points=args.points,
        y_max_initial=args.y_max,
        auto_scale=not args.no_auto_scale
    )

    with RttClient(args.host, args.port) as client:
        def update_callback(_event):
            # 1. Update Plot Data
            raw = client.receive_data()
            if raw:
                count = len(raw) // 4
                if count > 0:
                    new_values = struct.unpack('<' + 'I' * count, raw[:count*4])
                    scope.update_plot(new_values)

            # 2. Update Status Bar
            scope.update_status(client.is_connected, client.total_bytes_received)

        timer = app.Timer(interval=1/60.0, connect=update_callback, start=True)
        scope.run()

if __name__ == '__main__':
    main()
