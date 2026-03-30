# Process Data Viewer (Qt)

Desktop application for interactive analysis of CSV time-series data and WAV signals.

Built with Qt 6 (Widgets + Charts + Concurrent) and powered by a custom C++ analysis library (process-data-toolkit).

This project consists of:
- Process Data Toolkit (core C++ library)
- Process Data Viewer (Qt desktop application)

---

## Possible use cases

- analysis of industrial logs
- diagnostics of sensor data
- anomaly detection in control systems

---

## Features

### CSV Analysis
- Time-series filtering:
  - by sensor
  - by time range (from / to)
- Anomaly detection:
  - Z-score
  - IQR
  - MAD
- Top-N anomaly selection without full recomputation
- Statistics:
  - min / max / mean / stddev
- Data table view with filtering
- Plot with anomaly markers
- Export:
  - JSON report (global or per-sensor)
  - filtered CSV with anomaly markers
  - PNG plot

---

### WAV Analysis
- Signal segment selection (windowed analysis)
- Spectrum computation:
  - FFT / DFT / Auto
- Window functions:
  - Hann
  - Hamming
  - None
- Peak detection:
  - local maxima
  - threshold-based
- Top-N peak selection without recomputation
- Statistics:
  - signal min / max / mean / stddev
- Plots:
  - signal waveform
  - frequency spectrum
- Export:
  - PNG plots
  - spectrum CSV
  - text report

---

## Demo

### CSV Analysis
![CSV Demo](docs/demo/csv_demo.gif)

### WAV Analysis
![WAV Demo](docs/demo/wav_demo.gif)

---

### UX / Performance
- Asynchronous analysis using QtConcurrent
- UI remains responsive during computation
- Busy state feedback ("Wait for it.")
- Smart recomputation:
  - avoids full recompute when only Top-N changes

---

## Underlying Library (Process Data Toolkit)

This application is built on top of a separate C++20 library:

👉 https://github.com/r-lapins/Process-Data-Toolkit

The toolkit provides all core data processing and signal analysis functionality, including:

- CSV parsing and time-series filtering
- Statistical analysis (mean, stddev, quartiles)
- Anomaly detection (Z-score, IQR, MAD)
- WAV signal processing (DFT, FFT, windowing)
- Spectrum computation and peak detection
- CLI tools for batch processing and benchmarking

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
├── Core
│   ├── MainWindow
│   ├── FileLoaderService
│   └── AnalysisTab (factory)
│
├── CSV module
│   ├── CsvAnalysisTab
│   ├── CsvAnalysisControlsWidget
│   ├── CsvAnalysisController
│   ├── CsvAnalysisEngine
│   └── CsvAnalysisResultsPanel
│
├── WAV module
│   ├── WavAnalysisTab
│   ├── WavAnalysisControlsWidget
│   ├── WavAnalysisController
│   ├── WavAnalysisEngine
│   └── WavAnalysisResultsPanel
│
└── PDT (process-data-toolkit)
    ├── csv/
    ├── wav/
    └── core algorithms
```

### Key design decisions

- Controller pattern
  - UI (Tab) does not run analysis directly
  - Controllers handle orchestration + async execution

- Engine layer
  - pure computation (no Qt)
  - easy to test / reuse

- Separation CSV vs WAV
  - similar flow, independent modules

- SessionData
  - unified data entry point
  - contains:
    - CsvData
    - WavData

---

## Build Instructions

### Requirements

- C++20
- Qt 6 (Widgets, Charts, Concurrent)
- CMake ≥ 3.21

### Build

```bash
git clone https://github.com/r-lapins/Process-Data-Viewer-Qt/
cd process_data_viewer_qt

git submodule update --init --recursive

mkdir build
cd build

cmake ..
cmake --build .
```

Run:

```bash
./process_data_viewer
```

---

## Usage

### Open file

- File → Open
- or Quick Open (predefined examples folder)

### CSV workflow

1. Load CSV
2. Adjust:
   - sensor filter
   - time range
   - anomaly method + threshold
3. Toggle:
   - auto update / manual recompute
4. Inspect:
   - table
   - plot
   - alerts
5. Export results

---

### WAV workflow

1. Load WAV
2. Select:
   - segment (from, window size)
   - algorithm (FFT / DFT)
   - window function
3. Tune:
   - threshold
   - peak mode
4. Toggle plots:
   - signal
   - spectrum
5. Export:
   - PNG
   - CSV
   - report

---

## Example Outputs

- CSV anomaly report (JSON)
- CSV with anomaly markers
- Spectrum CSV (frequency vs magnitude)
- Spectrum report (text)
- Plot PNG exports

---

## Notable Implementation Details

- Uses QFutureWatcher + QtConcurrent::run
- Avoids blocking UI thread
- Partial recomputation optimization:
  - tryUpdateTopAnomaliesOnly()
  - tryUpdateDominantPeaksOnly()
- Plot downsampling (performance-safe rendering)
- Clean separation of:
  - UI
  - orchestration
  - computation

---

## Project Structure

```text
app/
  ├── core/
  ├── csv/
  ├── wav/
include/pdv/
external/process-data-toolkit/
```

---

## Future Improvements

- Shared base interface for CSV/WAV analysis
- Plugin-style analysis modules
- Unit tests for controllers
- Drag & drop file loading

---

## License

MIT (or your chosen license)
