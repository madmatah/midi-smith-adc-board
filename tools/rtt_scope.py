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
        self._host = host
        self._port = port
        self._socket: Optional[socket.socket] = None
        self.is_connected = False
        self.total_bytes_received = 0
        self._last_reconnect_attempt_time = 0
        self._reconnect_interval_seconds = 2.0

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def connect(self) -> bool:
        """Establishes connection to the RTT server."""
        self._last_reconnect_attempt_time = time.time()
        try:
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._socket.connect((self._host, self._port))
            self._socket.setblocking(False)
            self.is_connected = True
            print(f"Connected to RTT server at {self._host}:{self._port}")
            return True
        except Exception:
            self.is_connected = False
            return False

    def receive_data(self, buffer_size: int = 8192) -> Optional[bytes]:
        """Receives raw data from the socket in a non-blocking way."""
        if not self.is_connected:
            self._attempt_reconnect_if_needed()
            return None

        try:
            data = self._socket.recv(buffer_size)
            if not data:
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

    def _attempt_reconnect_if_needed(self) -> None:
        """Attempts to reconnect if the interval has passed."""
        if time.time() - self._last_reconnect_attempt_time > self._reconnect_interval_seconds:
            self.connect()

    def close(self) -> None:
        """Closes the socket connection."""
        self.is_connected = False
        if self._socket:
            try:
                self._socket.close()
            except Exception:
                pass
            self._socket = None


