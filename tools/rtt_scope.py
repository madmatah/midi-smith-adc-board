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

class ScrollBar:
    """A horizontal scrollbar widget for Vispy Scene."""
    def __init__(self, parent, size_callback):
        self.parent = parent
        self.visible = True
        self.get_total_size = size_callback

        # Visuals: Only the thumb, use container bgcolor as track
        self.thumb = visuals.Rectangle(
            center=(0,0,0), width=20, height=18,
            color=(0.6, 0.6, 0.6, 1), parent=parent
        )
        self.thumb.order = 100

        self.width = 100
        self.height = 20
        self.is_dragging = False
        self.last_mouse_x = 0

    def update_layout(self, x, y, width, height):
        self.width = width
        self.height = height

        # Calculate thumb position and size
        total, visible_size, offset = self.get_total_size()
        if total <= visible_size:
            thumb_w = width
            thumb_x = x
        else:
            ratio = visible_size / total
            thumb_w = max(20, width * ratio)

            # offset 0 means RIGHTMOST (Live)
            # offset max = total - visible_size
            max_offset = total - visible_size

            scroll_range_px = width - thumb_w
            if max_offset > 0:
                normalized_pos = offset / max_offset
                thumb_x = x + scroll_range_px - (normalized_pos * scroll_range_px)
            else:
                thumb_x = x

        # Thumb Visuals
        thumb_h = height - 4
        if thumb_h < 1: thumb_h = 1

        thumb_cx = thumb_x + thumb_w / 2.0
        thumb_cy = y + height / 2.0

        self.thumb.center = (thumb_cx, thumb_cy, 0)
        self.thumb.width = thumb_w
        self.thumb.height = thumb_h

        # Store thumb rect for mouse detection
        self.thumb_rect_x = (thumb_x, thumb_x + thumb_w)

        # Force a refresh of the container
        self.parent.update()

    def handle_mouse_press(self, event, x_rel):
        """Returns True if consumed."""
        if not hasattr(self, 'thumb_rect_x'): return False

        t_left, t_right = self.thumb_rect_x
        # Check against relative X passed from parent widget
        if t_left <= x_rel <= t_right:
            self.is_dragging = True
            self.last_mouse_x = event.pos[0]
            return True
        return False

    def handle_mouse_move(self, event) -> Optional[float]:
        """Returns new normalized offset (0.0 to 1.0) if dragging, else None."""
        if not self.is_dragging:
            return None

        dx = event.pos[0] - self.last_mouse_x
        self.last_mouse_x = event.pos[0]

        total, visible_size, _ = self.get_total_size()
        max_offset = total - visible_size

        if max_offset <= 0:
            return 0.0

        thumb_w = self.thumb.width
        scroll_range_px = self.width - thumb_w
        if scroll_range_px <= 0:
            return None

        offset_delta = -(dx / scroll_range_px) * max_offset
        return offset_delta

    def handle_mouse_release(self, event):
        self.is_dragging = False


