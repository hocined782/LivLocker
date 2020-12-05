#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <libpq-fe.h>
#include <stdlib.h>
#include <time.h> 
#include <unistd.h>

// DJEBRI_SAPA_STAMBOULI_GroupeA12
//gcc serveur.c-I $HOME/include -L $HOME/lib -l pq -o serveur

#define MAX 500
#define TIMEOUT 30

//Declaration of all functions we use

//receipt_buf allows to receive a message from the client
void receipt_buf(char *buf,int s_dial,int s_ecoute);

//send_buf allows to send a message towards the client
void send_buf(char *buf, int s_dial,char *req, char *buf1);

//display_select allows to display every results of user's request
void display_select(PGresult *res, char *req);

//request_select browses the table which used by the client
PGresult *request_select(PGconn *conn,char *request );

/*
comparison_user allows to compare the user given by the 
client with every users which are present in the data base
*/
void comparison_user(PGresult *res,char *userdata, char *user);

/*
comparison_password allows to compare the password given by the 
client with every passwords which are present in the data base
*/
void comparison_password(PGresult *res, char *passwordata, char *password, char *buf);

/*
check_data_ticket allows to compare the ticket's data given by the 
client with every tikets which are present in the data base
The server compares the ticket's number, date and hour
*/
void check_data_ticket(PGresult *res, char *data);

/*
machine allows to compare the ticket's data given by the 
client with every tikets which are present in the data base
The server check the ticket's number, date and hour
And answers towards the client
*/
void machine(PGconn *conn, PGresult *res, char *commandedata, char *commande, char *buf);

//receipt allows to receive a message from the client and answers according of the buf (character's chain)
void receipt(char *buf,char *buf1, int s_dial, int s_ecoute, char *request, char *req, char *commande, PGconn *conn, PGresult *res, int *valid);

//check_connection process every steps of the user's connection from the point of servers's view 
void check_connection(char *buf, char *buf1, char *buf2, char *userdata, char *passwordata, char *request, int s_dial, PGconn *conn, PGresult *res, int *valid1,int s_ecoute);

//------------------------------------------------------------------------------------------------------------




int main () {
	char buf[MAX], buf1[MAX],  buf2[MAX];
	
	int s_ecoute, s_dial, cli_len ;
	struct sockaddr_in serv_addr, cli_addr ;

	serv_addr.sin_family = AF_INET ;
	serv_addr.sin_addr.s_addr = INADDR_ANY ;
	serv_addr.sin_port = htons (5000) ;//port
	memset (&serv_addr.sin_zero, 0, sizeof(serv_addr.sin_zero));
	//Server send a socket to the client as answer to made a connection
	s_ecoute = socket (PF_INET, SOCK_STREAM, 0) ;
	//link of a socket to an address or data's port
	bind (s_ecoute, (struct sockaddr *)&serv_addr, sizeof serv_addr) ;
	listen (s_ecoute, 5) ;
	cli_len = sizeof (cli_addr) ;
	//Accept the connection from a socket
	s_dial = accept (s_ecoute, (struct sockaddr *)&cli_addr, &cli_len) ;
	printf ("Le client d'adresse IP %s s'est connecté depuis son port %d\n", \
	            inet_ntoa (cli_addr.sin_addr), ntohs (cli_addr.sin_port)) ;
//-------------------------------------------------------------------------------------------------------------------
	//Declaration of all arguments we use
	PGconn *conn;
	PGresult *res;
	char request[MAX];
	char req[MAX];
	char userdata[MAX];
	char passwordata[MAX];
	char commande[MAX];

	//Connection to the Data Base
	conn=PQconnectdb("host=localhost password=bdd78313 user=postgres dbname=LivLocker");

	//Recipt the message from the client who asks the connection
	receipt_buf(buf,s_dial,s_ecoute);

	/*
	Process the steps of the connection
	If the connection is etablished the server will answer that is ok
	Else he will send an error
	buf contains the message 
	*/
	if (PQstatus(conn)!=CONNECTION_OK){
		strcpy (buf, "Erreur de connexion à la base de données !! Vérifiez les paramètres") ;//Message à envoyer au client
        printf ("J'ai envoye une réponse au client\n") ;
	}
	else{
		strcpy (buf, "CONNEXION BDD REUSSIE\n") ;//Message à envoyer au client
        printf ("J'ai envoye une réponse au client\n") ;
	}
        write (s_dial, buf, strlen (buf)) ;//envoie le message au client 

//----------------------------------------------------------------------------------------------------------------------
	
	//We call the function check_connection one time
	int valid1 = 0;
	check_connection(buf,buf1,buf2,userdata,passwordata,request,s_dial,conn,res,&valid1,s_ecoute);
	//While the id are not good we try to connect again
	while(valid1!=0){
	check_connection(buf,buf1,buf2,userdata,passwordata,request,s_dial,conn,res,&valid1,s_ecoute);
	}
//--------------------------------------------------------------------------------------------------------------------

	//We call the function receipt one time
	int valid = 0;
	receipt(buf,buf1,s_dial,s_ecoute,request,req,commande,conn,res,&valid);
	/*
		While we don't choose to end the communication between the server and client
 		We receive again the user's choice from the client (what he wants to do)
 		And we process according to his choice 
 		The server will not close
	*/
	while(valid!=0){
	receipt(buf,buf1,s_dial,s_ecoute,request,req,commande,conn,res,&valid);
	}
}

