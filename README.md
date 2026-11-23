# BeaconView

Native kerkpresentatiesoftware voor beamer, tv en LED-schermen.
Gebouwd met **Qt 6 (C++ / QML)**, één codebase voor Windows, macOS en Linux.

## Wat werkt nu (v0.1)
- Dienstorde met onderdelen (welkom, lied, gebed, schriftlezing, …): toevoegen, bewerken, verplaatsen, verwijderen
- Liederenbibliotheek (lege regel in de tekst = nieuwe slide), liederen toevoegen aan de dienst
- Live-uitvoer op een tweede scherm (randloos, volledig scherm); bij één scherm een los venster
- Huidige/volgende slide, zwart scherm, logo, achtergrondkeuze (kleurverlopen of eigen afbeeldingen)
- Sneltoetsen: → / spatie / PgDn = volgende, ← / PgUp = vorige, **B** = zwart, **L** = logo
- Automatisch opslaan van dienst en liederen

## Bouwen
Vereist Qt 6.4+ (Quick, QuickControls2), CMake 3.21+ en een C++17-compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/beaconview
```

- **macOS**: `macdeployqt build/beaconview.app -qmldir=qml -dmg`
- **Windows**: `windeployqt --qmldir qml beaconview.exe`

De GitHub Actions-workflow in `.github/workflows/build.yml` bouwt voor Windows, macOS en Linux.

## Nog te doen
Bijbelmodule, overlays/timer, video-achtergronden, templates (lettertype/kleur), eigen logo,
importeren van liederen (OpenSong/CCLI), afstandsbediening.