class RttScope:
    """Visualizes RTT data using Vispy."""

    def __init__(self, sample_count: int, y_max_initial: float, auto_scale: bool = True):
        self.sample_count = sample_count
        self.auto_scale_enabled = auto_scale

        # Data buffer for visualization
        # Column 0: X (time/index), Column 1: Y (signal value)
        self.signal_buffer = np.zeros((sample_count, 2), dtype=np.float32)
        self.signal_buffer[:, 0] = np.arange(sample_count)

        # Performance and State
        self.last_update_time = time.time()
        self.last_bytes_count = 0
        self.throughput_kbps = 0.0
        self.is_paused = False
        self.paused_backlog = []  # Buffer to store data received while paused
        self.hover_data = None  # (index, value)

        # Snapshot feedback
        self.snapshot_message = ""
        self.snapshot_message_expiry = 0

        # Window geometry
        self._window_geometry = None
        self._is_fullscreen = False



        # Frames during which the help window is rendered but hidden off-screen to initialization GPU resources
        self.remaining_initialization_frames = 3

        self._setup_ui(y_max_initial)
        self._connect_events()

    def _setup_ui(self, y_max_initial: float) -> None:
        """Initializes all UI components."""
        self._setup_window()
        self._setup_plot_area(y_max_initial)
        self._setup_status_bar()
        self._setup_hover_tools()
        self._setup_help_window()

    def _setup_window(self) -> None:
        self.canvas = scene.SceneCanvas(
            keys='interactive',
            show=True,
            title="RTT Channel Monitor",
            bgcolor='black'
        )
        self.grid = self.canvas.central_widget.add_grid(spacing=0)

    def _setup_plot_area(self, y_max_initial: float) -> None:
        # Y Axis
        self.y_axis = scene.AxisWidget(orientation='left', text_color='white', axis_color='white')
        self.y_axis.width_max = 60
        self.grid.add_widget(self.y_axis, row=0, col=0)

        # Plot View
        self.view = self.grid.add_view(row=0, col=1, border_color='white')
        self.view.camera = 'panzoom'
        self.view.camera.set_range(x=(0, self.sample_count), y=(0, y_max_initial))
        self.y_axis.link_view(self.view)

        # Signal Line
        self.line = visuals.Line(
            pos=self.signal_buffer,
            color='cyan',
            parent=self.view.scene,
            antialias=True
        )

    def _setup_status_bar(self) -> None:
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

        self.help_hint = scene.Text(
            "Press H for help",
            color='gray',
            anchor_x='right',
            parent=self.status_box,
            pos=(self.canvas.size[0] - 10, 15),
            font_size=10
        )

    def _setup_hover_tools(self) -> None:
        self.hover_v_line = visuals.InfiniteLine(
            color=(1.0, 1.0, 1.0, 1.0),
            width=1,
            vertical=True,
            parent=self.view.scene,
            visible=False
        )
        # Initialize with a dummy point to avoid ValueError in Vispy's bounds calculation
        self.hover_marker = visuals.Markers(parent=self.view.scene)
        self.hover_marker.set_data(pos=np.zeros((1, 2), dtype=np.float32))
        self.hover_marker.visible = False

    def _setup_help_window(self) -> None:
        self.help_window = scene.Widget(
            parent=self.canvas.central_widget,
            bgcolor=(0.1, 0.1, 0.1, 0.9),
            border_color='white',
            border_width=1,
        )
        # Initially visible for warmup, but will be positioned off-screen
        self.help_window.visible = True

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

    def _update_help_layout(self) -> None:
        """Positions the help window and hint text."""
        canvas_width, canvas_height = self.canvas.size

        # Position help hint relative to its parent status_box
        if hasattr(self, 'help_hint') and hasattr(self, 'status_box'):
            # Use status_box width if available
            status_box_width = self.status_box.size[0]
            if status_box_width <= 1: # Not yet laid out
                status_box_width = canvas_width
            self.help_hint.pos = (status_box_width - 10, 15)

        # Position help window
        if hasattr(self, 'help_window'):
            # The help window is a child of central_widget, but we position it relative to the canvas
            help_window_width, help_window_height = 400, 200
            self.help_window.size = (help_window_width, help_window_height)

            if self.remaining_initialization_frames > 0:
                # Render off-screen during initialization
                self.help_window.pos = (-10000, -10000)
            else:
                self.help_window.pos = ((canvas_width - help_window_width) // 2, (canvas_height - help_window_height) // 2)

            # Y-down logic (0 is top)
            # Title at the top
            self.help_title.pos = (help_window_width // 2, 30)

            # Shortcut block centered vertically in the box
            column_y_center = 110 # Y position for the center of the block
            split_x_position = help_window_width // 2 - 80
            column_gap = 5
            self.help_keys.pos = (split_x_position - column_gap, column_y_center)
            self.help_desc.pos = (split_x_position + column_gap, column_y_center)
    def _connect_events(self) -> None:
        self.canvas.events.mouse_move.connect(self.on_mouse_move)
        self.canvas.events.mouse_wheel.connect(self.on_mouse_wheel)
        self.canvas.events.mouse_press.connect(self.on_mouse_press)
        self.canvas.events.key_press.connect(self.on_key_press)
        self.canvas.events.resize.connect(self.on_resize)

    def process_incoming_values(self, new_raw_values: Tuple[int, ...]) -> None:
        """Updates the plot data and performs auto-scaling."""
        if self.is_paused:
            self.paused_backlog.extend(new_raw_values)
            # Limit the backlog to avoid excessive memory usage (e.g., 10x the view width)
            max_backlog_size = self.sample_count * 10
            if len(self.paused_backlog) > max_backlog_size:
                self.paused_backlog = self.paused_backlog[-max_backlog_size:]
            return

        shift = len(new_raw_values)
        if shift == 0:
            return

        if shift >= self.sample_count:
            self.signal_buffer[:, 1] = new_raw_values[-self.sample_count:]
        else:
            # Shift existing data left and append new data
            self.signal_buffer[:-shift, 1] = self.signal_buffer[shift:, 1]
            self.signal_buffer[-shift:, 1] = new_raw_values

        self.line.set_data(pos=self.signal_buffer)

        if self.auto_scale_enabled:
            self._adjust_y_axis_limit()

    def _adjust_y_axis_limit(self) -> None:
        """Adjusts the Y axis scale to center the signal and fit it in the view."""
        y_min = np.min(self.signal_buffer[:, 1])
        y_max = np.max(self.signal_buffer[:, 1])

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
        transform = self.canvas.scene.node_transform(self.view.scene)
        pos = transform.map(event.pos)

        x_data = pos[0]

        if 0 <= x_data < self.sample_count:
            idx = int(round(x_data))
            idx = max(0, min(self.sample_count - 1, idx))
            val = self.signal_buffer[idx, 1]

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
        if event.key == ' ':
            self._toggle_pause()
            return

        text = event.text.lower()
        if text == 'a':
            self._enable_autoscale()
        elif text == 'd':
            self._take_snapshot()
        elif text == 'f':
            self._toggle_fullscreen()
        elif text == 'q':
            self._quit_app()
        elif text == 'h' or text == '?':
            self._toggle_help()

    def _toggle_pause(self) -> None:
        self.is_paused = not self.is_paused
        if not self.is_paused:
            if self.paused_backlog:
                backlog_data = tuple(self.paused_backlog)
                self.paused_backlog = []
                self.process_incoming_values(backlog_data)
                print(f"Resumed and processed {len(backlog_data)} backlog samples.")

            # Clear hover visuals when resuming
            self.hover_v_line.visible = False
            self.hover_marker.visible = False
            self.hover_data = None

    def _enable_autoscale(self) -> None:
        self.auto_scale_enabled = True
        print("Auto-scale re-enabled.")

    def _take_snapshot(self) -> None:
        if self.is_paused:
            self.export_snapshot()
        else:
            print("Snapshot is only available in PAUSE mode.")

    def _toggle_fullscreen(self) -> None:
        # Manual fullscreen toggle for GLFW backend
        try:
            window = self.canvas.native._id
            monitor = glfw.get_primary_monitor()
            vmode = glfw.get_video_mode(monitor)

            if not self._is_fullscreen:
                # Store current geometry
                self._window_geometry = (glfw.get_window_pos(window), glfw.get_window_size(window))
                # Switch to fullscreen
                glfw.set_window_monitor(window, monitor, 0, 0, vmode.size.width, vmode.size.height, vmode.refresh_rate)
                self._is_fullscreen = True
            else:
                # Restore windowed mode
                if self._window_geometry:
                    pos, size = self._window_geometry
                    glfw.set_window_monitor(window, None, pos[0], pos[1], size[0], size[1], 0)
                else:
                    glfw.set_window_monitor(window, None, 100, 100, 800, 600, 0)
                self._is_fullscreen = False
        except Exception as e:
            print(f"Failed to toggle fullscreen: {e}")

    def _quit_app(self) -> None:
        app.quit()

    def _toggle_help(self) -> None:
        self.help_window.visible = not self.help_window.visible

    def on_resize(self, event) -> None:
        """Updates UI components layout on window resize."""
        self._update_help_layout()

    def _process_initialization_sequence(self) -> None:
        if self.remaining_initialization_frames > 0:
            self.remaining_initialization_frames -= 1
            if self.remaining_initialization_frames == 0:
                self.help_window.visible = False
                self._update_help_layout()
                self.remaining_initialization_frames = -1

    def export_snapshot(self) -> None:
        """Exports the visible data points to a text file."""
        # Determine visible range
        rect = self.view.camera.rect
        x_min, x_max = rect.left, rect.right

        # Filter data points
        visible_indices = (self.signal_buffer[:, 0] >= x_min) & (self.signal_buffer[:, 0] <= x_max)
        visible_values = self.signal_buffer[visible_indices, 1]

        if len(visible_values) == 0:
            print("No data points visible in the current view.")
            return

        # Generate filename
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"rtt_snapshot_{timestamp}.txt"

        try:
            with open(filename, "w") as snapshot_file:
                for signal_value in visible_values:
                    snapshot_file.write(f"{signal_value:.0f}\n")

            message_text = f"Snapshot saved to {filename}"
            print(message_text)
            self.snapshot_message = message_text
            self.snapshot_message_expiry = time.time() + 3.0 # Show for 3 seconds
        except Exception as e:
            print(f"Failed to save snapshot: {e}")

    def update_ui_status(self, is_connected: bool, total_bytes: int) -> None:
        """Updates the status bar with connection state and throughput."""
        self._process_initialization_sequence()

        current_time = time.time()
        time_delta_seconds = current_time - self.last_update_time

        # Update throughput calculation every second
        if time_delta_seconds >= 1.0:
            bytes_diff = total_bytes - self.last_bytes_count
            self.throughput_kbps = (bytes_diff / 1024.0) / time_delta_seconds
            self.last_update_time = current_time
            self.last_bytes_count = total_bytes

        # Update UI text at every call (60Hz) for reactivity
        if not is_connected:
            status, color = "DISCONNECTED (Retrying...)", "#FF0000"
        elif self.is_paused:
            status, color = "PAUSED", "#FFA500"  # Orange
        else:
            status, color = "CONNECTED", "#00FF00"

        scale_mode = "[AUTO]" if self.auto_scale_enabled else "[MANUAL]"
        text = f"Status: {status} | Scale: {scale_mode} | Throughput: {self.throughput_kbps:.1f} kB/s"

        # Override with snapshot feedback if active
        if self.snapshot_message and current_time < self.snapshot_message_expiry:
            text = self.snapshot_message
            color = "#00FFFF" # Cyan
        elif self.hover_data:
            _, value = self.hover_data
            text += f" | Value: {value:.0f}"

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
        sample_count=args.points,
        y_max_initial=args.y_max,
        auto_scale=not args.no_auto_scale
    )

    with RttClient(args.host, args.port) as client:
        def update_callback(_event):
            # 1. Update Plot Data
            raw_data = client.receive_data()
            if raw_data:
                value_count = len(raw_data) // 4
                if value_count > 0:
                    new_values = struct.unpack('<' + 'I' * value_count, raw_data[:value_count*4])
                    scope.process_incoming_values(new_values)

            # 2. Update Status Bar
            scope.update_ui_status(client.is_connected, client.total_bytes_received)

        timer = app.Timer(interval=1/60.0, connect=update_callback, start=True)
        scope.run()

if __name__ == '__main__':
    main()
