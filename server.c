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

bool isRunning = true;

//int TCPSocket(unsigned short port);

/* ----- DEFINICJE FUNKCJI I METOD ----- */

int TCPListener(unsigned short port); //tworzenie nasłuchującego gniazda TCP
void* clientService(void *connectionPtr); //funkcja obsługi klienta

int main(int argc, char* argv[])
{
	//gniazdo listenera
	int listenerSocket;
	
	//struktura adresowa gniazda obsługi klienta i jego wielkość
	struct sockaddr_in clientAddr;
	unsigned int clientLength;
	
	//gniazdo dla klienta
	int *client;
	
	//identyfikator utworzonego watku
	pthread_t threadId = 0;
	
	//utworzenie gniazda listenera
	if ((listenerSocket = TCPListener(SERVER_PORT)) < 0)
	{
		fprintf(stderr, "%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	printf("Utworzono gniazdo nasluchujace...\n");
	printf("Zaczynam nasluchiwac...\n");
	
	//pętla nieskończona z nasłuchiwaniem i dołączaniem klientów
	while (1)
	{
		client = (int*)malloc(sizeof(int));
		clientLength = sizeof(clientAddr);
		if ((*client = accept(listenerSocket, (struct sockaddr *)&clientAddr, &clientLength)) < 0)
			fprintf(stderr, "%s\n", strerror(errno));
		printf("Przetwarzam klienta %s...\n", inet_ntoa(clientAddr.sin_addr));
		pthread_create(&threadId, 0, &clientService, (void*)client);
		pthread_detach(threadId);
	}
	//zamknięcie gniazda nasłuchującego
	close(listenerSocket);
	
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
	listenSocketAddr.sin_port = htons(port);
	
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
	
	printf("Deskryptor: %d\n", *connection);
	
	if ((getpeername(*connection, (struct sockaddr *) &clientAddr, &clientLength)) < 0)
	{
		perror("getpeername()");
	}
	printf("Klient: %s\n", inet_ntoa(clientAddr.sin_addr));
	close(*connection);
	free(connection);
	return 0;
}