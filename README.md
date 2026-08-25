# Gamma Engine

A cross-platform JUCE/C++ generative audio experiment with a deterministic gamma-frequency modulation layer.

## Current reconstructed milestone

This repository reconstructs the latest known working architecture from the development session:

- native JUCE GUI app
- macOS + Windows target
- procedural generative pad
- weighted harmonic Conductor
- adaptive Energy control
- slow timbral motion
- deterministic GammaEngine AM processor
- oscilloscope
- FFT spectrum view
- PianoEngine sampler class prepared for the next milestone

The reconstruction is intended to be functionally equivalent to the latest known state, but it is not a byte-for-byte copy of the project that remains on the original Mac.

## Architecture

```text
Energy
  ↓
Conductor
  ↓
MusicEngine
  ↓
GammaEngine
  ↓
Audio output

PianoEngine
  └─ next integration milestone
```

## Local build

Prerequisites:

- Git
- CMake 3.22+
- C++20 compiler

JUCE 9.0.0 is fetched automatically by CMake.

### macOS / Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

### Windows

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
```

## GitHub Codespaces

Open the repository in a Codespace. The `.devcontainer` installs the Linux build dependencies and configures the project.

Codespaces is useful for editing, refactoring and compile checks. Native macOS and Windows builds are produced by GitHub Actions.

## Automated builds

`.github/workflows/build.yml` builds on:

- `macos-latest`
- `windows-latest`

Build artifacts are available from the GitHub Actions run.

## Piano milestone

The `PianoEngine` exists but is not mixed into the audio path yet.

Next:

1. obtain a legally usable `piano_C4.wav`
2. load it
3. add a test trigger
4. mix it alongside the generated pad
5. decide which layer receives gamma modulation
