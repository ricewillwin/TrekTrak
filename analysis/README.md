# Analysis

Data analysis, ML modelling, and Node-RED flow for TrekTrak.

## Contents

| File / Directory | Description |
|---|---|
| `nodered.json` | Node-RED flow — subscribes to `trektrak/sensor/report`, parses JSON, routes Ultrasonic / PIR / GPS fields to InfluxDB |
| `initial_analysis.ipynb` | Exploratory analysis of `data/sim.csv`: descriptive stats, time-series plots, hourly pattern, sensor correlations |
| `ml_classifier.ipynb` | Pedestrian vs cyclist binary classifier (Random Forest) — training, evaluation, feature importances, and on-device deployment notes |
| `figures/` | Output plots saved by both notebooks |
| `models/` | Saved model files (`rf_pedestrian_cyclist.joblib`) |

## Importing the Node-RED Flow

1. Open Node-RED at `http://localhost:1880`.
2. Hamburger menu → **Import** → select `nodered.json`.
3. Open the **Ultrasonic**, **PIR**, and **GPS** InfluxDB out nodes and update:
   - **Server** URL → your host's LAN IP (not `localhost`)
   - **API Token** → the all-access token generated in the InfluxDB UI
4. Deploy.

The flow writes to org `trektrak`, bucket `sensor`, with separate measurements per sensor type.

## Running the Notebooks

```bash
# from repo root with the venv active
jupyter notebook analysis/initial_analysis.ipynb
jupyter notebook analysis/ml_classifier.ipynb
```

`initial_analysis.ipynb` expects `data/sim.csv` — generate it first:

```bash
python simulation/simulation.py --fast --csv data/sim.csv
```

`ml_classifier.ipynb` generates its own synthetic training data at runtime; no input file needed.
