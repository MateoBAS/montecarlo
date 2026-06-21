import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import qmc

# --- 1. CONFIGURACIÓN DE LA SIMULACIÓN ---
# Las secuencias de Sobol funcionan mejor si el número de puntos es una potencia de 2
n_puntos = 512

# --- 2. PARÁMETROS DE CONTROL DE EJES (AJÚSTALOS AQUÍ) ---
# Gráfica Izquierda (El cuadrado de lado 2 va de -1 a 1)
X_LIM_CUADRADO = [-1.1, 1.1]
Y_LIM_CUADRADO = [-1.1, 1.1]

# Gráfica Derecha (Evolución de Pi)
X_LIM_EVOLUCION = [0, n_puntos]
Y_LIM_EVOLUCION = [2.5, 3.8]  # Mismo rango para poder comparar la estabilidad


# --- 3. GENERACIÓN DE PUNTOS CON SOBOL (QUASI-MONTECARLO) ---
# Inicializamos el generador de Sobol para 2 dimensiones (X e Y)
sampler = qmc.Sobol(d=2, scramble=True, seed=42)

# Sobol genera datos estrictamente en el intervalo [0, 1)
puntos_sobol = sampler.random(n_puntos)

# Escalamos y trasladamos los puntos para que queden en el intervalo [-1, 1)
x = puntos_sobol[:, 0] * 2 - 1
y = puntos_sobol[:, 1] * 2 - 1

# --- 4. CÁLCULO DE PI ---
distancia_al_origen = x**2 + y**2
dentro_del_circulo = distancia_al_origen <= 1

puntos_dentro_acumulados = np.cumsum(dentro_del_circulo)
intentos = np.arange(1, n_puntos + 1)
pi_evolucion = 4 * puntos_dentro_acumulados / intentos


# --- 5. CREACIÓN DE LA GRÁFICA ---
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# --- PANEL IZQUIERDO: El Cuadrado y el Círculo (Gama Azul de antes) ---
ax1.scatter(
    x[dentro_del_circulo],
    y[dentro_del_circulo],
    color="skyblue",
    s=5,
    alpha=0.8,
    label="Dentro del círculo",
)
ax1.scatter(
    x[~dentro_del_circulo],
    y[~dentro_del_circulo],
    color="navy",
    s=5,
    alpha=0.5,
    label="Fuera del círculo",
)

# Dibujar el contorno del círculo ideal
dibujo_circulo = plt.Circle(
    (0, 0), 1, color="darkorange", fill=False, lw=2, zorder=5
)
ax1.add_artist(dibujo_circulo)

ax1.set_title(
    f"Muestreo de Sobol ({n_puntos} puntos)", fontsize=13, fontweight="bold"
)
ax1.set_xlabel("Eje X", fontsize=11)
ax1.set_ylabel("Eje Y", fontsize=11)
ax1.set_xlim(X_LIM_CUADRADO)
ax1.set_ylim(Y_LIM_CUADRADO)
ax1.set_aspect("equal")
ax1.grid(True, linestyle="--", alpha=0.5)
ax1.legend(loc="upper right", frameon=True)


# --- PANEL DERECHO: Convergencia de Pi ---
ax2.plot(
    intentos,
    pi_evolucion,
    color="navy",
    lw=1.5,
    label=f"Valor Estimado Sobol: {pi_evolucion[-1]:.4f}",
)

# Línea del valor real de Pi
ax2.axhline(
    np.pi,
    color="darkorange",
    linestyle="--",
    lw=2,
    label=f"Valor Real (π ≈ {np.pi:.4f})",
    zorder=5,
)

ax2.set_title(
    "Convergencia de π (Quasi-Montecarlo Sobol)", fontsize=13, fontweight="bold"
)
ax2.set_xlabel("Número de Intentos (Muestras)", fontsize=11)
ax2.set_ylabel("Valor de π Estimado", fontsize=11)
ax2.set_xlim(X_LIM_EVOLUCION)
ax2.set_ylim(Y_LIM_EVOLUCION)
ax2.grid(True, linestyle="--", alpha=0.5)
ax2.legend(loc="upper right", fontsize=10)

plt.tight_layout()
plt.show()