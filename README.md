# Awalé - Jeu en Réseau

# Jeu de LEONARDO HERON et NOE BAIOCCHI

Bienvenue dans Awalé, un jeu de société traditionnel africain maintenant disponible en version client-serveur ! Ce projet permet de jouer à distance via un serveur, avec des fonctionnalités comme la gestion des défis, des scores et un mode de discussion en temps réel.

## Configuration du Serveur

### Option 1: Utilisation du serveur public
Le serveur Awalé est actuellement disponible à l'IP suivante :

**IP du serveur** : `209.38.181.89`  
**Port** : `12345`  
**Nombre de rooms par défaut** : 10 rooms

Pour vous connecter, utilisez le client fourni avec le projet et exécutez la commande suivante :

```bash
./awale_client 209.38.181.89
```

- Après avoir entré votre pseudonyme, vous pourrez choisir une room où jouer. Si vous souhaitez défier un autre joueur, utilisez la commande `/defi <pseudo_du_joueur>`.

### Option 2: Héberger votre propre serveur

Si vous préférez héberger votre propre serveur, voici comment procéder :

#### Étapes pour héberger le serveur :

1. Clonez ce dépôt sur votre machine.
   
2. Assurez-vous que votre environnement supporte les bibliothèques standards nécessaires (POSIX Threads, Sockets).

3. Compilez les fichiers serveur avec la commande suivante (le `Makefile` est inclus) :

```bash
make
```

4. Démarrez le serveur avec un nombre de rooms spécifié. Par exemple, pour 5 rooms :

```bash
./awale_server 5
```

Cela démarre le serveur sur le port **12345**.

#### Étapes pour se connecter à votre serveur :

Une fois que le serveur est lancé, les joueurs peuvent se connecter en utilisant l'adresse IP de votre serveur. Par exemple, si l'IP du serveur est `192.168.1.100`, le client peut se connecter avec la commande suivante :

```bash
./awale_client 192.168.1.100
```

Si rien n'est spécifié, alors l'ip localhost sera automatiquement utilisée.

## Fonctionnalités

- **Gestion des défis** : Défiez d'autres joueurs avec la commande `/defi`.
- **Chat intégré** : Entrez en mode chat avec la commande `/chat`.
- **Tableau des scores** : Consultez le tableau des scores en tapant `/score`.
- **Reconnexion** : Si vous êtes déconnecté, vous pouvez revenir dans la partie en vous reconnectant.

## Compilation et Exécution

Pour compiler et lancer le serveur et le client, assurez-vous d'avoir les fichiers suivants :

- `server.c`
- `server.h`
- `client.c`
- `client.h`
- `Makefile`

Puis exécutez simplement :

```bash
make
```

Cela générera les exécutables `server` et `client`.

Bon jeu !
