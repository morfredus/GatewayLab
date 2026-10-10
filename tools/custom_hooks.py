"""custom_hooks.py - génère les ressources web avant CHAQUE compilation.

Ce que ça fait
--------------
Exécute tools/minify_web.py, qui produit à partir des sources versionnées :

    web_src/*.html, *.js, styles.css  ->  include/web_interface*.h   (pages en PROGMEM)
    data/oui.json                     ->  include/oui_table.h        (table des fabricants)

Ces en-têtes ne sont PAS versionnés (ils sont ignorés par Git) : ils n'existent donc pas
dans un clone neuf et se reconstruisent ici, à chaque `pio run`. Le build s'arrête si la
génération échoue, plutôt que de compiler avec des pages périmées ou absentes.

Pourquoi à l'import du script, et pas dans une pré-action
---------------------------------------------------------
Un script `pre:` est exécuté par PlatformIO avant le début de la compilation : y lancer la
génération garantit que les en-têtes existent AVANT que web_server.cpp et
network_scanner.cpp ne les incluent. Une pré-action accrochée au .elf (version précédente
de ce fichier) se déclenchait à l'édition de liens, donc trop tard.

Pourquoi sys.executable
-----------------------
La commande `python` du PATH peut ne pas exister ou pointer vers un autre interpréteur ;
sys.executable est celui qui fait tourner PlatformIO. PYTHONIOENCODING force l'UTF-8 : le
minificateur affiche des flèches que la console Windows (cp1252) ne sait pas écrire dans un tube.
"""

import os
import subprocess
import sys

try:
    Import("env")            # noqa: F821  (fourni par PlatformIO ; ne pas écrire « env = Import(...) »)
except NameError:
    raise SystemExit("Ce script doit être exécuté par PlatformIO (pio run).")

_PROJECT_DIR = env["PROJECT_DIR"]                                  # noqa: F821
_SCRIPT = os.path.join(_PROJECT_DIR, "tools", "minify_web.py")

if not os.path.isfile(_SCRIPT):
    raise SystemExit(f"Script de minification introuvable : {_SCRIPT}")

_result = subprocess.run(
    [sys.executable, _SCRIPT],
    cwd=_PROJECT_DIR,
    capture_output=True,
    text=True,
    encoding="utf-8",
    errors="replace",
    env={**os.environ, "PYTHONIOENCODING": "utf-8"},
)

if _result.returncode != 0:
    # Sortie complète seulement en cas d'échec ; la console Windows peut refuser certains
    # caractères, d'où le remplacement plutôt qu'une exception qui masquerait la vraie erreur.
    for _stream in (_result.stdout, _result.stderr):
        sys.stdout.write((_stream or "").encode("ascii", "replace").decode("ascii"))
    raise SystemExit("La génération des ressources web a échoué (tools/minify_web.py) : build interrompu.")

_written = _result.stdout.count("(ecrit)")
_same = _result.stdout.count("(inchange)")
# ASCII seulement : la console Windows (cp1252) abîme les accents dans la sortie de PlatformIO.
print(f"Ressources web : {_written} fichier(s) regenere(s), {_same} inchange(s)")
