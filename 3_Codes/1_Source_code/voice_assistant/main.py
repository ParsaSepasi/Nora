import os, sys, json, time, signal, traceback
from vosk import Model, KaldiRecognizer
import sounddevice as sd
import socketio
from websocket_client import _emit

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.append(HERE)

# تنظیمات
MODEL_PATH = os.path.join(HERE, "model")
RATE = 16000
BLOCKSIZE = 4000
CHANNELS = 1
WAKE_WORD = "nora"
COMMAND_TIMEOUT_SEC = 5
FINAL_SILENCE_MS = 1200

# ایمپورت پارسر
try:
    from command_parser import handle_command, COMMANDS
except Exception as e:
    print("[FATAL] Cannot import command_parser:", e)
    sys.exit(1)

def _as_bytes(chunk):
    """Ensure bytes for Vosk (fix cffi buffer issue)."""
    # اگر قبلاً bytes/bytearray هست، همونو بده؛ وگرنه تبدیل کن
    if isinstance(chunk, (bytes, bytearray)):
        return chunk
    try:
        # خیلی از آبجکت‌های بافرپذیر با memoryview.tobytes درست می‌شن
        return memoryview(chunk).tobytes()
    except Exception:
        return bytes(chunk)

def build_wake_recognizer(model: Model) -> KaldiRecognizer:
    grammar = json.dumps([WAKE_WORD])
    rec = KaldiRecognizer(model, RATE, grammar)
    try:
        rec.SetWords(True)
        rec.SetMaxAlternatives(0)
    except Exception:
        pass
    print("[ASR] Wake grammar active:", grammar)
    return rec

def build_command_recognizer(model: Model) -> KaldiRecognizer:
    phrases = list(COMMANDS.keys())
    grammar = json.dumps(phrases)
    rec = KaldiRecognizer(model, RATE, grammar)
    try:
        rec.SetWords(True)
        rec.SetMaxAlternatives(0)
    except Exception:
        pass
    print("[ASR] Strict grammar active:", grammar)
    return rec

def listen_command_exact(rec: KaldiRecognizer, stream_read, timeout_sec=COMMAND_TIMEOUT_SEC, silence_ms=FINAL_SILENCE_MS) -> str:
    start = time.time()
    last_voice_time = start
    time.sleep(0.35)  # کمی بیشتر برای جدا شدن ته‌ی wake
    try:
        rec.Reset()
    except Exception:
        pass

    exact_set = set(COMMANDS.keys())
    best_text = ""  # آخرین متن غیرخالی که گرفتیم (صرفاً برای لاگ)

    while True:
        data, _ = stream_read(BLOCKSIZE)
        if not data:
            if time.time() - start > timeout_sec:
                break
            continue

        buf = _as_bytes(data)
        accepted = rec.AcceptWaveform(buf)
        now = time.time()

        if accepted:
            res = json.loads(rec.Result())
            txt = (res.get("text") or "").strip()
            if txt:
                best_text = txt
                last_voice_time = now
                if txt in exact_set:  # ✅ به یکی از عبارات دقیق رسیدیم
                    return txt

            # اگر سکوت طولانی شد و هنوز exact نشد، خروجی فعلی رو برمی‌گردونیم
            if (now - last_voice_time) * 1000 >= silence_ms:
                return txt

        else:
            pres = json.loads(rec.PartialResult())
            ptxt = (pres.get("partial") or "").strip()
            if ptxt:
                last_voice_time = now
                # partial معمولاً کامل نیست؛ برای exact چک نمی‌کنیم

        if now - start > timeout_sec:
            break

    # تایم‌اوت: آخرین نتیجه نهایی
    final = json.loads(rec.FinalResult()).get("text", "").strip()
    return final or best_text

def main():
    if not os.path.isdir(MODEL_PATH):
        print(f"[FATAL] Model not found at: {MODEL_PATH}")
        sys.exit(1)

    print("Loading Vosk model...", MODEL_PATH)
    model = Model(MODEL_PATH)

    stream = sd.RawInputStream(
        samplerate=RATE,
        blocksize=BLOCKSIZE,
        dtype='int16',
        channels=CHANNELS,
    )
    stream.start()

    def _sigint_handler(sig, frame):
        print("\n[EXIT] Interrupted.")
        try:
            stream.stop(); stream.close()
        except Exception:
            pass
        sys.exit(0)
    signal.signal(signal.SIGINT, _sigint_handler)

    print("🎤 Say something... (say 'nora' to wake)")
    wake_rec = build_wake_recognizer(model)

    while True:
        try:
            data, _ = stream.read(BLOCKSIZE)
            if not data:
                continue

            buf = _as_bytes(data)                  # 👈 تبدیل قطعی به bytes
            if wake_rec.AcceptWaveform(buf):
                res = json.loads(wake_rec.Result())
                txt = (res.get("text") or "").strip()

                if txt == WAKE_WORD:
                    print(f"🔔 Wake word detected: {WAKE_WORD}")
                    va_light_var = 1
                    _emit('magic_light_sock', { "magic_light_state": va_light_var })
                    rec_cmd = build_command_recognizer(model)
                    cmd_text = listen_command_exact(
                        rec_cmd, stream.read,
                        timeout_sec=COMMAND_TIMEOUT_SEC,
                        silence_ms=FINAL_SILENCE_MS,
                    )
                    print("📥 Full command (strict):", cmd_text if cmd_text else "<empty>")

                    matched = handle_command(cmd_text)
                    if matched:
                        print("✅ exact command executed")
                        va_light_var = 0
                        _emit('magic_light_sock', { "magic_light_state": va_light_var })                        
                    else:
                        print("❌ not an exact command (ignored)")
                        
                        

                    # بازگشت به حالت ویک‌ورد با یک ریکاگنایزر تازه
                    wake_rec = build_wake_recognizer(model)
                    va_light_var = 0
                    _emit('magic_light_sock', { "magic_light_state": va_light_var })

        except KeyboardInterrupt:
            _sigint_handler(None, None)
        except Exception as e:
            print("[ERROR] Main loop exception:", e)
            traceback.print_exc()
            time.sleep(0.2)

if __name__ == "__main__":
    main()
