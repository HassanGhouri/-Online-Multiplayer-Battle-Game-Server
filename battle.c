#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#ifndef PORT
#define PORT 51495
#endif

/*Struct to hold Client Information. */
struct Client
{
    int socket_fd;
    char name[25];
    bool in_battle;
    int hitpoints[2];
    int powermoves[2];
    int current;
    struct Client *last_opponent;
    struct Client *opponent;
    struct Client *next;
};
/*Initalize a first Empty Client. */
/*Doing this becuase the code uses a linked List to keep track of Clients. */
struct Client *head = NULL;

/*Function to create new clients and to client linked list. */
struct Client *add_client(struct Client *head, int socket_fd, const char *name)
{
    struct Client *new_client = (struct Client *)malloc(sizeof(struct Client));
    if (new_client == NULL)
    {
        perror("malloc");
        exit(1);
    }
    new_client->socket_fd = socket_fd;
    strncpy(new_client->name, name, sizeof(new_client->name));
    new_client->next = NULL;
    new_client->in_battle = false;

    /*Cases for when this is the first client or not, if not add it to end of clients list. */
    if (head == NULL)
    {
        head = new_client;
    }
    else
    {
        struct Client *curr = head;
        while (curr->next != NULL)
        {
            curr = curr->next;
        }
        curr->next = new_client;
    }
    return head;
}

char *findClient(int socket_fd, struct Client *head)
{

    struct Client *curr = head;
    while (curr->next != NULL)
    {
        if ((curr->socket_fd) == socket_fd)
        {
            return curr->name;
        }
        curr = curr->next;
    }
    return "No such client.";
}

// Helper function for handle_battle to generate random number between min and max (inclusive)
int generateRandom(int min, int max)
{
    return min + rand() % (max - min + 1);
}

// Helper function for handle_battle to simulate a regular attack
int regularAttack()
{
    return generateRandom(2, 6);
}

// Helper function for handle_battle to simulate a power move
int powerMove()
{
    if (rand() % 2 == 0)
    { // 50% chance of missing
        return 0;
    }
    else
    {
        return 3 * regularAttack();
    }
}

// Helper function for handle_battle() and playerTurn()
void displayPokemon(struct Client *player)
{
    char text[100] = "Your Pokémon:\n";
    for (int i = 1; i < 3; i++)
    {
        // Initializing all the chars
        char x[100] = "Pokemon ";
        char z[100] = "Hitpoints = ";
        char a[100] = ", Powermoves = ";
        char c[5];
        char d[5];
        char e[5];

        snprintf(c, sizeof(c), "%d", i); // Changes integer to a char to be displayed in text
        strncat(x, c, strlen(c));
        strcat(x, ": ");

        snprintf(d, sizeof(d), "%d", player->hitpoints[i - 1]);
        strncat(z, d, strlen(d));
        strncat(x, z, strlen(z));

        snprintf(e, sizeof(e), "%d", player->powermoves[i - 1]);
        strncat(a, e, strlen(e));
        strncat(x, a, strlen(a));

        if (player->current == i)
        { // This is the current pokemon the client is battling with
            char b[100] = "(CURRENT)";
            strncat(x, b, strlen(b));
        }
        strcat(x, "\r\n");
        strncat(text, x, strlen(x));
    }
    write(player->socket_fd, text, strlen(text));
}

