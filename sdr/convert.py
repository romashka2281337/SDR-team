import numpy as np
import librosa
from pydub import AudioSegment
import sys
import os

def mp3_to_pcm(mp3_file, pcm_file="output.pcm"):
    """
    Конвертация MP3 → PCM (raw int16)
    """
    print(f"[INFO] Конвертация {mp3_file} → {pcm_file}")
    y, sr = librosa.load(mp3_file, sr=44100, mono=True)
    pcm_data = (y * 32767).astype(np.int16)
    pcm_data.tofile(pcm_file)
    print(f"[OK] PCM сохранён: {pcm_file}, {len(pcm_data)} сэмплов @ {sr} Гц")
    return pcm_file, sr

def pcm_to_mp3(pcm_file, mp3_file="restored.mp3", sr=44100):
    """
    Конвертация PCM → MP3
    """
    print(f"[INFO] Конвертация {pcm_file} → {mp3_file}")
    pcm_data = np.fromfile(pcm_file, dtype=np.int16)
    audio = AudioSegment(
        data=pcm_data.tobytes(),
        sample_width=2,  # 16 бит
        frame_rate=sr,
        channels=1
    )
    audio.export(mp3_file, format="mp3", bitrate="192k")
    print(f"[OK] MP3 сохранён: {mp3_file}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Использование:\n  python audio_convert.py mp3_to_pcm input.mp3\n  python audio_convert.py pcm_to_mp3 input.pcm")
        sys.exit(1)

    mode = sys.argv[1]
    infile = sys.argv[2]

    if mode == "mp3_to_pcm":
        mp3_to_pcm(infile)
    elif mode == "pcm_to_mp3":
        pcm_to_mp3(infile)
    else:
        print("Неверный режим! Используй mp3_to_pcm или pcm_to_mp3")