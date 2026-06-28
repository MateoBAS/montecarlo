"""
Benchmark RowMajor vs ColMajor — speedup en función del nº de activos.

Lee `benchmark_layout_speedup_activos.csv` generado por:
  integration_tests.exe --run_test=Escenario_Benchmark_Layout/Layout_Speedup_vs_NumActivos

Uso:
    python benchmark_layout_speedup.py
"""

from __future__ import annotations

from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import pandas as pd

SCRIPT_DIR = Path(__file__).resolve().parent
CSV_PATH = SCRIPT_DIR / "benchmark_layout_speedup_activos.csv"

# Paleta alineada con convergencia_cartera_base.py
SERIES_STYLE: dict[str, dict[str, str]] = {
    "RowMajor": {"label": "RowMajor", "color": "#8B4513", "marker": "o"},
    "ColMajor": {"label": "ColMajor", "color": "#B22222", "marker": "s"},
    "Speedup": {"label": r"ColMajor / RowMajor", "color": "#E76F51", "marker": "^"},
}


def _apply_axes_style(ax: plt.Axes) -> None:
    ax.set_facecolor("white")
    for spine in ax.spines.values():
        spine.set_edgecolor("black")
        spine.set_linewidth(0.8)
    ax.tick_params(direction="in", top=True, right=True, colors="black", labelsize=9)
    ax.title.set_color("black")
    ax.xaxis.label.set_color("black")
    ax.yaxis.label.set_color("black")
    ax.grid(True, linestyle=":", alpha=0.25, color="#888888", linewidth=0.6)


def _configure_log2_xaxis(ax: plt.Axes, values: np.ndarray) -> None:
    ax.set_xscale("log", base=2)
    ax.xaxis.set_major_formatter(
        ticker.FuncFormatter(lambda x, _pos: f"$2^{{{int(np.log2(x))}}}$")
    )
    v_min, v_max = values.min(), values.max()
    ticks = 2 ** np.arange(
        int(np.floor(np.log2(v_min))),
        int(np.ceil(np.log2(v_max))) + 1,
    )
    ax.set_xticks(ticks)
    ax.set_xlim(v_min * 0.92, v_max * 1.08)


def _configure_log_log_axes(ax: plt.Axes, values: np.ndarray) -> None:
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.xaxis.set_major_formatter(
        ticker.FuncFormatter(lambda x, _pos: f"$2^{{{int(np.log2(x))}}}$")
    )
    v_min, v_max = values.min(), values.max()
    ticks = 2 ** np.arange(
        int(np.floor(np.log2(v_min))),
        int(np.ceil(np.log2(v_max))) + 1,
    )
    ax.set_xticks(ticks)
    ax.set_xlim(v_min * 0.92, v_max * 1.08)


def _plot_series(
    ax: plt.Axes,
    x: np.ndarray,
    y: np.ndarray,
    series_key: str,
) -> plt.Line2D:
    style = SERIES_STYLE[series_key]
    color = style["color"]
    (line,) = ax.plot(
        x,
        y,
        label=style["label"],
        color=color,
        marker=style["marker"],
        linestyle="-",
        linewidth=1.0,
        markersize=6.0,
        markerfacecolor=color,
        markeredgecolor="none",
        zorder=3,
    )
    return line


def _add_legend(ax: plt.Axes, handles: list[plt.Line2D]) -> None:
    ax.legend(
        handles,
        [h.get_label() for h in handles],
        loc="upper left",
        fontsize=9,
        frameon=True,
        facecolor="white",
        edgecolor="black",
        labelcolor="black",
    )