//---------------------------------------Fonctions--------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------------

void receipt_buf(char *buf,int s_dial, int s_ecoute)
{

	/*
	Initialization of character's chain buf
	Reading of the chain buf from the server
	Notification on the server to see the client's demand
	*/
	memset (buf, 0, MAX);
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(s_dial, &rfds);
	struct timeval tv;
	int retval;
	tv.tv_sec=TIMEOUT;
	tv.tv_usec=0;
	retval = select(FD_SETSIZE,&rfds,NULL,NULL,&tv);
	if(retval==-1)
		perror("select()");
	else if(retval==0){
		//printf("Aucune réception, Déconnexion\n");
		send_buf("Timeout",s_dial,NULL,"Aucune réception, Déconnexion\n");
		close(s_dial);
		close(s_ecoute);
		printf("Fermeture du serveur\n");
		exit(0);
	}
	int retour = recv(s_dial,buf,MAX,0);
	if(retour == 0){
		printf ("DECONNEXION ANORMALE DU CLIENT\n") ;
		close(s_dial);
		close(s_ecoute);
		printf("Fermeture du serveur\n");
		exit (0);
	}
	printf ("J'ai recu [%s] du client\n", buf) ;
}

//---------------------------------------------------------------------------------------------------------
void display_select(PGresult *res, char *req)
{
	/*
	Initialization of character's chain req references the request
	Reading of the chain buf from the client
	Concatenation of chain req to stock the result
	Process the table we use
	*/
	int i,j;
	int nFields;

	strcpy(req,"");

	nFields = PQnfields(res);
	for (i = 0; i < nFields; i++){
		strcat(req, PQfname(res, i));
	}
	strcat(req,"\n\n");//Cncat de req
	for (i = 0; i < PQntuples(res); i++){
        	for (j = 0; j < nFields; j++){
           		strcat(req, PQgetvalue(res, i, j));
		}
       		strcat(req,"\n");
    	}
}
//-------------------------------------------------------------------------------------------------------------

PGresult *request_select(PGconn *conn,char *request )
{
	/*
	Execution of the Postgres request 
	If the execution is done it's ok 
	Else we receive an error
	Return a result res which will be reused
	*/
	PGresult   *res;
	res=PQexec(conn,request);
	if (PQresultStatus(res) != 2/*PGRES_COMMAND_OK*/){
        	fprintf(stderr, "%s", PQerrorMessage(conn));
       		PQclear(res);
    	}
	return res;
}

