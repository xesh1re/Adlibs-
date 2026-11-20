# 🌸 Adlib Sakura

Plugin **VST3** hecho con **C++ y JUCE**, pensado solo para **adlibs**.
Interfaz rosa pálido con ramas y pétalos de cerezo japonés cayendo. Cada knob es una flor
de sakura que gira, y al pasar el mouse sobre cualquiera aparece su descripción en español.

## Efectos

Cadena: `Entrada → Distorsión → Tono → Lo-Fi → Doble → Eco → Reverb → Salida`

| Knob | Qué hace |
|---|---|
| Entrada | Volumen de la voz antes de los efectos |
| Distorsión | Saturación suave a agresiva |
| Tono | Oscurece o abre el brillo |
| Lo-Fi | Efecto sucio de radio/teléfono (bitcrusher) |
| Doble | Duplica la voz con chorus estéreo |
| Salida | Volumen final |
| Eco Tiempo | Separación entre repeticiones (40–900 ms) |
| Eco Repetición | Cantidad de repeticiones |
| Eco Mezcla | Cuánto eco se escucha |
| Reverb Tamaño | De cuarto pequeño a catedral |
| Reverb Oscuridad | Reverb más suave y lejana |
| Reverb Mezcla | Cuánta reverb se escucha |

## Compilar

Necesitas **CMake ≥ 3.22**, **Git** y un compilador de C++17
(Visual Studio 2022 en Windows, Xcode en macOS, GCC/Clang en Linux).
JUCE se descarga solo.

```bash
git clone <tu-repo>
cd AdlibSakura
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

El VST3 queda en `build/AdlibSakura_artefacts/Release/VST3/`.
También se genera una versión **Standalone** para probarlo sin DAW.

Para instalarlo copia la carpeta `Adlib Sakura.vst3` a:

- Windows: `C:\Program Files\Common Files\VST3`
- macOS: `~/Library/Audio/Plug-Ins/VST3`
- Linux: `~/.vst3`

## GitHub Actions

Al hacer push, `.github/workflows/build.yml` compila el plugin en Windows, macOS y Linux
y deja los binarios descargables en la pestaña **Actions → Artifacts**.

## Personalizar

- `COMPANY_NAME`, `PLUGIN_MANUFACTURER_CODE` y `PLUGIN_CODE` en `CMakeLists.txt`.
- Textos de los knobs: arreglo `kDefs` en `Source/PluginEditor.cpp`.
- Colores: namespace `sakura` en `Source/Sakura.h`.
- DSP: `Source/PluginProcessor.cpp`.
