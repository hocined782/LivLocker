#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

// DJEBRI_SAPA_STAMBOULI_GroupeA12

//gcc client_tcp_simple.c -o client_tcp_simple

#define MAX 500
//#define TIMEOUT 30

//Declaration of all functions we use
//receipt_buf allows to receive a message from the server
void receipt_buf(char *buf,int s_cli);

//send_buf allows to send a message towards the server
void send_buf(char *buf, int s_cli, char *buf1);

//request allows to choose the action that the user wants
void request(char *buf, char *buf1, int s_cli, int *valid1);

//connection process every steps of the user's connection 
void connection(char *buf, char *buf1, int s_cli, int *valid);


//-----------------------------------------------------------------------------
int main () {
	char buf [MAX], buf1[MAX];
	int s_cli ;
	struct sockaddr_in serv_addr ;
	//Client send a socket to the server in order to made a connection
	s_cli = socket (PF_INET, SOCK_STREAM, 0) ;
	serv_addr.sin_family = AF_INET ;
	serv_addr.sin_addr.s_addr = inet_addr ("127.0.0.1") ;
	serv_addr.sin_port = htons (5000) ;
	memset (&serv_addr.sin_zero, 0, sizeof(serv_addr.sin_zero));
	//Connection of client to the server on an port and display a message
	connect (s_cli, (struct sockaddr *)&serv_addr, sizeof serv_addr) ;
        printf ("Je me suis connecté au serveur %s sur son port %d\n", \
                   inet_ntoa (serv_addr.sin_addr), ntohs (serv_addr.sin_port)) ;
//-----------------------------------------------------------------------------

	//Ask the connection to the Data Base
	send_buf(buf,s_cli,"Demande de connexion à la BDD");

	//Receipt of the message from the server (connection or not)
	receipt_buf(buf,s_cli);

	//We call the function connection one time
	int valid = 0;
	connection(buf,buf1,s_cli,&valid);
	
	//While the id are not good we try to connect again
	while(valid!=0){
		connection(buf,buf1,s_cli,&valid);
	}

	//We call the function request one time
	int valid1 = 0;
	request(buf,buf1,s_cli,&valid1);
	
	/*
		While we don't choose to end the communication between the server and client
 		We ask again to the client what he wants to do
	*/
	while(valid1!=0){
		printf("\n\nQue voulez vous faire ?\n Pour voir les commandes , entrez COMMANDE\n Pour vérifier une commande, entrez VERIFIER\n Pour quitter, entrez FIN\n");//Affectation de req dans le buffer
		request(buf,buf1,s_cli,&valid1);
	}		
}

//----------------------------------------------------Functions------------------------------------------------------

void send_buf(char *buf, int s_cli, char *buf1)
{
	/*
	Initialization of the character's chain buf
	Copy of chain buf1 on buf
	Sending message contains buf to the client
	Notification of the action on the server
	*/
	memset (buf, 0, MAX);
	strcpy (buf,buf1);
	write (s_cli, buf, strlen (buf));
	printf ("J'ai envoye [%s] au serveur\n", buf1);
}



//-----------------------------------------------Fonctions---------------------------------------------------------

void receipt_buf(char *buf,int s_cli)
{
	/*
	Initialization of character's chain buf
	Reading of the chain buf from the server
	Notification on the client to see the result
	*/
	memset (buf, 0, MAX);
	int retour = recv(s_cli,buf,MAX,0);
	if(retour == 0){
		printf ("DECONNEXION ANORMALE DU SERVEUR\n") ;
		close(s_cli);
		printf("Fermeture du client\n");
		exit (0);
	}
	printf ("Réception: %s\n", buf);

}

//------------------------------------------------------------------------------------------------------------------

