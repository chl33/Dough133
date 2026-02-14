# Copyright (c) 2026 Chris Lee and contributors.
# Licensed under the MIT license. See LICENSE file in the project root for details.

"""A program to fit and plot cooling curve for Dough133."""

# ruff: noqa: T201, INP001

from pathlib import Path

import numpy as np
import pandas as pd
import plotly.graph_objects as go
from scipy.optimize import curve_fit


def f_to_c(f: float) -> float:
    """Convert Fahrenheit to Celsius."""
    return (f - 32) * 5 / 9


def exponential_decay(
    t: np.ndarray,
    t_initial: float,
    tau: float,
    t_asymptote: float,
) -> np.ndarray:
    """Exponential decay function."""
    return t_asymptote + (t_initial - t_asymptote) * np.exp(-t / tau)


def main() -> None:
    """Command line interface."""
    csv_path = Path("2026-2-11-cooling.csv")
    if not csv_path.exists():
        print(f"Error: {csv_path} not found.")
        return

    # Read the CSV. Since it has inconsistent columns, we might need to handle it carefully.
    # The header is: entity_id,state,last_changed,current_temperature
    df = pd.read_csv(csv_path)

    # Filter for the enclosure thermostat
    # Thermostat data has current_temperature in the 4th column
    enclosure_df = df[df["entity_id"] == "climate.doughl33f_thermostat"].copy()
    enclosure_df["time"] = pd.to_datetime(enclosure_df["last_changed"])
    enclosure_df["temp_f"] = pd.to_numeric(enclosure_df["current_temperature"], errors="coerce")
    enclosure_df = enclosure_df.dropna(subset=["temp_f"])
    enclosure_df["temp_c"] = f_to_c(enclosure_df["temp_f"])

    # Filter for the room temperature sensor
    # Fridge sensor data has value in the 'state' column
    ambient_df = df[df["entity_id"] == "sensor.fridge_temperature"].copy()
    ambient_df["time"] = pd.to_datetime(ambient_df["last_changed"])
    ambient_df["temp_f"] = pd.to_numeric(ambient_df["state"], errors="coerce")
    ambient_df = ambient_df.dropna(subset=["temp_f"])
    ambient_df["temp_c"] = f_to_c(ambient_df["temp_f"])

    # Sort by time
    enclosure_df = enclosure_df.sort_values("time")
    ambient_df = ambient_df.sort_values("time")

    # Align times: interpolate ambient temperature at enclosure time points
    # We use timestamps in seconds for interpolation
    enclosure_times = (
        (enclosure_df["time"] - enclosure_df["time"].iloc[0]).dt.total_seconds().to_numpy()
    )
    ambient_times = (
        (ambient_df["time"] - enclosure_df["time"].iloc[0]).dt.total_seconds().to_numpy()
    )

    ambient_interp_c = np.interp(enclosure_times, ambient_times, ambient_df["temp_c"].to_numpy())

    # The user wanted to adjust enclosure temperature based on how room temperature changes.
    # T_adj(t) = T_enclosure(t) - (T_ambient(t) - T_ambient(0))
    ambient_change_c = ambient_interp_c - ambient_interp_c[0]
    enclosure_df["temp_c_adj"] = enclosure_df["temp_c"] - ambient_change_c

    # Prepare data for fitting
    t_fit = enclosure_times
    y_fit = enclosure_df["temp_c_adj"].to_numpy()

    # Initial guesses:
    # T_initial: first value
    # T_asymptote: last value or ambient[0]
    # tau: some reasonable value, say 3600 seconds (1 hour)
    p0 = [y_fit[0], 3600, ambient_interp_c[0]]

    try:
        popt, _ = curve_fit(exponential_decay, t_fit, y_fit, p0=p0)
        _, tau_fit, _ = popt
        y_pred = exponential_decay(t_fit, *popt)
        fit_success = True
    except RuntimeError as e:
        print(f"Fitting failed: {e}")
        fit_success = False

    # Create Plotly graph
    fig = go.Figure()

    # Raw enclosure temp
    fig.add_trace(
        go.Scatter(
            x=enclosure_df["time"],
            y=enclosure_df["temp_c"],
            mode="lines",
            name="Enclosure Temp (C)",
        ),
    )

    # Adjusted enclosure temp
    fig.add_trace(
        go.Scatter(
            x=enclosure_df["time"],
            y=enclosure_df["temp_c_adj"],
            mode="lines",
            name="Enclosure Temp Adjusted (C)",
            line={"dash": "dash"},
        ),
    )

    # Ambient temp (interpolated)
    fig.add_trace(
        go.Scatter(
            x=enclosure_df["time"],
            y=ambient_interp_c,
            mode="lines",
            name="Ambient Temp (C)",
            line={"color": "rgba(100,100,100,0.5)"},
        ),
    )

    if fit_success:
        fig.add_trace(
            go.Scatter(
                x=enclosure_df["time"],
                y=y_pred,
                mode="lines",
                name=f"Exponential Fit (tau={tau_fit / 60:.1f} min)",
                line={"color": "red"},
            ),
        )

        # Add annotation for the time constant
        fig.add_annotation(
            x=enclosure_df["time"].iloc[len(enclosure_df) // 2],
            y=y_pred[len(y_pred) // 2],
            text=f"RC Time Constant (tau): {tau_fit / 60:.2f} minutes",
            showarrow=True,
            arrowhead=1,
        )

    fig.update_layout(
        title="Sourdough Proofer Cooling Analysis",
        xaxis_title="Time",
        yaxis_title="Temperature (°C)",
        legend={"x": 0.7, "y": 0.9},
        template="plotly_white",
    )

    # Save the plot
    output_json = Path("cooling_analysis.json")
    fig.write_json(output_json)
    print(f"Analysis complete. Plot saved to {output_json}")
    if fit_success:
        print(f"Time constant (tau): {tau_fit:.1f} seconds ({tau_fit / 60:.1f} minutes)")


if __name__ == "__main__":
    main()
