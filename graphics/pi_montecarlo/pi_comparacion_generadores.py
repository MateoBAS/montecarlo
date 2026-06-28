from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.lines import Line2D
from matplotlib.patches import Circle, Rectangle
from scipy.stats import qmc

SCRIPT_DIR = Path(__file__).resolve().parent

COLOR_MARRON = "#8B4513"
COLOR_ROJO = "#B22222"
COLOR_SALMON = "#E76F51"

N = 2048  # Potencia de 2: requisito de equilibrio de la secuencia Sobol
TICKS = [-1, 0, 1]
LIM = (-1.12, 1.12)
SEED = 42


def _apply_axes_style(ax: plt.Axes) -> None:
    ax.set_facecolor("white")
    for spine in ax.spines.values():
        spine.set_edgecolor("black")
        spine.set_linewidth(0.8)
    ax.tick_params(direction="in", top=True, right=True, colors="black", labelsize=9)
    ax.set_xticks(TICKS)
    ax.set_yticks(TICKS)
    ax.grid(True, linestyle=":", alpha=0.25, color="#888888", linewidth=0.6)


def _dibujar_cuadrado_circulo(ax: plt.Axes) -> None:
    ax.add_patch(
        Circle(
            (0, 0),
            1,
            fill=False,
            edgecolor=COLOR_ROJO,
            linewidth=1.4,
            zorder=4,
        )
    )
    ax.add_patch(
        Rectangle(
            (-1, -1),
            2,
            2,
            fill=False,
            edgecolor="black",
            linewidth=1.4,
            zorder=5,
        )
    )


def _generar_aleatorio(n: int) -> tuple[np.ndarray, np.ndarray]:
    rng = np.random.default_rng(SEED)
    x = rng.uniform(-1, 1, n)
    y = rng.uniform(-1, 1, n)
    return x, y


def _generar_sobol(n: int) -> tuple[np.ndarray, np.ndarray]:
    sampler = qmc.Sobol(d=2, scramble=True, seed=SEED)
    puntos = sampler.random(n)
    x = puntos[:, 0] * 2 - 1
    y = puntos[:, 1] * 2 - 1
    return x, y


def _dibujar_panel(
    ax: plt.Axes,
    x: np.ndarray,
    y: np.ndarray,
    *,
    titulo: str,
) -> None:
    dentro = x**2 + y**2 <= 1
    n_dentro = int(np.sum(dentro))
    pi_hat = 4 * n_dentro / len(x)

    ax.scatter(
        x[dentro],
        y[dentro],
        color=COLOR_SALMON,
        s=3,
        alpha=0.75,
        rasterized=True,
        edgecolors="none",
        zorder=2,
    )
    ax.scatter(
        x[~dentro],
        y[~dentro],
        color=COLOR_MARRON,
        s=3,
        alpha=0.4,
        rasterized=True,
        edgecolors="none",
        zorder=2,
    )
    _dibujar_cuadrado_circulo(ax)

    leyenda = [
        Line2D(
            [0],
            [0],
            marker="o",
            color="w",
            markerfacecolor=COLOR_SALMON,
            markeredgecolor=COLOR_SALMON,
            markersize=6,
            label="Dentro",
        ),
        Line2D(
            [0],
            [0],
            marker="o",
            color="w",
            markerfacecolor=COLOR_MARRON,
            markeredgecolor=COLOR_MARRON,
            markersize=6,
            label="Fuera",
        ),
    ]
    ax.legend(
        handles=leyenda,
        loc="upper right",
        frameon=True,
        framealpha=0.9,
        edgecolor="black",
        fontsize=9,
    )

    ax.set_title(titulo, fontsize=11, fontweight="bold", color="black", pad=8)
    ax.set_xlabel(r"$x$", fontsize=10, color="black")
    ax.set_ylabel(r"$y$", fontsize=10, color="black")
    ax.set_xlim(*LIM)
    ax.set_ylim(*LIM)
    ax.set_aspect("equal")
    _apply_axes_style(ax)

    ax.text(
        0.5,
        -0.13,
        rf"$\hat{{\pi}} = 4\,\dfrac{{N_{{\mathrm{{dentro}}}}}}{{N}}"
        rf" = 4 \cdot \dfrac{{{n_dentro}}}{{{len(x)}}}"
        rf" \approx {pi_hat:.3f}$",
        transform=ax.transAxes,
        ha="center",
        va="top",
        fontsize=10,
        color="black",
    )


x_mc, y_mc = _generar_aleatorio(N)
x_sobol, y_sobol = _generar_sobol(N)

fig, (ax_a, ax_b) = plt.subplots(1, 2, figsize=(9.5, 4.5))
fig.subplots_adjust(wspace=0.22, bottom=0.14)

_dibujar_panel(
    ax_a,
    x_mc,
    y_mc,
    titulo=rf"(a) Monte Carlo: $N = {N}$ dardos aleatorios",
)
_dibujar_panel(
    ax_b,
    x_sobol,
    y_sobol,
    titulo=rf"(b) Quasi-Monte Carlo: $N = {N}$ dardos Sobol",
)

png_path = SCRIPT_DIR / "pi_comparacion_generadores.png"
pdf_path = SCRIPT_DIR / "pi_comparacion_generadores.pdf"
fig.savefig(png_path, dpi=300, bbox_inches="tight", facecolor="white")
fig.savefig(pdf_path, bbox_inches="tight", facecolor="white")
plt.show()
