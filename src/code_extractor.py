import os

def consolidar_cpp_filtrado(directorio_raiz, archivo_salida):
    # Extensiones de archivos de código C++
    extensiones = ('.cpp', '.h', '.hpp', '.cxx')
    # Nombre de la carpeta que queremos ignorar
    carpeta_a_ignorar = "tests"
    
    archivos_procesados = 0
    
    with open(archivo_salida, 'w', encoding='utf-8') as f_out:
        for root, dirs, files in os.walk(directorio_raiz):
            
            # Lógica para ignorar carpetas llamadas 'tests'
            # 'root' contiene la ruta actual. Dividimos la ruta en sus componentes
            # y comprobamos si 'tests' es uno de ellos.
            partes_ruta = os.path.normpath(root).split(os.sep)
            if carpeta_a_ignorar in partes_ruta:
                continue  # Salta todo lo que esté dentro de esta carpeta

            for file in files:
                if file.endswith(extensiones):
                    ruta_completa = os.path.join(root, file)
                    
                    # Separador visual en el archivo de texto
                    f_out.write(f"\n{'='*1}\n")
                    f_out.write(f" ARCHIVO: {ruta_completa}\n")
                    f_out.write(f"{'='*1}\n\n")
                    
                    try:
                        with open(ruta_completa, 'r', encoding='utf-8', errors='replace') as f_in:
                            f_out.write(f_in.read())
                        archivos_procesados += 1
                    except Exception as e:
                        f_out.write(f"Error al leer el archivo: {e}\n")
                    
                    f_out.write("\n\n")

    return archivos_procesados

if __name__ == "__main__":
    ruta_proyecto = "./"  # Directorio actual
    nombre_resultado = "codigo_proyecto.txt"
    
    print(f"Iniciando consolidación en: {os.path.abspath(ruta_proyecto)}")
    total = consolidar_cpp_filtrado(ruta_proyecto, nombre_resultado)
    print(f"¡Hecho! Se han consolidado {total} archivos en '{nombre_resultado}'.")
    print(f"Se han ignorado automáticamente todas las carpetas llamadas 'tests'.")