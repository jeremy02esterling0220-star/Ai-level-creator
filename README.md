# AI Level Creator (mod de Geode para Geometry Dash)

Autora: Rosa Melano

Añade un botón **IA** arriba a la izquierda en el editor. Al pulsarlo:
- Escribes una descripción (ej: `nivel dificil con picos y escaleras`).
- Ajustas dificultad (1-5) y longitud.
- Pulsas **Generar nivel** y se colocan los objetos en el editor.

Palabras clave: facil, normal, dificil, demon, largo, corto, "sin picos",
"sin bloques", "sin escaleras", "sin pads". La misma descripción siempre
genera el mismo nivel.

## Cómo conseguir el archivo .geode (sin instalar nada en el PC)

1. Instala Geode en tu Geometry Dash (https://geode-sdk.org).
2. Crea una cuenta gratis en GitHub y un repositorio nuevo.
3. Sube estos archivos: `mod.json`, `CMakeLists.txt`, `main.cpp`
   y `.github/workflows/build.yml`.
4. Ve a la pestaña **Actions**, ejecuta "Build Mod" y descarga el
   resultado (`.geode`).
5. Copia el `.geode` a la carpeta `geode/mods` de tu juego (o ábrelo con
   Geode en Windows/Android).

## Compilar en local (opcional)

Instala el Geode SDK y la CLI, y ejecuta: `geode build`.

## Nota

Está hecho para GD 2.2074 y Geode 4.x. Si tu versión de Geode es 5.x,
cambia `Popup<LevelEditorLayer*>` por la nueva API de `Popup`.
