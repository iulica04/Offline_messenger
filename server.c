#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <sqlite3.h>
#include <time.h>

#define PORT 2222
extern int errno;

typedef struct thData
{
    int idThread; // id_il thread-ului tinut in evidenta de acest program
    int cl; // descriptorul intors la accept
}thData;

sqlite3 *baza_date;
char *Mesaj_de_eroare;
int cod_returnare;
bool utilizator_gasit = false;
bool parola_gasita = false;
bool conectat = false;
char utilizatori_online[1000];
int contor = 0;
char raspuns_parola[3];
char nr_telefon_gasit = false;
char nume_utilizator_destinatie[100];
char nume_prieten[100];
int descriptor_utilizator;
char mesaj[2000];
char auxiliar[2000];
char istoric[2000] = "";
bool existenta_mesaje_offline = false;
char mesaje_offline[2000];
char id[100];
char id_mesaj[100];
char existenta_id_mesaj = false;
char existenta_corelatie_intre_mesaj_si_utilizator = false;
char utilizator[100];
char data_trimitere[20];
char ora_trimitere[10];
char id_raspuns[100];
char timp[25];
char nume_utilizator_blocat[100];
bool existenta_blocare = false;



int callback(void *nume_utilizator, int argc, char **argv, char **azColName);
int callback1(void *parola, int argc, char **argv, char **azColName) ;
bool verificare_existenta_utilizator(char *nume_utilizator);
void adaugare_utilizator_in_baza_de_date (char *nume_utilizator, char *nr_telefon, char*parola);
bool verificare_parola(char *nume_utilizator, char *parola);
void schimbare_status_offline(char * nume_utilizator);
void schimbare_status_online(char *nume_utilizator, int descriptor);
bool verificare_status(char *nume_utilizator);
void schimbare_parola(char *nume_utilizator, char* parola_noua);
bool verificare_nr_telefon(char *nume_utilizator, char *nr_telefon);
void afisare_descriptor(char *nume_utilizator);
void adaugare_mesaj_in_baza_de_date (char *nume_utilizator1, char *nume_utilizator2, char *mesaj);
void adaugare_mesaj_offline_in_baza_de_date (char *nume_utilizator1, char *nume_utilizator2, char *mesaj);
void afisare_istoric(char *utilizator_sursa, char *utilizator_destinatie);
bool verificare_existenta_mesaje_offline(char *nume_utilizator);
void afisare_mesaje_offline(char *nume_utilizator);
void stergere_descriptor(char *nume_utilizator);
void mutare_mesaje_offline(char *nume_utilizator);
void stergere_mesaje_offline(char *nume_utilizator);
bool verificare_id_mesaj(char *id_mesaj);
bool verificare_daca_utilizatorului_ii_corespunde_mesajul_cu_id(char *nume_utilizator, char *id_mesaj);
void adaugare_raspuns_in_baza_de_date (char *utilizator_sursa, char *nume_utilizator_destinatie, char *mesaj, int id_raspuns);
void adaugare_raspuns_offline_in_baza_de_date (char *nume_utilizator1, char *nume_utilizator2, char *mesaj, char *id_raspuns);
void afisare_data_trimitere(char *id_mesaj);
void afisare_ora_trimitere(char *id_mesaj);
void afisare_utilizatori_online();
void blocare(char *nume_utilizator1, char *nume_utilizator2);
void deblocare(char *nume_utilizarator1, char *nume_utilizator2);
bool verificare_blocare(char *nume_utilizator1, char *nume_utilizator2);


static void *treat(void *); /*functie executata de fiecare thread ce realizeaza comunicarea cu clientii*/
void verificare_comanda(void *);

