#! /bin/sh
if [ ! -d venv ]; then
    python3 -m venv venv && ./venv/bin/pip install pandas plotly numpy scikit-learn
fi

./venv/bin/python analyze_pwm.py
 
