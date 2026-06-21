import matplotlib.pyplot as plt
import numpy as np

# --- 1. CONFIGURACIÓN DE LA SIMULACIÓN ---
np.random.seed(42)
n_puntos = 10000  # Número de puntos para la gráfica de dispersión

# --- 2. PARÁMETROS DE CONTROL DE EJES (AJÚSTALOS AQUÍ) ---
# Gráfica Izquierda (El cuadrado de lado 2 va de -1 a 1)
X_LIM_CUADRADO = [-1.1, 1.1]
Y_LIM_CUADRADO = [-1.1, 1.1]

# Gráfica Derecha (Evolución de Pi)
X_LIM_EVOLUCION = [0, n_puntos]
Y_LIM_EVOLUCION = [2.5, 3.8]  # Centrado alrededor de 3.1416


# --- 3. CÁLCULO DE PI POR MONTECARLO ---
# Generamos coordenadas X e Y aleatorias entre -1 y 1
x = np.random.uniform(-1, 1, n_puntos)
y = np.random.uniform(-1, 1, n_puntos)

# Calculamos la distancia al origen para cada punto (Radio)
distancia_al_origen = x**2 + y**2

# Un punto está dentro del círculo si su distancia al origen es menor o igual a 1
dentro_del_circulo = distancia_al_origen <= 1

# Cálculo evolutivo de Pi paso a paso
puntos_dentro_acumulados = np.cumsum(dentro_del_circulo)
intentos = np.arange(1, n_puntos + 1)
pi_evolucion = 4 * puntos_dentro_acumulados / intentos


# --- 4. CREACIÓN DE LA GRÁFICA ---
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# --- PANEL IZQUIERDO: El Cuadrado y el Círculo ---
# Separamos los puntos para pintarlos de diferente color
ax1.scatter(
    x[dentro_del_circulo],
    y[dentro_del_circulo],
    color="skyblue",
    s=4,
    alpha=0.8,
    label="Dentro del círculo",
)
ax1.scatter(
    x[~dentro_del_circulo],
    y[~dentro_del_circulo],
    color="navy",
    s=4,
    alpha=0.5,
    label="Fuera del círculo",
)

# Dibujar el contorno del círculo ideal para referencia
dibujo_circulo = plt.Circle(
    (0, 0), 1, color="darkorange", fill=False, lw=2, zorder=5
)
ax1.add_artist(dibujo_circulo)

ax1.set_title(
    f"Simulación de Puntos ({n_puntos} puntos)", fontsize=13, fontweight="bold"
)
ax1.set_xlabel("Eje X", fontsize=11)
ax1.set_ylabel("Eje Y", fontsize=11)
ax1.set_xlim(X_LIM_CUADRADO)
ax1.set_ylim(Y_LIM_CUADRADO)
ax1.set_aspect("equal")  # Fuerza a que el círculo sea redondo y no un óvalo
ax1.grid(True, linestyle="--", alpha=0.5)
ax1.legend(loc="upper right", frameon=True)


# --- PANEL DERECHO: Convergencia de Pi ---
# Graficamos cómo cambia el valor estimado con cada nuevo punto
ax2.plot(
    intentos,
    pi_evolucion,
    color="navy",
    lw=1.5,
    label=f"Valor Estimado Final: {pi_evolucion[-1]:.4f}",
)

# Línea del valor real de Pi (3.14159...)
ax2.axhline(
    np.pi,
    color="darkorange",
    linestyle="--",
    lw=2,
    label=f"Valor Real (π ≈ {np.pi:.4f})",
    zorder=5,
)

ax2.set_title("Convergencia del Valor de π", fontsize=13, fontweight="bold")
ax2.set_xlabel("Número de Intentos (Muestras)", fontsize=11)
ax2.set_ylabel("Valor de π Estimado", fontsize=11)
ax2.set_xlim(X_LIM_EVOLUCION)
ax2.set_ylim(Y_LIM_EVOLUCION)
ax2.grid(True, linestyle="--", alpha=0.5)
ax2.legend(loc="upper right", fontsize=10)

plt.tight_layout()
plt.show()