int main()
{
    printf("[server]Serverul este pornit!\n Asteptam comenzi din partea clientilor.\n");
   
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int server_descriptor;
    pthread_t th[100];
    int i=0;
    int on=1;

    server_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(server_descriptor == -1)
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }

    setsockopt(server_descriptor, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    //reutilizarea unei adrese locale asociate unui socket care a fost recent închis

    bzero(&server_addr, sizeof(server_addr));
    bzero(&client_addr, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if(bind(server_descriptor, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    if(listen(server_descriptor, 5) == -1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    while(1)
    {
        int client_descriptor;
        int length = sizeof(client_addr);
        thData *td;

        printf("[server]Asteptam la portul %d...\n", PORT);
        fflush(stdout);

        if((client_descriptor = accept(server_descriptor, (struct sockaddr *) &client_addr, &length)) < 0)
        {
            perror("[server] Eroare la accept().\n");
            continue;
        }

        td = (struct thData*)malloc(sizeof(struct thData));
        td->idThread = i++;
        td->cl = client_descriptor;

        pthread_create(&th[i], NULL, &treat, td);
    }
};

static void *treat(void *arg)
{
    struct thData threat;
    threat = *((struct thData*)arg);

    printf("[thread %d]Asteptam mesajul....\n", threat.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());
    verificare_comanda((struct thData*)arg);

    //am terminat cu acest client 

    close((intptr_t)arg);
    return(NULL);

};

void verificare_comanda(void *arg)
{
    int i=0;
    char comanda[1000];
    char raspuns[1000] = "";
    int size;
    int cod_read, cod_write;   
    int conectat = 0;
    char nume_utilizator[100];
    char parola[100];
    char parola_noua[100];
    char nr_telefon[13];
    struct thData thread;
    thread = *((struct thData*)arg);

    while(1)
    {
        memset(comanda, 0, sizeof(comanda));
        cod_read = read(thread.cl, &comanda, sizeof(comanda));
        if(cod_read == -1)
        {
            printf("[thread %d]", thread.idThread);
            printf("Eroare la read() de la client.\n");
        }

        comanda[strlen(comanda)] = '\0';

        printf("[thread %d]Am primit comanda: %s\n", thread.idThread, comanda);
        fflush(stdout);
     

        if(strcmp(comanda, "inregistrare") == 0) 
        {
            do{
                memset(raspuns, 0, sizeof(raspuns));
                memset(nume_utilizator, 0, sizeof(nume_utilizator));
                cod_read = read(thread.cl, &nume_utilizator, sizeof(nume_utilizator));
                if(cod_read == -1)
                {
                    perror("[client]Eroare la read() de la server.n");
                    exit(EXIT_FAILURE);
                }
                raspuns[strlen(nume_utilizator)] = '\0';
                printf("[thread %d]Am primit numele de utilizator: %s \n", thread.idThread, nume_utilizator);
                fflush(stdout);

                memset(raspuns, 0, sizeof(raspuns));
                if(verificare_existenta_utilizator(nume_utilizator) == false)
                {
                    strcat(raspuns, "Numele de utilizator ales este disponibil.");
                }
                else
                {
                    strcat(raspuns, "Numele de utilizator ales este indisponibil.Alegeti un alt nume de utilizator.");
                }
                cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                if(cod_write == -1)
                {
                    perror("Eroare la write() catre client.\n");
                    exit(EXIT_FAILURE);
                }
            }while(verificare_existenta_utilizator(nume_utilizator) == true);

            
            do{
                memset(parola, 0, sizeof(parola)); 

                cod_read = read(thread.cl, &parola, sizeof(parola));
                if(cod_read == -1)
                {
                    perror("[client]Eroare la read() de la server.n");
                    exit(EXIT_FAILURE);
                }
                parola[strlen(parola)] = '\0';
                printf("[thread %d]Am primit parola: %s \n", thread.idThread, parola);
                fflush(stdout);
            
                memset(raspuns, 0, sizeof(raspuns));
                if(strchr(parola, ' ') != NULL)
                {
                    strcat(raspuns, "Ai introdus spatii in parola. Incearca o alta parola fara saptii.");
                    printf("[server]Parola incorecta : contine spatii.\n");
                    fflush(stdout);
                }
                else
                {
                    strcat(raspuns, "Ai introdus o parola care respecta regulile aplicatiei.");
                }

                cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                if(cod_write == -1)
                {
                    perror("Eroare la write() catre client.\n");
                    exit(EXIT_FAILURE);
                } 

            }while(strchr(parola, ' ') != NULL);

            do{
                memset(nr_telefon, 0, sizeof(nr_telefon)); 
                 
                cod_read = read(thread.cl, &nr_telefon, sizeof(nr_telefon));
                if(cod_read == -1)
                {
                    perror("[client]Eroare la read() de la server.n");
                    exit(EXIT_FAILURE);
                }
                
                nr_telefon[strlen(nr_telefon)] = '\0';
                printf("[thread %d]Am primit numarul de telefon: %s \n", thread.idThread, nr_telefon);

                memset(raspuns, 0, sizeof(raspuns));
                if(strlen(nr_telefon) != 12)
                {
                     strcat(raspuns, "Numarul de telefon introdus este invalid.");
                }
                else
                {
                    strcat(raspuns, "Numarul de telefon introdus este valid.");
                }
  
                cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                if(cod_write == -1)
                {
                    perror("Eroare la write() catre client.\n");
                    exit(EXIT_FAILURE);
                } 

            }while(strlen(nr_telefon) != 12);

            //acum putem adauga in tabela de utilizatori noul utilizator
            adaugare_utilizator_in_baza_de_date(nume_utilizator, nr_telefon, parola);

            memset(raspuns, 0, sizeof(raspuns));
            strcat(raspuns, "Contul a fost creat cu succes! Acum te poti autentifica in aplicatie.");
            cod_write = write(thread.cl, &raspuns, strlen(raspuns));
            if(cod_write == -1)
            {
                perror("Eroare la write() catre client.\n");
                exit(EXIT_FAILURE);
            } 
        } else if(strcmp(comanda, "conectare") == 0 && verificare_status(nume_utilizator) == false)
        {
            do{
                memset(raspuns, 0, sizeof(raspuns));
                memset(nume_utilizator, 0, sizeof(nume_utilizator));
                cod_read = read(thread.cl, &nume_utilizator, sizeof(nume_utilizator));
                if(cod_read == -1)
                {
                    perror("[client]Eroare la read() de la server.n");
                    exit(EXIT_FAILURE);
                }
                raspuns[strlen(nume_utilizator)] = '\0';
                printf("[thread %d]Am primit numele de utilizator: %s \n", thread.idThread, nume_utilizator);
                fflush(stdout);

                
                memset(raspuns, 0, sizeof(raspuns));
                if(verificare_existenta_utilizator(nume_utilizator) == true)
                {
                    strcat(raspuns, "Numele de utilizator introdus este corect.");
                }
                else
                {
                    strcat(raspuns, "Numele de utilizator introdus este incorect.");
                }
                cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                if(cod_write == -1)
                {
                    perror("Eroare la write() catre client.\n");
                    exit(EXIT_FAILURE);
                }
            }while(verificare_existenta_utilizator(nume_utilizator) == false);


            do{
                memset(parola, 0, sizeof(parola)); 
                cod_read = read(thread.cl, &parola, sizeof(parola));
                if(cod_read == -1)
                {
                    perror("[client]Eroare la read() de la server.n");
                    exit(EXIT_FAILURE);
                }
                parola[strlen(parola)] = '\0';
                printf("[thread %d]Am primit parola: %s \n", thread.idThread, parola);
                fflush(stdout);

                memset(raspuns, 0, sizeof(raspuns));
                if(verificare_parola(nume_utilizator, parola) == true)
                {
                    strcat(raspuns, "Parola introdusa este corecta.");
                }
                else
                {
                    strcat(raspuns, "Parola introdusa nu este corecta.");
                }

                cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                if(cod_write == -1)
                {
                    perror("Eroare la write() catre client.\n");
                    exit(EXIT_FAILURE);
                } 

                if(strcmp(raspuns, "Parola introdusa este corecta.") != 0)
                {
                    memset(raspuns_parola, 0, sizeof(raspuns_parola));
                    cod_read = read(thread.cl, &raspuns_parola, sizeof(raspuns_parola));
                    if(cod_read == -1)
                    {
                        perror("[client]Eroare la read() de la server.\n");
                        exit(EXIT_FAILURE);
                    }
                    raspuns_parola[strlen(raspuns_parola)] = '\0';
                    printf("[thread %d]Am primit raspunsul pentru schimbare parola: %s \n", thread.idThread, raspuns_parola);
                    fflush(stdout);

                    if(strcmp(raspuns_parola, "Da") == 0)
                    {
                        do{
                            memset(nr_telefon, 0, sizeof(nr_telefon));
                            cod_read = read(thread.cl, &nr_telefon, sizeof(nr_telefon));
                            if(cod_read == -1)
                            {   
                                perror("[client]Eroare la read() de la server.\n");
                                exit(EXIT_FAILURE);
                            }
                            nr_telefon[strlen(nr_telefon)] = '\0';
                            printf("[thread %d]Am primit numarul de telefon: %s \n", thread.idThread, nr_telefon);

                            memset(raspuns, 0, sizeof(raspuns));
                            if(verificare_nr_telefon(nume_utilizator, nr_telefon) == true)
                            {
                                strcat(raspuns, "Numarul de telefon este corect.");
                            }
                            else
                            {
                                strcat(raspuns, "Numarul de telefon este incorect.");
                            }
                    
                            cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                            if(cod_write == -1)
                            {
                                perror("Eroare la write() catre client.\n");
                                exit(EXIT_FAILURE);
                            }
                        }while(strcmp(raspuns, "Numarul de telefon este corect.") != 0);

                        printf("[server]Acum clientul isi poate actualiza parola.\n");
                        fflush(stdout);
                        memset(parola_noua, 0, sizeof(parola_noua));
                        cod_read = read(thread.cl, &parola_noua, sizeof(parola_noua));
                        if(cod_read == -1)
                        {
                            perror("[server]Eroare la read() de la client.\n");
                            exit(EXIT_FAILURE);
                        }
                        parola_noua[strlen(parola_noua)] = '\0';
                        printf("[server]Am primit noua parola de la client : %s.\n", parola_noua);

                        schimbare_parola(nume_utilizator, parola_noua );

                        memset(raspuns, 0, sizeof(raspuns));
                        strcat(raspuns, "Actualizarea parolei a reusit.");
                        cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                        if(cod_write == -1)
                        {
                            perror("[server]Eroare la write() catre client.\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }while(strcmp(raspuns, "Parola introdusa este corecta.") != 0);

            schimbare_status_online(nume_utilizator, thread.cl);

            printf("[server]Conectarea la aplicatie a reusit.\n");
            fflush(stdout);
            
            memset(raspuns, 0, sizeof(raspuns));
            if(verificare_existenta_mesaje_offline(nume_utilizator) == true)
            {
                strcat(raspuns, "Ati primit mesaje cat timp ati fost deconectat de la aplicatie. Pentru a le afisa utilizati comanda : afisare mesaje offline");
            }
            else
            {
                strcat(raspuns, "Nu ati primit niciun mesaj cat ati fost deconectat de la aplicatie.");
            }
            cod_write = write(thread.cl, &raspuns, strlen(raspuns));
            if(cod_write == -1)
            {
                perror("[server]Eroare la write() catre client.\n");
                exit(EXIT_FAILURE);
            }

        } else if(strcmp(comanda, "deconectare") == 0)
        {
            if(verificare_status(nume_utilizator) == true)
            {
                schimbare_status_offline(nume_utilizator);

                memset(raspuns, 0, sizeof(raspuns));
                strcat(raspuns, "Deconectare reusita.");
                printf("[thread %d]Deconectare reusita.\n", thread.idThread);
                fflush(stdout);
            }
            else
            {
                memset(raspuns, 0, sizeof(raspuns));
                strcat(raspuns, "Deconectare esuata. Nu sunteti conectat la aplicatie.");
                printf("[thread %d]Deconectare esuata.\n", thread.idThread);
                fflush(stdout);
            }

            cod_write = write(thread.cl, &raspuns, strlen(raspuns));
            if(cod_write == -1)
            {
                perror("Eroare la write() catre client.\n");
                exit(EXIT_FAILURE);
            } 

            stergere_descriptor(nume_utilizator);

        } else if(strcmp(comanda, "iesire") == 0)
        {

            if(verificare_status(nume_utilizator) == true)
            {
                schimbare_status_offline(nume_utilizator);
            }
            printf("[thread %d]Utilizatorul %s a iesit din aplicatie.\n", thread.idThread, nume_utilizator);
            fflush(stdout);

            stergere_descriptor(nume_utilizator);

            return; 

        } else if(strcmp(comanda, "schimbare parola") == 0)
        {
            memset(parola_noua, 0, sizeof(parola_noua));
            cod_read = read(thread.cl, &parola_noua, sizeof(parola_noua));
            if(cod_read == -1)
            {
                perror("[server]Eroare la read() de la client.\n");
                exit(EXIT_FAILURE);
            }
            parola_noua[strlen(parola_noua)] = '\0';

            printf("[server]Am primit noua parola de la client : %s.\n", parola_noua);

            schimbare_parola(nume_utilizator, parola_noua );

            memset(raspuns, 0, sizeof(raspuns));
            strcat(raspuns, "Actualizarea parolei a reusit.");
            cod_write = write(thread.cl, &raspuns, strlen(raspuns));
            if(cod_write == -1)
            {
                perror("[server]Eroare la write() catre client.\n");
                exit(EXIT_FAILURE);
            }
        } else if(strcmp(comanda, "trimite mesaj") == 0)
        {
            memset(nume_utilizator_destinatie, 0, sizeof(nume_utilizator_destinatie));
            cod_read = read(thread.cl, &nume_utilizator_destinatie, sizeof(nume_utilizator_destinatie));
            if(cod_read == -1)
            {
                perror("[server]Eroare la read() de la client.\n");
                exit(EXIT_FAILURE);
            }
            nume_utilizator_destinatie[strlen(nume_utilizator_destinatie)] = '\0';

            if(verificare_blocare(nume_utilizator, nume_utilizator_destinatie) == false)
            {
                memset(raspuns, 0, sizeof(raspuns));
                if(verificare_existenta_utilizator(nume_utilizator_destinatie) == true)
                {
                    strcat(raspuns, "Utilizator gasit.");

                    cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                    if(cod_write == -1)
                    {
                        perror("[server]Eroare la write() catre client.\n");
                        exit(EXIT_FAILURE);
                    }

                    memset(mesaj, 0, sizeof(mesaj));
                    cod_read = read(thread.cl, &mesaj, sizeof(mesaj));
                    if(cod_read == -1)
                    {   
                        perror("[server]Eroare la read() de la client.\n");
                        exit(EXIT_FAILURE);
                    }
                    mesaj[strlen(mesaj)] = '\0';

                    printf("[server]Am primit mesajul: %s\n", mesaj);

                    if(verificare_status(nume_utilizator_destinatie) == true) //adica conectat
                    {
                        afisare_descriptor(nume_utilizator_destinatie);
                        printf("[server]Clientul %s are descriptorul asociat %d.\n", nume_utilizator_destinatie, descriptor_utilizator);

                        adaugare_mesaj_in_baza_de_date(nume_utilizator, nume_utilizator_destinatie, mesaj);

                        strcpy(auxiliar, mesaj);
                        memset(mesaj, 0, sizeof(mesaj));
                        strcat(mesaj, id);
                        strcat(mesaj, " [");
                        strcat(mesaj, nume_utilizator);
                        strcat(mesaj, "]");
                        strcat(mesaj, auxiliar);

                        cod_write = write(descriptor_utilizator, &mesaj, strlen(mesaj));
                        if(cod_write == -1)
                        {
                            perror("[server]Eroare la write catre client.\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        adaugare_mesaj_offline_in_baza_de_date(nume_utilizator, nume_utilizator_destinatie, mesaj);
                    }
                }
                else
                {
                    strcat(raspuns, "Utilizatorul nu exista.");
                    cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                    if(cod_write == -1)
                    {
                        perror("[server]Eroare la write() catre client.\n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                memset(raspuns, 0, sizeof(raspuns));
                snprintf(raspuns, sizeof(raspuns), "Nu puteti trimite mesaje utilizatorului %s. Daca l-ati blocat, incercati optiunea de deblocare.", nume_utilizator_destinatie);

                cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                if(cod_write == -1)
                {
                    perror("[server]Eroare la write() catre client.\n");
                    exit(EXIT_FAILURE);
                }

            }

        } else if(strcmp(comanda, "afisare istoric conversatie") == 0)
        {
            memset(nume_prieten, 0, sizeof(nume_prieten));
            memset(istoric, 0, sizeof(istoric));
            memset(raspuns, 0, sizeof(raspuns));

            cod_read = read(thread.cl, &nume_prieten, sizeof(nume_prieten));
            if(cod_read == -1)
            {
                perror("[server]Eroare la read() de la client.\n");
                exit(EXIT_FAILURE);
            }
            nume_prieten[strlen(nume_prieten)] = '\0';

            if(verificare_existenta_utilizator(nume_prieten) == false)
            {
                strcat(raspuns, "Numele de utilizator introdus nu exista in baza de date a aplicatiei.");
                printf("[server]Numele de utilizator introdus nu exista in baza de date a aplicatiei.");
            }
            else
            {
                char auxiliar[2000];
                afisare_istoric(nume_utilizator, nume_prieten);
                strcat(raspuns, istoric);
                memset(istoric, 0, sizeof(istoric));
                afisare_istoric( nume_prieten, nume_utilizator);
                strcat(raspuns, istoric);
                printf("[server]Istoric: \n%s", raspuns);
            }

            cod_write = write(thread.cl, &raspuns, strlen(raspuns));
            if(cod_write == -1)
            {
                perror("[server]Eroare la write() catre client.\n");
                exit(EXIT_FAILURE);
            }
        } else if(strcmp(comanda, "afisare mesaje offline") == 0)
        {
            if(verificare_status(nume_utilizator) == true)
            {
                memset(mesaje_offline, 0, sizeof(mesaje_offline));
                memset(raspuns, 0, sizeof(raspuns));
                afisare_mesaje_offline(nume_utilizator);
                strcat(raspuns, mesaje_offline);
                printf("[server] Mesajele offline ale utilizatorului %s : %s", nume_utilizator, mesaje_offline);

                cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                if(cod_write == -1)
                {
                    perror("[server]Eroare la write() catre client.\n"); 
                }

                // adaugam in tabela mesaje, mesajele offline
                mutare_mesaje_offline(nume_utilizator);
                stergere_mesaje_offline(nume_utilizator);
            }
        } else if(strcmp(comanda, "raspunde") == 0)
        {
            if(verificare_status(nume_utilizator) == true)
            {
                memset(id_mesaj, 0, sizeof(id_mesaj));
                memset(raspuns, 0, sizeof(raspuns));
                memset(utilizator, 0, sizeof(utilizator));
                cod_read = read(thread.cl, &id_mesaj, sizeof(id_mesaj));
                if(cod_read == -1)
                {
                    perror("[server]Eroare la read() de la client.\n");
                    exit(EXIT_FAILURE);
                }
                id_mesaj[strlen(id_mesaj)] = '\0';

                if(verificare_id_mesaj(id_mesaj) == true)
                {
                    if(verificare_daca_utilizatorului_ii_corespunde_mesajul_cu_id(nume_utilizator, id_mesaj) == true)
                    {
                        strcat(raspuns, "Id-ul introdus apartine unei conversatii care va apartine.");

                        cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                        if(cod_write == -1)
                        {
                            perror("[server]Eroare la write() catre client.\n"); 
                        }

                        memset(mesaj, 0, sizeof(mesaj));
                        cod_read = read(thread.cl, &mesaj, sizeof(mesaj));
                        if(cod_read == -1)
                        {
                            perror("[server]Eroare la read() de la client.\n");
                            exit(EXIT_FAILURE);
                        }
                        mesaj[strlen(mesaj)] = '\0';

                        printf("[server]Am primit raspunsul: %s\n", mesaj);

                        if(verificare_status(utilizator) == true)
                        {
                            afisare_descriptor(utilizator);
                            printf("[server]Clientul %s are descriptorul asociat %d.\n", utilizator, descriptor_utilizator);

                            adaugare_raspuns_in_baza_de_date(nume_utilizator, utilizator, mesaj, atoi(id_mesaj));

                            strcpy(auxiliar, mesaj);
                            memset(mesaj, 0, sizeof(mesaj));
                            strcat(mesaj, id);
                            strcat(mesaj, "->");
                            strcat(mesaj, id_mesaj);
                            strcat(mesaj, " [");
                            strcat(mesaj, nume_utilizator);
                            strcat(mesaj, "]");
                            strcat(mesaj, auxiliar);

                            cod_write = write(descriptor_utilizator, &mesaj, strlen(mesaj));
                            if(cod_write == -1)
                            {
                                perror("[server]Eroare la write catre client.\n");
                                exit(EXIT_FAILURE);
                            }   

                        }
                        else
                        {
                            adaugare_raspuns_offline_in_baza_de_date(nume_utilizator, utilizator, mesaj, id_mesaj);
                        }
                    }
                    else
                    {
                        strcat(raspuns, "Id-ul introdus nu apartine unei conversatii care va apartine.");

                        cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                        if(cod_write == -1)
                        {
                            perror("[server]Eroare la write() catre client.\n"); 
                        }
                    } 
                }
                else
                {
                    strcat(raspuns, "In baza de date, nu exista niciun mesaj caruia sa ii corespunda id-ul introdus de dumneavoastra.");
                    cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                    if(cod_write == -1)
                    {
                        perror("[server]Eroare la write() catre client.\n"); 
                    }
                }
            } 
        } else if(strcmp(comanda, "afisare data trimitere mesaj") == 0)
        {
            if(verificare_status(nume_utilizator) == true)
            {
                memset(id_mesaj, 0, sizeof(id_mesaj));
                memset(raspuns, 0, sizeof(raspuns));
                memset(data_trimitere, 0, sizeof(data_trimitere));

                cod_read = read(thread.cl, &id_mesaj, sizeof(id_mesaj));
                if(cod_read == -1)
                {
                    perror("[server]Eroare la read() de la client.\n");
                    exit(EXIT_FAILURE);
                }
                id_mesaj[strlen(id_mesaj)] = '\0';

                printf("[server]Am primit id-ul %s.\n", id_mesaj);

                if(verificare_id_mesaj(id_mesaj) == true)
                {
                    if(verificare_daca_utilizatorului_ii_corespunde_mesajul_cu_id(nume_utilizator, id_mesaj) == true)
                    {
                        strcat(raspuns, "Id-ul introdus pentru afisarea datei trimiterii apartine unei conversatii care va apartine.");

                        cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                        if(cod_write == -1)
                        {
                            perror("[server]Eroare la write() catre client.\n"); 
                        }

                        afisare_data_trimitere(id_mesaj);

                        printf("[server]Data trimiterii mesajului cu id-ul %s este %s.\n", id_mesaj, data_trimitere);
                        memset(raspuns, 0, sizeof(raspuns));
                        snprintf(raspuns, sizeof(raspuns), "Data trimiterii mesajului cu id-ul %s este %s", id_mesaj, data_trimitere);
                        raspuns[strlen(raspuns)] = '\0';

                        cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                        if(cod_write == -1)
                        {
                            perror("[server]Eroare la write() catre client.\n"); 
                        }

                    }
                    else
                    {
                        strcat(raspuns, "Id-ul introdus pentru afisarea datei trimiterii nu apartine unei conversatii care va apartine.");

                        cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                        if(cod_write == -1)
                        {
                            perror("[server]Eroare la write() catre client.\n"); 
                        }
                    }
                } 
                else
                {
                    strcat(raspuns, "In baza de date, nu exista niciun mesaj caruia sa ii corespunda id-ul introdus de dumneavoastra.");
                    cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                    if(cod_write == -1)
                    {
                        perror("[server]Eroare la write() catre client.\n"); 
                    }
                }
            }
        } else if(strcmp(comanda, "afisare ora trimitere mesaj") == 0)
        {
            if(verificare_status(nume_utilizator) == true)
            {
                memset(id_mesaj, 0, sizeof(id_mesaj));
                memset(raspuns, 0, sizeof(raspuns));
                memset(ora_trimitere, 0, sizeof(ora_trimitere));

                cod_read = read(thread.cl, &id_mesaj, sizeof(id_mesaj));
                if(cod_read == -1)
                {
                    perror("[server]Eroare la read() de la client.\n");
                    exit(EXIT_FAILURE);
                }
                id_mesaj[strlen(id_mesaj)] = '\0';

                printf("[server]Am primit id-ul %s.\n", id_mesaj);

                if(verificare_id_mesaj(id_mesaj) == true)
                {
                    if(verificare_daca_utilizatorului_ii_corespunde_mesajul_cu_id(nume_utilizator, id_mesaj) == true)
                    {
                        strcat(raspuns, "Id-ul introdus pentru afisarea orei trimiterii apartine unei conversatii care va apartine.");

                        cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                        if(cod_write == -1)
                        {
                            perror("[server]Eroare la write() catre client.\n"); 
                        }

                        afisare_ora_trimitere(id_mesaj);

                        printf("[server]Ora trimiterii mesajului cu id-ul %s este %s.\n", id_mesaj, ora_trimitere);
                        memset(raspuns, 0, sizeof(raspuns));
                        snprintf(raspuns, sizeof(raspuns), "Ora trimiterii mesajului cu id-ul %s este %s", id_mesaj, ora_trimitere);
                        raspuns[strlen(raspuns)] = '\0';

                        cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                        if(cod_write == -1)
                        {
                            perror("[server]Eroare la write() catre client.\n"); 
                        }

                    }
                    else
                    {
                        strcat(raspuns, "Id-ul introdus pentru afisarea orei trimiterii nu apartine unei conversatii care va apartine.");

                        cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                        if(cod_write == -1)
                        {
                            perror("[server]Eroare la write() catre client.\n"); 
                        }
                    }
                } 
                else
                {
                    strcat(raspuns, "In baza de date, nu exista niciun mesaj caruia sa ii corespunda id-ul introdus de dumneavoastra.");
                    cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                    if(cod_write == -1)
                    {
                        perror("[server]Eroare la write() catre client.\n"); 
                        exit(EXIT_FAILURE);
                    }
                }
            }
        } else if(strcmp(comanda, "afisare utilizatori online") == 0)
        {
            if(verificare_status(nume_utilizator) == true)
            {
                afisare_utilizatori_online();

                printf("[server]Utilizatori onlie: \n %s", utilizatori_online);

                cod_write = write(thread.cl, &utilizatori_online, strlen(utilizatori_online));
                if(cod_write == -1)
                {
                    perror("[server]Eroare la write() catre client.\n"); 
                    exit(EXIT_FAILURE);
                }
            }
        } else if(strcmp(comanda, "blocare") == 0)
        {
            if(verificare_status(nume_utilizator) == true)
            {
                memset(nume_utilizator_blocat, 0, sizeof(nume_utilizator_blocat));
                memset(raspuns, 0, sizeof(raspuns));

                cod_read = read(thread.cl, &nume_utilizator_blocat, sizeof(nume_utilizator_blocat));
                if(cod_read == -1)
                {
                    perror("[server]Eroare la read() de la client.\n");
                    exit(EXIT_FAILURE);
                }
                nume_utilizator_blocat[strlen(nume_utilizator_blocat)] = '\0';
                printf("[server]Am primit numele pentru blocare: %s\n", nume_utilizator_blocat);

                if(verificare_existenta_utilizator(nume_utilizator_blocat) == true)
                {
                    blocare(nume_utilizator, nume_utilizator_blocat);
                    memset(raspuns, 0, sizeof(raspuns));
                    strcat(raspuns, "Blocare reusita.");
                        
                    cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                    if(cod_write == -1)
                    {
                        perror("[server]Eroare la write() catre client.\n"); 
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    strcat(raspuns, "Numele de utilizator introdus nu exista.");

                    cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                    if(cod_write == -1)
                    {
                        perror("[server]Eroare la write() catre client.\n"); 
                        exit(EXIT_FAILURE);
                    }
                }
            }
        } else if(strcmp(comanda, "deblocare") == 0)
        {
            if(verificare_status(nume_utilizator) == true)
            {
                memset(nume_utilizator_blocat, 0, sizeof(nume_utilizator_blocat));
                memset(raspuns, 0, sizeof(raspuns));

                cod_read = read(thread.cl, &nume_utilizator_blocat, sizeof(nume_utilizator_blocat));
                if(cod_read == -1)
                {
                    perror("[server]Eroare la read() de la client.\n");
                    exit(EXIT_FAILURE);
                }
                nume_utilizator_blocat[strlen(nume_utilizator_blocat)] = '\0';
                printf("[server]Am primit numele pentru deblocare: %s\n", nume_utilizator_blocat);

                if(verificare_existenta_utilizator(nume_utilizator_blocat) == true)
                {
                    if(verificare_blocare(nume_utilizator, nume_utilizator_blocat) == true)
                    {
                        deblocare(nume_utilizator, nume_utilizator_blocat);
                        memset(raspuns, 0, sizeof(raspuns));
                        strcat(raspuns, "Deblocare reusita.");
                        
                        cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                        if(cod_write == -1)
                        {
                            perror("[server]Eroare la write() catre client.\n"); 
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        memset(raspuns, 0, sizeof(raspuns));
                        snprintf(raspuns, sizeof(raspuns), "Deblocare esuata. NU exista blocare intre dumneavoastra si utilizatorul %s.", nume_utilizator_blocat);

                        cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                        if(cod_write == -1)
                        {
                            perror("[server]Eroare la write() catre client.\n"); 
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                else
                {
                    strcat(raspuns, "Numele de utilizator introdus nu exista.");

                    cod_write = write(thread.cl, &raspuns, strlen(raspuns));
                    if(cod_write == -1)
                    {
                        perror("[server]Eroare la write() catre client.\n"); 
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
}


int callback(void *nume_utilizator, int argc, char **argv, char **azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        if(argv[i] != NULL)
        {
            if(strcmp(argv[i], nume_utilizator) == 0)
            {
                utilizator_gasit = true;
            }
        }
    }
    return 0;
};

int callback1(void *parola, int argc, char **argv, char **azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        if(argv[i] != NULL)
        {
            if(strcmp(argv[i], parola) == 0)
            {
                parola_gasita = true; 
            }
        }
    }
    return 0;
};

int callback2(void *data, int argc, char **argv, char **azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        if(argv[i] != NULL)
        {
               if(strcmp(argv[i], "online") == 0)
               {
                conectat = true;
               } else 
               {
                conectat = false;
               }
        }
    }
    return 0;
};

int callback3(void *data, int argc, char **argv, char **azColName) 
{
     for (int i = 0; i < argc; i++) 
    {
        if(argv[i] != NULL)
        {
           strcat(utilizatori_online, argv[i]);
           strcat(utilizatori_online, "\n");
        }
    }
    return 0;
};

int callback4(void *nr_telefon, int argc, char **argv, char **azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        if(argv[i] != NULL)
        {
            if(strcmp(argv[i], nr_telefon) == 0)
            {
                nr_telefon_gasit = true; 
            }
        }
    }
    return 0;
};

int callback5(void *data, int argc, char **argv, char **azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        descriptor_utilizator = atoi(argv[i]);
    }
    return 0;
};

int callback6(void*data, int argc, char **argv, char **azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        if(strcmp(azColName[i], "id_mesaj") == 0)
        {
            strcat(istoric, argv[i]);
            strcat(istoric, ": ");
        } else if(strcmp(azColName[i], "utilizator_sursa") == 0)
        {
            strcat(istoric, "[");
            strcat(istoric, argv[i]);
            strcat(istoric, "-");
        } else if(strcmp(azColName[i], "utilizator_destinatie") == 0)
        {
            strcat(istoric, argv[i]);
            strcat(istoric, "] : ");
        } else if(strcmp(azColName[i], "mesaj") == 0)
        {
            strcat(istoric, argv[i]);
        }
        
    }
    strcat(istoric, "\n");
    return 0;
};

int callback7(void *data, int argc, char **argv, char **azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        if(argv[i] != NULL)
        {
            existenta_mesaje_offline = true;
        }
    }
    return 0;
};

int callback8(void *data, int argc, char **argv, char **azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        if(strcmp(azColName[i], "id") == 0)
        {
            strcat(mesaje_offline, argv[i]);
            strcat(mesaje_offline, ": ");
        } else if(strcmp(azColName[i], "utilizator_sursa") == 0)
        {
            strcat(mesaje_offline, "[");
            strcat(mesaje_offline, argv[i]);
            strcat(mesaje_offline, "] : ");
        }  else if(strcmp(azColName[i], "mesaj") == 0)
        {
            strcat(mesaje_offline, argv[i]);
        }
        
    }
    strcat(mesaje_offline, "\n");
    return 0;
}

int callback9(void *id_mesaj, int argc, char **argv, char **azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        if(argv[i] != NULL)
        {
            if(strcmp(argv[i], id_mesaj) == 0)
            {
                existenta_id_mesaj = true;
            }
        }
    }
    return 0;
};

int callback10(void *nume_utilizator, int argc, char **argv, char **azColName)
{
    
    for(int i=0; i<argc; i++)
    {
        if(argv[i] != NULL)
        {
            if(strcmp(azColName[i], "utilizator_sursa") == 0)
            {
                if(strcmp(argv[i], nume_utilizator) != 0)
                {
                    if(strcmp(utilizator, argv[i]) != 0)
                    {
                        strcat(utilizator, argv[i]);
                    }
                }
            }else if(strcmp(azColName[i], "utilizator_destinatie") == 0)
            {
                if(strcmp(argv[i], nume_utilizator) != 0)
                {
                    if(strcmp(utilizator, argv[i]) != 0)
                    {
                        strcat(utilizator, argv[i]);
                    }
                    
                }
            } else if(strcmp(azColName[i], "id_mesaj") == 0)
            {
                existenta_corelatie_intre_mesaj_si_utilizator = true;
            }
        }
    }
    return 0;
}

int callback11(void *data, int argc, char **argv, char **azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        if(argv[i] != NULL)
        {
            strcat(data_trimitere, argv[i]);
        }
    }
    return 0;
};

int callback12(void *data, int argc, char **argv, char **azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        if(argv[i] != NULL)
        {
            strcat(ora_trimitere, argv[i]);
        }
    }
    return 0;
};

int callback13(void *nume_utilizator, int argc, char **argv, char **azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        if(strcmp(nume_utilizator, argv[i]) == 0)
        {
            existenta_blocare = true;
        }
    }
    return 0;
};



bool verificare_existenta_utilizator(char *nume_utilizator)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[100];
    snprintf(comanda_sql, sizeof(comanda_sql), "SELECT nume_utilizator FROM Utilizatori WHERE nume_utilizator = '%s';", nume_utilizator);

    utilizator_gasit = false;
    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback, nume_utilizator, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    if(utilizator_gasit == true)
    {
        printf("[server]Am gasit numele %s in baza de date Utilizatori.\n", nume_utilizator);
        return true;
    }
    else
    {
        printf("[server]Nu am gasit numele %s in baza de date Utilizatori.\n", nume_utilizator);
        return false;
    }

    sqlite3_close(baza_date);
}

void adaugare_utilizator_in_baza_de_date (char *nume_utilizator, char *nr_telefon, char*parola)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char status[] = "offline";
    char comanda_sql[150];
    snprintf(comanda_sql, sizeof(comanda_sql),"INSERT INTO Utilizatori (nume_utilizator, parola, numar_telefon, status) VALUES('%s', '%s', '%s', '%s');", nume_utilizator, parola, nr_telefon, status);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, NULL, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);
}

bool verificare_parola(char *nume_utilizator, char * parola)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[100];
    snprintf(comanda_sql, sizeof(comanda_sql), "SELECT parola FROM Utilizatori WHERE nume_utilizator = '%s';", nume_utilizator);

    parola_gasita = false;
    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback1, parola, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    if(parola_gasita == true)
    {
        printf("[server]Parola %s corespune utilizatorului %s.\n",parola, nume_utilizator);
        return true;
    }
    else
    {
        printf("[server]Parola %s nu corespune utilizatorului %s.\n",parola, nume_utilizator);
        return false;
    }

    sqlite3_close(baza_date);
}

void schimbare_status_online(char *nume_utilizator, int descriptor)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[100];
    snprintf(comanda_sql, sizeof(comanda_sql), "UPDATE Utilizatori SET status = 'online', descriptor = %d WHERE nume_utilizator = '%s'",descriptor, nume_utilizator);
    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_free(Mesaj_de_eroare);
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);

    printf("[baza de date]Statusul utilizatorului %s a devenit online.\n", nume_utilizator);
}

void schimbare_status_offline(char *nume_utilizator)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[100];
    snprintf(comanda_sql, sizeof(comanda_sql), "UPDATE Utilizatori SET status = 'offline' WHERE nume_utilizator = '%s'", nume_utilizator);
    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_free(Mesaj_de_eroare);
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);

    printf("[baza de date]Statusul utilizatorului %s a devenit offline.\n", nume_utilizator);
}

bool verificare_status(char *nume_utilizator)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[100];
    snprintf(comanda_sql, sizeof(comanda_sql), "SELECT status FROM Utilizatori WHERE nume_utilizator = '%s';", nume_utilizator);

    conectat = false;
    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback2, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);

    if(conectat == true)
    {
        printf("[server]Statusul utilizatorului %s este 'online'.\n", nume_utilizator);
        return true;
    }
    else
    {
        printf("[server]Statusul utilizatorului %s este 'offline'.\n", nume_utilizator);
        return false;
    }
}


void schimbare_parola(char *nume_utilizator, char* parola_noua)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }


    char comanda_sql[100];
    snprintf(comanda_sql, sizeof(comanda_sql), "UPDATE Utilizatori SET parola = '%s' WHERE nume_utilizator = '%s'", parola_noua, nume_utilizator);
    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_free(Mesaj_de_eroare);
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);

    printf("[baza de date]Am actualizat parola cu succes.\n");
    fflush(stdout);
}

