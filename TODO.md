# TODO

Backlog aus einer Durchsicht des Codes (Stand 2026-11-07). Reihenfolge = empfohlene Abarbeitung.
Offene Entscheidungen, die nur du treffen kannst, stehen in [DEV_TODO.md](DEV_TODO.md).

## 1. Repo-Struktur

- [ ] Ordnerstruktur aufräumen
  - Approach: Quellcode nach `src/`, Video-Skript nach `scripts/make_video.py`, Startskripte nach `scripts/`, Bilder/Video für die README nach `docs/media/`. Root enthält nur noch `README.md`, `Makefile`/`CMakeLists.txt`, `.gitignore`, `LICENSE`, `TODO.md`.
    ```
    src/        main.cpp, simulation.{hpp,cpp}, image_writer.{hpp,cpp}
    scripts/    make_video.py, run.cmd, run.sh
    docs/media/ result_dla.png, demo.mp4 (oder .gif)
    build/      (ignoriert) Binary
    out/        (ignoriert) frames/, Videos
    ```
  - Touches: `main.cpp`, `cristal_video.py`, `start.cmd`, `results/`, `Makefile`, `README.md`
  - Done when: Root hat ≤ 7 Einträge, alle Pfade in Makefile/Skripten/README zeigen auf die neuen Orte und der Build läuft.

- [ ] `main.cpp` in Module aufteilen
  - Approach: `get_index`/`put_particles`/`move_walkers` in eine `Simulation`-Klasse (`src/simulation.{hpp,cpp}`) mit Grid, RNG und Parametern als Member; `createImage` nach `src/image_writer.{hpp,cpp}`; `main.cpp` macht nur noch Argumente parsen + Schleife. Zell-Werte `0/1/2` durch `enum class Cell : uint8_t { Empty, Crystal, Walker }` ersetzen (dabei Grid von `std::vector<int>` auf `std::vector<Cell>` → 4× weniger Speicher).
  - Touches: `main.cpp` → `src/*`
  - Done when: Keine magischen Zahlen 0/1/2 mehr im Code und `main()` ist unter 30 Zeilen lang.

- [ ] Build-System für Windows und Linux
  - Approach: `CMakeLists.txt` (C++20, `-O3`/`/O2`, Warnungen) **oder** Makefile so erweitern, dass es `src/*.cpp` nach `build/cristal(.exe)` baut. Aktuell erzeugt das Makefile `main.out`, `start.cmd` ruft das unter Windows als `main.out` auf, was nur zufällig funktioniert.
  - Touches: `Makefile` / neu `CMakeLists.txt`, `start.cmd`
  - Done when: `cmake -B build && cmake --build build` (bzw. `make`) liefert unter Windows und Linux ein lauffähiges Binary.

- [ ] `.gitignore` vervollständigen
  - Approach: `build/`, `out/`, `frames/`, `*.ppm`, `__pycache__/`, `.venv/` ergänzen.
  - Touches: `.gitignore`
  - Done when: Nach einem kompletten Lauf zeigt `git status` keine neuen Dateien.

## 2. Bugs in der Simulation (`main.cpp`)

- [ ] Out-of-bounds-Zugriff in `put_particles`
  - Approach: `uniform_int_distribution<int>(0, matrix_size)` ist inklusiv, also kann `index` hinter das Array zeigen. Außerdem lesen `matrix[index-1]`, `matrix[index+1]` und `matrix[index±matrix_size]` am Rand außerhalb des Arrays bzw. springen in die Nachbarzeile. Fix: Verteilung `(0, matrix_size-1)` und Nachbarn über eine `in_bounds(x,y)`-Hilfsfunktion prüfen.
  - Touches: `main.cpp:15`, `main.cpp:28`
  - Done when: Ein Lauf mit `-fsanitize=address,undefined` meldet nichts.

- [ ] `put_particles` bricht still ab
  - Approach: Nach 25 Fehlversuchen in Folge (`count_break == 25`) gibt die Funktion auf und es werden weniger Partikel gesetzt als verlangt. Stattdessen die Anzahl der wirklich gesetzten Partikel zurückgeben und eine Warnung ausgeben, oder freie Zellen vorher sammeln und per `std::shuffle` auswählen.
  - Touches: `main.cpp:20-40`
  - Done when: Die Anzahl gesetzter Partikel ist garantiert oder wird ausgegeben.

- [ ] Abprall-Logik ist toter Code
  - Approach: In `move_walkers` werden nach einem Treffer auf einen Kristall `new_x`/`new_y` umgedreht, aber `new_index` wird nicht neu berechnet. Weil danach `temp[new_index] == 0` geprüft wird, bleibt der Walker einfach stehen. Entweder `new_index` neu berechnen (und prüfen, ob die Zelle frei ist) oder den Block löschen, denn bei 8er-Nachbarschaft hätte er sich ohnehin schon angelagert.
  - Touches: `main.cpp:76-91`
  - Done when: Das Verhalten ist bewusst gewählt und es gibt keinen unerreichbaren Zweig mehr.