//---------------------------------------------------------------------------------------------------------

void comparison_user(PGresult *res,char *userdata, char *user)
{
	/*
	Process of the result res from the execution of the request
	Process of the table to find the user contains in the data base
	Comparison of the user given by the client
	Display of a message said that it's found or not
	*/
	int i,j;
	int nFields;

	nFields = PQnfields(res);


	for (i = 0; i < PQntuples(res); i++){
        	for (j = 0; j < nFields; j++)
            		strcat(userdata, PQgetvalue(res, i, j));
        strcat(userdata,"\n");
    	}
	if(strstr(userdata,user)!=0){
		printf("UTILISATEUR TROUVE\n");
	}else{
		printf("ERREUR, UTILISATEUR NON TROUVE\n");
	}

}
//-----------------------------------------------------------------------------------------------------------

void comparison_password(PGresult *res, char *passwordata, char *password, char *buf)
{
	/*
	Process of the result res from the execution of the request
	Process of the table to find the password contains in the data base
	Comparison of the password given by the client
	Concatenation of a message for the client which said that it's found or not
	*/
	int i,j;
	int nFields;

	nFields = PQnfields(res);

	for (i = 0; i < PQntuples(res); i++){
        	for (j = 0; j < nFields; j++)
            		strcat(passwordata, PQgetvalue(res, i, j));//Concat de req (les elements des tables)
        strcat(passwordata,"\n");

    	}
	if(strstr(passwordata,password)!=0){
		strcat(buf,"Vous êtes connecté !!!!\n\nQue voulez vous faire ?\nPour voir les commandes , entrez COMMANDE\n Pour vérifier une COMMANDE, entrez VERIFIER\n Pour quitter, entrez FIN\n");

	}else{
		strcat(buf,"Erreur de connexion, entrez des identifiants valides !!!\n");
	}
}

//-------------------------------------------------------------------------------------------------------------------

void check_data_commande(PGresult *res, char *data)
{
	/*
	Process of the result res from the execution of the request
	Process of the table to find every ticket's data in parameters given by the client contains in the data base
	Concatenation of a chain data contains the data founded in the data base
	*/
	int i,j;
	int nFields;
	nFields = PQnfields(res);

	strcpy(data,"");
	for (i = 0; i < PQntuples(res); i++){
        	for (j = 0; j < nFields; j++)
        		strcat(data, PQgetvalue(res, i, j));
        	strcat(data,"\n");
    	}		
}

//--------------------------------------------------------------------------------------------------------------------

