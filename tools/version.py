"""Injecte PROJECT_VERSION depuis le fichier VERSION a la racine.

Pourquoi ce script existe
-------------------------
La version etait ecrite en dur dans platformio.ini :

    -D PROJECT_VERSION='"1.6.0"'

C'etait la seule facon de la connaitre, et elle vivait donc a un endroit
different de celui utilise par les onze autres projets de l'ecosysteme, ou un
fichier VERSION a la racine fait autorite et est lu par CMake.

Deux consequences. D'abord, aucun outil d'ecosysteme ne pouvait etablir
l'inventaire des versions du parc, puisque deux projets sur quatorze publiaient
la leur autrement. Ensuite, ajouter un fichier VERSION SANS ce script aurait
cree deux sources de verite pour une meme donnee — le defaut que ce script
existe precisement pour eviter.

Le fichier VERSION fait desormais autorite ; platformio.ini ne porte plus la
valeur, il la lit.
"""

import os

try:
    Import("env")            # noqa: F821  (fourni par PlatformIO)
except NameError:
    raise SystemExit("Ce script doit etre execute par PlatformIO (pio run).")

_PROJECT_DIR = env["PROJECT_DIR"]           # noqa: F821
_VERSION_FILE = os.path.join(_PROJECT_DIR, "VERSION")

try:
    with open(_VERSION_FILE, encoding="utf-8") as handle:
        version = handle.read().strip()
except OSError as error:
    raise SystemExit(f"VERSION illisible ({_VERSION_FILE}) : {error}")

if not version:
    raise SystemExit(f"VERSION est vide : {_VERSION_FILE}")

env.Append(CPPDEFINES=[("PROJECT_VERSION", env.StringifyMacro(version))])  # noqa: F821
print(f"PROJECT_VERSION = {version}  (lu depuis VERSION)")
