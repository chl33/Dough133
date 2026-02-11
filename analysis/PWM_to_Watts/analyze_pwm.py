import os

import numpy as np
import pandas as pd
import plotly.graph_objects as go
from sklearn.linear_model import LinearRegression


def main():
    """Load the data"""
    csv_path = "pwm_to_watts.csv"
    if not os.path.exists(csv_path):
        print(f"Error: {csv_path} not found.")
        return

    df = pd.read_csv(csv_path)

    # Ensure column names match
    # Expecting "PWM" and "Watts"
    X = df[["PWM"]].values
    y = df["Watts"].values

    # Fit linear regression
    model = LinearRegression()
    model.fit(X, y)

    # Calculate R-squared
    r_squared = model.score(X, y)
    slope = model.coef_[0]
    intercept = model.intercept_

    # Generate points for the line
    x_range = np.linspace(X.min(), X.max(), 100).reshape(-1, 1)
    y_range = model.predict(x_range)

    # Create Plotly figure
    fig = go.Figure()

    # Add data points
    fig.add_trace(
        go.Scatter(
            x=df["PWM"],
            y=df["Watts"],
            mode="markers",
            name="Measured Data",
            marker=dict(size=10, color="blue"),
        )
    )

    # Add regression line
    fig.add_trace(
        go.Scatter(
            x=x_range.flatten(),
            y=y_range,
            mode="lines",
            name=f"Fit: y = {slope:.2f}x + {intercept:.2f}",
            line=dict(color="red", width=2),
        )
    )

    # Update layout
    fig.update_layout(
        title=f"PWM to Watts Relationship (R² = {r_squared:.4f})",
        xaxis_title="PWM Command",
        yaxis_title="Power (Watts)",
        template="plotly_white",
        legend=dict(yanchor="top", y=0.99, xanchor="left", x=0.01),
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