bool verificare_nr_telefon(char *nume_utilizator, char *nr_telefon)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[100];
    snprintf(comanda_sql, sizeof(comanda_sql), "SELECT numar_telefon FROM Utilizatori WHERE nume_utilizator = '%s';", nume_utilizator);

    nr_telefon_gasit = false;
    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback4, nr_telefon, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    if(nr_telefon_gasit == true)
    {
        printf("[server]Numarul de telefon %s corespune utilizatorului %s.\n",nr_telefon, nume_utilizator);
        return true;
    }
    else
    {
        printf("[server]Numarl de telefon %s nu corespune utilizatorului %s.\n",nr_telefon, nume_utilizator);
        return false;
    }

    sqlite3_close(baza_date);
}

void afisare_descriptor(char *nume_utilizator)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[100];
    snprintf(comanda_sql, sizeof(comanda_sql), "SELECT descriptor FROM Utilizatori WHERE nume_utilizator = '%s';", nume_utilizator);


    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback5, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

     sqlite3_close(baza_date);
}

void adaugare_mesaj_in_baza_de_date (char *nume_utilizator1, char *nume_utilizator2, char *mesaj)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    memset(timp, 0, sizeof(timp));
    time_t timp_curent = time(NULL);
    strftime(timp, sizeof(timp), "%Y-%m-%d %H:%M:%S", localtime(&timp_curent));

    memset(data_trimitere, 0, sizeof(data_trimitere));
    strncpy(data_trimitere, timp, 10);

    memset(ora_trimitere, 0, sizeof(ora_trimitere));
    strcpy(ora_trimitere, timp + 11);
    

    char comanda_sql[2000];
    snprintf(comanda_sql, sizeof(comanda_sql),"INSERT INTO Mesaje (utilizator_sursa, utilizator_destinatie, mesaj, data_trimitere, ora_trimitere) VALUES('%s', '%s', '%s', '%s', '%s');", nume_utilizator1, nume_utilizator2, mesaj, data_trimitere, ora_trimitere);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, NULL, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    memset(id, 0, sizeof(id));
    sqlite3_int64 ultimul_id = sqlite3_last_insert_rowid(baza_date);
    sprintf(id, "%lld", ultimul_id);

    sqlite3_close(baza_date);
}