- [ ] RNG nur einmal anlegen
  - Approach: `std::random_device` + `std::mt19937` werden pro Walker und pro Schritt neu erzeugt (`main.cpp:49-51`), das ist sehr langsam und statistisch schlecht. Einen Generator als Member der `Simulation` halten und optional per `--seed` deterministisch machen.
  - Touches: `main.cpp:13-15`, `main.cpp:49-51`
  - Done when: Gleicher `--seed` ⇒ identisches Endbild, und die Laufzeit ist messbar kürzer.

- [ ] Kopie des ganzen Grids pro Schritt vermeiden
  - Approach: `std::vector<int> temp = matrix;` kopiert jeden Schritt das komplette Grid. Walker-Positionen stattdessen in einer eigenen Liste `std::vector<int> walkers` halten und nur diese iterieren. Das ist ein deutlicher Speedup, weil die meisten Zellen leer sind.
  - Touches: `main.cpp:44-119`
  - Done when: Pro Schritt wird nur über Walker iteriert, nicht über `matrix_size²` Zellen.

- [ ] Simulation stoppen, wenn keine Walker mehr übrig sind
  - Approach: Ein Zähler aktiver Walker; die Schleife in `main` endet bei 0 statt immer volle 2000 Iterationen zu laufen und dabei identische Frames zu schreiben.
  - Touches: `main.cpp:172-177`
  - Done when: Die Konsole zeigt, nach wie vielen Schritten alles kristallisiert ist.

- [ ] Zeitmessung korrigieren
  - Approach: Mikrosekunden werden mit `0.0000001` (1e-7) statt `1e-6` multipliziert, also ist die angezeigte Zeit 10× zu klein. `std::chrono::duration<double>` verwenden.
  - Touches: `main.cpp:183-186`
  - Done when: Die angezeigte Zeit stimmt mit einer Stoppuhr überein.

- [ ] Fehler beim Schreiben der Frames erkennen
  - Approach: `std::ofstream` in `createImage` scheitert still, wenn `frames/` nicht existiert (der Ordner wird nur von `start.cmd` angelegt). Ordner per `std::filesystem::create_directories` selbst anlegen und `if (!imageFile)` prüfen.
  - Touches: `main.cpp:121-151`
  - Done when: Das Binary läuft auch ohne vorher angelegten `frames/`-Ordner.

## 3. Bedienbarkeit

- [ ] Parameter per Kommandozeile statt hartkodiert
  - Approach: `matrix_size`, `particles`, `seeds`, `iterations`, `--seed`, `--frame-every N`, `--out DIR` als Argumente (einfacher eigener Parser oder `cxxopts` als Header). Defaults = heutige Werte.
  - Touches: `main.cpp:153-158`
  - Done when: `cristal --size 512 --particles 20000 --seeds 1` funktioniert und `--help` listet alle Optionen.

- [ ] Nicht jeden Schritt einen Frame schreiben
  - Approach: Heute 2000 PPMs à 300 KB ≈ 600 MB Disk-I/O. Mit `--frame-every N` nur jeden N-ten Schritt speichern; optional PNG via `stb_image_write.h` (Single-Header) statt rohem PPM.
  - Touches: `main.cpp:175-176`, `createImage`
  - Done when: Ein Standardlauf schreibt weniger als 100 MB Zwischendateien.

- [ ] Video-Skript robuster machen
  - Approach: `hevc_nvenc` funktioniert nur mit NVIDIA-GPU, deshalb `libx264` als Default und `--encoder` als Option. Die ungenutzte Variable `output_video` tatsächlich verwenden (Zeile 6 vs. 27), die Meldung am Ende nennt den falschen Pfad. Prüfen, ob `ffmpeg` im PATH ist (`shutil.which`). `argparse` für fps/scale/Ordner. `shutil.rmtree("frames")` nur mit `--cleanup`, damit Frames nicht ungefragt gelöscht werden.
  - Touches: `cristal_video.py`
  - Done when: Das Skript läuft auf einem Rechner ohne NVIDIA-GPU und gibt verständliche Fehlermeldungen aus.

- [ ] Ein-Befehl-Start für beide Plattformen
  - Approach: `start.cmd` scheitert beim zweiten Lauf an `mkdir frames` (Ordner existiert) und prüft keine Fehler. `scripts/run.cmd` und `scripts/run.sh` mit Abbruch bei Fehler (`|| exit /b 1` bzw. `set -e`): bauen → simulieren → Video.
  - Touches: `start.cmd`
  - Done when: Zweimal hintereinander `scripts/run.cmd` funktioniert ohne Fehler.

## 4. Schöner machen (Optik und Physik)