// Helper function for handle_battle to perform a player's turn
int playerTurn(struct Client *player, struct Client *player2)
{
    // initializers
    char choice[3];
    char text[100];
    int damage = 0;

    write(player->socket_fd, "Your turn!\n", 11);
    write(player->socket_fd, "Commands: a (Attack), p (Powermove), h (Speak), q (Quit), s (Switch Pokémon)\n", 80);
    int x = read(player->socket_fd, choice, sizeof(choice));
    write(player->socket_fd, "\n", 1);
    if (x <= 0) // error checking
    {
        perror("read");
        exit(1);
    }
    choice[x] = '\0';

    if (choice[0] == 'a')
    {
        damage = regularAttack();
    }
    else if (choice[0] == 'p')
    {
        if (player->powermoves > 0)
        {
            damage = powerMove();
            (player->powermoves[player->current - 1])--;
        }
        else
        {
            write(player->socket_fd, "No power moves available for this Pokémon!\n", 45);
            return playerTurn(player, player2); // recursive call if there are no powermoves remaining
        }
    }
    else if (choice[0] == 'h') // If client wants to speak
    {
        int y = read(player->socket_fd, text, sizeof(text));
        if (y <= 0)
        {
            perror("read");
            exit(1);
        }
        text[y] = '\0';
        write(player2->socket_fd, text, strlen(text));
        return playerTurn(player, player2); // recursive call after speaking
    }
    else if (choice[0] == 'q') // client wants to quit bgame but stay in the server
    {
        player->hitpoints[0] = 0;
        player->hitpoints[1] = 0;
        return -1;
    }
    else if (choice[0] == 's') // client wants to switch
    {
        player->current = (player->current == 1) ? 2 : 1;
        displayPokemon(player);
        return playerTurn(player, player2); // recursive call after switching pokemon
    }
    else
    {
        write(player->socket_fd, "Invalid command!\n", 18);
        return playerTurn(player, player2); // recursive call if client enters an invalid command
    }

    return damage;
}

void handle_battle(struct Client *curr, struct Client *opponent)
{

    srand(time(NULL)); // Seed the random number generator

    struct Client *p1 = curr;
    struct Client *p2 = opponent;

    p1->hitpoints[0] = rand() % (30 - 20) + 20;
    p2->hitpoints[0] = rand() % (30 - 20) + 20;
    p1->hitpoints[1] = rand() % (30 - 20) + 20;
    p2->hitpoints[1] = rand() % (30 - 20) + 20;

    p1->powermoves[0] = rand() % (3 - 1) + 1;
    p2->powermoves[0] = rand() % (3 - 1) + 1;
    p1->powermoves[1] = rand() % (3 - 1) + 1;
    p2->powermoves[1] = rand() % (3 - 1) + 1;

    // allocate space for strings
    char intro1[100];
    char intro2[100];

    // add beginning of strings
    strcpy(intro1, "You are now battling ");
    strcpy(intro2, "You are now battling ");

    // add opponent name to string
    strncat(intro1, p2->name, sizeof(intro1) - strlen(intro1) - 1);
    strncat(intro2, p1->name, sizeof(intro2) - strlen(intro2) - 1);

    // add \r\n characters
    strcat(intro1, "\r\n");
    strcat(intro2, "\r\n");

    // write messages to sockets
    write(p1->socket_fd, intro1, strlen(intro1));
    write(p2->socket_fd, intro2, strlen(intro2));
    int currentPlayer = generateRandom(0, 1);

    // set current pokemon to random and display pokemon
    p1->current = generateRandom(1, 2);
    p2->current = generateRandom(1, 2);
    displayPokemon(p1);
    displayPokemon(p2);

    while ((p1->hitpoints[0] > 0 || p1->hitpoints[1] > 0) && (p2->hitpoints[0] > 0 || p2->hitpoints[1] > 0))
    {
        int damage = 0;
        if (currentPlayer == 0)
        {
            damage = playerTurn(p1, p2);
            if (damage == -1)
            {
                break;
            }
            p2->hitpoints[p2->current - 1] -= damage;
            char text[200];
            snprintf(text, sizeof(text), "%s (Pokémon %d) dealt %d damage to %s (Pokémon %d)\n", p1->name, p1->current, damage, p2->name, p2->current);
            write(p1->socket_fd, text, strlen(text));
            write(p2->socket_fd, text, strlen(text));
        }
        else
        {
            damage = playerTurn(p2, p1);
            if (damage == -1)
            {
                break;
            }
            p1->hitpoints[p2->current - 1] -= damage;
            char text[200];
            snprintf(text, sizeof(text), "%s (Pokémon %d) dealt %d damage to %s (Pokémon %d)\n", p2->name, p2->current, damage, p1->name, p1->current);
            write(p1->socket_fd, text, strlen(text));
            write(p2->socket_fd, text, strlen(text));
        }
        currentPlayer = (currentPlayer == 0) ? 1 : 0;
    }

    if (p1->hitpoints <= 0 && p1->hitpoints <= 0)
    {
        char buf[100];
        snprintf(buf, sizeof(buf), "%s wins!\n", p2->name);
        write(p1->socket_fd, buf, strlen(buf));
        write(p2->socket_fd, buf, strlen(buf));
    }
    else
    {
        char buf[100];
        snprintf(buf, sizeof(buf), "%s wins!\n", p1->name);
        write(p1->socket_fd, buf, strlen(buf));
        write(p2->socket_fd, buf, strlen(buf));
    }
}

