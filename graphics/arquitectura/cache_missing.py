import matplotlib.patches as mpatches
import matplotlib.pyplot as plt

# --- 1. CONFIGURACIÓN DEL LIENZO ---
X_LIM = [0, 20]
Y_LIM = [0, 12]
fig, ax = plt.subplots(figsize=(15, 9))

# Colores de la infografía
COL_MATRIZ = "#2c3e50"
COL_HIT = "#2ecc71"  # Verde para el Caso Bueno
COL_MISS = "#e74c3c"  # Rojo para el Caso Malo
COL_TEXTO = "#34495e"


# --- 2. PARTE SUPERIOR: EL PRODUCTO MATRICIAL GLOBAL ---
def dibujar_producto_global(ax):
    ax.text(
        10,
        11.2,
        "OPERACIÓN: Producto Matricial C = A × B",
        fontsize=14,
        fontweight="bold",
        ha="center",
    )
    ax.text(
        10,
        10.7,
        r"$C[i][j] = \sum_{k} (A[i][k] \times B[k][j])$  →  Calculando la celda C[0][0]",
        fontsize=12,
        style="italic",
        ha="center",
        color=COL_TEXTO,
    )

    # Dibujar la ecuación visual simplificada: [Fila A] x [Columna B]
    # Fila de A
    for k in range(3):
        rect = mpatches.Rectangle(
            (5 + k * 0.8, 9.5), 0.7, 0.5, facecolor="#3498db", edgecolor="black"
        )
        ax.add_patch(rect)
        ax.text(
            5.35 + k * 0.8,
            9.75,
            f"A[0][{k}]",
            color="white",
            fontsize=8,
            ha="center",
            va="center",
        )

    ax.text(7.8, 9.7, "×", fontsize=16, fontweight="bold", ha="center")

    # Columna de B
    for k in range(3):
        rect = mpatches.Rectangle(
            (8.5, 10.1 - k * 0.6),
            0.7,
            0.5,
            facecolor="#ff9f43",
            edgecolor="black",
        )
        ax.add_patch(rect)
        ax.text(
            8.85,
            10.35 - k * 0.6,
            f"B[{k}][0]",
            color="white",
            fontsize=8,
            ha="center",
            va="center",
        )

    ax.text(10.0, 9.7, "=", fontsize=16, fontweight="bold", ha="center")

    # Celda Resultante C
    rect_c = mpatches.Rectangle(
        (10.8, 9.5), 0.7, 0.5, facecolor="#9b59b6", edgecolor="black"
    )
    ax.add_patch(rect_c)
    ax.text(
        11.15,
        9.75,
        "C[0][0]",
        color="white",
        fontsize=8,
        ha="center",
        va="center",
    )


# --- 3. PARTE INFERIOR IZQUIERDA: CASO 2 (EL MALO - RECORRIDO POR COLUMNAS) ---
def dibujar_caso_malo(ax):
    x_off = 1
    ax.text(
        x_off,
        7.5,
        "CASO 2: Recorrido por Columnas (B[k][j])",
        fontsize=12,
        fontweight="bold",
        color=COL_MISS,
    )
    ax.text(
        x_off,
        7.1,
        "El índice 'k' salta entre filas. Los datos están separados en la RAM.",
        fontsize=9,
        color=COL_TEXTO,
    )

    pasadas = [
        ("k=0", "Pide B[0][0]", "MISS (Obligado)", COL_MISS),
        ("k=1", "Pide B[1][0]", "MISS (Salto de fila)", COL_MISS),
        ("k=2", "Pide B[2][0]", "MISS (Salto de fila)", COL_MISS),
        ]

    for idx, (k_ver, pide, resultado, col) in enumerate(pasadas):
        y_pos = 5.5 - idx * 1.8

        # Línea de tiempo / Llamada
        ax.text(
            x_off,
            y_pos + 0.5,
            f"Iteración {k_ver} → {pide}",
            fontsize=9.5,
            fontweight="bold",
        )

        # Contenedor de la Caché L1 en ese instante
        cache_box = mpatches.Rectangle(
            (x_off, y_pos),
            7.5,
            0.4,
            facecolor="none",
            edgecolor=col,
            lw=1.5,
            linestyle="--",
        )
        ax.add_patch(cache_box)

        # Contenido de la caché L1 (Trae datos contiguos de la fila que NO se van a usar ahora)
        for i in range(4):
            rect = mpatches.Rectangle(
                (x_off + 0.1 + i * 1.4, y_pos + 0.05),
                1.3,
                0.3,
                facecolor="#f2f2f2" if i > 0 else col,
                edgecolor="gray",
                alpha=0.7,
            )
            ax.add_patch(rect)
            txt_celda = (
                f"B[{idx}][{i}]" if i == 0 else f"B[{idx}][{i}]\n(Inútil)"
            )
            ax.text(
                x_off + 0.75 + i * 1.4,
                y_pos + 0.2,
                txt_celda,
                fontsize=7,
                ha="center",
                va="center",
                color="black" if i == 0 else "gray",
            )

        # Cartel de resultado
        ax.text(
            x_off + 6.0,
            y_pos + 0.5,
            f"Result: {resultado}",
            color=col,
            fontsize=9,
            fontweight="bold",
        )


