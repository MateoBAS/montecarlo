## 1) Estructura del proyecto

- `CMakeLists.txt`: entrada de compilación del proyecto.
- `cmake-lib`: soporte de configuración CMake.
- `src`: implementación principal (motor Monte Carlo, activos, métricas y tests).
- `evaluacion/memoria`: material de memoria, figuras y explicaciones de resultados.
- `scrapping`: scripts auxiliares de recopilación/procesado de información.

## 2) Compilar (Windows / PowerShell)

Desde la **raíz del repositorio**:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

Si el entorno usa Ninja:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## 3) Ejecutar tests

Tras compilar:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Con Ninja normalmente basta:

```powershell
ctest --test-dir build --output-on-failure
```
