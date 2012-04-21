#include <stdio.h>

/* niezbędna do close() */
#include <unistd.h>

/* da możliwość korzystania ze zmiennych boolowskich */
#include <stdbool.h>

/* obsługa błędów perror() */
#include <errno.h>
/* do exitów */
#include <stdlib.h>

/* memset() */
#include <string.h>

/* do utworzenia gniazda
 * 1. socket()
 * 2. connect()
 */
#include <sys/types.h>
#include <sys/socket.h>

/* do struktury sockaddr_in */
#include <netinet/in.h>

/* do funkcji konwertujących adresy IP */
#include <arpa/inet.h>

/* biblioteka wątków */
#include <pthread.h>

/* ----- DEFINICJE STAŁYCH I ZMIENNYCH GLOBALNYCH -----*/

#define SERVER_PORT		16300
#define QUEUE_LEN		100

//ustawienie zmiennej informującej czy serwer jest włączony
int isRunning = 0;

//bufor przechowujący to co wpisano w linii poleceń serwera
char line[BUFSIZ];

//blokowanie zmiennej globalnej
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//int TCPSocket(unsigned short port);

/* ------------ DEFINICJE FUNKCJI I METOD ------------ */

int TCPListener(unsigned short port); //tworzenie nasłuchującego gniazda TCP
void* clientService(void *connectionPtr); //funkcja obsługi klienta
void* listenClient(); //funkcja nasłuchująca połączeń

void showHelp(); //wyświetlanie pomocy
void status(); //informacja o statusie serwera

/* --------------- DEFINICJE STRUKTUR ---------------- */

typedef struct
{
	char address[50];
	int tableIndex;
} client;

/* --------------------------------------------------- */