void adaugare_mesaj_offline_in_baza_de_date (char *nume_utilizator1, char *nume_utilizator2, char *mesaj)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }


    memset(timp, 0, sizeof(timp));
    time_t timp_curent = time(NULL);
    strftime(timp, sizeof(timp), "%Y-%m-%d %H:%M:%S", localtime(&timp_curent));

    memset(data_trimitere, 0, sizeof(data_trimitere));
    strncpy(data_trimitere, timp, 10);

    memset(ora_trimitere, 0, sizeof(ora_trimitere));
    strcpy(ora_trimitere, timp + 11);

    char comanda_sql[2000];
    snprintf(comanda_sql, sizeof(comanda_sql),"INSERT INTO Mesaje_offline (utilizator_sursa, utilizator_destinatie, mesaj, data_trimitere, ora_trimitere) VALUES('%s', '%s', '%s', '%s', '%s');", nume_utilizator1, nume_utilizator2, mesaj, data_trimitere, ora_trimitere);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, NULL, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }
}

void afisare_istoric(char *utilizator_sursa, char *utilizator_destinatie)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    
    char comanda_sql[200];
    snprintf(comanda_sql, sizeof(comanda_sql),"SELECT id_mesaj, utilizator_sursa, utilizator_destinatie, mesaj FROM Mesaje WHERE utilizator_sursa = '%s' AND utilizator_destinatie = '%s';", utilizator_sursa, utilizator_destinatie);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback6, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    istoric[strlen(istoric)] = '\0';

    sqlite3_close(baza_date);
}

