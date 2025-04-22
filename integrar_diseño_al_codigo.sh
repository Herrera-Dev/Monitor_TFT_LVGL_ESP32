#!/bin/bash

# ==== CONFIGURACIÓN ====
ORIGEN="DIRECTORIO_ORIGEN"
DESTINO="DIRECTORIO_DESTINO"
IGNORAR=("example.h" "example.c")  # WARNING: Si tiene archivos.c o .h que no quiere que se eliminen, añádalos aquí.

es_ignorado() {
    local archivo=$(basename "$1")
    for ignorado in "${IGNORAR[@]}"; do
        if [[ "$ignorado" == "$archivo" ]]; then
            return 0  # true
        fi
    done
    return 1  # false
}

echo "🧹 Eliminando archivos .c y .h visibles en $DESTINO (excepto los ignorados)..."
shopt -s nullglob  # Evita errores si no hay archivos que coincidan
for archivo in "$DESTINO"/*.c "$DESTINO"/*.h; do
    [[ "$(basename "$archivo")" == .* ]] && continue
    if ! es_ignorado "$archivo"; then
        echo "Eliminando: $(basename "$archivo")"
        rm "$archivo"
    else
        echo "Ignorado: $(basename "$archivo")"
    fi
done

echo "📂 Copiando archivos visibles desde $ORIGEN a $DESTINO..."
shopt -s dotglob nullglob
for archivo in "$ORIGEN"/*; do
    nombre=$(basename "$archivo")
    [[ "$nombre" == .* ]] && continue
    cp -u "$archivo" "$DESTINO"/
    echo "Copiado: $nombre"
done

echo "✅ Operación completada."