def load_results(csv_path: Path = CSV_PATH) -> pd.DataFrame:
    if not csv_path.is_file():
        raise FileNotFoundError(
            f"No se encontró {csv_path}. Ejecuta antes:\n"
            "  integration_tests.exe "
            "--run_test=Escenario_Benchmark_Layout/Layout_Speedup_vs_NumActivos"
        )
    df = pd.read_csv(csv_path)
    df = df.loc[:, ~df.columns.astype(str).str.match(r"Unnamed")]
    required = {
        "NumReplicas",
        "NumAssets",
        "N",
        "TimeRowMs",
        "TimeColMs",
        "SpeedupColOverRow",
    }
    missing = required - set(df.columns)
    if missing:
        raise ValueError(f"Columnas ausentes en CSV: {sorted(missing)}")
    return df.sort_values("NumAssets").reset_index(drop=True)


def plot_benchmark(df: pd.DataFrame, output_stem: str = "benchmark_layout_speedup_activos") -> None:
    assets = df["NumAssets"].to_numpy()
    speedup = df["SpeedupColOverRow"].to_numpy()
    row_ms = df["TimeRowMs"].to_numpy()
    col_ms = df["TimeColMs"].to_numpy()

    fig, axes = plt.subplots(2, 1, figsize=(8, 7.5), sharex=True)
    fig.patch.set_facecolor("white")

    ax_speed, ax_time = axes

    speed_handle = _plot_series(ax_speed, assets, speedup, "Speedup")
    ax_speed.axhline(
        1.0,
        color="#555555",
        linestyle=":",
        linewidth=1.0,
        zorder=1,
    )
    _configure_log2_xaxis(ax_speed, assets)
    _apply_axes_style(ax_speed)
    ax_speed.set_ylabel("Speedup (Col / Row)", fontsize=10, color="black")
    ax_speed.set_title(
        r"Speedup ColMajor / RowMajor",
        fontsize=11,
        fontweight="bold",
        color="black",
    )
    _add_legend(ax_speed, [speed_handle])

    row_handle = _plot_series(ax_time, assets, row_ms, "RowMajor")
    col_handle = _plot_series(ax_time, assets, col_ms, "ColMajor")
    _configure_log_log_axes(ax_time, assets)
    _apply_axes_style(ax_time)
    ax_time.set_xlabel(r"Número de activos ($2^k$)", fontsize=10, color="black")
    ax_time.set_ylabel("Tiempo total (ms)", fontsize=10, color="black")
    ax_time.set_title(
        "Tiempo de ejecución — escala log-log",
        fontsize=11,
        fontweight="bold",
        color="black",
    )
    _add_legend(ax_time, [row_handle, col_handle])

    """fig.suptitle(
        r"Benchmark de layout — cartera benchmark1 replicada ($T=1$ año, $S=50$ pasos)",
        fontsize=13,
        fontweight="bold",
        color="black",
        y=0.98,
    )"""

    fig.tight_layout(rect=(0, 0, 1, 0.96))

    png_path = SCRIPT_DIR / f"{output_stem}.png"
    pdf_path = SCRIPT_DIR / f"{output_stem}.pdf"
    fig.savefig(png_path, dpi=300, bbox_inches="tight")
    fig.savefig(pdf_path, bbox_inches="tight")
    print(f"Guardado: {png_path}")
    print(f"Guardado: {pdf_path}")
    plt.close(fig)


def print_summary(df: pd.DataFrame) -> None:
    peak = df.loc[df["SpeedupColOverRow"].idxmax()]
    print("\n--- Resumen benchmark layout ---")
    print(f"Pico speedup: {peak['SpeedupColOverRow']:.3f}x "
          f"({int(peak['NumReplicas'])} replicas, {int(peak['NumAssets'])} activos)")
    print(f"  Row={peak['TimeRowMs']:.1f} ms  Col={peak['TimeColMs']:.1f} ms  "
          f"ahorro={peak['TimeColMs'] - peak['TimeRowMs']:.1f} ms")
    last = df.iloc[-1]
    print(f"Extremo ({int(last['NumReplicas'])} replicas): "
          f"speedup={last['SpeedupColOverRow']:.3f}x, "
          f"Row={last['TimeRowMs']:.0f} ms, Col={last['TimeColMs']:.0f} ms")


def main() -> None:
    df = load_results()
    print_summary(df)
    plot_benchmark(df)


if __name__ == "__main__":
    main()