- [ ] Farbverlauf nach Anlagerungszeitpunkt
  - Approach: Statt einfarbiger Kristalle für jede Zelle den Schritt speichern, in dem sie kristallisiert ist (`std::vector<uint32_t> age`), und beim Rendern auf eine Farbskala abbilden (z. B. Viridis/Magma, Tabelle mit 256 Einträgen im Code). So sieht man die Wachstumsringe, und das ist der größte optische Gewinn.
  - Touches: `move_walkers`, `createImage`
  - Done when: Das Endbild zeigt einen Verlauf von innen (alt) nach außen (neu).

- [ ] Farben und Kommentare in `createImage` aufräumen
  - Approach: Die Kommentare stimmen nicht (`// Green for seeds` ist Blau `0,0,255`, `// Red for walkers` ist Hellblau, `// Black for empty` ist Lavendel). Eine Palette als `constexpr`-Struct definieren; dunkler Hintergrund + leuchtender Kristall + gedämpfte Walker wirken meist besser.
  - Touches: `main.cpp:129-144`
  - Done when: Kommentare und Farben stimmen überein und die Palette steht an einer Stelle.

- [ ] Klassisches DLA-Wachstum als Modus
  - Approach: Heute werden alle Walker am Anfang gleichmäßig verteilt (das ergibt eher dichte Klumpen). Für die typischen verästelten Fraktale einen Modus `--mode radial`: ein Keim in der Mitte, Walker einzeln auf einem Kreis um den Cluster starten lassen und verwerfen, wenn sie zu weit weglaufen. Den bestehenden Modus als `--mode uniform` behalten. Die auskommentierte Zeile `main.cpp:167` deutet diesen Plan schon an.
  - Touches: `Simulation`, `main.cpp:165-169`
  - Done when: `--mode radial` erzeugt ein verzweigtes Fraktal mit einem Keim in der Mitte.

- [ ] Haft-Wahrscheinlichkeit (Sticking Probability)
  - Approach: Parameter `--stick 0.0..1.0`: Ein Walker lagert sich beim Kontakt nur mit dieser Wahrscheinlichkeit an. Kleinere Werte ergeben dichtere, „kristallinere“ Strukturen, das ist ein schöner Regler zum Experimentieren.
  - Touches: `move_walkers` (Kontakt-Zweig `main.cpp:109-113`)
  - Done when: Bilder mit `--stick 1` und `--stick 0.1` sehen sichtbar verschieden aus.

- [ ] Optional: Live-Ansicht
  - Approach: Mit raylib oder SDL2 ein Fenster, das das Grid jeden Frame als Textur zeigt. Damit fällt der Umweg über PPM-Dateien und ffmpeg zum Anschauen weg. Hinter Build-Option `-DCRISTAL_GUI=ON`, damit der Headless-Build ohne Abhängigkeiten bleibt.
  - Touches: neu `src/viewer.cpp`, Build-Datei
  - Done when: `cristal --live` zeigt das Wachstum in Echtzeit.

## 5. Qualität

- [ ] Kleine Tests
  - Approach: Mit festem Seed prüfen: Partikelzahl bleibt erhalten (Walker + Kristall = Seeds + Partikel), kein Zugriff außerhalb des Grids, Wachstum ist deterministisch. Einfaches `tests/test_simulation.cpp` mit `assert` oder doctest (Single-Header).
  - Touches: neu `tests/`
  - Done when: `ctest` bzw. `make test` läuft grün.

- [ ] CI mit GitHub Actions
  - Approach: `.github/workflows/build.yml`: Build + Tests auf `ubuntu-latest` und `windows-latest`, zusätzlich ein Lauf mit Sanitizern.
  - Touches: neu `.github/workflows/build.yml`
  - Done when: Push auf `main` zeigt einen grünen Haken.

## 6. README

- [ ] README um Bedienung ergänzen
  - Approach: Die README erklärt DLA, aber nicht, wie man das Projekt baut und startet. Abschnitte „Voraussetzungen“ (Compiler, Python, ffmpeg), „Bauen“, „Starten“, „Parameter“ ergänzen. Den allgemeinen Text über Halbleiter/Metallurgie kürzen, der Abschnitt zu DLA ist der interessante Teil.
  - Touches: `README.md`
  - Done when: Jemand Fremdes kann das Projekt nur mit der README bauen und starten.

- [ ] Demo direkt in der README sichtbar machen
  - Approach: GitHub spielt ein verlinktes `.mp4` aus dem Repo nicht inline ab. Ein kurzes, kleines GIF/WebP (`ffmpeg … -vf fps=20,scale=400 demo.gif`) einbetten und das große Video verlinken. Zusätzlich eine kleine Galerie mit Bildern verschiedener Parameter (Modus, Stick-Wert).
  - Touches: `README.md`, `results/` → `docs/media/`
  - Done when: Die Startseite des Repos auf GitHub zeigt eine Animation.