bool verificare_existenta_mesaje_offline(char *nume_utilizator)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    existenta_mesaje_offline = false;
    char comanda_sql[150];
    snprintf(comanda_sql, sizeof(comanda_sql),"SELECT mesaj FROM Mesaje_offline WHERE utilizator_destinatie = '%s';", nume_utilizator);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback7, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);

    if(existenta_mesaje_offline == true)
    {
        printf("[server]Utilizatorul %s a primit mesaje cat timp a fost deconectata de la aplicatie.\n", nume_utilizator);
        return true;
    }
    else
    {
        printf("[server]Utilizatorul %s nu a primit mesaje cat timp a fost deconectat de la aplicatie.\n", nume_utilizator);
        return false;
    }
}

void stergere_descriptor(char *nume_utilizator)
{

    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[150];
    snprintf(comanda_sql, sizeof(comanda_sql),"UPDATE Utilizatori SET descriptor = NULL WHERE nume_utilizator = '%s'", nume_utilizator);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, NULL, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);
}

void afisare_mesaje_offline(char *nume_utilizator)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    
    char comanda_sql[200];
    snprintf(comanda_sql, sizeof(comanda_sql),"SELECT id, utilizator_sursa, utilizator_destinatie, mesaj FROM Mesaje_offline WHERE utilizator_destinatie = '%s';", nume_utilizator);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback8, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    mesaje_offline[strlen(mesaje_offline)] = '\0';

    sqlite3_close(baza_date);
}

