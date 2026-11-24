# BeaconView

Native kerkpresentatiesoftware voor beamer, tv en LED-schermen.
Gebouwd met **Qt 6 (C++ / QML)**, één codebase voor Windows, macOS en Linux.

## Wat werkt nu (v0.05.202)
- Dienstorde met onderdelen (welkom, lied, gebed, schriftlezing, …): toevoegen, bewerken, verplaatsen, verwijderen
- **Presentaties**: PDF, PowerPoint (`.pptx`, `.ppt`, `.odp`) en afbeeldingen importeren als slides; elke dia wordt een beeld op het scherm. PDF wordt rechtstreeks gelezen; PowerPoint wordt omgezet met LibreOffice (gratis) of, op Windows en macOS, met PowerPoint zelf. Lukt dat niet, dan staat er hoe je het als PDF exporteert.
- **Mededelingenlus**: slides (tekst en/of afbeeldingen) die vanzelf wisselen en opnieuw beginnen, met instelbare tijd per slide; pauzeren kan, "Volgende slide" gaat naar het volgende onderdeel
- **Video als eigen onderdeel**: afspelen met geluid, pauzeren, zoeken en volume; optioneel herhalen of na afloop automatisch naar het volgende onderdeel. Het videobestand blijft staan waar het staat (er wordt niets gekopieerd); ontbreekt het, dan zie je een waarschuwing.
- **Lege slide / alleen achtergrond**: een slide zonder tekst toont alleen de achtergrond; tijdens de dienst haalt **Geen tekst** (of **T**) de tekst van het scherm en laat de achtergrond staan, bij de volgende slide is de tekst er weer
- **Schermeditor (WYSIWYG)** voor alle onderdelen behalve liederen: typ direct op het scherm met het gekozen template en de echte achtergrond, voeg slides toe/verplaats/verwijder ze, kies template en achtergrond en zie rechts het exacte resultaat
- Liederenbibliotheek (lege regel in de tekst = nieuwe slide), liederen toevoegen aan de dienst
- Live-uitvoer op een tweede scherm (randloos, volledig scherm); bij één scherm een los venster
- Alle slides van het huidige onderdeel als aanklikbare lijst (klik = die slide live; de lijst scrolt mee), zwart scherm, logo, achtergrondkeuze (kleurverlopen, eigen afbeeldingen of **video's**: mp4/mov/webm/mkv, stil en in een lus)
- Sneltoetsen: → / spatie / PgDn = volgende, ← / PgUp = vorige, **B** = zwart, **L** = logo
- **Timer** (aftellen of stopwatch, groot in het midden of klein in de hoek, met eigen tekst erboven), **klok** en **meldingen** (opgeslagen tekstbalken die je met één klik toont of verbergt) als overlays bovenop de slide
- **Eigen logo** (Instellingen): PNG, SVG of JPG als logo-scherm (met eigen achtergrondkleur en grootte) en als optioneel watermerk in een hoek van elke slide
- **Templates**: lettertype, grootte, kleur, uitlijning, schaduw, regelafstand, marge, verduistering en bijbelverwijzing; 5 ingebouwde templates, eigen templates via dupliceren; het actieve template geldt voor de hele dienst, een standaardtemplate per soort onderdeel (Templates → Per onderdeel), en elk onderdeel kan een eigen template krijgen (⋮ → Template)
- **Bijbel** (Statenvertaling): typ een verwijzing (`Joh 3:16`, `1 Kor 13:4-7`, `Psalm 23`) of zoek op een woord; kies het aantal verzen per slide en de verwijzing staat onderaan elke slide
- Automatisch opslaan van dienst en liederen

## Logo en iconen
`assets/branding/` bevat de aangeleverde bestanden (logo, programma-icoon, documenticoon; PNG en ICO). Het programma-icoon wordt gebruikt voor het venster, `beaconview.exe` en de installer (Windows) en de app (macOS, `.icns`). Het documenticoon is nog niet in gebruik: het is bedoeld voor een eigen bestandstype voor diensten.

## Installeren
Bij elke release (tag `v*`) bouwt de CI:
- **Windows**: `BeaconView-Setup-<versie>.exe` (installer, Inno Setup) en `BeaconView-Windows.zip` (zonder installatie)
- **macOS**: `BeaconView-macOS.dmg`

De pakketten zijn nog niet ondertekend: Windows (SmartScreen) en macOS (Gatekeeper) tonen daarom een waarschuwing bij het eerste openen.

## Bouwen
Vereist Qt 6.4+ (Quick, QuickControls2, Multimedia, Svg, Pdf), CMake 3.21+ en een C++17-compiler. Op Linux heb je voor de bestandsdialogen ook `qml6-module-qt-labs-folderlistmodel` nodig, en voor PowerPoint-import LibreOffice.

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
Meer bijbelvertalingen (licentie nodig voor NBV21/HSV/BGT), videominiaturen, timer die automatisch de dienst start, 
importeren van liederen (OpenSong/CCLI), afstandsbediening.
