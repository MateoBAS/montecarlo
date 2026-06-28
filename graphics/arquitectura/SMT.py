import matplotlib.patches as mpatches
import matplotlib.pyplot as plt

# --- 1. CONFIGURACIÓN DE VISTA (AJÚSTALOS AQUÍ) ---
X_LIM = [-0.5, 11.5]
Y_LIM = [-0.5, 5.5]

fig, ax = plt.subplots(figsize=(12, 6))

# Paleta de colores clásica
COL_HILO_A = "#1f77b4"  # Azul oscuro
COL_HILO_B = "#ff7f0e"  # Naranja
COL_VACIO = "#e0e0e0"  # Gris (Desperdicio de CPU)


# --- 2. FUNCIÓN PARA DIBUJAR EL NÚCLEO DE LA CPU ---
def dibujar_nucleo(ax, x_base, y_base, unidades, titulo):
    """Dibuja un núcleo físico con sus 4 unidades de ejecución internas."""
    # Caja contenedora del Núcleo Físico
    nucleo_box = mpatches.FancyBboxPatch(
        (x_base, y_base),
        3.0,
        4.0,
        facecolor="#f8f9fa",
        edgecolor="#2c3e50",
        lw=2,
        boxstyle="round,pad=0.1,rounding_size=0.1",
    )
    ax.add_patch(nucleo_box)

    # Título del modelo
    ax.text(
        x_base + 1.5,
        y_base + 4.3,
        titulo,
        ha="center",
        va="bottom",
        fontsize=11,
        fontweight="bold",
    )

    # Nombres de las unidades de ejecución internas de una CPU
    nombres_unidades = [
        "ALU 1\n(Enteros)",
        "ALU 2\n(Enteros)",
        "FPU\n(Decimales)",
        "AGU\n(Memoria)",
    ]
    posiciones = [(0.2, 2.1), (1.6, 2.1), (0.2, 0.5), (1.6, 0.5)]

    # Dibujar cada unidad funcional dentro de este núcleo
    for (x_off, y_off), nombre, estado in zip(posiciones, nombres_unidades, unidades):
        if estado == "A":
            col = COL_HILO_A
            txt_col = "white"
            txt_estado = "\n[Ejecutando A]"
        elif estado == "B":
            col = COL_HILO_B
            txt_col = "white"
            txt_estado = "\n[Ejecutando B]"
        else:
            col = COL_VACIO
            txt_col = "#7f8c8d"
            txt_estado = "\n[INACTIVA]"

        # Caja de la unidad de cálculo (ALU/FPU...)
        u_box = mpatches.FancyBboxPatch(
            (x_base + x_off, y_base + y_off),
            1.2,
            1.4,
            facecolor=col,
            edgecolor="black",
            lw=1,
            boxstyle="round,pad=0.02,rounding_size=0.05",
        )
        ax.add_patch(u_box)

        # Texto de la unidad
        ax.text(
            x_base + x_off + 0.6,
            y_base + y_off + 0.7,
            f"{nombre}{txt_estado}",
            color=txt_col,
            ha="center",
            va="center",
            fontsize=8,
            fontweight="bold" if estado != "V" else "normal",
        )


# --- 3. DATOS DE OCUPACIÓN DEL NÚCLEO EN UN INSTANTE DADO ---
# Cada lista representa las 4 unidades funcionales: [ALU1, ALU2, FPU, AGU]
unidades_single = ["A", "V", "V", "V"]  # El hilo A no llena el núcleo. Mucho gris.
unidades_temporal = [
    "B",
    "B",
    "V",
    "V",
]  # Turno de B. Sigue habiendo huecos libres.
unidades_smt = [
    "A",
    "B",
    "A",
    "B",
]  # SMT junta ambos hilos a la vez. ¡Cero desperdicio!


# --- 4. CONFIGURACIÓN DEL LIENZO ---
dibujar_nucleo(ax, 0.5, 0.5, unidades_single, "1. Monohilo\n(Mucha unidad ociosa)")
dibujar_nucleo(
    ax,
    4.5,
    0.5,
    unidades_temporal,
    "2. Multithreading Temporal\n(Intercalado / Huecos libres)",
)
dibujar_nucleo(
    ax,
    8.5,
    0.5,
    unidades_smt,
    "3. SMT / Hyper-Threading\n(Simultáneo / Núcleo lleno)",
)

# Título General
ax.set_title(
    "Distribución Interna de un Núcleo de CPU en un Instante de Tiempo",
    fontsize=14,
    fontweight="bold",
    pad=30,
)

# Limpieza absoluta de ejes para dejarlo tipo esquema de arquitectura limpio
ax.spines["top"].set_visible(False)
ax.spines["right"].set_visible(False)
ax.spines["left"].set_visible(False)
ax.spines["bottom"].set_visible(False)
ax.get_xaxis().set_visible(False)
ax.get_yaxis().set_visible(False)

ax.set_xlim(X_LIM)
ax.set_ylim(Y_LIM)

# Leyendas explicativas
patch_a = mpatches.Patch(color=COL_HILO_A, label="Unidad ocupada por el Hilo A")
patch_b = mpatches.Patch(color=COL_HILO_B, label="Unidad ocupada por el Hilo B")
patch_v = mpatches.Patch(
    color=COL_VACIO, label="Burbuja de CPU (Potencia desperdiciada)"
)
ax.legend(
    handles=[patch_a, patch_b, patch_v],
    loc="lower center",
    bbox_to_anchor=(0.5, -0.05),
    ncol=3,
    frameon=True,
    fontsize=10,
)

plt.tight_layout()
plt.show()