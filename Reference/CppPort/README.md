# C++-Port als Verhaltensreferenz (nicht Teil des Builds)

Dieser Ordner enthält den vollständigen C++-Port des Gameplays (Tile-Welt, Physik, Bros, Gegner, Projektile,
Explosionen, GameMode/State, Kamera, HUD). Er wird **nicht kompiliert** – das Projekt ist ein reines
Blueprint-Projekt. Der Code dient als exakte, lesbare Spezifikation beim Aufbau der Blueprints
(siehe `docs/BLUEPRINTS.md`): jede Funktion dort entspricht einer Funktion hier, inklusive aller Zahlenwerte.

Falls Blueprint-Performance irgendwann nicht reicht (unwahrscheinlich bei dieser Spielgröße), kann daraus
ein schlankes C++-Modul für `TileWorld` werden, das per `BlueprintCallable` in die bestehenden Blueprints
eingehängt wird. Dann `Source/` zurück ins Projektverzeichnis kopieren und das Modul in der `.uproject` eintragen.