# --- 4. PARTE INFERIOR DERECHA: CASO 1 (EL BUENO - RECORRIDO POR FILAS) ---
def dibujar_caso_bueno(ax):
    x_off = 11.5
    ax.text(
        x_off,
        7.5,
        "CASO 1: Recorrido por Filas (A[i][k])",
        fontsize=12,
        fontweight="bold",
        color=COL_HIT,
    )
    ax.text(
        x_off,
        7.1,
        "El índice 'k' avanza por la misma fila. Los datos están juntos en la RAM.",
        fontsize=9,
        color=COL_TEXTO,
    )

    pasadas = [
        ("k=0", "Pide A[0][0]", "MISS Inicial (Carga bloque)", COL_MISS),
        ("k=1", "Pide A[0][1]", "CACHE HIT (¡Ya estaba!)", COL_HIT),
        ("k=2", "Pide A[0][2]", "CACHE HIT (¡Ya estaba!)", COL_HIT),
    ]

    for idx, (k_ver, pide, resultado, col) in enumerate(pasadas):
        y_pos = 5.5 - idx * 1.8

        # Línea de tiempo / Llamada
        ax.text(
            x_off,
            y_pos + 0.5,
            f"Iteración {k_ver} → {pide}",
            fontsize=9.5,
            fontweight="bold",
        )

        # Contenedor de la Caché L1 (Se mantiene constante el bloque en los hits)
        cache_box = mpatches.Rectangle(
            (x_off, y_pos),
            7.5,
            0.4,
            facecolor="none",
            edgecolor=COL_HIT if idx > 0 else COL_MISS,
            lw=1.5,
        )
        ax.add_patch(cache_box)

        # Contenido de la caché L1 (Se precargó la fila entera desde k=0)
        for i in range(4):
            # Resaltar en verde el elemento que estamos consumiendo en esta iteración exacta
            es_el_solicitado = i == idx
            face_col = (
                COL_HIT
                if es_el_solicitado and idx > 0
                else (COL_MISS if es_el_solicitado else "#e8f8f5")
            )
            txt_col = "white" if es_el_solicitado else "black"

            rect = mpatches.Rectangle(
                (x_off + 0.1 + i * 1.4, y_pos + 0.05),
                1.3,
                0.3,
                facecolor=face_col,
                edgecolor="gray",
            )
            ax.add_patch(rect)
            ax.text(
                x_off + 0.75 + i * 1.4,
                y_pos + 0.2,
                f"A[0][{i}]",
                fontsize=7.5,
                ha="center",
                va="center",
                color=txt_col,
                fontweight="bold" if es_el_solicitado else "normal",
            )

        # Cartel de resultado
        ax.text(
            x_off + 5.8,
            y_pos + 0.5,
            f"Result: {resultado}",
            color=col,
            fontsize=9,
            fontweight="bold",
        )


# --- 5. MONTAJE Y LIMPIEZA DEL LIENZO ---
dibujar_producto_global(ax)
dibujar_caso_malo(ax)
dibujar_caso_bueno(ax)

# Línea vertical divisoria entre el caso malo (izquierda) y bueno (derecha)
ax.axvline(10.2, ymin=0.05, ymax=0.7, color="#bdc3c7", linestyle="--", alpha=0.6)

# Configuración de estética tipo póster científico
ax.set_xlim(X_LIM)
ax.set_ylim(Y_LIM)
ax.spines["top"].set_visible(False)
ax.spines["right"].set_visible(False)
ax.spines["left"].set_visible(False)
ax.spines["bottom"].set_visible(False)
ax.get_xaxis().set_visible(False)
ax.get_yaxis().set_visible(False)

plt.tight_layout()
plt.show()