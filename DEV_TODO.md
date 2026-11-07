# DEV_TODO

Dinge, die nur du entscheiden oder liefern kannst. Zugehörige Code-Arbeit steht in [TODO.md](TODO.md).

- [ ] Lizenz für das öffentliche Repo
  - Needs from you: Welche Lizenz (z. B. MIT, Apache-2.0, GPL-3.0) und welcher Name im Copyright-Vermerk. Das Repo ist öffentlich, hat aber keine `LICENSE`, also darf rechtlich niemand den Code nutzen.

- [ ] Build-System
  - Needs from you: Die bestehende Makefile erweitern oder auf CMake umsteigen? (CMake ist für Windows + Linux + CI bequemer, die Makefile ist einfacher.) Gehört zu „Build-System für Windows und Linux“ in TODO.md.

- [ ] Großes Video im Git-Verlauf
  - Needs from you: Entscheidung, ob `results/output_video.mp4` im Repo bleiben soll, durch ein GIF ersetzt wird oder über Git LFS bzw. ein GitHub Release ausgeliefert wird. Aus dem Verlauf entfernen hieße History umschreiben + Force-Push.

- [ ] Projektname / Schreibweise
  - Needs from you: Bleibt es bei „Cristal“ (Repo-Name) oder soll es überall „Crystal“ heißen (README-Titel nutzt schon „Crystal“)? Betrifft Binary-Namen, Ordner, README und ggf. den GitHub-Repo-Namen.

- [ ] Umfang der Live-Ansicht
  - Needs from you: Soll es überhaupt eine GUI geben, und falls ja, raylib oder SDL2? Das bringt eine externe Abhängigkeit ins Projekt. Gehört zu „Optional: Live-Ansicht“ in TODO.md.

- [ ] Sprache der Doku
  - Needs from you: README nur Englisch, oder wieder eine deutsche Version (`README_GER.md` gab es laut Commit-Historie schon einmal)?