void machine(PGconn *conn, PGresult *res, char *commandedata, char *commande, char *buf)
{
	/*
	Comparison of the data from the delivery given by the client
	Checking of the delivery's number and data
	*/
	int i,j;
	int nFields;
	char request1[MAX];

	//Call of the function which compares the data
	check_data_commande(res,commandedata);
	/*
	If the delivery's number is found, we retrieve the date of today
	Then we request (request1) the date according to this delivery
	We execute the request
	And we compare these two dates 
	*/
	if(strstr(commandedata,commande)!=0){

		char date[256]; 
		time_t t = time(NULL);
 		strftime(date, sizeof(date), "%Y-%m-%d", localtime(&t)); 

		printf("NUMERO DE LA COMMANDE TROUVE\n");

		memset (request1, 0, MAX);
		strcpy(request1,"");
		strcat(request1,"SELECT date FROM commandes WHERE id='");
		strcat(request1,ticket);
		strcat(request1,"';");
		res=request_select(conn,request1);
		
		char datedata[MAX];
		check_data_commande(res,datedata);

		/*
		If the delivery's date is ok, we retrieve the hour of the moment
		Then we request (request1) the session's hour according to this delivery's date
		We execute the request 
		*/
		if(strstr(datedata,date)!=0){
			printf("DATE DE LA COMMANDE: %s\n",datedata);
			printf("DATE DU JOUR: %s\n",date);
			printf("DATE TROUVEE\n");

			strcat(buf,"La commande à bien été effectué à ce jour");

			// char hour[256]; 
			// time_t t = time(NULL);
 			// strftime(hour, sizeof(hour), "%k", localtime(&t)); 

			// memset (request1, 0, MAX);
			// strcpy(request1,"");
			// strcat(request1,"SELECT hour FROM commandes WHERE id='");
			// strcat(request1,commande);
			// strcat(request1,"' AND date='");
			// strcat(request1,date);	
			// strcat(request1,"';");			
			// res=request_select(conn,request1);

			// char hourdata[256]; 
			// check_data_commande(res,hourdata);


			// /*
			// We convert the type of hour into integer
			// Then we request compare the hour by a boundary
			// If the delivery's hour is contained on this boundary the delivery is valid
			// We send a message to the client
			
			// */
			// int h_commande = atoi(hourdata);
			// int h_now = atoi(hour);

			// if((h_now-1<=h_commande)&&(h_commande<=h_now+1)){
			// 	printf("HEURE DE LA COMMANDE: %d\n",h_commande);
			// 	printf("HEURE DU MOMENT: %d\n",h_now);
			// 	strcat(buf,"La commande est en cours de preparation...");

				
			// }
			// else{
			// 	/*
			// 	If the delivery's hour is not contained on this boundary the delivery is not valid
			// 	And we send a message to the client 

			// 	And we delete this delivery from the data base 
			// 	*/
			// 	printf("HEURE DE LA COMMANDE: %d\n",h_commande);
			// 	printf("HEURE DU MOMENT: %d\n",h_now);
			// 	strcat(buf,"La commande à été envoyée !");

			// 	memset (request1, 0, MAX);
			// 	strcpy(request1,"");
			// 	strcat(request1,"DELETE FROM commandes WHERE id='");
			// 	strcat(request1,commande);
			// 	strcat(request1,"';");			
			// 	res=request_select(conn,request1);
			// }

		}
		else{
			/*
			If the delivery's date is not contained on this boundary the delivery is not valid
			And we send a message to the client 
			And we delete this delivery from the data base 
			*/
			printf("DATE DE LA COMMANDE: %s\n",datedata);
			printf("DATE DU JOUR: %s\n",date);
			strcat(buf,"PAS DE DATE POUR CETTE COMMANDE");

			memset (request1, 0, MAX);
			strcpy(request1,"");
			strcat(request1,"DELETE FROM commandes WHERE id='");
			strcat(request1,commande);
			strcat(request1,"';");			
			res=request_select(conn,request1);
		}
	}
	else{
		/*
		If the delivery's number is not contained on this boundary the delivery is not valid
		And we send a message to the client 
		*/
		printf("COMMANDE NON TROUVE\n");
		strcat(buf,"Aucune information disponible, commande non présente !!");
	}
}
//------------------------------------------------------------------------------------------------------------------

void send_buf(char *buf, int s_dial,char *req, char *buf1)
{
		/*
		Initialization of the character's chain buf
		Copy of chain req on buf
		Sending message contains buf to the client
		Notification of the action
		*/
		memset (buf, 0, MAX);
		strcpy(buf,req);
        write (s_dial, buf, strlen (buf)) ;
        printf ("%s",buf1) ;
}