/*
Helper for match_clients. This function moves matched partners to the end of the client list.
*/
void matched_to_end(struct Client *head, struct Client *client1, struct Client *client2)
{
    struct Client *curr = head;
    struct Client *curr1 = head;
    struct Client *curr2 = head;
    struct Client *prev1 = NULL;
    struct Client *prev2 = NULL;
    struct Client *last;

    // find the last node
    while (curr->next != NULL)
    {
        curr = curr->next;
    }
    last = curr;

    // remove client1 from the list
    while (curr1 != NULL && curr1->socket_fd != client1->socket_fd) // DOUBLE CHECK THIS INCASE OF ERROR, PUT curr1 = client1
    {
        prev1 = curr1;
        curr1 = curr1->next;
    }
    if (curr1 != NULL)
    {
        if (prev1 != NULL)
        {
            prev1->next = curr1->next;
        }
        else
        {
            head = curr1->next;
        }
    }

    // remove client2 from the list
    while (curr2 != NULL && curr2->socket_fd != client2->socket_fd)
    {
        prev2 = curr2;
        curr2 = curr2->next;
    }
    if (curr2 != NULL)
    {
        if (prev2 != NULL)
        {
            prev2->next = curr2->next;
        }
        else
        {
            head = curr2->next;
        }
    }

    last->next = client1;
    client1->next = client2;
}

/*
Helper for match_clients. This function finds an opponent for a given client.
*/
struct Client *find_opponent(struct Client *head)
{
    struct Client *client = head;
    struct Client *curr = client->next;
    struct Client *opponent = NULL;

    // find opponent
    while (curr != NULL)
    {
        // check if curr is not battling
        if (!curr->in_battle)
        {
            // check if last opponents exist
            if (client->last_opponent != NULL && curr->last_opponent != NULL)
            {

                // if both clients did not last battle each other, match them.
                if (!((client->last_opponent->socket_fd == curr->socket_fd) && (client->socket_fd == curr->last_opponent->socket_fd)))
                {
                    opponent = curr;
                    break;
                }
            }
            // if last opponents DNE, then match them.
            else
            {
                opponent = curr;
                break;
            }
        }
        // no match, check next client.
        curr = curr->next;
    }
    return opponent;
}

/*
Function for matching clients.

Note: For the sake of this function, two new fields have been added to struct Client:
- last_opponent: the last opponent
- in_battle: boolean in battle or not.
*/
void match_clients(struct Client *head)
{
    struct Client *curr = head;
    struct Client *opponent = NULL;

    // iterate through clients, see who's available for matching.
    while (curr != NULL)
    {
        // printf("%s", curr->name);
        //  if client is not in a battle
        if (!curr->in_battle)
        {
            // find opponent for client
            opponent = find_opponent(curr);
            // check if it's a match
            if (opponent != NULL)
            {
                write(curr->socket_fd, "Entering battle\n", 17);
                write(opponent->socket_fd, "Entering battle\n", 17);

                curr->opponent = opponent;
                opponent->opponent = curr;
            }
        }

        curr = curr->next;
    }
}

int handleclient(struct Client *client, struct Client *head)
{
    char buf[256];
    int len = read(client->socket_fd, buf, sizeof(buf) - 1);

    if (len > 0)
    {
    }

    if (client->opponent != NULL)
    {

        client->in_battle = true;
        client->opponent->in_battle = true;

        handle_battle(client, client->opponent);
        matched_to_end(head, client, client->opponent);

        client->in_battle = false;
        client->last_opponent->in_battle = false;

        client->last_opponent = client->opponent;
        client->opponent->last_opponent = client;

        return 0; // if successful
    }

    return 1; // if unsuccessful
}

