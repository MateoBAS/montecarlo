import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

# Lista de tareas realistas adaptadas al TFM (Código, (Semana de inicio, Duración en semanas), Color)
tasks_raw = [
    # Fase 1: Investigación Inicial, Fundamentos y Prototipado
    ("T1.1 Búsqueda bibliográfica y estado del arte", (0.0, 1.8), "#264653"),
    ("T1.1 Formulación matemática (Sobol, Puentes, Var/ES)", (0.5, 1.5), "#264653"),
    ("T1.2 Reunión inicial de alcance con el tutor", (1, 0.2), "#264653"),
    ("T1.3 Script en Python y simulación de trayectorias", (1.0, 1.2), "#264653"),
    ("T1.3 Generación y exportación de dataset de control", (1.6, 0.6), "#264653"),

    # Fase 2: Diseño de Arquitectura Base y Entorno
    ("T2.1 Diseño estructural e implementacion de clases core", (2.0, 1.8), "#2a9d8f"),
    ("T2.2 Configuración del entorno base con CMake", (3.8, 0.8), "#2a9d8f"),
    ("T2.2 Inclusión Boost para esquema de tests y Eigen para álgebra", (4.2, 0.8), "#2a9d8f"),

    # Fase 3: Extensión de Activos y Simulación Avanzada
    ("T3.1 Modelado y valoración de opciones europeas y barrera", (5, 2.2), "#e9c46a"),
    ("T3.1 Antitéticas, Sobol y Puentes Brownianos", (5.5, 2.0), "#e9c46a"),
    ("T3.2 Integración de la dinámica de tipos (Hull-White)", (6.5, 1.8), "#e9c46a"),
    ("T3.2 Extensión del simulador para la valoración de bonos", (7.2, 1.3), "#e9c46a"),

    # Fase 4: Calibración, Perfilado y Optimización

    ("T4.1 Benchmarking de CPU y eficiencia de cores físicos", (8.2, 0.8), "#f4a261"),
    ("T4.2 Pruebas de rendimiento y análisis de cache missing", (8.8, 2.2), "#f4a261"),
    ("T4.2 Generalización de la Ley de Amdahl y optimización", (8.8, 2.2), "#f4a261"),

    # Fase 5: Documentación, Redacción de la Memoria y Cierre
    ("T5.1 Redacción del marco teórico y arquitectura C++", (1.5, 5.0), "#e76f51"),
    ("T5.1 Redacción de optimizaciones, resultados y conclusiones", (6.5, 4.5), "#e76f51"),
    ("T5.2 Tutorías periódicas de revisión con el tutor", (2.0, 9.0), "#e76f51"),
    ("T5.2 Preparación del material audiovisual para la defensa", (11.0, 1.0), "#e76f51")
]
# Invertimos el orden para que T1.1 aparezca en la parte superior del gráfico
tasks_plotting_order = tasks_raw[::-1] 

tasks_for_plot = []
task_labels = []
task_colors = []

for name, (start, duration), color in tasks_plotting_order:
    tasks_for_plot.append([(start, duration)])
    task_labels.append(name)
    task_colors.append(color)

# Crear la figura (formato panorámico 16:9)
fig, ax = plt.subplots(figsize=(16, 9))

for i, periods in enumerate(tasks_for_plot):
    ax.broken_barh(periods, (i*10 + 2, 6), facecolors=task_colors[i], edgecolors='black', alpha=0.9)

# Configuración de la Leyenda con las nuevas 5 fases del TFM
legend_phases = {
    "Fase 1: Investigación y Prototipado": "#264653",
    "Fase 2: Arquitectura Base y Entorno": "#2a9d8f",
    "Fase 3: Activos y Simulación Avanzada": "#e9c46a",
    "Fase 4: Calibración y Optimización": "#f4a261",
    "Fase 5: Documentación y Cierre": "#e76f51"
}

legend_patches = [mpatches.Patch(color=color, label=label) for label, color in legend_phases.items()]
ax.legend(handles=legend_patches, title="Fases del TFM", title_fontsize='12', 
          fontsize='10', loc='upper right', framealpha=1, edgecolor='black')

# Configuración de ejes y estética académica
ax.set_yticks([i*10 + 5 for i in range(len(task_labels))])
ax.set_xlabel('Semanas de Desarrollo (Cronograma Estimado)', fontsize=12, fontweight='bold')
ax.set_title('Diagrama de Gantt del Proyecto', fontsize=18, pad=25, fontweight='bold')

# Definimos el rango del eje X (12 semanas)
ax.set_xticks(range(13))
ax.set_xlim(0, 12)
ax.grid(True, axis='x', linestyle='--', alpha=0.4)

# Ajuste de margen izquierdo para que no se corten los nombres de las tareas
plt.subplots_adjust(left=0.32) 

# Guardar la imagen con alta resolución
plt.savefig('Gantt_TFM.png', dpi=300, bbox_inches='tight')
plt.show()