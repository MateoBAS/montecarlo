"""
Amdahl — speedup empírico con incertidumbre (benchmark1).

Lee `amdahl_estadistico_benchmark1.csv` generado por:
  integration_tests.exe --run_test=Escenario_Rendimiento_Estadistico/Amdahl_Con_Incertidumbre

Ajusta el modelo Amdahl por tramos (SMT):
    S(k) = 1 / ((1-P) + P/k   + beta*(k-1))           si k <= N
    S(k) = 1 / ((1-P) + P/(k*c) + beta*(k-1))         si N < k <= 2N

Uso:
    python amdahl_estadistico_benchmark1.py
"""

from __future__ import annotations

from pathlib import Path
from typing import Callable

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy.optimize import curve_fit

SCRIPT_DIR = Path(__file__).resolve().parent
CSV_PATH = SCRIPT_DIR / "amdahl_estadistico_benchmark1.csv"
OUTPUT_STEM = "amdahl_estadistico_benchmark1"

# Estilo sobrio: datos en negro, acento cálido solo en la curva ajustada
COLOR_FIT = "#B22222"   # Antithetic (paleta convergencia)
COLOR_IDEAL = "#888888"

# N: nucleos fisicos (limite k <= N; rama SMT para N < k <= 2N)
HT_EFFICIENCY = 0.73  # c


def speedup_amdahl(
    k: np.ndarray,
    P: float,
    beta: float,
    *,
    physical_cores: int,
    c: float = HT_EFFICIENCY,
) -> np.ndarray:
    k_arr = np.asarray(k, dtype=float)
    n = float(physical_cores)
    out = np.empty_like(k_arr)

    mask_phys = k_arr <= n
    mask_smt = (k_arr > n) & (k_arr <= 2.0 * n)

    out[mask_phys] = 1.0 / (
        (1.0 - P) + P / k_arr[mask_phys] + beta * (k_arr[mask_phys] - 1.0)
    )
    out[mask_smt] = 1.0 / (
        (1.0 - P) + P / (k_arr[mask_smt] * c) + beta * (k_arr[mask_smt] - 1.0)
    )

    mask_beyond = k_arr > 2.0 * n
    if np.any(mask_beyond):
        out[mask_beyond] = 1.0 / (
            (1.0 - P)
            + P / (k_arr[mask_beyond] * c)
            + beta * (k_arr[mask_beyond] - 1.0)
        )

    return out


def make_fit_model(physical_cores: int, c: float = HT_EFFICIENCY) -> Callable[..., np.ndarray]:
    def fit_model(k: np.ndarray, P: float, beta: float) -> np.ndarray:
        return speedup_amdahl(k, P, beta, physical_cores=physical_cores, c=c)

    return fit_model


def _apply_axes_style(ax: plt.Axes) -> None:
    ax.set_facecolor("white")
    for spine in ax.spines.values():
        spine.set_edgecolor("black")
        spine.set_linewidth(0.8)
    ax.tick_params(direction="in", top=True, right=True, colors="black", labelsize=9)
    ax.grid(True, linestyle=":", alpha=0.25, color="#888888", linewidth=0.6)


def load_results(csv_path: Path = CSV_PATH) -> pd.DataFrame:
    if not csv_path.is_file():
        raise FileNotFoundError(
            f"No se encontró {csv_path}. Ejecuta antes:\n"
            "  integration_tests.exe "
            "--run_test=Escenario_Rendimiento_Estadistico/Amdahl_Con_Incertidumbre"
        )
    df = pd.read_csv(csv_path)
    required = {"Hilos", "Tiempo_Medio_ms", "Desviacion_ms"}
    missing = required - set(df.columns)
    if missing:
        raise ValueError(f"Columnas ausentes en CSV: {sorted(missing)}")
    return df.sort_values("Hilos").reset_index(drop=True)


