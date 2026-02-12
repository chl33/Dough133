#! /bin/sh
set -e
# Build venv for running script, if needed.
if [ ! -d venv ]; then
    python3 -m venv venv && ./venv/bin/pip install pandas plotly numpy scikit-learn
fi
# Run the analysis.
./venv/bin/python analyze_pwm.py
# Possibly copy the plot to be published.
DEST=$HOME/Home/Blog/projects/static/json/
if [ "$DEST" ]; then
    cp pwm_to_watts_plot.json "$DEST"
fi