void mutare_mesaje_offline(char *nume_utilizator)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    
    char comanda_sql[300];
    snprintf(comanda_sql, sizeof(comanda_sql),"INSERT INTO Mesaje (utilizator_sursa, utilizator_destinatie, mesaj, data_trimitere, ora_trimitere, id_raspuns) SELECT utilizator_sursa, utilizator_destinatie, mesaj, data_trimitere, ora_trimitere, id_raspuns FROM Mesaje_offline WHERE utilizator_destinatie = '%s';", nume_utilizator);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, NULL, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);
}

void stergere_mesaje_offline(char *nume_utilizator)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    
    char comanda_sql[250];
    snprintf(comanda_sql, sizeof(comanda_sql),"DELETE FROM Mesaje_offline WHERE utilizator_destinatie = '%s';", nume_utilizator);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, NULL, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);
}

bool verificare_id_mesaj(char *id_mesaj)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    existenta_id_mesaj = false;
    char comanda_sql[250];
    snprintf(comanda_sql, sizeof(comanda_sql),"SELECT id_mesaj FROM Mesaje;");

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback9, id_mesaj, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);

    if(existenta_id_mesaj == true)
    {
        printf("[baza de date]Id-ul %s corespunde unui mesaj din baza de date.\n", id_mesaj);
        return true;
    }
    else
    {
        printf("[baza de date]Id-ul %s nu corespunde unui mesaj din baza de date.\n", id_mesaj);
        return false;
    }
}

