# BeaconView

Native kerkpresentatiesoftware voor beamer, tv en LED-schermen.
Gebouwd met **Qt 6 (C++ / QML)**, één codebase voor Windows, macOS en Linux.

## Wat werkt nu (v0.0.1.0)
- Dienstorde met onderdelen (welkom, lied, gebed, schriftlezing, …): toevoegen, bewerken, verplaatsen, verwijderen
- Liederenbibliotheek (lege regel in de tekst = nieuwe slide), liederen toevoegen aan de dienst
- Live-uitvoer op een tweede scherm (randloos, volledig scherm); bij één scherm een los venster
- Huidige/volgende slide, zwart scherm, logo, achtergrondkeuze (kleurverlopen, eigen afbeeldingen of **video's**: mp4/mov/webm/mkv, stil en in een lus)
- Sneltoetsen: → / spatie / PgDn = volgende, ← / PgUp = vorige, **B** = zwart, **L** = logo
- **Timer** (aftellen of stopwatch, groot in het midden of klein in de hoek, met eigen tekst erboven), **klok** en **meldingen** (opgeslagen tekstbalken die je met één klik toont of verbergt) als overlays bovenop de slide
- **Eigen logo** (Instellingen): PNG, SVG of JPG als logo-scherm (met eigen achtergrondkleur en grootte) en als optioneel watermerk in een hoek van elke slide
- **Templates**: lettertype, grootte, kleur, uitlijning, schaduw, regelafstand, marge, verduistering en bijbelverwijzing; 5 ingebouwde templates, eigen templates via dupliceren; het actieve template geldt voor de live-uitvoer
- **Bijbel** (Statenvertaling): typ een verwijzing (`Joh 3:16`, `1 Kor 13:4-7`, `Psalm 23`) of zoek op een woord; kies het aantal verzen per slide en de verwijzing staat onderaan elke slide
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

## Tests
```sh
cmake -S . -B build -DBUILD_TESTING=ON && cmake --build build && ctest --test-dir build --output-on-failure
```

## Bijbeldata
Zie `data/README.md` (bron en licentie) en `tools/make_bible.py` (conversie).

## Nog te doen
Meer bijbelvertalingen (licentie nodig voor NBV21/HSV/BGT), videominiaturen, timer die automatisch de dienst start, template per onderdeel,
importeren van liederen (OpenSong/CCLI), afstandsbediening.
