# Projektregeln und Guidelines

Diese Regeln gelten für das gesamte Projekt und sollen einen klaren, wiederholbaren Ablauf sicherstellen.

## 1. KI-/Werkzeugnutzung
- Es darf ausschließlich GPT‑5 verwendet werden ("Bitte nur gpt5").
- Keine alternativen Modelle oder Tools ohne ausdrückliche Zustimmung.

## 2. Arbeitsablauf (immer in dieser Reihenfolge)
1) Aufgabenliste/Plan erstellen (kurz, klar priorisiert)
2) Testfälle definieren (Unit-/Integrations‑/Smoke‑Tests, Akzeptanzkriterien)
3) Umsetzung/Implementierung (kleine, nachvollziehbare Commits)
4) Tests ausführen und Ergebnisse dokumentieren (alle Tests grün, Logs kurz festhalten)

Hinweise:
- Für nicht triviale Änderungen erst eine kompakte Aufgabenliste anlegen (Tickets/Tasks).
- Testfälle sollen die Anforderungen messbar machen und vor der Umsetzung stehen.

## 3. Branching- und Release-Strategie
- Entwicklung findet ausschließlich im Branch `development` statt.
- Änderungen in `stable` erfolgen nur nach Rückfrage/Approval (Review/Bestätigung erforderlich).
- Feature-Flow:
  - Von `development` aus Branch anlegen: `feature/<kurzbeschreibung>` oder `fix/<kurzbeschreibung>`
  - Pull Request zurück nach `development`, Code Review, alle Checks grün
  - Merge nach `stable` erst nach expliziter Freigabe

## 4. Git-/GitHub‑Vorgehen
- "Bitte immer GitHub aktuell halten":
  - Häufig committen mit aussagekräftigen Messages (z. B. Conventional Commits: `feat: …`, `fix: …`)
  - Vor dem Push: `git pull --rebase` aus dem Zielbranch, Konflikte bereinigen
  - Pushen auf den Remote und zeitnah PRs erstellen
- Schutzmaßnahmen (Empfehlung):
  - Required Reviews und Status Checks für `development`/`stable`

- WICHTIG: Secrets dürfen NIEMALS ins Repository oder auf GitHub gelangen (keine API-Keys, Passwörter, Tokens, private Zertifikate im Repo).
## 5. PlatformIO (Pflicht)
- PlatformIO ist das Build‑/Flash‑/Monitor‑Tool. Bitte ausschließlich PlatformIO verwenden.
- Typischer Workflow:
  - Bauen: `pio run`
  - Flashen: `pio run -t upload`
  - Nach jedem Flashen den Monitor starten: `pio device monitor`
    - Baudrate richtet sich nach `platformio.ini` (Feld `monitor_speed`). Beispiel: `pio device monitor -b 115200`
  - Monitor beenden: `Ctrl+C`
- Die Datei `platformio.ini` ist führend für Umgebungen/Settings.

## 6. Testen & Qualitätssicherung
- Tests sind verpflichtend und laufen lokal vor jedem PR:
  - Kleinster Umfang zuerst (Unit), dann Integrations-/Smoke‑Tests
  - Alle relevanten Targets/Environments aus `platformio.ini` berücksichtigen
- CI (falls vorhanden) muss grün sein, bevor gemerged wird.

## 7. Dokumentation
- Änderungen kurz im PR beschreiben (Was, Warum, Wie verifiziert?)
- Wichtige Nutzer‑/Entwicklerhinweise im README/Docs ergänzen

## 8. Verantwortlichkeiten & Freigaben
- Merge nach `stable` nur nach expliziter Rückfrage/Bestätigung (Owner/Maintainer).
- Breaking Changes frühzeitig ankündigen, Migrationshinweise beilegen.

## 9. Checkliste pro Änderung
- [ ] Aufgabenliste/Plan erstellt
- [ ] Testfälle definiert und abgedeckt
- [ ] Implementierung abgeschlossen (kleine Commits)
- [ ] Alle Tests lokal grün
- [ ] Build/Flash erfolgreich (falls relevant)
- [ ] Nach Flash Monitor gestartet und geprüft
- [ ] PR erstellt, Beschreibung verfasst, Review angefragt
- [ ] Nach Approval gemäß Strategie gemerged

---
Stand: automatisch erstellt (GPT‑5).
