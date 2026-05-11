# pitchshift~

Pure Data external for real-time mono pitch shifting, using the [SoundTouch](https://www.surina.net/soundtouch/) library.

## Usage in Pure Data

```
[pitchshift~]          — default: 0 semitones, quickseek off, antialias off
[pitchshift~ 7]        — start at +7 semitones
```

### Inlet / Outlet

- **Inlet 1** : audio signal (mono)
- **Outlet 1** : pitch-shifted audio signal (mono)

### Messages

| Message | Effect |
|---|---|
| `pitch <float>` | Set pitch shift in semitones (range: -36 … +36) |
| `quickseek <0\|1>` | Enable fast (lower quality) seek — saves CPU |
| `antialias <0\|1>` | Enable anti-alias filter |

> **Note:** SoundTouch introduces a startup latency of a few hundred samples. The first blocks of output will be silent — this is normal.

---

## Compilation

### Prerequisites

| Platform | Command |
|---|---|
| Raspberry Pi OS (32-bit or 64-bit) | `sudo apt install puredata-dev build-essential` |
| macOS | install Pd, Xcode Command Line Tools |
| Linux (Debian/Ubuntu) | `sudo apt install puredata-dev build-essential` |

### Build

```bash
make
```

The Makefile automatically:
1. Detects the architecture (`uname -m`)
2. Compiles SoundTouch as a static library (`soundtouch.a`)
3. Compiles and links the external

### Architecture-specific behaviour

#### Raspberry Pi 32-bit (armhf — `armv7l`)

SoundTouch is compiled without the x86 SIMD files (`sse_optimized.cpp`, `mmx_optimized.cpp`, `cpu_detect_x86.cpp`) and with:

```
-O2 -mfpu=neon -mfloat-abi=hard
```

This enables NEON SIMD auto-vectorization. Requires **armhf** toolchain (the default on Raspberry Pi OS 32-bit).

#### Raspberry Pi 64-bit (`aarch64`)

NEON is native on aarch64 — no extra flags are needed. SoundTouch is compiled with `-O2` only, without the x86 SIMD files.

#### x86 / macOS

SoundTouch is compiled with its native SSE/MMX optimized files and `-O2`.

### Install (Raspberry Pi / Linux)

Copy the compiled external to your Pd search path, for example:

```bash
cp pitchshift~.pd_linux ~/pd-externals/
```

Then in Pd: **Preferences → Path** — add `~/pd-externals`.

### Clean

```bash
make clean
rm -f soundtouch.a soundtouch/source/SoundTouch/*.o
```
