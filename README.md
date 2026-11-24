# BeaconView

Native kerkpresentatiesoftware voor beamer, tv en LED-schermen.
Gebouwd met **Qt 6 (C++ / QML)**, één codebase voor Windows, macOS en Linux.

## Wat werkt nu (v0.1.2.0)
- Dienstorde met onderdelen (welkom, lied, gebed, schriftlezing, …): toevoegen, bewerken, verplaatsen, verwijderen
- **Media-bibliotheek** (tab Media): alle achtergronden, afbeeldingen en video's in één overzicht met miniaturen (bij video's het eerste beeld), filteren, meerdere bestanden tegelijk importeren, voorbeeld en gegevens, gebruiken als achtergrond van het huidige onderdeel of als standaardachtergrond, een video als video-onderdeel toevoegen, en **verwijderen** (alleen je eigen kopie in de app; verwijzingen worden opgeruimd)
- **Presentaties**: PDF, PowerPoint (`.pptx`, `.ppt`, `.odp`) en afbeeldingen importeren als slides; elke dia wordt een beeld op het scherm. PDF wordt rechtstreeks gelezen; PowerPoint wordt omgezet met LibreOffice (gratis) of, op Windows en macOS, met PowerPoint zelf. Lukt dat niet, dan staat er hoe je het als PDF exporteert.
- **Mededelingenlus**: slides (tekst en/of afbeeldingen) die vanzelf wisselen en opnieuw beginnen, met instelbare tijd per slide; pauzeren kan, "Volgende slide" gaat naar het volgende onderdeel
- **Video start betrouwbaar**: de video wordt pas afgespeeld als het bestand klaar is, staat de video direct na het huidige onderdeel dan wordt hij al voorgeladen (starten duurt dan een fractie van een seconde), en lukt het niet dan volgt een duidelijke melding. Wat er gebeurt staat in een logbestand (Instellingen → Problemen oplossen).
- **Video als eigen onderdeel**: afspelen met geluid, pauzeren, zoeken en volume; optioneel herhalen of na afloop automatisch naar het volgende onderdeel. Het videobestand blijft staan waar het staat (er wordt niets gekopieerd); ontbreekt het, dan zie je een waarschuwing.
- **Lege slide / alleen achtergrond**: een slide zonder tekst toont alleen de achtergrond; tijdens de dienst haalt **Geen tekst** (of **T**) de tekst van het scherm en laat de achtergrond staan, bij de volgende slide is de tekst er weer
- **Schermeditor (WYSIWYG)** voor alle onderdelen behalve liederen: typ direct op het scherm met het gekozen template en de echte achtergrond, voeg slides toe/verplaats/verwijder ze, kies template en achtergrond en zie rechts het exacte resultaat
- **Extra schermen** (pagina Schermen): een stage-scherm voor wie op het podium staat (huidige en volgende tekst met de naam van het deel, klok, timer, zwart-scherm-melding) en een ondertitelscherm met chroma-key (alleen de tekst met rand op groen, blauw, magenta of zwart; positie, grootte en tekstkleur instelbaar). Per scherm kies je een beeldscherm of een los venster (de venstertitel is de naam, voor een venster-opname in OBS of vMix); schermen kunnen met de uitvoer meeopenen
- Liederenbibliotheek (lege regel in de tekst = nieuwe slide), liederen toevoegen aan de dienst
- **Liederen importeren uit SongSelect (CCLI)**: de tekstbestanden (.txt) en USR-bestanden (.usr) die je zelf met je account downloadt; elk deel wordt een slide en houdt zijn naam (Verse 1, Chorus, …): die staat in de lied-editor als `[Verse 1]` boven het deel en in de slidelijst, maar nooit op het scherm; titel, auteurs en CCLI-nummer worden bewaard en dubbele liederen worden overgeslagen. Er is geen directe koppeling met SongSelect (die is niet beschikbaar voor nieuwe programma's)
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
importeren van liederen uit andere formaten (OpenSong, PDF), CCLI-nummer in de uitvoer tonen, afstandsbediening.
