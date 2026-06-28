import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent

# --- 1. CONFIGURACIÓN DE LA SIMULACIÓN ---
np.random.seed(41)
S0 = 100  # Valor inicial
T = 1.0  # Tiempo total (1 año)
N = 252  # Número de pasos de tiempo (días)
dt = T / N
mu = 0.05  # Rendimiento esperado
sigma = 0.2  # Volatilidad
n_simulaciones = 500

# --- 2. PARÁMETROS DE CONTROL DE EJES ---
Y_LIM_MIN = 50
Y_LIM_MAX = 200

X_LIM_CAMINOS = [0, N]  # Rango de pasos (Eje X gráfica izquierda)
X_LIM_DIST = [0, 0.03]  # Rango de densidad (Eje X gráfica derecha)


# --- 3. GENERACIÓN DE CAMINOS DE MONTECARLO ---
caminos = np.zeros((N + 1, n_simulaciones))
caminos[0] = S0

for t in range(1, N + 1):
    Z = np.random.standard_normal(n_simulaciones)
    caminos[t] = caminos[t - 1] * np.exp(
        (mu - 0.5 * sigma**2) * dt + sigma * np.sqrt(dt) * Z
    )

media_por_paso = np.mean(caminos, axis=1)
resultados_finales = caminos[-1]


# --- 4. CREACIÓN DE LA GRÁFICA ---
fig, (ax1, ax2) = plt.subplots(
    1, 2, figsize=(14, 6), sharey=True, gridspec_kw={"width_ratios": [3, 1]}
)

# Gráfica 1: Caminos de Montecarlo (Gama Marrón/Granate con transparencia)
ax1.plot(caminos[:, :n_simulaciones], color="brown", lw=1, alpha=0.25)

# Configuración y límites del panel izquierdo (Notación alineada al texto)
ax1.set_title(
    f"Simulación de Montecarlo ({n_simulaciones} escenarios)",
    fontsize=14,
    fontweight="bold",
)
ax1.set_xlabel("Pasos de tiempo ($t$)", fontsize=12)
ax1.set_ylabel("Valor del activo ($S_t$)", fontsize=12)  # Mismo símbolo que en el texto
ax1.set_xlim(X_LIM_CAMINOS)
ax1.set_ylim(Y_LIM_MIN, Y_LIM_MAX)
ax1.grid(True, linestyle="--", alpha=0.5)

# Gráfica 2: Distribución Final (Histograma Mistyrose)
ax2.hist(
    resultados_finales,
    bins=40,
    orientation="horizontal",
    color="mistyrose",
    edgecolor="black",
    alpha=0.7,
    density=True,
)

# Configuración y límites del panel derecho (Cierre de LaTeX corregido)
ax2.set_title(r"Distribución Final $p(S_T)$", fontsize=14, fontweight="bold")
ax2.set_xlabel(
    r"Densidad de probabilidad $p(S_T)$", fontsize=12
)  # Corregido el $ faltante
ax2.set_xlim(X_LIM_DIST)
ax2.grid(True, linestyle="--", alpha=0.5)

# Ajuste automático del lienzo y guardado en formato vectorial para LaTeX
plt.tight_layout()
plt.savefig(SCRIPT_DIR / "simulacion_montecarlo2.pdf", bbox_inches="tight")
plt.show()