bool verificare_daca_utilizatorului_ii_corespunde_mesajul_cu_id(char *nume_utilizator, char *id_mesaj)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    memset(utilizator, 0, sizeof(utilizator));
    existenta_corelatie_intre_mesaj_si_utilizator = false;
    char comanda_sql[250];
    snprintf(comanda_sql, sizeof(comanda_sql),"SELECT id_mesaj, utilizator_destinatie, utilizator_sursa FROM Mesaje WHERE (utilizator_sursa = '%s' OR utilizator_destinatie = '%s') AND id_mesaj = %d;", nume_utilizator, nume_utilizator, atoi(id_mesaj));

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback10, nume_utilizator, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }
    sqlite3_close(baza_date);

    if(existenta_corelatie_intre_mesaj_si_utilizator == true)
    {
        printf("[baza de date]Id-ul %s corespunde unui mesaj primit sau trimis de utilizatorul %s.\n", id_mesaj, nume_utilizator);
        return true;
    }
    else
    {
        printf("[baza de date]Id-ul %s nu corespunde unui mesaj primit sau trimis de utilizatorul %s.\n", id_mesaj, nume_utilizator);
        return false;
    }

    
}

void adaugare_raspuns_in_baza_de_date(char *utilizator_sursa, char *utilizator_destinatie, char *mesaj, int id_raspuns)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    memset(timp, 0, sizeof(timp));
    time_t timp_curent = time(NULL);
    strftime(timp, sizeof(timp), "%Y-%m-%d %H:%M:%S", localtime(&timp_curent));

    memset(data_trimitere, 0, sizeof(data_trimitere));
    strncpy(data_trimitere, timp, 10);

    memset(ora_trimitere, 0, sizeof(ora_trimitere));
    strcpy(ora_trimitere, timp + 11);

    char comanda_sql[2000];
    snprintf(comanda_sql, sizeof(comanda_sql),"INSERT INTO Mesaje (utilizator_sursa, utilizator_destinatie, mesaj, data_trimitere, ora_trimitere, id_raspuns) VALUES('%s', '%s', '%s', '%s', '%s', %d);", utilizator_sursa, utilizator_destinatie, mesaj, data_trimitere, ora_trimitere, id_raspuns);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, NULL, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare add replay: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    memset(id, 0, sizeof(id));
    sqlite3_int64 ultimul_id = sqlite3_last_insert_rowid(baza_date);
    sprintf(id, "%lld", ultimul_id);

    sqlite3_close(baza_date);
}

