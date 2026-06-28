import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import qmc

# --- 1. CONFIGURACIÓN DE LA SIMULACIÓN ---
# Sobol funciona de manera óptima con potencias de 2 (ej: 256, 512, 1024, 2048...)
n_puntos = 1024

# --- 2. PARÁMETROS DE CONTROL DE EJES (AJÚSTALOS AQUÍ) ---
# El espacio nativo de Sobol va de 0 a 1 en ambos ejes
X_LIM_MAPA = [-0.05, 1.05]
Y_LIM_MAPA = [-0.05, 1.05]


# --- 3. GENERACIÓN DE LA SECUENCIA DE SOBOL ---
# Inicializamos el generador para 2 dimensiones
sampler = qmc.Sobol(d=2, scramble=True, seed=42)

# Generamos los puntos en el espacio [0, 1)
puntos_sobol = sampler.random(n_puntos)

# Separamos las coordenadas X e Y
x = puntos_sobol[:, 0]
y = puntos_sobol[:, 1]


# --- 4. CREACIÓN DE LA GRÁFICA (MAPA DE PUNTOS) ---
fig, ax = plt.subplots(figsize=(8, 8))

# Dibujamos los puntos con los colores clásicos que pediste antes
ax.scatter(
    x,
    y,
    color="navy",
    s=12,
    alpha=0.7,
    edgecolor="skyblue",
    linewidth=0.5,
    label=f"Puntos Sobol ({n_puntos})",
)

# Configuración del diseño
ax.set_title(
    f"Distribución Espacial de la Secuencia de Sobol",
    fontsize=14,
    fontweight="bold",
)
ax.set_xlabel("Eje X", fontsize=12)
ax.set_ylabel("Eje Y", fontsize=12)

# Aplicamos los límites configurados
ax.set_xlim(X_LIM_MAPA)
ax.set_ylim(Y_LIM_MAPA)

# Forzamos proporción 1:1 para que el mapa no se deforme
ax.set_aspect("equal")

# Rejilla fina de fondo para apreciar la regularidad del muestreo
ax.grid(True, linestyle="--", alpha=0.5, color="#bdc3c7")
ax.legend(loc="upper right", frameon=True)

plt.tight_layout()
plt.show()