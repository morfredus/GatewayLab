# morfBeacon — emetteur embarque (copie vendoree)

Copie de `arduino/morfbeacon_emitter.h` du depot **morfBeacon**.

Ne pas editer ici : toute correction se fait en amont, puis la copie est
resynchronisee. Comme pour la copie C++/Qt de `third_party/morf/beacon`, la
recopie garantit un firmware compilable sans dependre d'un depot externe.

Le partage porte sur le **protocole**, pas sur le code : la bibliotheque Qt ne
peut pas tourner sur un ESP32, donc les deux emetteurs sont deux
implementations d'un meme format. Elles vivent dans le meme depot amont pour que
lire l'une a cote de l'autre reste possible — c'est le seul garde-fou contre une
divergence silencieuse.
