# Copyright (c) 2026 Chris Lee and contributors.
# Licensed under the MIT license. See LICENSE file in the project root for details.

"""A program to fit and plot PWM command -> Watts for Dough133."""

# ruff: noqa: T201, ERA001, INP001

from pathlib import Path

import numpy as np
import pandas as pd
import plotly.graph_objects as go
from sklearn.linear_model import LinearRegression


def main() -> None:
    """Command line interface."""
    # Load the data
    csv_path = Path("pwm_to_watts.csv")
    if not csv_path.exists():
        print(f"Error: {csv_path} not found.")
        return

    df = pd.read_csv(csv_path)

    pwm_label = "PWM"
    watts_label = "Watts"

    # Ensure column names match
    # Expecting "PWM" and "Watts"
    pwm_mat = df[[pwm_label]].to_numpy()
    watt_vec = df[watts_label].to_numpy()

    # Fit linear regression
    model = LinearRegression()
    model.fit(pwm_mat, watt_vec)

    # Calculate R-squared
    r_squared = model.score(pwm_mat, watt_vec)
    slope = model.coef_[0]
    intercept = model.intercept_

    # Generate points for the line
    x_range = np.linspace(pwm_mat.min(), pwm_mat.max(), 100).reshape(-1, 1)
    y_range = model.predict(x_range)

    # Create Plotly figure
    fig = go.Figure()

    # Add data points
    fig.add_trace(
        go.Scatter(
            x=df[pwm_label],
            y=df[watts_label],
            mode="markers",
            name="Measured Data",
            marker={"size": 10, "color": "blue"},
        ),
    )

    # Add regression line
    fig.add_trace(
        go.Scatter(
            x=x_range.flatten(),
            y=y_range,
            mode="lines",
            name=f"Fit: y = {slope:.2f}x + {intercept:.2f}",
            line={"color": "red", "width": 2},
        ),
    )

    # Update layout
    fig.update_layout(
        title=f"PWM to Watts Relationship (R² = {r_squared:.4f})",
        xaxis_title="PWM Command",
        yaxis_title="Power (Watts)",
        template="plotly_white",
        legend={"yanchor": "top", "y": 0.99, "xanchor": "left", "x": 0.01},
    )

    # Save as HTML snippet for Hugo
    # include_plotlyjs=False if you have it loaded globally in Hugo,
    # but 'cdn' is safer for standalone snippets.
    #    output_html = 'pwm_to_watts_plot.html'
    #    fig.write_html(output_html, include_plotlyjs='cdn', full_html=False)

    # fig.update_layout(template="plotly_dark", title="Mt Bruno Elevation")
    output_json = "pwm_to_watts_plot.json"
    fig.write_json(output_json)

    # print(f"Plot saved to {output_html}")
    print(f"Plot saved to {output_json}")
    print(f"Equation: Watts = {slope:.4f} * PWM + {intercept:.4f}")
    print(f"R-squared: {r_squared:.4f}")


if __name__ == "__main__":
    main()
