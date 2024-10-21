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


## Branches du Projet

### 1. **Branche `main`**
La branche `main` contient les fichiers de base pour utiliser et tester le jeu Awalé. Pour utiliser cette branche :
- Compilez les fichiers avec la commande suivante :  
  ```bash
  gcc server.c -o server -lpthread
  gcc client.c -o client
  ```
Lancez le serveur en spécifiant le nombre de rooms :
```bash
./server <nombre_de_rooms>
```

Lancez ensuite un client en précisant l'adresse IP du serveur (par défaut, il est configuré pour 127.0.0.1) :
```bash
./client <adresse_ip_du_serveur>
```
Dans cette version, les fonctionnalités de base du jeu fonctionnent correctement, incluant la gestion des parties, des scores, et des connexions/déconnexions des joueurs.

### 2. Branche defi
La branche defi ajoute une fonctionnalité supplémentaire permettant de lancer des défis entre joueurs connectés. Cette fonctionnalité est activée via la commande `/defi` côté client. Voici comment elle fonctionne :

Un joueur peut lancer un défi en entrant la commande `/defi` suivie du pseudo du joueur à défier.
Le joueur défié peut répondre avec `/accepter` ou `/refuser`.
Cependant, cette branche comporte une limitation :

La gestion des déconnexions n'est pas complètement fonctionnelle. Si un joueur se déconnecte, la gestion des états des parties en cours peut rencontrer des problèmes, et nous n'avons pas eu le temps de corriger ce point avant la livraison finale.
Commandes Disponibles

## Voici une liste des commandes disponibles pour le client :

`/defi <pseudo>` : Défie un autre joueur en utilisant son pseudo.

`/accepter` : Accepter un défi reçu d’un autre joueur.

`/refuser` : Refuser un défi reçu d’un autre joueur.

`/liste` : Affiche la liste des joueurs actuellement connectés.

`/chat` : Passe en mode chat pour discuter avec d'autres joueurs.

`/game` : Revient en mode jeu.

`/score` : Affiche le tableau des scores.

`/bio <pseudo>` : Affiche la biographie d’un joueur en spécifiant son pseudo.

`exit` ou `disconnect` : Se déconnecter du serveur.

Ces commandes permettent de gérer la communication, les défis, et les interactions pendant le jeu.

# Issues

En cas de problème de syncro avec le serveur, appuyer sur la touche `m` puis `entrer` pour resyncroniser le client bugger et le serveur.