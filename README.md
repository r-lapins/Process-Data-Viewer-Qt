# Process Data Viewer (Qt)

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![Qt6](https://img.shields.io/badge/Qt-6-green)
![CUDA](https://img.shields.io/badge/CUDA-optional-success)
![License](https://img.shields.io/badge/license-MIT-lightgrey)
![Build](https://img.shields.io/badge/build-CMake-blue)
![Platform](https://img.shields.io/badge/platform-Linux-important)

Desktop application for interactive analysis of CSV time-series data and WAV signals.

Built with Qt 6 (Widgets + Charts + Concurrent) and powered by a custom C++ library:
👉 https://github.com/r-lapins/Process-Data-Toolkit

---

## Features

### CSV Analysis
- Filtering: sensor / time range
- Anomaly detection: Z-score, IQR, MAD
- Statistics: min / max / mean / stddev
- Table + plot with anomaly markers
- Export: JSON, CSV, PNG

---

### WAV Analysis
- Backend: CPU (DFT/FFT) / GPU (cuFFT)
- FFT sizes:
- recommended (power-of-two)
- advanced (cuFFT optimized)
- Windowing: Hann / Hamming / None
- Peak detection: local maxima / threshold
- Plots: signal + spectrum
- Export: PNG, CSV, report

---

## Demo

### CSV Analysis
![CSV Demo](docs/demo/csv_demo.gif)

### WAV Analysis
![WAV Demo](docs/demo/wav_demo.gif)

---

### UX / Performance
- Async execution (QtConcurrent)
- Non-blocking UI
- Cached analysis via WavAnalysisSession

---

### Why this separation?

The project is intentionally split into:

- **PDT (library)** → reusable, testable, CLI-capable core
- **PDV (this app)** → interactive Qt UI on top of the library

This design:
- enforces clean architecture
- enables reuse outside GUI applications
- improves testability and maintainability

---

## Architecture

The application is split into three layers:

```text
PDV (Qt UI)
├── core/
├── csv/
├── wav/
└── PDT
    ├── io/
    ├── dsp/
    ├── compute/
    ├── pipeline/
    └── csv/
```

- UI → controllers → PDT
- no computation in Qt layer
- reusable backend (CLI + GUI)

---

### Build

```bash
git clone https://github.com/r-lapins/Process-Data-Viewer-Qt/
cd process_data_viewer_qt

git submodule update --init --recursive

cmake --preset debug
cmake --build --preset debug
```

**CUDA:**

```bash
cmake --preset debug-cuda
cmake --build --preset debug-cuda
```

Run:

```bash
./process_data_viewer
```

---

## Usage

### CSV

1. Load file
2. Select filter + method
3. Analyze → inspect → export

---

### WAV

1. Load file
2. Select segment + backend
3. Analyze → inspect → export

---

## Notes

- Uses QFutureWatcher + QtConcurrent
- Plot downsampling for performance
- Analysis caching handled in PDT

---

## Future Improvements

- Shared base interface for CSV/WAV analysis
- Plugin-style analysis modules
- Unit tests for controllers
- Drag & drop file loading

---

## License

MIT