/* bind and listen, abort on error
 * returns FD of listening socket
 */
int bindandlisten(void)
{
    struct sockaddr_in r;
    int listenfd;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }
    int yes = 1;
    if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == -1)
    {
        perror("setsockopt");
    }
    memset(&r, '\0', sizeof(r));
    r.sin_family = AF_INET;
    r.sin_addr.s_addr = INADDR_ANY;
    r.sin_port = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&r, sizeof r))
    {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, 5))
    {
        perror("listen");
        exit(1);
    }
    return listenfd;
}

int main()
{
    struct Client *head = NULL;
    struct timeval tv;

    /*Create listening Socket. */
    int listen_soc = bindandlisten();

    /*Create fd_set to store all the clients to read from. */
    fd_set read_fds, rset;
    FD_ZERO(&read_fds);
    FD_SET(listen_soc, &read_fds);

    /*Create a genaric socket addr for client. */
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;

    /*set Inital maxfd to the fd of the listening socket, the fd we use in select has to be 1+ the fd of
    the pipe with the largest fd, code maintains this property. */
    int maxfd = listen_soc;

    FD_SET(listen_soc, &read_fds);

    /*Run an inifinte loop that listens for new connects, and creates a new socket for each client, also adds client to clients
    list. */
    while (1)
    {
        rset = read_fds;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        /*Calls select to liten for all clients. */
        if (select(maxfd + 1, &rset, NULL, NULL, &tv) == -1)
        {
            perror("select");
            exit(1);
        }

        if (FD_ISSET(listen_soc, &rset))
        {
            unsigned int client_len = sizeof(client_addr);
            int client_soc = accept(listen_soc, (struct sockaddr *)&client_addr, &client_len);

            if (client_soc < 0)
            {
                perror("accept");
                exit(1);
            };

            FD_SET(client_soc, &read_fds);

            if (client_soc > maxfd)
            {
                maxfd = client_soc;
            }

            char name[25];
            write(client_soc, "Enter your name:\r\n", 18);

            int bytes_read = 0;
            while (bytes_read < 25 - 1)
            {
                int read_result = read(client_soc, name + bytes_read, 25 - 1 - bytes_read);
                if (read_result < 0)
                {
                    perror("read");
                    exit(1);
                }
                else if (read_result == 0)
                {
                    char *name = findClient(client_soc, head);
                    if (strcmp(name, "No such client.") != 0)
                    {

                        char message[256];
                        snprintf(message, sizeof(message), "Client %s Disconnected\r\n", name);
                        for (int j = 0; j < maxfd + 1; j++)
                        {
                            write(j, message, strlen(message));
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    bytes_read += read_result;
                    if (name[bytes_read - 1] == '\n')
                    {
                        name[bytes_read - 1] = '\0';
                        break;
                    }
                }
            }
            write(client_soc, "Welcome! Awaiting Opponent...", 30);

            // add client to linked list
            head = add_client(head, client_soc, name);

            // match clients and set opponents
            match_clients(head);
        }

        for (int i = 0; i <= maxfd; i++)
        {
            if (FD_ISSET(i, &rset))
            {
                for (struct Client *p = head; p != NULL; p = p->next)
                {
                    if (p->socket_fd == i)
                    {
                        handleclient(p, head);
                        break;
                    }
                }
            }
        }
    }
    return 0;
}

/*This code that will allow for names to be read even if they do not come as one piece
while (bytes_read < bytes_to_read) {
    int read_result = read(client_soc, name + bytes_read, bytes_to_read - bytes_read);
    if (read_result < 0) {
        perror("read");
        exit(1);
    } else if (read_result == 0) {
        printf("Connection closed by client.\n");
        break;
    } else {
        bytes_read += read_result;
    }
}

close(client_soc);
close(listen_soc);
return 0;
*/