int main(int argc, char* argv[])
{
	//identyfikator utworzonego watku listenera
	pthread_t listenerThId = 0;
	
	while(1)
	{
		//wyczyszczenie bufora linii
		memset(line, 0, BUFSIZ * sizeof(char));
		
		printf("Server > ");
		//jeżeli nic nie wpisano to po prostu przejdź do nowej linii
        if (fgets(line, sizeof(line), stdin) == NULL) {
            putchar('\n');
            exit(0);
        }
		
		//usunięcie znaku nowej linii z line
		line[strlen(line)-1] = 0;
		
		//jakaś masakra, że switch łyka tylko porównywanie integerów ;/
		
		if (strcmp(line, "help") == 0)
		{
			showHelp();
		}
		else if (strcmp(line, "start") == 0)
		{
			//jeżeli serwer wyłączony
			if (!isRunning)
			{
				//to ustaw zmienną informującą czy serwer jest włączony
				//pthread_mutex_lock(&lock);
				isRunning = 1;
				//pthread_mutex_unlock(&lock);
				//uruchomienie wątku nasłuchującego
				pthread_create(&listenerThId, 0, &listenClient, NULL);
				//ustawienie zwalniania zasobów dla wątku
				pthread_detach(listenerThId);
				
				printf("Rozpoczeto nasluchiwanie...\n");
			}
			else
			{
				printf("Serwer jest juz aktywny.\n");
			}
		}
		else if (strcmp(line, "stop") == 0)
		{
			//jeżeli serwer włączony
			if (isRunning)
			{
				//to ustaw zmienną informującą czy serwer jest wyłączony
				//pthread_mutex_lock(&lock);
				isRunning = 0;
				//pthread_mutex_unlock(&lock);
				//anuluj wątek nasłuchujący
				pthread_cancel(listenerThId);
				//i włącz go do wątku głównego
				pthread_join(listenerThId, NULL);
				
				printf("Wstrzymano nasluchiwanie.\n");
			}
			else
			{
				printf("Serwer jest juz nieaktywny.\n");
			}
		}
		else if (strcmp(line, "status") == 0)
		{
			status();
		}
		else if (strcmp(line, "exit") == 0)
		{
			isRunning = 0;
			pthread_cancel(listenerThId);
			pthread_join(listenerThId, NULL);
			return 0;
		}
		else
		{
			printf("Nieznana komenda. Wpisz 'help' aby wyswietlic dostepne komendy.\n");
		}
	}
	
	return 0;
}
int TCPListener(unsigned short port) 
{
	//deskryptor gniazda do nasłuchiwania
	int listenSocket;
	
	//struktura adresowa gniazda do nasłuchiwania
	struct sockaddr_in listenSocketAddr;
	
	//inicjalizacja gniazda nasłuchującego
	if ((listenSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Nie udalo sie zainicjalizowac gniazda! ");
		return -1;
	}
	
	//wyczyszczenie struktury - wypełnienie zerami
	memset(&listenSocketAddr, 0, sizeof(listenSocketAddr));
	
	listenSocketAddr.sin_family = AF_INET;
	listenSocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	listenSocketAddr.sin_port = htons((u_short)port);
	
	//przypisanie adresu lokalnego do gniazda
	if (bind(listenSocket, (struct sockaddr *) &listenSocketAddr, sizeof(listenSocketAddr)) < 0)
	{
		perror("Blad podczas przypisywania gniazdu lokalnego adresu! ");
		return -1;
	}
	//otwarcie portu do nasłuchiwania
	if (listen(listenSocket, QUEUE_LEN) < 0)
	{
		perror("Blad podczas nasluchiwania! ");
		return -1;
	}
	
	return listenSocket;
}
void* clientService(void* connectionPtr)
{
	int *connection = (int*)connectionPtr;
	
	//struktura adresowa gniazda do nasłuchiwania
	struct sockaddr_in clientAddr;
	
	socklen_t clientLength = sizeof(clientAddr);
	
	//wyczyszczenie struktury - wypełnienie zerami
	memset(&clientAddr, 0, sizeof(clientAddr));
	
	//printf("Deskryptor: %d\n", *connection);
	
	if ((getpeername(*connection, (struct sockaddr *) &clientAddr, &clientLength)) < 0)
	{
		perror("getpeername()");
	}
	//printf("Klient: %s\n", inet_ntoa(clientAddr.sin_addr));
	close(*connection);
	free(connection);
	return 0;
}
void* listenClient()
{
	//identyfikator utworzonego watku
	pthread_t threadId = 0;
	
	//struktura adresowa gniazda obsługi klienta i jego wielkość
	struct sockaddr_in clientAddr;
	unsigned int clientLength;
	
	//gniazdo dla klienta
	int *clientSock;
	
	//gniazdo listenera
	int listenerSocket;
	
	//utworzenie gniazda listenera
	if ((listenerSocket = TCPListener(SERVER_PORT)) < 0)
	{
		fprintf(stderr, "%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	//printf("Utworzono gniazdo nasluchujace...\n");
	//printf("Zaczynam nasluchiwac...\n");
	
	//pętla nieskończona z nasłuchiwaniem i dołączaniem klientów
	while (isRunning)
	{
		clientSock = (int*)malloc(sizeof(int));
		clientLength = sizeof(clientAddr);
		if ((*clientSock = accept(listenerSocket, (struct sockaddr *)&clientAddr, &clientLength)) < 0)
			fprintf(stderr, "%s\n", strerror(errno));
		printf("Przetwarzam klienta %s...\n", inet_ntoa(clientAddr.sin_addr));
		pthread_create(&threadId, 0, &clientService, (void*)clientSock);
		pthread_detach(threadId);
	}
	//zamknięcie gniazda nasłuchującego
	close(listenerSocket);
	printf("Doszlo do konca\n");
	
	return NULL;
}
void showHelp()
{
	printf("Sterowanie serwerem: \n\n");
	printf("exit - wyjscie z programu\n");
	printf("start - rozpoczoczecie nasluchiwania polaczen klientow\n");
	printf("stop - wstrzymanie nasluchiwania polaczen klientow\n");
	
	printf("\nInformacje o serwerze: \n\n");
	printf("list - wyswietlenie listy klientow\n");
	printf("status - informacja na temat dzialania serwera\n");
	
	printf("\n");
}
void status()
{
	if (isRunning)
		printf("Serwer jest aktywny i nasluchuje polaczen.\n");
	else
		printf("Serwer jest nieaktywny i nie nasluchuje polaczen.\n");
}