void adaugare_raspuns_offline_in_baza_de_date (char *nume_utilizator1, char *nume_utilizator2, char *mesaj, char *id_raspuns)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }


    memset(timp, 0, sizeof(timp));
    time_t timp_curent = time(NULL);
    strftime(timp, sizeof(timp), "%Y-%m-%d %H:%M:%S", localtime(&timp_curent));

    memset(data_trimitere, 0, sizeof(data_trimitere));
    strncpy(data_trimitere, timp, 10);

    memset(ora_trimitere, 0, sizeof(ora_trimitere));
    strcpy(ora_trimitere, timp + 11);

    char comanda_sql[2000];
    snprintf(comanda_sql, sizeof(comanda_sql),"INSERT INTO Mesaje_offline (utilizator_sursa, utilizator_destinatie, mesaj, data_trimitere, ora_trimitere, id_raspuns) VALUES('%s', '%s', '%s', '%s', '%s', %d);", nume_utilizator1, nume_utilizator2, mesaj, data_trimitere, ora_trimitere, atoi(id_raspuns));

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, NULL, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }
}

void afisare_data_trimitere(char *id_mesaj)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[200];
    snprintf(comanda_sql, sizeof(comanda_sql),"SELECT data_trimitere FROM Mesaje WHERE id_mesaj=%d;", atoi(id_mesaj));

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback11, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);
}

void afisare_ora_trimitere(char *id_mesaj)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[200];
    snprintf(comanda_sql, sizeof(comanda_sql),"SELECT ora_trimitere FROM Mesaje WHERE id_mesaj=%d;", atoi(id_mesaj));

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback12, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);
}

void afisare_utilizatori_online()

{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[200];
    snprintf(comanda_sql, sizeof(comanda_sql),"SELECT nume_utilizator FROM Utilizatori WHERE status = 'online'");

    memset(utilizatori_online, 0, sizeof(utilizatori_online));

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback3, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    utilizatori_online[strlen(utilizatori_online)] = '\0';

    sqlite3_close(baza_date);
}

void blocare(char *nume_utilizator1, char *nume_utilizator2)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[200];
    snprintf(comanda_sql, sizeof(comanda_sql),"INSERT INTO Blocati (nume_utilizator1, nume_utilizator2) VALUES('%s', '%s');", nume_utilizator1, nume_utilizator2);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, NULL, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);
}

bool verificare_blocare(char *nume_utilizator1, char *nume_utilizator2)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    existenta_blocare= false;
    char comanda_sql[200];
    snprintf(comanda_sql, sizeof(comanda_sql),"SELECT nume_utilizator1, nume_utilizator2 FROM Blocati WHERE (nume_utilizator1 = '%s' OR nume_utilizator2 = '%s');", nume_utilizator1, nume_utilizator1);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, callback13, nume_utilizator2, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);

    if(existenta_blocare == true)
    {
        printf("[baza de date]Utilizatori %s si %s au conexiunea de convorbire blocata.", nume_utilizator1, nume_utilizator2);
        return true;
    }
    else
    {
        printf("[baza de date]Utilizatori %s si %s nu au conexiunea de convorbire blocata.", nume_utilizator1, nume_utilizator2);
        return false;
    }
}

void deblocare(char *nume_utilizator1, char *nume_utilizator2)
{
    cod_returnare = sqlite3_open("baza_de_date.db", &baza_date);

    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    char comanda_sql[200];
    snprintf(comanda_sql, sizeof(comanda_sql),"DELETE FROM Blocati WHERE nume_utilizator1 = '%s' AND nume_utilizator2 = '%s';", nume_utilizator1, nume_utilizator2);

    cod_returnare = sqlite3_exec(baza_date, comanda_sql, NULL, 0, &Mesaj_de_eroare);
    if(cod_returnare != SQLITE_OK)
    {
        printf("[server]Eroare : %s\n", sqlite3_errmsg(baza_date));
        sqlite3_close(baza_date);
        exit(EXIT_FAILURE);
    }

    sqlite3_close(baza_date);
}