def infer_physical_cores(max_threads: int) -> int:
    return max(1, max_threads // 2)


def compute_speedup(df: pd.DataFrame) -> tuple[np.ndarray, np.ndarray, np.ndarray, float, float]:
    hilos = df["Hilos"].to_numpy(dtype=float)
    t_medios = df["Tiempo_Medio_ms"].to_numpy(dtype=float)
    t_std = df["Desviacion_ms"].to_numpy(dtype=float)

    t1_medio = t_medios[0]
    t1_std = t_std[0]

    speedup_medio = t1_medio / t_medios
    speedup_err = speedup_medio * np.sqrt((t1_std / t1_medio) ** 2 + (t_std / t_medios) ** 2)

    return hilos, speedup_medio, speedup_err, t1_medio, t1_std


def fit_amdahl(
    hilos: np.ndarray,
    speedup_medio: np.ndarray,
    speedup_err: np.ndarray,
    physical_cores: int,
    c: float = HT_EFFICIENCY,
) -> tuple[tuple[float, float], tuple[float, float]]:
    model = make_fit_model(physical_cores, c)
    popt, pcov = curve_fit(
        model,
        hilos,
        speedup_medio,
        sigma=speedup_err,
        absolute_sigma=True,
        p0=(0.95, 1e-4),
        bounds=([0.0, 0.0], [1.0, np.inf]),
    )
    p_est, beta_est = popt
    p_err, beta_err = np.sqrt(np.diag(pcov))
    return (p_est, beta_est), (p_err, beta_err)


def plot_amdahl(
    hilos: np.ndarray,
    speedup_medio: np.ndarray,
    speedup_err: np.ndarray,
    params: tuple[float, float],
    param_errs: tuple[float, float],
    physical_cores: int,
    c: float = HT_EFFICIENCY,
    output_stem: str = OUTPUT_STEM,
) -> None:
    p_est, beta_est = params
    p_err, beta_err = param_errs

    k_curve = np.linspace(1.0, float(hilos.max()), 300)
    speedup_ideal = k_curve
    speedup_fit = speedup_amdahl(
        k_curve, p_est, beta_est, physical_cores=physical_cores, c=c
    )

    fig, ax = plt.subplots(figsize=(8, 6))
    fig.patch.set_facecolor("white")

    ax.plot(k_curve, speedup_ideal, color=COLOR_IDEAL, linestyle="--", linewidth=1.5, label="Ideal (lineal)")

    etiqueta_modelo = (
        r"Modelo Amdahl (SMT)"
        "\n"
        rf"$P = {p_est:.3f} \pm {p_err:.3f}$"
        "\n"
        rf"$\beta = {beta_est:.5f} \pm {beta_err:.5f}$"
        "\n"
        rf"$N = {physical_cores}$, $c = {c:.2f}$"
    )
    ax.plot(k_curve, speedup_fit, color=COLOR_FIT, linewidth=2.0, label=etiqueta_modelo)

    ax.errorbar(
        hilos,
        speedup_medio,
        yerr=speedup_err,
        fmt="x",
        color="black",
        markersize=8,
        markeredgewidth=1.3,
        capsize=4,
        capthick=1.3,
        elinewidth=1.3,
        ecolor="black",
        label=r"Medidas $\pm 1\sigma$",
    )

    _apply_axes_style(ax)
    ax.set_xlabel(r"Número de hilos ($k$)", fontsize=10, color="black")
    ax.set_ylabel(r"Speedup $S(k)$", fontsize=10, color="black")
    ax.set_title(
        r"Escalabilidad paralela — \emph{benchmark1} (ajuste Amdahl ponderado)",
        fontsize=11,
        fontweight="bold",
        color="black",
        pad=10,
    )

    ax.set_xticks(hilos)
    ax.set_xlim(0.5, float(hilos.max()) + 0.5)
    y_max = max(
        float(speedup_ideal.max()),
        float(speedup_medio.max() + speedup_err.max()),
        float(speedup_fit.max()),
    )
    ax.set_ylim(0.0, y_max * 0.55 + 1.0)

    ax.legend(
        loc="upper left",
        fontsize=9,
        frameon=True,
        facecolor="white",
        edgecolor="black",
        labelcolor="black",
    )

    fig.tight_layout()

    png_path = SCRIPT_DIR / f"{output_stem}.png"
    pdf_path = SCRIPT_DIR / f"{output_stem}.pdf"
    fig.savefig(png_path, dpi=300, bbox_inches="tight")
    fig.savefig(pdf_path, bbox_inches="tight")
    print(f"Guardado: {png_path}")
    print(f"Guardado: {pdf_path}")
    plt.close(fig)


def print_summary(
    hilos: np.ndarray,
    speedup_medio: np.ndarray,
    params: tuple[float, float],
    param_errs: tuple[float, float],
    physical_cores: int,
    t1_medio: float,
    t1_std: float,
    c: float = HT_EFFICIENCY,
) -> None:
    p_est, beta_est = params
    p_err, beta_err = param_errs
    peak_idx = int(np.argmax(speedup_medio))
    print("\n--- Resumen Amdahl (benchmark1) ---")
    print(f"N (nucleos fisicos) = {physical_cores}, c = {c}")
    print(f"T1 = {t1_medio:.1f} +/- {t1_std:.1f} ms")
    print(f"Speedup max medido: {speedup_medio[peak_idx]:.3f}x ({int(hilos[peak_idx])} hilos)")
    print(f"P = {p_est:.4f} +/- {p_err:.4f}, beta = {beta_est:.6f} +/- {beta_err:.6f}")


def main() -> None:
    df = load_results()
    hilos, speedup_medio, speedup_err, t1_medio, t1_std = compute_speedup(df)
    physical_cores = infer_physical_cores(int(hilos.max()))
    params, param_errs = fit_amdahl(hilos, speedup_medio, speedup_err, physical_cores)
    print_summary(hilos, speedup_medio, params, param_errs, physical_cores, t1_medio, t1_std)
    plot_amdahl(hilos, speedup_medio, speedup_err, params, param_errs, physical_cores)


if __name__ == "__main__":
    main()
