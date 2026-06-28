import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
from scipy.stats import norm

SCRIPT_DIR = Path(__file__).resolve().parent

# Nivel de confianza
alpha = 0.95

# Eje horizontal (P&L)
x = np.linspace(-6, 4, 2000)

# Distribución principal
w1 = 0.92
mu1 = 0
sigma1 = 0.7

# Componente de cola pesada (el "bulto")
w2 = 0.04
mu2 = -3.5
sigma2 = 0.3

# Densidad de mezcla
pdf = (
    w1 * norm.pdf(x, mu1, sigma1)
    + w2 * norm.pdf(x, mu2, sigma2)
)

# CDF numérica
dx = x[1] - x[0]
cdf = np.cumsum(pdf) * dx
cdf /= cdf[-1]

# VaR (percentil 5%)
var = np.interp(1 - alpha, cdf, x)

# ES numérico
mask = x <= var
es = np.sum(x[mask] * pdf[mask]) * dx / np.sum(pdf[mask] * dx)

# Figura
plt.figure(figsize=(10, 6))

# Distribución
plt.plot(
    x,
    pdf,
    lw=2,
    color='brown',
    label='Distribución de P&L'
)

# Región de cola
plt.fill_between(
    x[mask],
    pdf[mask],
    color='lightcoral',
    alpha=0.8,
    label='Peores escenarios (5%)'
)

# VaR
plt.axvline(
    var,
    color='brown',
    linestyle='--',
    linewidth=2,
    label=f'VaR {int(alpha*100)}%'
)

# ES
plt.axvline(
    es,
    color='brown',
    linestyle=':',
    linewidth=2,
    label=f'ES {int(alpha*100)}%'
)

# Destacar el bulto de cola
plt.annotate(
    'Evento extremo\n(cola pesada)',
    xy=(mu2, np.interp(mu2, x, pdf)),
    xytext=(-5.5, 0.08),
    arrowprops=dict(arrowstyle='->')
)

plt.xlabel('Pérdidas y ganancias (P&L)')
plt.ylabel('Densidad')
plt.title('VaR y Expected Shortfall en una distribución con cola pesada')
plt.legend()
plt.tight_layout()

plt.savefig(SCRIPT_DIR / 'var_es_heavy_tail.pdf', dpi=300, bbox_inches='tight')
plt.show()