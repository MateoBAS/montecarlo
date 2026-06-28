import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
from scipy.stats import norm

SCRIPT_DIR = Path(__file__).resolve().parent

# --- 1. CONFIGURACIÓN DE PARÁMETROS ---
np.random.seed(41)
num_simulaciones = 400000
valores_N = [1, 2, 5, 10]

# Parámetros teóricos de un solo dado (Uniforme discreta de 1 a 6)
mu_dado = 3.5
sigma_dado = np.sqrt((6**2 - 1) / 12)  # ~1.7078

# Creamos una cuadrícula de 2 filas (Fila 0: Suma, Fila 1: Media) y 4 columnas
fig, axes = plt.subplots(2, 4, figsize=(18, 10), sharex=False, sharey=False)

# --- 2. CONFIGURACIÓN FIJA DE EJES ---
X_LIM_SUMA = [0, 65]
Y_LIM_SUMA = [0, 0.25]  

X_LIM_MEDIA = [0.5, 6.5]
Y_LIM_MEDIA = [0, 1.0]  

# --- 3. BUCLE DE SIMULACIÓN Y RENDERIZADO ---
for idx, N in enumerate(valores_N):
    # Simular el lanzamiento de N dados
    lanzamientos = np.random.randint(1, 7, size=(num_simulaciones, N))
    
    # Cálculos estadísticos muestrales de las simulaciones
    sumas_muestrales = np.sum(lanzamientos, axis=1)
    medias_muestrales = np.mean(lanzamientos, axis=1)
    
    # Desviaciones estándar reales de la simulación (Empíricas)
    sigma_emp_suma = np.std(sumas_muestrales)
    sigma_emp_media = np.std(medias_muestrales)
    
    # -----------------------------------------------------------------
    # FILA 0: FILA DE LAS SUMAS (La dispersión aumenta con raíz de N)
    # -----------------------------------------------------------------
    ax_suma = axes[0, idx]
    
    # Bins perfectos para las sumas
    bins_suma = np.arange(N - 0.5, (6 * N) + 1.5, 1)
    
    # Histograma de la Suma
    ax_suma.hist(
        sumas_muestrales, 
        bins=bins_suma, 
        density=True, 
        alpha=0.6, 
        color='mistyrose', 
        edgecolor='black',
        linewidth=0.5,
        label=f'Empírica ($N={N}$)'
    )
    
    # Parámetros teóricos de la Suma según el TLC
    mu_teorica_suma = N * mu_dado
    sigma_teorica_suma = sigma_dado * np.sqrt(N)
    
    x_suma = np.linspace(X_LIM_SUMA[0], X_LIM_SUMA[1], 500)
    ax_suma.plot(
        x_suma, 
        norm.pdf(x_suma, mu_teorica_suma, sigma_teorica_suma), 
        color='firebrick', 
        linewidth=1.5,
        label='Teórica TLC'
    )
    
    # TEXTO DE LAS SIGMAS NUMÉRICAS (SUMA)
    texto_sigma_suma = (fr"$\sigma_{{tlc}}$: {sigma_teorica_suma:.4f}" + "\n" +
                        fr"$\sigma_{{emp}}$: {sigma_emp_suma:.4f}")
    
    # CAJAS APILADAS EN LA DERECHA (Para todos los cuadros de la fila superior)
    ax_suma.text(0.95, 0.93, r"$\sigma \propto \sqrt{N}$", transform=ax_suma.transAxes, fontsize=10,
                 fontweight='bold', color='firebrick', verticalalignment='top', horizontalalignment='right',
                 bbox=dict(boxstyle='square,pad=0.3', facecolor='#fff5f5', alpha=0.9, edgecolor='gray'))
    
    ax_suma.text(0.95, 0.83, texto_sigma_suma, transform=ax_suma.transAxes, fontsize=9,
                 verticalalignment='top', horizontalalignment='right',
                 bbox=dict(boxstyle='round,pad=0.3', facecolor='white', alpha=0.8, edgecolor='gray'))
    
    # Ajustes estéticos fijos de la fila superior
    ax_suma.set_title(f'Suma de Dados ($N = {N}$)', fontsize=12, fontweight='bold')
    ax_suma.set_xlabel('Valor de la suma acumulada', fontsize=9)
    ax_suma.set_ylabel('Densidad de Probabilidad', fontsize=9)
    ax_suma.set_xlim(X_LIM_SUMA)
    ax_suma.set_ylim(Y_LIM_SUMA)
    ax_suma.grid(True, linestyle='--', alpha=0.4)
    ax_suma.legend(loc='upper left', fontsize=8.5)

    # -----------------------------------------------------------------
    # FILA 1: FILA DE LAS MEDIAS (La dispersión decrece con raíz de N)
    # -----------------------------------------------------------------
    ax_media = axes[1, idx]
    
    # Reparación de bins discretos para la media
    valores_posibles = np.arange(N, 6 * N + 1) / N
    paso = 1 / N
    bins_media = np.append(valores_posibles - paso / 2, valores_posibles[-1] + paso / 2)
    
    # Histograma de la Media
    ax_media.hist(
        medias_muestrales, 
        bins=bins_media, 
        density=True, 
        alpha=0.6, 
        color='skyblue', 
        edgecolor='black',
        linewidth=0.5,
        label=f'Empírica ($N={N}$)'
    )
    
    # Parámetros teóricos de la Media según el TLC
    mu_teorica_media = mu_dado
    sigma_teorica_media = sigma_dado / np.sqrt(N)
    
    x_media = np.linspace(X_LIM_MEDIA[0], X_LIM_MEDIA[1], 500)
    ax_media.plot(
        x_media, 
        norm.pdf(x_media, mu_teorica_media, sigma_teorica_media), 
        color='navy', 
        linewidth=1.5,
        label='Teórica TLC'
    )
    
    # TEXTO DE LAS SIGMAS NUMÉRICAS (MEDIA)
    texto_sigma_media = (fr"$\sigma_{{tlc}}$: {sigma_teorica_media:.4f}" + "\n" +
                         fr"$\sigma_{{emp}}$: {sigma_emp_media:.4f}")
    
    # CAJAS APILADAS EN LA DERECHA (Para todos los cuadros de la fila inferior)
    ax_media.text(0.95, 0.93, r"$\sigma \propto 1/\sqrt{N}$", transform=ax_media.transAxes, fontsize=10,
                  fontweight='bold', color='navy', verticalalignment='top', horizontalalignment='right',
                  bbox=dict(boxstyle='square,pad=0.3', facecolor='#f0f7ff', alpha=0.9, edgecolor='gray'))
    
    ax_media.text(0.95, 0.83, texto_sigma_media, transform=ax_media.transAxes, fontsize=9,
                  verticalalignment='top', horizontalalignment='right',
                  bbox=dict(boxstyle='round,pad=0.3', facecolor='white', alpha=0.8, edgecolor='gray'))
    
    # Ajustes estéticos fijos de la fila inferior
    ax_media.set_title(f'Media de Dados ($N = {N}$)', fontsize=12, fontweight='bold')
    ax_media.set_xlabel('Valor estimado de la media muestral', fontsize=9)
    ax_media.set_ylabel('Densidad de Probabilidad', fontsize=9)
    ax_media.set_xlim(X_LIM_MEDIA)
    ax_media.set_ylim(Y_LIM_MEDIA)
    ax_media.grid(True, linestyle='--', alpha=0.4)
    ax_media.legend(loc='upper left', fontsize=8.5)

# --- 4. MAQUETACIÓN FINAL Y EXPORTACIÓN ---
plt.tight_layout()
plt.savefig(SCRIPT_DIR / "comparativa_tlc_suma_media.pdf", bbox_inches="tight", dpi=300)
plt.show()