//------------------------------------------------------------------------------------------------------------------
void receipt(char *buf,char *buf1, int s_dial, int s_ecoute, char *request, char *req, char *ticket, PGconn *conn, PGresult *res, int *valid)
{

	//Reception a message from the client
	receipt_buf(buf,s_dial,s_ecoute);


	if (strstr(buf,"COMMANDE")!=0) {
		/*
		If the server receive COMMANDE, the server will send the delivery's list
		The server request the execution to answer by the functions reques_select and display_select
		The server send the message to the client
		valid1 = 1 so the server will receive an action to do again
		*/
		*valid=1;
		strcpy(request,"");
		strcat(request,"SELECT id FROM commandes;");
		res=request_select(conn,request);
		display_select(res,req);
		send_buf(buf,s_dial,req,"VOIR CLIENT POUR REPONSE\n");

	}else if(strstr(buf,"VERIFIER")!=0){
			/*
			If the server receive VERIFIER, he asks to the client the delivery's number
			Then he receives the delivery's number
			Then he checks the validity thanks to the function machine
			Then he send to the client the answer
			valid1 = 1 so the user will can do an action again
			*/

			*valid=1;
			send_buf(buf,s_dial,"Entrez le numéro de la commande à vérifier","Je demande au client la commande\n");

			receipt_buf(buf1,s_dial,s_ecoute);

			memset (request, 0, MAX);
			strcpy(request,"");
			strcat(request,"SELECT id from commandes;");
			res=request_select(conn,request);
			strcpy(commande,"");	
			memset (buf, 0, MAX);	
			strcpy (buf, "") ;
			machine(conn,res,commande,buf1,buf);
      		write (s_dial, buf, strlen (buf)) ;

	}else if(strstr(buf,"FIN")!=0){
			/*
			If the server receive FIN, he will receive what is inside the buf
			This will end the communication 
			valid1 = 0 so the client and the server will close
			*/
			*valid=0;
			send_buf(buf,s_dial,"Fin de la communication","Au revoir\n");
			close(s_dial);
			close(s_ecoute);

	}else{
			/*
			If the server receive anything else, he will send an error
			Then the client will need to send a valid message again 
			valid1 = 1 so the user will can do an action again
			*/
			*valid=1;
			send_buf(buf,s_dial,"Erreur, entrez un mot valide","Erreur de saisie\n");	
	}
}

//-----------------------------------------------------------------------------------------------------------------------------

void check_connection(char *buf, char *buf1, char *buf2, char *userdata, char *passwordata, char *request, int s_dial, PGconn *conn, PGresult *res, int *valid1,int s_ecoute)
{
	//Receive from the client a request for connection
	receipt_buf(buf,s_dial,s_ecoute);

	//Send a message asking the User to the client 
	send_buf(buf,s_dial,"Connectez vous\n","Je demande au client l'utilisateur\n");	

	//Reception of User from the client
	receipt_buf(buf1,s_dial,s_ecoute);

	//Selection of all user contained in the data base
	memset (request, 0, MAX);
	strcpy(request,"");
	strcat(request,"SELECT username FROM users;");
	//Request's execution
	res=request_select(conn,request);
	strcpy(userdata,"");
	//Comparison of the User given by the client with User in data base
	comparison_user(res,userdata,buf1);

	//Send a message asking the password to the client
	send_buf(buf,s_dial,"Mot de passe svp\n","Je demande au client son mot de passe\n");	

	//Reception of the password from the client
	receipt_buf(buf2,s_dial,s_ecoute);

	//Selection of the password of the User given by the client
	memset (request, 0, MAX);
	strcpy(request,"");
	strcat(request,"SELECT password FROM users WHERE username LIKE '");
	strcat(request,buf1);
	strcat(request,"';");
	//Reques's execution
	res=request_select(conn,request);
	strcpy(passwordata,"");

	memset (buf, 0, MAX);
	strcpy (buf, "") ;

	//Comparison of the User given by the client with User in data base
	comparison_password(res,passwordata,buf2,buf);

	//Display a notification on the server
    printf ("Je continue\n") ;

    //Send the answer to the client if he finds the id
    write (s_dial, buf, strlen (buf)) ;
    /*
	If the server said that the id are wrong, he will ask 
	to send again an User and Password (valid=1)
	Else if, the user send any word that the server don't recognize he will
	ask again to send a correct word(valid=1)
	Else the Id are correct so valid=0 and the connection is made
	*/
    if(strstr(buf,"Erreur de connexion, entrez des identifiants valides !!!\n")){
    	*valid1=1;
    }
    else{
    	*valid1=0;
    }
}