void request(char *buf, char *buf1, int s_cli, int *valid1)
{
	/*
	User's choice:
	Initialization of chain buf and buf1
	buf1 is the character given by the client
	Copy of buf1 on buf
	Sending a message contains buf to the server
	Notification on the client telling what the client sends
	*/
	memset (buf, 0, MAX);
	memset (buf1, 0, MAX);
	scanf("%s", buf1);
	printf("-->");
	strcpy (buf,buf1);
	write (s_cli, buf1, strlen (buf1));
	printf ("Demande au serveur: [%s]\n", buf1);


	//Receipt from the server an answer according to the user's choice
	receipt_buf(buf,s_cli);
		if(strstr(buf,"Entrez le numéro de la commande à vérifier")!=0){
			/*
			If the User send VERIFIER, the User wants to check the state of the delivery
			The server will ask the delivery's number
			The User will give the delivery's number to the server
			by the chain called 'buf'
			Notification which give the delivery sended
			valid1 = 1 so the user will can do an action again
			*/
			*valid1 =1;
			memset (buf, 0, MAX);
			memset (buf1, 0, MAX);
			printf("Commande : ");
			scanf("%s", buf1);				
			printf("\n-->");
			strcpy (buf,buf1);
			write (s_cli, buf1, strlen (buf1));
			printf ("Commande en paramètre : [%s]\n", buf1);
			receipt_buf(buf,s_cli);
		}
		else if(strstr(buf1,"COMMANDE")!=0){
			/*
			If the User send COMMANDE, the User wants to display the delivery's list
			The answer will be display on the client
			valid1 = 1 so the user will can do an action again
			*/
			*valid1=1;
		}
		else if(strstr(buf,"Fin de la communication")!=0){
			/*
			If the User send FIN, he will receive what is inside the buf
			This will end the communication 
			valid1 = 0 so the client and the server will close
			*/
			*valid1=0;
			printf("Au revoir\n");
			close (s_cli);
		}
		else if (strstr(buf,"Timeout")!=0){
			*valid1=0;
			printf("Au revoir\n");
			close (s_cli);
		}
		else{
			/*
			If the User send anything else, he will receive an error
			Then the User will need to send a valid message again 
			valid1 = 1 so the user will can do an action again
			*/
			*valid1=1;
		}
}

//----------------------------------------------------------------------------------------------------------------------

void connection(char *buf, char *buf1, int s_cli, int *valid)
{

	//Ask to the server for a connection
	send_buf(buf,s_cli,"Demande de connexion au compte");
	//Receipt from the server to give a User 
	receipt_buf(buf,s_cli);
	//The client give an User thanks to buf1
	memset (buf, 0, MAX);
	memset (buf1, 0, MAX);
	printf("Utilisateur: ");
	scanf("%s", buf1);	
	printf("-->");
	strcpy (buf,buf1) ;
	//Sending the message to the server contains the User
	write (s_cli, buf1, strlen (buf1));
	//Notification
	printf ("User en paramètre: [%s]\n", buf1);
	//Receipt from the server if the User is found or not and ask for the password
	receipt_buf(buf,s_cli);
	//The client give an Password thanks to buf1
	memset (buf, 0, MAX);
	memset (buf1, 0, MAX);
	printf("Mot de passe: ");
	scanf("%s", buf1);
	printf("-->");
	strcpy (buf,buf1);
	//Sending the message to the server contains the Password
	write (s_cli, buf1, strlen (buf1));
	//Notification
	printf ("Password en paramètre: [%s]\n", buf1);
	//Receipt from the server if the Password is correct or not
	receipt_buf(buf,s_cli);
	/*
	If the server said that the id are wrong, he will ask 
	to send again an User and Password (valid=1)
	Else if, the user send any word that the server don't recognize he will
	ask again to send a correct word(valid=1)
	Else the Id are correct so valid=0 and the connection is made
	*/
	if(strstr(buf,"Erreur de connexion, entrez des identifiants valides !!!\n")!=0){
		*valid=1;
	}
	else if(strstr(buf,"Erreur, entrez un mot valide")!=0){
		*valid=1;
	}
	else{
		*valid=0;
	}
}