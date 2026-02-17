<!--
Suggested GitHub Topics:
- telemetry
- rally
- racing
- data-visualization
- chartjs
- motorsport
- csv-parser
- javascript
- html5
- canvas
-->

# HartUI - Rally Telemetry Visualizer

A browser-based tool for visualizing rally car telemetry data from CSV files. Upload your data, plot multiple parameters on interactive charts, and create custom calculated metrics on the fly.

## Why It Exists

Racing teams collect massive amounts of sensor data during rallies, but analyzing it often requires expensive proprietary software. This tool provides a lightweight, accessible way to explore telemetry data—useful for engineers, drivers, and enthusiasts who want to understand vehicle performance without complex tooling.

## Tech Stack

| Category | Technology |
|----------|------------|
| Frontend | Vanilla JavaScript (ES6+) |
| Styling | CSS3 with CSS Variables |
| Charts | [Chart.js](https://www.chartjs.org/) |
| Math Engine | [mathjs](https://mathjs.org/) |
| Interactions | Hammer.js (touch/pan/zoom) |

## How to Run

This is a static web application—no build step required.

```bash
# Option 1: Open directly
open index.html

# Option 2: Serve locally (recommended for file API)
npx serve .
# or
python3 -m http.server 8080
```

Then navigate to `http://localhost:8080` and upload a CSV telemetry file.

## Project Structure

```
rallyTelemetryWeb/
├── index.html          # Main UI layout
├── main.js             # Core logic: CSV parsing, charting, calculations
├── settings.js         # Configuration for pan/zoom behavior
├── styles.css          # Styling with dark/light mode support
└── README.md           # This file
```

## Key Features

- **CSV Upload**: Drag-and-drop tab-delimited telemetry files
- **Multi-Parameter Plotting**: Select any number of data channels to visualize
- **Display Modes**: Stack charts fully, semi-stacked, or overlay them
- **Custom Parameters**: Create new metrics using mathematical expressions (e.g., `RPM / Speed`)
- **Interactive Charts**: Pan and zoom with mouse or touch gestures
- **Dark/Light Mode**: Auto-detects system preference, toggle via UI

## Key Learnings

- **Chart.js Scales**: Learned to dynamically generate multiple Y-axes with different scales for comparing unrelated metrics (e.g., RPM vs. temperature)
- **CSV Parsing in Browser**: Implemented efficient client-side parsing using FileReader API without server dependencies
- **Math Expression Evaluation**: Integrated mathjs to safely evaluate user-defined formulas against telemetry data sets
- **Responsive Canvas**: Handled chart resizing and dark mode theme switching with CSS custom properties

## License

MIT License - see [LICENSE](LICENSE) for details.
