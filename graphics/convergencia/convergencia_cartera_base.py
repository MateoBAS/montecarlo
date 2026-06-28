"""
Convergencia VaR/ES — cartera base multi-activo + Hull-White.

Lee `convergencia_cartera_base.csv` y produce gráficas con bandas de error MC (± SE).

Uso:
    python convergencia_cartera_base.py
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
CSV_PATH = SCRIPT_DIR / "convergencia_cartera_base.csv"

# Color solo para las líneas de datos
LINE_COLORS = {
    "MersenneTwister": "#8B4513",
    "Antithetic": "#B22222",
    "Sobol": "#E76F51",
}

RNG_STYLE: dict[str, dict[str, str]] = {
    "MersenneTwister": {"label": "Mersenne Twister", "marker": "o"},
    "Antithetic": {"label": "Antitéticas", "marker": "s"},
    "Sobol": {"label": "Sobol", "marker": "^"},
}

METRICS = (
    ("VaR95", "VaR95_SE", r"VaR 95 %"),
    ("VaR99", "VaR99_SE", r"VaR 99 %"),
    ("ES95", "ES95_SE", r"ES 95 %"),
    ("ES975", "ES975_SE", r"ES 97.5 %"),
)

FOCUS_METRICS = (
    ("VaR99", "VaR99_SE", r"VaR 99 %"),
    ("ES975", "ES975_SE", r"ES 97.5 %"),
)

SE_METRICS = tuple((val_col, se_col, title) for val_col, se_col, title in METRICS)


def _relative_se(se: pd.Series, value: pd.Series) -> pd.Series:
    return se / value.abs()


def load_results(csv_path: Path = CSV_PATH) -> pd.DataFrame:
    if not csv_path.is_file():
        raise FileNotFoundError(
            f"No se encontró {csv_path}. Ejecuta antes:\n"
            "  integration_tests.exe "
            "--run_test=Escenario_Cartera_Base/Convergencia_Generadores_Con_Errores"
        )
    df = pd.read_csv(csv_path)
    df = df.loc[:, ~df.columns.astype(str).str.match(r"Unnamed")]
    df["N"] = pd.to_numeric(df["N"])
    required = {"N", "RNG", *(m[0] for m in METRICS), *(m[1] for m in METRICS)}
    missing = required - set(df.columns)
    if missing:
        raise ValueError(f"Columnas ausentes en CSV: {sorted(missing)}")
    return df


def _loss_magnitude(series: pd.Series) -> pd.Series:
    return -series


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


def _configure_log2_xaxis(ax: plt.Axes, n_values: np.ndarray) -> None:
    ax.set_xscale("log", base=2)
    ax.xaxis.set_major_formatter(
        ticker.FuncFormatter(lambda x, _pos: f"$2^{{{int(np.log2(x))}}}$")
    )
    n_min, n_max = n_values.min(), n_values.max()
    ticks = 2 ** np.arange(
        int(np.floor(np.log2(n_min))),
        int(np.ceil(np.log2(n_max))) + 1,
    )
    ax.set_xticks(ticks)
    ax.set_xlim(n_min * 0.92, n_max * 1.08)


def _configure_log_log_axes(
    ax: plt.Axes,
    n_values: np.ndarray,
    *,
    relative: bool = False,
) -> None:
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.xaxis.set_major_formatter(
        ticker.FuncFormatter(lambda x, _pos: f"$2^{{{int(np.log2(x))}}}$")
    )
    if relative:
        ax.yaxis.set_major_formatter(
            ticker.FuncFormatter(lambda y, _pos: f"{y * 100:.2g}%")
        )
    n_min, n_max = n_values.min(), n_values.max()
    ticks = 2 ** np.arange(
        int(np.floor(np.log2(n_min))),
        int(np.ceil(np.log2(n_max))) + 1,
    )
    ax.set_xticks(ticks)
    ax.set_xlim(n_min * 0.92, n_max * 1.08)


def _power_law_fit(n: np.ndarray, se: np.ndarray) -> tuple[float, float, float]:
    mask = se > 0
    k = int(mask.sum())
    if k < 2:
        return float("nan"), float("nan"), float("nan")

    log_n = np.log(n[mask])
    log_se = np.log(se[mask])
    alpha, log_c = np.polyfit(log_n, log_se, 1)

    dof = k - 2
    if dof <= 0:
        alpha_se = float("nan")
    else:
        residuals = log_se - (alpha * log_n + log_c)
        mse = float(np.sum(residuals**2) / dof)
        ss_x = float(np.sum((log_n - np.mean(log_n)) ** 2))
        alpha_se = float(np.sqrt(mse / ss_x)) if ss_x > 0 else float("nan")

    return float(np.exp(log_c)), float(alpha), alpha_se


def _format_alpha_label(alpha: float, alpha_se: float) -> str:
    if np.isfinite(alpha_se):
        return rf"$\alpha={alpha:.2f}\pm{alpha_se:.2f}$"
    return rf"$\alpha={alpha:.2f}$"


def _plot_se_rng_series(
    ax: plt.Axes,
    sub: pd.DataFrame,
    rng: str,
    *,
    show_fit: bool = True,
) -> plt.Line2D:
    style = RNG_STYLE[rng]
    color = LINE_COLORS[rng]
    x = sub["N"].to_numpy()
    y = sub["se"].to_numpy()

    c_fit, alpha, alpha_se = _power_law_fit(x, y)
    if show_fit and np.isfinite(alpha):
        x_line = np.array([x.min(), x.max()], dtype=float)
        y_line = c_fit * x_line**alpha
        ax.plot(
            x_line,
            y_line,
            color=color,
            linestyle="--",
            linewidth=0.9,
            alpha=0.85,
            zorder=2,
        )

    label = f"{style['label']} ({_format_alpha_label(alpha, alpha_se)})"
    (line,) = ax.plot(
        x,
        y,
        label=label,
        color=color,
        marker=style["marker"],
        linestyle="none",
        markersize=6.0,
        markerfacecolor=color,
        markeredgecolor="none",
        zorder=3,
    )
    return line


def _plot_half_slope_reference(
    ax: plt.Axes,
    n_values: np.ndarray,
    anchor_n: float,
    anchor_se: float,
) -> plt.Line2D:
    c_ref = anchor_se * np.sqrt(anchor_n)
    n_line = np.array([n_values.min(), n_values.max()], dtype=float)
    y_line = c_ref * n_line ** (-0.5)
    (line,) = ax.plot(
        n_line,
        y_line,
        color="#555555",
        linestyle=":",
        linewidth=1.0,
        label=r"$\mathcal{O}(N^{-1/2})$",
        zorder=1,
    )
    return line


def plot_se_loglog_panel(
    df: pd.DataFrame,
    se_col: str,
    title: str,
    ax: plt.Axes,
    *,
    value_col: str | None = None,
    show_reference: bool = False,
    relative: bool = False,
) -> list[plt.Line2D]:
    n_values = df["N"].unique()
    handles: list[plt.Line2D] = []
    metric_col = value_col if value_col is not None else se_col.removesuffix("_SE")

    for rng in RNG_STYLE:
        sub = df[df["RNG"] == rng].sort_values("N").copy()
        if sub.empty:
            continue
        if relative:
            sub["se"] = _relative_se(sub[se_col], sub[metric_col])
        else:
            sub["se"] = sub[se_col]
        handles.append(_plot_se_rng_series(ax, sub, rng))

    if show_reference:
        mt = df[df["RNG"] == "MersenneTwister"].sort_values("N")
        if not mt.empty:
            mid_idx = len(mt) // 2
            anchor_n = float(mt.iloc[mid_idx]["N"])
            anchor_se = float(mt.iloc[mid_idx][se_col])
            if relative:
                anchor_value = abs(float(mt.iloc[mid_idx][metric_col]))
                anchor_se = anchor_se / anchor_value if anchor_value > 0 else 0.0
            if anchor_se > 0:
                handles.append(
                    _plot_half_slope_reference(ax, n_values, anchor_n, anchor_se)
                )

    _configure_log_log_axes(ax, n_values, relative=relative)
    ax.set_title(title, fontsize=11, fontweight="bold", color="black")
    if relative:
        ax.set_ylabel(r"Error relativo", fontsize=10, color="black")
    else:
        ax.set_ylabel("Error estándar (SE)", fontsize=10, color="black")
    _apply_axes_style(ax)
    return handles


def _plot_rng_series(
    ax: plt.Axes,
    sub: pd.DataFrame,
    rng: str,
) -> plt.Line2D:
    style = RNG_STYLE[rng]
    color = LINE_COLORS[rng]
    x = sub["N"].to_numpy()
    y = _loss_magnitude(sub["value"]).to_numpy()
    yerr = sub["se"].fillna(0.0).to_numpy()

    ax.fill_between(
        x,
        y - yerr,
        y + yerr,
        color=color,
        alpha=0.11,
        linewidth=0,
        zorder=1,
        interpolate=True,
    )

    (line,) = ax.plot(
        x,
        y,
        label=style["label"],
        color=color,
        marker=style["marker"],
        linewidth=1.0,
        markersize=3.5,
        markerfacecolor=color,
        markeredgecolor="none",
        zorder=3,
    )
    return line


def _add_legend(ax: plt.Axes, handles: list[plt.Line2D]) -> None:
    ax.legend(
        handles,
        [h.get_label() for h in handles],
        loc="upper right",
        fontsize=9,
        frameon=True,
        facecolor="white",
        edgecolor="black",
        labelcolor="black",
    )


def plot_convergence_panel(
    df: pd.DataFrame,
    value_col: str,
    se_col: str,
    title: str,
    ax: plt.Axes,
) -> list[plt.Line2D]:
    n_values = df["N"].unique()
    handles: list[plt.Line2D] = []

    for rng in RNG_STYLE:
        sub = df[df["RNG"] == rng].sort_values("N").copy()
        if sub.empty:
            continue
        sub["value"] = sub[value_col]
        sub["se"] = sub[se_col]
        handles.append(_plot_rng_series(ax, sub, rng))

    _configure_log2_xaxis(ax, n_values)
    ax.set_title(title, fontsize=11, fontweight="bold", color="black")
    ax.set_ylabel("Magnitud de la pérdida", fontsize=10, color="black")
    _apply_axes_style(ax)
    return handles


def plot_all_metrics(df: pd.DataFrame, output_stem: str = "convergencia_cartera_base") -> None:
    fig, axes = plt.subplots(2, 2, figsize=(11, 8), sharex=True)
    fig.patch.set_facecolor("white")
    axes_flat = axes.ravel()

    for ax, (value_col, se_col, title) in zip(axes_flat, METRICS):
        handles = plot_convergence_panel(df, value_col, se_col, title, ax)
        _add_legend(ax, handles)

    for ax in axes_flat[2:]:
        ax.set_xlabel("Número de simulaciones ($N$)", fontsize=10, color="black")
    axes_flat[0].set_xlabel("")
    axes_flat[1].set_xlabel("")

    fig.suptitle(
        "Convergencia MC — cartera base (GBM + opciones + bono HW)",
        fontsize=13,
        fontweight="bold",
        color="black",
        y=0.98,
    )

    fig.tight_layout(rect=(0, 0, 1, 0.96))

    png_path = SCRIPT_DIR / f"{output_stem}.png"
    pdf_path = SCRIPT_DIR / f"{output_stem}.pdf"
    fig.savefig(png_path, dpi=300, bbox_inches="tight")
    fig.savefig(pdf_path, bbox_inches="tight")
    print(f"Guardado: {png_path}")
    print(f"Guardado: {pdf_path}")
    plt.close(fig)


def plot_standard_errors_loglog(
    df: pd.DataFrame,
    output_stem: str = "convergencia_cartera_base_errores",
) -> None:
    fig, axes = plt.subplots(2, 2, figsize=(11, 8), sharex=True)
    fig.patch.set_facecolor("white")
    axes_flat = axes.ravel()

    for ax, (value_col, se_col, title) in zip(axes_flat, METRICS):
        handles = plot_se_loglog_panel(
            df,
            se_col,
            title,
            ax,
            value_col=value_col,
            show_reference=(ax is axes_flat[0]),
            relative=True,
        )
        _add_legend(ax, handles)

    for ax in axes_flat[2:]:
        ax.set_xlabel("Número de simulaciones ($N$)", fontsize=10, color="black")
    axes_flat[0].set_xlabel("")
    axes_flat[1].set_xlabel("")

    fig.suptitle(
        "Decaimiento del error relativo MC — escala log-log y ajuste $\\propto N^{\\alpha}$",
        fontsize=13,
        fontweight="bold",
        color="black",
        y=0.98,
    )

    fig.tight_layout(rect=(0, 0, 1, 0.96))

    png_path = SCRIPT_DIR / f"{output_stem}.png"
    pdf_path = SCRIPT_DIR / f"{output_stem}.pdf"
    fig.savefig(png_path, dpi=300, bbox_inches="tight")
    fig.savefig(pdf_path, bbox_inches="tight")
    print(f"Guardado: {png_path}")
    print(f"Guardado: {pdf_path}")
    plt.close(fig)


def plot_se_var99_focus(
    df: pd.DataFrame,
    output_stem: str = "convergencia_cartera_base_errores_var99",
) -> None:
    fig, ax = plt.subplots(figsize=(8, 4.8))
    fig.patch.set_facecolor("white")

    handles = plot_se_loglog_panel(
        df,
        "VaR99_SE",
        r"VaR 99 % — error relativo vs. $N$",
        ax,
        value_col="VaR99",
        show_reference=True,
        relative=True,
    )
    ax.set_xlabel("Número de simulaciones ($N$)", fontsize=10, color="black")
    _add_legend(ax, handles)

    fig.tight_layout()

    png_path = SCRIPT_DIR / f"{output_stem}.png"
    pdf_path = SCRIPT_DIR / f"{output_stem}.pdf"
    fig.savefig(png_path, dpi=300, bbox_inches="tight")
    fig.savefig(pdf_path, bbox_inches="tight")
    print(f"Guardado: {png_path}")
    print(f"Guardado: {pdf_path}")
    plt.close(fig)


def plot_var99_es975_composite(
    df: pd.DataFrame,
    output_stem: str = "convergencia_cartera_base_var99_es975",
) -> None:
    fig, axes = plt.subplots(2, 2, figsize=(11, 9), sharex="col")
    fig.patch.set_facecolor("white")

    for col_idx, (value_col, se_col, title) in enumerate(FOCUS_METRICS):
        conv_ax = axes[0, col_idx]
        se_ax = axes[1, col_idx]

        conv_handles = plot_convergence_panel(df, value_col, se_col, title, conv_ax)
        _add_legend(conv_ax, conv_handles)

        se_handles = plot_se_loglog_panel(
            df,
            se_col,
            f"{title} — error relativo",
            se_ax,
            value_col=value_col,
            show_reference=True,
            relative=True,
        )
        _add_legend(se_ax, se_handles)

        se_ax.set_xlabel("Número de simulaciones ($N$)", fontsize=10, color="black")

    """fig.suptitle(
        "Convergencia MC — VaR 99 % y ES 97.5 % (cartera base)",
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


def plot_var99_focus(df: pd.DataFrame, output_stem: str = "convergencia_cartera_base_var99") -> None:
    fig, ax = plt.subplots(figsize=(8, 4.8))
    fig.patch.set_facecolor("white")

    handles = plot_convergence_panel(
        df, "VaR99", "VaR99_SE", r"VaR 99 % — convergencia por generador", ax
    )
    ax.set_xlabel("Número de simulaciones ($N$)", fontsize=10, color="black")
    _add_legend(ax, handles)

    fig.tight_layout()

    png_path = SCRIPT_DIR / f"{output_stem}.png"
    pdf_path = SCRIPT_DIR / f"{output_stem}.pdf"
    fig.savefig(png_path, dpi=300, bbox_inches="tight")
    fig.savefig(pdf_path, bbox_inches="tight")
    print(f"Guardado: {png_path}")
    print(f"Guardado: {pdf_path}")
    plt.close(fig)


def main() -> None:
    df = load_results()
    plot_all_metrics(df)
    plot_var99_es975_composite(df)
    plot_var99_focus(df)
    plot_standard_errors_loglog(df)
    plot_se_var99_focus(df)


if __name__ == "__main__":
    main()
