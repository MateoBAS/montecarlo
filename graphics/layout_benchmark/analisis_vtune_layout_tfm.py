"""
Perfil VTune — distribución CPU ColMajor vs RowMajor por escala de activos.

Uso:
    python analisis_vtune_layout_tfm.py
"""

from __future__ import annotations

from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Patch

SCRIPT_DIR = Path(__file__).resolve().parent
OUTPUT_STEM = "analisis_vtune_layout_tfm"

# VTune: puntos medidos con Layout_Speedup_vs_NumActivos (replicaCounts).
# Cada réplica añade 4 activos (makeCarteraBenchmarkReplicada, kAssetsPerReplica=4).
ASSETS_PER_REPLICA = 4
REPLICA_COUNTS = (64, 512, 2048)

COLOR_MEM = "#473F3E"
COLOR_CALC = "#E6D0C8"
COLOR_MUTED = "#735A52"


def _apply_panel_style(ax: plt.Axes, *, show_ylabel: bool) -> None:
    ax.set_facecolor("white")
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.spines["left"].set_color("#CCCCCC")
    ax.spines["bottom"].set_color("#CCCCCC")
    ax.spines["left"].set_linewidth(0.8)
    ax.spines["bottom"].set_linewidth(0.8)
    ax.tick_params(axis="y", colors=COLOR_MUTED, labelsize=9, length=0)
    ax.tick_params(axis="x", colors=COLOR_MUTED, labelsize=9, length=0)
    ax.yaxis.grid(True, linestyle="-", alpha=0.35, color="#E8E8E8", linewidth=0.8)
    ax.set_axisbelow(True)
    if not show_ylabel:
        ax.set_yticklabels([])


def _stacked_bar(ax: plt.Axes, x: float, mem: float, calc: float, width: float) -> None:
    ax.bar(
        x, mem, width,
        color=COLOR_MEM, edgecolor="white", linewidth=1.2, zorder=3,
    )
    ax.bar(
        x, calc, width, bottom=mem,
        color=COLOR_CALC, edgecolor="white", linewidth=1.2, zorder=3,
    )

    if mem >= 11.0:
        ax.text(
            x, mem / 2, f"{mem:.0f}%",
            ha="center", va="center", fontsize=8.5, color="white", fontweight="medium",
        )
    if calc >= 11.0:
        ax.text(
            x, mem + calc / 2, f"{calc:.0f}%",
            ha="center", va="center", fontsize=8.5, color=COLOR_MUTED,
        )


def plot_vtune_layout() -> None:
    escalas = [
        f"{r * ASSETS_PER_REPLICA} activos"
        for r in REPLICA_COUNTS
    ]

    calculo_puro = [50.1, 75.1, 39.3, 60.8, 24.1, 39.8]
    acceso_memoria = [49.9, 24.9, 60.7, 39.2, 75.9, 60.2]

    fig, axes = plt.subplots(1, 3, figsize=(9.0, 4.0), sharey=True)
    fig.patch.set_facecolor("white")

    bar_w = 0.40
    x_col = 0.0
    x_row = 1.0

    for idx, (ax, titulo) in enumerate(zip(axes, escalas)):
        mem_c, calc_c = acceso_memoria[2 * idx], calculo_puro[2 * idx]
        mem_r, calc_r = acceso_memoria[2 * idx + 1], calculo_puro[2 * idx + 1]

        _stacked_bar(ax, x_col, mem_c, calc_c, bar_w)
        _stacked_bar(ax, x_row, mem_r, calc_r, bar_w)

        ax.set_xticks([x_col, x_row])
        ax.set_xticklabels(["ColMajor", "RowMajor"], fontsize=9)

        ax.set_title(titulo, fontsize=10.5, fontweight="bold", color="#2A2A2A", pad=10)
        ax.set_ylim(0, 100)
        ax.set_xlim(-0.55, 1.55)
        _apply_panel_style(ax, show_ylabel=(idx == 0))

    axes[0].set_ylabel("Tiempo de CPU relativo (%)", fontsize=10, color="#2A2A2A")

    """fig.suptitle(
        "Perfil VTune — ColMajor vs RowMajor",
        fontsize=12,
        fontweight="bold",
        color="#2A2A2A",
        y=1.02,
    )"""

    fig.legend(
        handles=[
            Patch(facecolor=COLOR_MEM, edgecolor="none", label="Acceso y gestión de memoria"),
            Patch(facecolor=COLOR_CALC, edgecolor="none", label="Cálculo (aritmética / RNG)"),
        ],
        loc="lower center",
        ncol=2,
        fontsize=8.5,
        frameon=False,
        bbox_to_anchor=(0.5, -0.02),
    )

    fig.subplots_adjust(wspace=0.12, bottom=0.16, top=0.88)

    png_path = SCRIPT_DIR / f"{OUTPUT_STEM}.png"
    pdf_path = SCRIPT_DIR / f"{OUTPUT_STEM}.pdf"
    fig.savefig(png_path, dpi=300, bbox_inches="tight")
    fig.savefig(pdf_path, bbox_inches="tight")
    print(f"Guardado: {png_path}")
    print(f"Guardado: {pdf_path}")
    plt.close(fig)


def main() -> None:
    plot_vtune_layout()


if __name__ == "__main__":
    main()
