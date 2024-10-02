# Fichiers sources
SRC_CLIENT	=	./client/client.c
SRC_SERVER	=	./server/server.c
SRC_GAME	=	./game/awale.c

# Fichiers objets
OBJ_CLIENT	=	$(SRC_CLIENT:.c=.o)
OBJ_SERVER	=	$(SRC_SERVER:.c=.o)
OBJ_GAME	=	$(SRC_GAME:.c=.o)

# Noms des exécutables
NAME_CLIENT	=	cli
NAME_SERVER	=	serv
NAME_GAME	=	awale

# Flags de compilation
CFLAGS	=	-W -Wall -Wextra -g

# Compilation de tous les exécutables
all:	$(NAME_CLIENT) $(NAME_SERVER) $(NAME_GAME)

# Compilation du client
$(NAME_CLIENT):	$(OBJ_CLIENT)
	gcc $(OBJ_CLIENT) -o $(NAME_CLIENT)
	@echo -e "\033[0;34m<client has been created>\033[00m"

# Compilation du server
$(NAME_SERVER):	$(OBJ_SERVER)
	gcc $(OBJ_SERVER) -o $(NAME_SERVER)
	@echo -e "\033[0;34m<server has been created>\033[00m"

# Compilation du jeu
$(NAME_GAME):	$(OBJ_GAME)
	gcc $(OBJ_GAME) -o $(NAME_GAME)
	@echo -e "\033[0;34m<game has been created>\033[00m"

# Nettoyage des fichiers objets
clean:
	@echo -e "\033[0;34m<Deleting useless files>\033[00m"
	rm -f $(OBJ_CLIENT) $(OBJ_SERVER) $(OBJ_GAME)

# Nettoyage des fichiers objets et exécutables
fclean: clean
	rm -f $(NAME_CLIENT) $(NAME_SERVER) $(NAME_GAME)

# Rebuild complet
re:	fclean all