class RttScope:
    """Visualizes RTT data using Vispy."""

    def __init__(self, sample_count: int, y_max_initial: float, auto_scale: bool = True):
        self.sample_count = sample_count
        self.auto_scale_enabled = auto_scale

        # History Configuration
        self.history_factor = 30
        self.buffer_size = sample_count * self.history_factor

        # Data buffer
        self.signal_buffer = np.zeros((self.buffer_size, 2), dtype=np.float32)
        self.signal_buffer[:, 0] = np.arange(-self.buffer_size + sample_count, sample_count)

        # View State
        self.view_offset = 0
        self.total_samples_received = 0

        # Performance and State
        self.last_update_time = time.time()
        self.last_bytes_count = 0
        self.throughput_kbps = 0.0

        self.hover_data = None  # (index, value)

        # Snapshot feedback
        self.snapshot_message = ""
        self.snapshot_message_expiry = 0

        # Window geometry
        self._window_geometry = None
        self._is_fullscreen = False

        self.remaining_initialization_frames = 3
        self.manual_range_active = False

        self._setup_ui(y_max_initial)
        self._connect_events()

    def _setup_ui(self, y_max_initial: float) -> None:
        """Initializes all UI components."""
        self._setup_window()
        self._setup_plot_area(y_max_initial)
        self._setup_scrollbar()
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
        self.y_axis = scene.AxisWidget(orientation='left', text_color='white', axis_color='white')
        self.y_axis.width_max = 60
        self.grid.add_widget(self.y_axis, row=0, col=0)

        self.view = self.grid.add_view(row=0, col=1, border_color='white')
        self.view.camera = 'panzoom'
        self.view.camera.set_range(x=(0, self.sample_count), y=(0, y_max_initial))
        self.y_axis.link_view(self.view)

        self.line = visuals.Line(
            pos=np.zeros((self.sample_count, 2)),
            color='cyan',
            parent=self.view.scene,
            antialias=True
        )

    def _setup_scrollbar(self) -> None:
        self.scrollbar_widget = self.grid.add_widget(row=1, col=0, col_span=2)
        self.scrollbar_widget.height_max = 20
        self.scrollbar_widget.height_min = 20
        self.scrollbar_widget.bgcolor = (0.2, 0.2, 0.2, 1) # Dark gray track

        self.scrollbar = ScrollBar(
            parent=self.scrollbar_widget,
            size_callback=lambda: (self.buffer_size, self.sample_count, self.view_offset)
        )

    def _setup_status_bar(self) -> None:
        self.status_box = self.grid.add_widget(row=2, col=0, col_span=2)
        self.status_box.height_max = 30
        self.status_box.height_min = 30

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
        canvas_width, canvas_height = self.canvas.size

        if hasattr(self, 'help_hint') and hasattr(self, 'status_box'):
            status_box_width = self.status_box.size[0]
            if status_box_width <= 1:
                status_box_width = canvas_width
            self.help_hint.pos = (status_box_width - 10, 15)

        if hasattr(self, 'help_window'):
            help_window_width, help_window_height = 400, 200
            self.help_window.size = (help_window_width, help_window_height)

            if self.remaining_initialization_frames > 0:
                self.help_window.pos = (-10000, -10000)
            else:
                self.help_window.pos = ((canvas_width - help_window_width) // 2, (canvas_height - help_window_height) // 2)

            self.help_title.pos = (help_window_width // 2, 30)

            column_y_center = 110
            split_x_position = help_window_width // 2 - 80
            column_gap = 5
            self.help_keys.pos = (split_x_position - column_gap, column_y_center)
            self.help_desc.pos = (split_x_position + column_gap, column_y_center)

    def _connect_events(self) -> None:
        self.canvas.events.mouse_move.connect(self.on_mouse_move)
        self.canvas.events.mouse_wheel.connect(self.on_mouse_wheel)
        self.canvas.events.mouse_press.connect(self.on_mouse_press)
        self.canvas.events.mouse_release.connect(self.on_mouse_release)
        self.canvas.events.key_press.connect(self.on_key_press)
        self.canvas.events.resize.connect(self.on_resize)

    def process_incoming_values(self, new_raw_values: Tuple[int, ...]) -> None:
        count = len(new_raw_values)
        if count == 0:
            return

        self.total_samples_received += count

        if count >= self.buffer_size:
            self.signal_buffer[:, 1] = new_raw_values[-self.buffer_size:]
            end_idx = self.total_samples_received
            start_idx = end_idx - self.buffer_size
            self.signal_buffer[:, 0] = np.arange(start_idx, end_idx)
        else:
            self.signal_buffer = np.roll(self.signal_buffer, -count, axis=0)
            self.signal_buffer[-count:, 1] = new_raw_values

            buffer_end_time = self.total_samples_received
            buffer_start_time = buffer_end_time - self.buffer_size
            self.signal_buffer[:, 0] = np.linspace(buffer_start_time, buffer_end_time - 1, self.buffer_size)

        if self.view_offset > 0:
            self.view_offset += count
            max_offset = self.buffer_size - self.sample_count
            if self.view_offset > max_offset:
                self.view_offset = max_offset

        self._update_plot()

    def _update_plot(self) -> None:
        end_idx = self.buffer_size - int(self.view_offset)
        start_idx = end_idx - self.sample_count

        if start_idx < 0:
            start_idx = 0

        data_slice = self.signal_buffer[start_idx:end_idx]

        if len(data_slice) == 0:
            return

        self.line.set_data(pos=data_slice)

        if hasattr(self, 'scrollbar_widget'):
            w, h = self.scrollbar_widget.size
            if w > 0:
                self.scrollbar.update_layout(0, 0, w, h)

        # Always calculate the current data bounds
        x_min = data_slice[0, 0]
        x_max = data_slice[-1, 0]

        if self.view_offset == 0: # LIVE
            if self.manual_range_active:
                # Keep current zoom/scale but shift window to follow the latest data (aligned to right edge)
                rect = self.view.camera.rect
                cam_width = rect.width
                # Slide the camera: keep the width but move the right edge to x_max
                # CRITICAL: Use margin=0 to avoid infinite growth loop when reading from camera.rect
                self.view.camera.set_range(x=(x_max - cam_width, x_max), y=(rect.bottom, rect.top), margin=0)
            else:
                y_range = self._calculate_auto_scale_y(data_slice) if self.auto_scale_enabled else None
                self.view.camera.set_range(x=(x_min, x_max), y=y_range)
        else: # HISTORY
            # In history, only snap the camera if we are NOT in manual mode (e.g. just moved scrollbar)
            if not self.manual_range_active:
                y_range = self._calculate_auto_scale_y(data_slice) if self.auto_scale_enabled else None
                self.view.camera.set_range(x=(x_min, x_max), y=y_range)

    def _calculate_auto_scale_y(self, data_slice) -> Tuple[float, float]:
        """Calculates target Y range for auto-scaling."""
        y_vals = data_slice[:, 1]
        y_min = np.min(y_vals)
        y_max = np.max(y_vals)

        center = (y_min + y_max) / 2.0
        signal_span = y_max - y_min

        if signal_span < 1.0:
            signal_span = 100.0

        # Add 10% padding
        target_span = signal_span * 1.1

        # Enforce minimum span
        if target_span < 10.0: target_span = 10.0

        return (center - target_span/2.0, center + target_span/2.0)

    def on_mouse_move(self, event) -> None:
        if hasattr(self, 'scrollbar'):
            offset_delta = self.scrollbar.handle_mouse_move(event)
            if offset_delta is not None:
                self.view_offset += offset_delta

                max_offset = self.buffer_size - self.sample_count
                if self.view_offset > max_offset: self.view_offset = max_offset
                if self.view_offset < 0: self.view_offset = 0

                self.manual_range_active = False
                self._update_plot()
                self.update_ui_status(True, self.last_bytes_count, force=True)
                return

        if self.view_offset > 0:
            transform = self.canvas.scene.node_transform(self.view.scene)
            pos = transform.map(event.pos)

            x_cursor = pos[0]

            end_idx = self.buffer_size - int(self.view_offset)
            start_idx = end_idx - self.sample_count
            if start_idx < 0: start_idx = 0

            view_data = self.signal_buffer[start_idx:end_idx]
            if len(view_data) > 0:
                first_x = view_data[0, 0]
                last_x = view_data[-1, 0]

                if first_x <= x_cursor <= last_x:
                    idx_in_slice = int(round(x_cursor - first_x))
                    if 0 <= idx_in_slice < len(view_data):
                        val = view_data[idx_in_slice, 1]
                        real_x = view_data[idx_in_slice, 0]

                        self.hover_data = (real_x, val)
                        self.hover_v_line.set_data(pos=real_x)
                        self.hover_v_line.visible = True

                        self.hover_marker.set_data(
                            pos=np.array([[real_x, val]], dtype=np.float32),
                            face_color='yellow',
                            size=8
                        )
                        self.hover_marker.visible = True

                        self.update_ui_status(True, self.last_bytes_count, force=True)
                        return

        self.hover_v_line.visible = False
        self.hover_marker.visible = False
        self.hover_data = None
        if self.view_offset > 0:
             self.update_ui_status(True, self.last_bytes_count, force=True)

        # Explicit update removed to avoid fighting with main loop
        # self.canvas.update()

    def on_mouse_wheel(self, event) -> None:
        self.manual_range_active = True
        if self.view_offset == 0 and self.auto_scale_enabled:
            self.auto_scale_enabled = False
            print("Manual interaction detected: Auto-scale disabled.")

    def on_mouse_press(self, event) -> None:
        # Check if click is within scrollbar widget vertically
        sb_y = self.scrollbar_widget.pos[1]
        sb_h = self.scrollbar_widget.size[1]

        if sb_y <= event.pos[1] <= sb_y + sb_h:
            x_rel = event.pos[0] - self.scrollbar_widget.pos[0]
            if hasattr(self, 'scrollbar'):
                if self.scrollbar.handle_mouse_press(event, x_rel):
                    return
        else:
            # Any click on the plot area (Zoom/Pan/Scale) enters manual mode
            self.manual_range_active = True

            # If we were in auto-scale, disable it to let user control Y
            if self.view_offset == 0 and self.auto_scale_enabled and event.button in [2, 3]:
                self.auto_scale_enabled = False
                print("Manual interaction detected: Auto-scale disabled.")

    def on_mouse_release(self, event) -> None:
        if hasattr(self, 'scrollbar'):
            self.scrollbar.handle_mouse_release(event)

    def on_key_press(self, event) -> None:
        if event.key == ' ':
            self._toggle_pause_history()
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

    def _toggle_pause_history(self) -> None:
        if self.view_offset == 0:
            self.view_offset = 0.1
            print("Paused at current.")
        else:
            self.view_offset = 0
            print("Resumed Live View.")

            self.hover_v_line.visible = False
            self.hover_marker.visible = False
            self.hover_data = None

        self.manual_range_active = False
        self._update_plot()
        self.update_ui_status(True, self.last_bytes_count, force=True)

    def _enable_autoscale(self) -> None:
        self.auto_scale_enabled = True
        self.manual_range_active = False
        print("Auto-scale re-enabled.")

    def _take_snapshot(self) -> None:
        if self.view_offset > 0:
            self.export_snapshot()
        else:
            print("Snapshot is only available in PAUSE/HISTORY mode.")

    def _toggle_fullscreen(self) -> None:
        try:
            window = self.canvas.native._id
            monitor = glfw.get_primary_monitor()
            vmode = glfw.get_video_mode(monitor)

            if not self._is_fullscreen:
                self._window_geometry = (glfw.get_window_pos(window), glfw.get_window_size(window))
                glfw.set_window_monitor(window, monitor, 0, 0, vmode.size.width, vmode.size.height, vmode.refresh_rate)
                self._is_fullscreen = True
            else:
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
        self._update_help_layout()
        self._update_plot()

    def _process_initialization_sequence(self) -> None:
        if self.remaining_initialization_frames > 0:
            self.remaining_initialization_frames -= 1
            if self.remaining_initialization_frames == 0:
                self.help_window.visible = False
                self._update_help_layout()
                self.remaining_initialization_frames = -1

    def export_snapshot(self) -> None:
        end_idx = self.buffer_size - int(self.view_offset)
        start_idx = end_idx - self.sample_count
        if start_idx < 0: start_idx = 0

        view_data = self.signal_buffer[start_idx:end_idx]

        if len(view_data) == 0:
            print("No data points visible in the current view.")
            return

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"rtt_snapshot_{timestamp}.txt"

        try:
            with open(filename, "w") as snapshot_file:
                for row in view_data:
                    snapshot_file.write(f"{row[1]:.0f}\n")

            message_text = f"Snapshot saved to {filename}"
            print(message_text)
            self.snapshot_message = message_text
            self.snapshot_message_expiry = time.time() + 3.0
        except Exception as e:
            print(f"Failed to save snapshot: {e}")


    def update_ui_status(self, is_connected: bool, total_bytes: int, force: bool = False) -> None:
        self._process_initialization_sequence()

        current_time = time.time()
        time_delta_seconds = current_time - self.last_update_time

        if time_delta_seconds >= 1.0:
            bytes_diff = total_bytes - self.last_bytes_count
            self.throughput_kbps = (bytes_diff / 1024.0) / time_delta_seconds
            self.last_update_time = current_time
            self.last_bytes_count = total_bytes
            # Debug heartbeat
            # print(f"UI Heartbeat: {self.throughput_kbps} kB/s, Connected: {is_connected}")

        if not is_connected:
            status, color = "DISCONNECTED (Retrying...)", "#FF0000"
        elif self.view_offset > 0:
            status, color = "HISTORY/PAUSED", "#FFA500"
        else:
            status, color = "CONNECTED", "#00FF00"

        scale_mode = "[AUTO]" if self.auto_scale_enabled else "[MANUAL]"
        text = f"Status: {status} | Scale: {scale_mode} | Throughput: {self.throughput_kbps:.1f} kB/s"

        if self.snapshot_message and current_time < self.snapshot_message_expiry:
            text = self.snapshot_message
            color = "#00FFFF"
        elif self.hover_data and self.view_offset > 0:
            val = self.hover_data[1]
            text += f" | Value: {val:.0f}"

        self.status_text.text = text
        self.status_text.color = color

    def run(self) -> None:
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
            try:
                # 1. Update Plot Data
                raw_data = client.receive_data()
                processed = False
                if raw_data:
                    value_count = len(raw_data) // 4
                    if value_count > 0:
                        new_values = struct.unpack('<' + 'I' * value_count, raw_data[:value_count*4])
                        scope.process_incoming_values(new_values)
                        processed = True

                # If we didn't process data (or even if we did, to ensure frequent UI updates)
                if not processed or scope.view_offset > 0:
                     scope.update_ui_status(client.is_connected, client.total_bytes_received)
                elif processed:
                     # Even if processed, update status bar (throughput etc)
                     scope.update_ui_status(client.is_connected, client.total_bytes_received)

            except Exception as e:
                print(f"Error in update loop: {e}")
                import traceback
                traceback.print_exc()

        timer = app.Timer(interval=0.01, connect=update_callback, start=True)
        scope.run()

if __name__ == '__main__':
    main()
