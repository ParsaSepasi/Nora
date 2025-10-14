# -*- coding: utf-8 -*-
# voice_assistant/websocket_client.py
#
# کلاینت Python-SocketIO هماهنگ با سرور Flask-SocketIO شما.
# ایونت‌ها و payload ها مطابق فایل سرور شما تنظیم شده‌اند.

import os
import sys
import threading
from typing import Optional

# مطمئن شو ایمپورت از همین پوشه کار می‌کند (وقتی به صورت ماژول اجرا نمی‌کنی)
HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.append(HERE)

# نیاز به نصب: pip install "python-socketio[client]"
import socketio

# ---------- تنظیمات ----------
# پورت/هاست سرور Flask-SocketIO
SERVER_URL = os.getenv("WS_SERVER_URL", "http://127.0.0.1:5050")
# سرورت نام‌فضای خاص تعریف نکرده، پس پیش‌فرض "/" است
NAMESPACE = "/"

# یک کلاینت سراسری بسازیم
_sio: Optional[socketio.Client] = None
_sio_lock = threading.Lock()


def _get_client() -> socketio.Client:
    """Singleton socketio.Client با اتصال مطمئن به نام‌فضای پیش‌فرض /"""
    global _sio
    with _sio_lock:
        if _sio is None:
            _sio = socketio.Client(logger=False, engineio_logger=False, reconnection=True)

            @_sio.event
            def connect():
                print(f"[SOCKET] Connected to {SERVER_URL} (ns=/)")

            @_sio.event
            def disconnect():
                print("[SOCKET] Disconnected")

            # اگر رو سرور هندلر connect نوشتی، اینجا نیاز به چیز خاصی نیست
        # اگه وصل نیست، وصل شو
        if not _sio.connected:
            try:
                # به نام‌فضای / وصل می‌شیم (پیش‌فرض)
                _sio.connect(SERVER_URL, transports=["websocket"])
            except Exception as e:
                print("[SOCKET] Connect failed:", e)
        return _sio


# ---------- Helper های ارسال ----------
def _emit(event: str, data: dict):
    """
    Emit ساده روی namespace پیش‌فرض "/" (بدون namespace=... تا هم‌خوان با اتصال باشد)
    """
    sio = _get_client()
    if sio and sio.connected:
        try:
            sio.emit(event, data)  # default namespace "/"
        except Exception as e:
            print(f"[SOCKET] emit('{event}') failed:", e)
    else:
        print("[WS-Fallback] Not connected ->", event, data)


# ---------- API های موردنیاز voice assistant ----------
def send_reading_light(state: bool):
    """
    رویداد سرور: @Socketio.on('reading_light_sock')
    payload: {"state": bool}
    """
    _emit("reading_light_sock", {"state": bool(state)})


def send_backlight(state: bool):
    """
    رویداد سرور: @Socketio.on('back_light_sock')
    payload: {"state": bool}
    """
    _emit("back_light_sock", {"state": bool(state)})


# سرورت رویدادی برای box نداره؛ فعلاً فقط هشدار لاگ می‌زنیم
def send_open_box():
    _emit("party_mode_toggle", {"party_mode_state": True})


def send_close_box():
    _emit("party_mode_toggle", {"party_mode_state": False})

def send_equalizer1():
    _emit("magic_light_sock", {"magic_light_state": "1"})


def send_equalizer2():
    _emit("magic_light_sock", {"magic_light_state": "2"})


def send_equalizer3():
    _emit("magic_light_sock", {"magic_light_state": "3"})


def send_equalizer_off():
    # بر اساس کد سرور، خاموشی با کد 7 انجام می‌شود
    _emit("magic_light_sock", {"magic_light_state": "0"})


def send_custom_rgb(r: int, g: int, b: int, brightness: int = 128):
    """
    رویداد سرور: @Socketio.on('light_custom_config')
    payload: {"color": "#RRGGBB", "brightness": int(0..255)}
    """
    r = max(0, min(255, int(r)))
    g = max(0, min(255, int(g)))
    b = max(0, min(255, int(b)))
    brightness = max(0, min(255, int(brightness)))
    color_hex = f"#{r:02x}{g:02x}{b:02x}"
    _emit("light_custom_config", {"color": color_hex, "brightness": brightness})


# (اختیاری) گرفتن وضعیت اولیه reading_light از سرور
def request_state():
    """
    @Socketio.on('state') -> سرور emit('reading_light_state', reading_light_state)
    """
    _emit("state", {})
