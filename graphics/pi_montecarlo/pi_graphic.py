from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.lines import Line2D
from matplotlib.patches import Circle, Rectangle

SCRIPT_DIR = Path(__file__).resolve().parent

COLOR_MARRON = "#8B4513"
COLOR_ROJO = "#B22222"
COLOR_SALMON = "#E76F51"

np.random.seed(42)
N = 5000
TICKS = [-1, 0, 1]
LIM = (-1.12, 1.12)


def _apply_axes_style(ax: plt.Axes) -> None:
    ax.set_facecolor("white")
    for spine in ax.spines.values():
        spine.set_edgecolor("black")
        spine.set_linewidth(0.8)
    ax.tick_params(direction="in", top=True, right=True, colors="black", labelsize=9)
    ax.set_xticks(TICKS)
    ax.set_yticks(TICKS)
    ax.grid(True, linestyle=":", alpha=0.25, color="#888888", linewidth=0.6)


def _dibujar_cuadrado_circulo(ax: plt.Axes, *, rellenar_circulo: bool = False) -> None:
    if rellenar_circulo:
        ax.add_patch(
            Circle(
                (0, 0),
                1,
                facecolor=COLOR_SALMON,
                edgecolor=COLOR_ROJO,
                linewidth=1.4,
                alpha=0.32,
                zorder=1,
            )
        )
    else:
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


x = np.random.uniform(-1, 1, N)
y = np.random.uniform(-1, 1, N)
dentro = x**2 + y**2 <= 1
N_dentro = int(np.sum(dentro))
proporcion = N_dentro / N

fig, (ax_a, ax_b) = plt.subplots(1, 2, figsize=(9.5, 4.5))
fig.subplots_adjust(wspace=0.22, bottom=0.14)

# --- (a) Geometría y proporción de áreas ---
_dibujar_cuadrado_circulo(ax_a, rellenar_circulo=True)
ax_a.text(0, 0.15, r"$\pi$", ha="center", va="center", fontsize=13, color="black")
ax_a.text(0.72, 0.72, r"$4$", ha="center", va="center", fontsize=12, color="black")

ax_a.set_title(
    r"(a) Cuadrado de área $4$ y círculo de área $\pi$",
    fontsize=11,
    fontweight="bold",
    color="black",
    pad=8,
)
ax_a.set_xlabel(r"$x$", fontsize=10, color="black")
ax_a.set_ylabel(r"$y$", fontsize=10, color="black")
ax_a.set_xlim(*LIM)
ax_a.set_ylim(*LIM)
ax_a.set_aspect("equal")
_apply_axes_style(ax_a)

ax_a.text(
    0.5,
    -0.13,
    r"$P(\mathrm{dentro}) = \dfrac{\pi}{4}$",
    transform=ax_a.transAxes,
    ha="center",
    va="top",
    fontsize=10,
    color="black",
)

# --- (b) Lanzamiento de dardos ---
ax_b.scatter(
    x[dentro],
    y[dentro],
    color=COLOR_SALMON,
    s=3,
    alpha=0.75,
    rasterized=True,
    edgecolors="none",
    zorder=2,
)
ax_b.scatter(
    x[~dentro],
    y[~dentro],
    color=COLOR_MARRON,
    s=3,
    alpha=0.4,
    rasterized=True,
    edgecolors="none",
    zorder=2,
)
_dibujar_cuadrado_circulo(ax_b)

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
ax_b.legend(
    handles=leyenda,
    loc="upper right",
    frameon=True,
    framealpha=0.9,
    edgecolor="black",
    fontsize=9,
)

ax_b.set_title(
    rf"(b) Lanzamiento de $N = {N}$ dardos aleatorios",
    fontsize=11,
    fontweight="bold",
    color="black",
    pad=8,
)
ax_b.set_xlabel(r"$x$", fontsize=10, color="black")
ax_b.set_ylabel(r"$y$", fontsize=10, color="black")
ax_b.set_xlim(*LIM)
ax_b.set_ylim(*LIM)
ax_b.set_aspect("equal")
_apply_axes_style(ax_b)

ax_b.text(
    0.5,
    -0.13,
    rf"$\dfrac{{N_{{\mathrm{{dentro}}}}}}{{N}}"
    rf" = \dfrac{{{N_dentro}}}{{{N}}}"
    rf" \approx {proporcion:.3f}"
    rf" \approx \dfrac{{\pi}}{{4}}$",
    transform=ax_b.transAxes,
    ha="center",
    va="top",
    fontsize=10,
    color="black",
)

png_path = SCRIPT_DIR / "pi_montecarlo.png"
pdf_path = SCRIPT_DIR / "pi_montecarlo.pdf"
fig.savefig(png_path, dpi=300, bbox_inches="tight", facecolor="white")
fig.savefig(pdf_path, bbox_inches="tight", facecolor="white")
plt.show()
