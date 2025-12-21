# Projet Sherlock 13 (SH13)

Ce dépôt contient l'implémentation du jeu de société **Sherlock 13** en C utilisant la bibliothèque SDL2 pour l'interface graphique et les sockets TCP pour le multijoueur.

## Structure du projet

- **server.c** : Le serveur de jeu qui gère la logique, la distribution des cartes et la synchronisation entre les joueurs.
- **sh13.c** : Le client de jeu (interface graphique SDL) que chaque joueur lance.

## Prérequis

Pour compiler et exécuter ce projet sur Linux, vous avez besoin des bibliothèques SDL2 :

```bash
sudo apt-get update
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
```

## Compilation

Rendez-vous dans le dossier de travail (par exemple `Sherlock_13/`) et utilisez le script de compilation :

```bash
./cmd.sh
```

Cela générera les exécutables `server` et `sh13`.

## Exécution

Le jeu nécessite 1 serveur et 4 clients (joueurs).

1. **Lancer le serveur** (dans un terminal) :
   ```bash
   ./server 32000
   ```

2. **Lancer les 4 joueurs**

### Jeu en local

Si les 4 terminaux sont sur le même ordinateur :

   ```bash
   ./sh13 localhost 32000 localhost 32001 Jean
   ./sh13 localhost 32000 localhost 32002 Pierre - Louis
   ./sh13 localhost 32000 localhost 32003 Esteban
   ./sh13 localhost 32000 localhost 32004 Emile
   ```

### Jeu en réseau

Si les joueurs sont sur des machines différentes, utilisez la syntaxe suivante :

   ```bash
   ./sh13 <Main server ip address> <Main server port> <Client ip address> <Client port> <player name>
   ```
