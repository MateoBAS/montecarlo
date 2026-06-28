# Prompt para IA: corregir sección de arquitectura del TFM (LaTeX)

Copia y pega el bloque siguiente en un asistente con acceso al repositorio (o adjunta los archivos indicados).

---

## Prompt

Eres un redactor académico de un TFM de FinTech sobre un motor Monte Carlo en C++.

**Tarea:** Revisa, corrige y completa la sección «Arquitectura del sistema propuesto y flujo de ejecución» en **LaTeX** (español, tono formal de memoria). El borrador original tiene imprecisiones; el código fuente y los diagramas UML son la fuente de verdad.

### Texto original a corregir

```
[Pegar aquí el párrafo original del TFM sobre SimConfig, Asset/GBMAsset, MonteCarloEngine, RiskCalculator, Figura 7, test_acciones.cpp, etc.]
```

### Fuente de verdad (repositorio)

Lee estos archivos antes de escribir:

| Área | Archivos clave |
|------|----------------|
| Configuración | `src/MontecarloEngine/MontecarloEngine.h` (`SimConfig`, `RNGType`, `MatrixStorageLayout`) |
| Motor | `src/MontecarloEngine/MontecarloEngine.cpp` |
| Activos | `src/Asset/Asset.h`, `GBMAsset.h`, `EuropeanOption.h`, `BarrierOption.h`, `CouponBond.h` |
| Cartera | `src/Portfolio/Portfolio.h` |
| Tipos | `src/InterestRate/InterestRateModel.h`, `HullWhiteModel.h` |
| RNG | `src/Random/RandomGenerator.h`, `BrownianBridge.h` |
| Métricas | `src/Metrics/RiskCalculator.h`, `MonteCarloErrorPolicy.h`, `RiskEstimate.h` |
| UML | `graphics/uml/uml_dominio_body.puml`, `graphics/uml/uml_motor_body.puml` |
| Tests | `src/Integration/test_acciones.cpp`, `src/MontecarloEngine/tests/test_montecarloEngine.cpp` |

### Errores conocidos del borrador (DEBES corregirlos)

1. **`SimConfig` NO almacena rutas de ficheros** de mercado; todo va en memoria (`corrMatrix`, `rateModel`, etc.).
2. **`MonteCarloEngine` NO instancia la cartera**; firma: `run(const Portfolio&, const SimConfig&)`.
3. **`RiskCalculator` recibe un `vector<double>` de P&L**, no una «matriz contigua».
4. **Activos:** no solo `GBMAsset`; hay también `EuropeanOption`, `BarrierOption`, `CouponBond`. Hot path: `simulateFinalValue`; cold path: `generatePath`.
5. **Módulos omitidos:** `Portfolio`/`Position`, `Random` (MT, antitéticas, Sobol, `BrownianBridge`, `MathUtils`), `InterestRate` (`HullWhiteModel`, `YieldCurve`), `MonteCarloErrorPolicy`, `RiskEstimate`, errores estándar MC.
6. **«Análisis de riesgo en colas»** es ambiguo; usar «sobre la distribución empírica de P&L (cuantiles y colas)».
7. **Pipeline Sobol:** `BoostSobolGenerator` → N(0,1) vía `MathUtils`; `MonteCarloEngine` aplica `BrownianBridge` por factor; luego Cholesky.
8. **Correlación:** `numFactores = numActivos + (rateModel ? 1 : 0)`.

### Diagramas UML (Figura 7 dividida)

- **Figura 7a / `fig:uml-dominio`:** `graphics/uml/uml_dominio.pdf` — Activos, Cartera, Tipos de interés.
- **Figura 7b / `fig:uml-motor`:** `graphics/uml/uml_motor.pdf` — SimConfig, MonteCarloEngine, Aleatoriedad, Métricas.

El texto debe referenciar **ambas figuras** y explicar qué muestra cada una. No describir un único diagrama monolítico.

### Formato de salida

1. **Un único fragmento LaTeX** listo para `\input{}` (no documento completo).
2. Estructura: `\subsection{...}`, párrafo introductorio (3 fases), `\begin{itemize}` con un `\item` por módulo, párrafo puente a las figuras, dos `\begin{figure}` con `\includegraphics` y `\label{fig:uml-dominio}` / `\label{fig:uml-motor}`, párrafo final sobre flexibilidad y tests.
3. Español con acentos LaTeX (`\'o`, `\~n`, `--` para rayas).
4. Nombres de clases/métodos en `\texttt{}`.
5. Ecuaciones inline donde proceda (Cholesky, factores de correlación).
6. **No inventar** funcionalidades no presentes en el código (p. ej. carga de CSV en `SimConfig`, colas de mensajes, instanciación de cartera por el engine).

### Criterios de calidad

- Coherente con el código y los diagramas actuales.
- Longitud similar o ligeramente mayor que el original (~1,5 páginas).
- Sin redundancia entre ítems del listado y el pie de figura.
- Mencionar `test_acciones.cpp` y `test_montecarloEngine.cpp` como entornos de reconfiguración dinámica.

### Referencia ya validada

Existe un borrador corregido en `docs/arquitectura_sistema.tex`. Puedes usarlo como base, pero vuelve a contrastarlo con el código antes de entregar la versión final.

Entrega **solo el LaTeX**, sin comentarios meta ni markdown envolvente.

---
