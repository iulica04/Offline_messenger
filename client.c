#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

extern int errno;
int port;

bool conectat = false;
bool afisare = true;
bool destinatar_gasit = false;

void *receptor(void * arg);

pthread_t listener;
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

void meniu()
{
    printf("Comenzile oferite de sistem: \n");
    printf("   conectare\n");
    printf("   deconectare\n");       
    printf("   inregistrare\n");
    printf("   afisare mesajele offline\n");
    printf("   afisare istoric conversatie\n");
    printf("   trimite mesaj\n");
    printf("   raspunde la un mesaj\n");
    printf("   schimbare parola\n");
    printf("   afisare utilizatori online\n");
    printf("   iesire\n");
    printf("Pentru a revedea comenzile posibile apelati comanda : meniu\n");
}

void clearBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main(int argc, char *argv[])
{
    printf("Bine ati venit la Offline Mesagger.\n");
    printf("Comenzile oferite de sistem: \n");
    printf("   conectare\n");
    printf("   deconectare\n");       
    printf("   inregistrare\n");
    printf("   afisare mesaje offline\n");
    printf("   afisare istoric conversatie\n");
    printf("   trimite mesaj\n");
    printf("   raspunde la un mesaj\n");
    printf("   schimbare parola\n");
    printf("   afisare utilizatori online\n");
    printf("   iesire\n");
    printf("Pentru a revedea comenzile posibile apelati comanda : menu\n");
    fflush(stdout);

    int server_descriptor;
    struct sockaddr_in server_addr;
    char comanda[100];
    char raspuns[100];
    int cod_write, cod_read;
    int contor = 0;
    char nume_utilizator[100];
    char parola[100];
    char nr_telefon[13];
    char parola_noua[100];
    char raspuns_parola[10];
    char raspuns_iesire[10];
    char nume_utilizator_destinatie[100];
    char nume_prieten[100];
    char mesaj[2000];

    if(argc != 3)
    {
        printf ("Sintaza: %s <adresa_server> <port>.\n", argv[0]);
        return -1;
    }

    port = atoi(argv[2]);

    server_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(server_descriptor == -1)
    {
        perror("[client]Eroare la socket().\n");
        return errno;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(port);

    if(connect(server_descriptor, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("[client]Eroare la connect().\n");
        return errno;
    }

    while(1)
    {
        if(afisare == true)
        {
        sleep(1);
        printf("[client]Introduceti comanda dorita: ");

        fgets(comanda, sizeof(comanda), stdin);
        comanda[strlen(comanda)-1] = '\0';

        cod_write = write(server_descriptor, &comanda, strlen(comanda));    
        if(cod_write == -1)
        {
             perror("[client]Eroare la write() catre server.\n");
             exit(EXIT_FAILURE);
        }

       if(strcmp(comanda, "inregistrare") == 0)
        {
            do{

                memset(nume_utilizator, 0, sizeof(nume_utilizator));
                printf("[client]Introduceti un nume de utilizator: ");
                fflush(stdout);

                fgets(nume_utilizator, sizeof(nume_utilizator), stdin);
                nume_utilizator[strlen(nume_utilizator)-1] = '\0';
 
                cod_write = write(server_descriptor, &nume_utilizator, strlen(nume_utilizator));
                if(cod_write == -1)
                {
                     perror("[client]Eroare la write() catre server.\n");
                     exit(EXIT_FAILURE);
                }

                memset(raspuns, 0, sizeof(raspuns));
                cod_read = read(server_descriptor, &raspuns, sizeof(raspuns));
                if(cod_read == -1)
                {
                     perror("[client]Eroare la read() de la server\n");
                     exit(EXIT_FAILURE);
                }
                raspuns[strlen(raspuns)] = '\0';
                printf("[client]%s\n", raspuns);
                fflush(stdout);

            }while(strcmp(raspuns, "Numele de utilizator ales este disponibil.") != 0);

            do{
                memset(parola, 0, sizeof(parola));
                printf("[client]Introduceti o parola: ");
                fflush(stdout);

                fgets(parola, sizeof(parola), stdin);
                parola[strlen(parola)-1] = '\0';

                cod_write = write(server_descriptor, &parola, strlen(parola));
                if(cod_write == -1)
                {
                    perror("[client]Eroare la write() catre server.\n");
                    exit(EXIT_FAILURE);
                }

                memset(raspuns, 0, sizeof(raspuns));
                cod_read = read(server_descriptor, &raspuns, sizeof(raspuns));
                if(cod_read == -1)
                {
                     perror("[client]Eroare la read() de la server.n");
                     exit(EXIT_FAILURE);
                }
                raspuns[strlen(raspuns)] = '\0';
                printf("[client]%s\n", raspuns); 
                fflush(stdout);
                
            }while(strcmp(raspuns, "Ai introdus o parola care respecta regulile aplicatiei.") != 0);

            do{
                memset(nr_telefon, 0, sizeof(nr_telefon));                   
                printf("[client]Introduceti un numar de telefon: ");
                fflush(stdout);

                fgets(nr_telefon, sizeof(nr_telefon), stdin);
                nr_telefon[strlen(nr_telefon)] = '\0';


                cod_write = write(server_descriptor, &nr_telefon, strlen(nr_telefon));
                if(cod_write == -1)
                {
                     perror("[client]Eroare la write() catre server.\n");
                     exit(EXIT_FAILURE);
                }

                memset(raspuns, 0, sizeof(raspuns));
                cod_read = read(server_descriptor, &raspuns, sizeof(raspuns));
                if(cod_read == -1)
                {
                    perror("[client]Eroare la read() de la server.n");
                    exit(EXIT_FAILURE);
                }
                raspuns[strlen(raspuns)] = '\0';
                printf("[client]%s\n", raspuns);
                fflush(stdout);
 
            }while(strcmp(raspuns, "Numarul de telefon introdus este valid.") != 0);

            
            memset(raspuns, 0, sizeof(raspuns));
            cod_read = read(server_descriptor, &raspuns, sizeof(raspuns));
            if(cod_read == -1)
            {
                perror("[client]Eroare la read() de la server.n");
                exit(EXIT_FAILURE);
            }
            raspuns[strlen(raspuns)] = '\0';
            printf("[client]%s\n", raspuns);
            fflush(stdout);
        } else if(strcmp(comanda, "conectare") == 0)
        {
            if(conectat == false)
            {
            
                do{

                    memset(nume_utilizator, 0, sizeof(nume_utilizator));
                    printf("[client]Introduceti numele de utilizator: ");
                    fflush(stdout);

                    fgets(nume_utilizator, sizeof(nume_utilizator), stdin);
                    nume_utilizator[strlen(nume_utilizator)-1] = '\0';
 
                    cod_write = write(server_descriptor, &nume_utilizator, strlen(nume_utilizator));
                    if(cod_write == -1)
                    {
                        perror("[client]Eroare la write() catre server.\n");
                        exit(EXIT_FAILURE);
                    }

                    memset(raspuns, 0, sizeof(raspuns));
                    cod_read = read(server_descriptor, &raspuns, sizeof(raspuns));
                    if(cod_read == -1)
                    {
                        perror("[client]Eroare la read() de la server\n");
                        exit(EXIT_FAILURE);
                    }
                    raspuns[strlen(raspuns)] = '\0';
                    printf("[client]%s\n", raspuns);
                    fflush(stdout);

                }while(strcmp(raspuns, "Numele de utilizator introdus este corect.") != 0);


                do{
                    memset(parola, 0, sizeof(parola));
                    printf("[client]Introduceti parola: ");
                    fflush(stdout);
                
                    fgets(parola, sizeof(parola), stdin);
                    parola[strlen(parola)-1] = '\0';

                    cod_write = write(server_descriptor, &parola, strlen(parola));
                    if(cod_write == -1)
                    {
                        perror("[client]Eroare la write() catre server.\n");
                        exit(EXIT_FAILURE);
                    }

                    memset(raspuns, 0, sizeof(raspuns));
                    cod_read = read(server_descriptor, &raspuns, sizeof(raspuns));
                    if(cod_read == -1)
                    {
                        perror("[client]Eroare la read() de la server.n");
                        exit(EXIT_FAILURE);
                    }
                    raspuns[strlen(raspuns)] = '\0';
                    printf("[client]%s\n", raspuns); 
                    fflush(stdout);

                    if(strcmp(raspuns, "Parola introdusa este corecta.") != 0)
                    {
                        do{
                            printf("[client]Doriti sa va schimbati parola? Da/Nu \n Raspuns: ");
                            fflush(stdout);
                            fgets(raspuns_parola, sizeof(raspuns_parola), stdin);
                            raspuns_parola[strlen(raspuns_parola)-1] = '\0';

                        }while(strcmp(raspuns_parola, "Da")!=0 && strcmp(raspuns_parola, "Nu")!=0);

                        cod_write = write(server_descriptor, &raspuns_parola, strlen(raspuns_parola));
                        if(cod_write == -1)
                        {
                            perror("[client]Eroare la write() catre server.\n");
                            exit(EXIT_FAILURE);
                        }

                        if(strcmp(raspuns_parola, "Da") == 0)
                        {
                            do{  
                                memset(nr_telefon, 0, sizeof(nr_telefon));
                                printf("[client]Introduceti numarul de telefon cu care s-a creat contul: ");
        

                                fgets(nr_telefon, sizeof(nr_telefon), stdin);
                                nr_telefon[strlen(nr_telefon)] = '\0';

                                cod_write = write(server_descriptor, &nr_telefon, strlen(nr_telefon));
                                if(cod_write == -1)
                                {
                                    perror("[client]Eroare la write() catre server.\n");
                                    exit(EXIT_FAILURE);
                                }

                                memset(raspuns, 0, sizeof(raspuns));
                                cod_read = read(server_descriptor, &raspuns, sizeof(raspuns));
                                if(cod_read == -1)
                                {
                                    perror("[client]Eroare la read() de la server\n");
                                    exit(EXIT_FAILURE);
                                }
                                raspuns[strlen(raspuns)] = '\0';
                                printf("[client]%s\n", raspuns);
                                fflush(stdout);

                            }while(strcmp(raspuns, "Numarul de telefon este corect.") != 0);

                            memset(parola_noua, 0, sizeof(parola_noua));
                            printf("[client]Acum iti poti schimba parola! Introduceti o parola noua pentru actualizare: \n");

                            fscanf(stdin,"%s" ,parola_noua);
                            parola_noua[strlen(parola_noua)] = '\0';


                            cod_write = write(server_descriptor, &parola_noua, strlen(parola_noua));
                            if(cod_write == -1)
                            {
                                perror("[client]Eroare la write() catre server.\n");
                                exit(EXIT_FAILURE);
                            }

                            memset(raspuns, 0, sizeof(raspuns));
                            cod_read = read(server_descriptor, &raspuns, sizeof(raspuns));
                            if(cod_read == -1)
                            {
                                perror("[client]Eroare la write() de la client.\n");
                                exit(EXIT_FAILURE);
                            }
                            raspuns[strlen(raspuns)] = '\0';
                            printf("[server]%s\n", raspuns);
                            fflush(stdout);
                            clearBuffer(); // curatam bufferul
                        }
                    }
                
                }while(strcmp(raspuns, "Parola introdusa este corecta.") != 0);

                printf("[client]Conectarea la aplicatie a rusit.\n");
                fflush(stdout);
                conectat = true;
                memset(raspuns, 0, sizeof(raspuns));
                cod_read = read(server_descriptor, &raspuns, sizeof(raspuns));
                if(cod_read == -1)
                {
                    perror("[client]Eroare la write() de la client.\n");
                    exit(EXIT_FAILURE);
                }
                raspuns[strlen(raspuns)] = '\0';
                printf("[server]%s\n", raspuns);
                
                pthread_create(&listener,NULL,&receptor,(void *)server_descriptor);
                pthread_detach(listener);

            }
            else
            {
                printf("[client]Esti deja conectat la aplicatie.\n");
            }

        }  else if(strcmp(comanda, "iesire") == 0)
        {
            printf("[client]Iesire reusita.\n");
            fflush(stdout);

            exit(1);
        } else if(strcmp(comanda, "schimbare parola") == 0)
        {
            if(conectat == true)
            {
                memset(parola_noua, 0, sizeof(parola_noua));
                memset(raspuns, 0, sizeof(raspuns));

                printf("[client]Introduceti noua parola pentru actualizare: ");

                fgets(parola_noua, sizeof(parola_noua), stdin);
                parola_noua[strlen(parola_noua)-1] = '\0';

                cod_write = write(server_descriptor, &parola_noua, strlen(parola_noua));
                if(cod_write == -1)
                {
                    perror("[client]Eroare la write() catre server.\n");
                    exit(EXIT_FAILURE);
                }

            }
            else
            {
                printf("[clinet]Nu sunteti conectat. Pentru a va schimba parola, este necesar introducerea numelui de utilizator in aplicatie.\n");
                fflush(stdout);
            }

        } else if(strcmp(comanda, "meniu") == 0)
        {
            meniu();

        } else if(strcmp(comanda, "trimite mesaj") == 0)
        {
            if(conectat == true)
            {
                memset(nume_utilizator_destinatie, 0, sizeof(nume_utilizator_destinatie));
                printf("[client]Introduceti numele de utilizator caruia doriti sa ii trimiteti mesaj: ");
                fflush(stdout);

                fgets(nume_utilizator_destinatie, sizeof(nume_utilizator_destinatie), stdin);
                nume_utilizator_destinatie[strlen(nume_utilizator_destinatie) - 1] = '\0';

                cod_write = write(server_descriptor, &nume_utilizator_destinatie, strlen(nume_utilizator_destinatie));
                if(cod_write == -1)
                {
                    perror("[client]Eroare la write() catre server.\n");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                printf("[client]Nu sunteti conectat la aplicatie. Pentru a trimite un mesaj, conectati-va sau creati-va un cont.\n");
            }
        } else if(strcmp(comanda, "afisare istoric conversatie") == 0)
        {
            if(conectat == true)
            {
                memset(nume_prieten, 0, sizeof(nume_prieten));
                printf("Introduceti numele utilizatorului cu care doriti afisarea istoricului conversatiei: ");
                fflush(stdout);
                fgets(nume_prieten, sizeof(nume_prieten), stdin);
                nume_prieten[strlen(nume_prieten) -1] ='\0';

                cod_write = write(server_descriptor, &nume_prieten, strlen(nume_prieten));
                if(cod_write == -1)
                {
                    perror("[client]Eroare la write() catre server.\n");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                printf("[client]Nu suntieti conectat la aplicatie.");
            }
        } else if(strstr(comanda, "afisare mesaje offline") == 0)
        {
            if(conectat == false)
            {
                printf("[client]Nu sunteti conectat la aplicatie.\n");
            }
        }
        }

    }
    close(server_descriptor);
 }
  

void *receptor(void *arg)
{
    int server_descriptor = (int)arg;
    char raspuns[100];
    int cod_read, cod_write;
    char mesaj[2000];

    while(conectat == true)
    {
        memset(raspuns, 0, sizeof(raspuns));
        cod_read = read(server_descriptor, &raspuns, sizeof(raspuns));
        if(cod_read == -1)
        {
            perror("[client]Eroare la write() de la client.\n");
            exit(EXIT_FAILURE);
        }
        raspuns[strlen(raspuns)] = '\0';

        if(strcmp(raspuns, "Deconectare reusita.") == 0)
        {
            printf("[server]%s\n", raspuns);
            fflush(stdout);
            conectat = false;
            afisare = true;
        }
        else if(strcmp(raspuns, "Deconectare esuata. Nu sunteti conectat la aplicatie.") == 0)
        {
            printf("[server]%s", raspuns);
            fflush(stdout);
            afisare = true;
        } else if(strcmp(raspuns, "Actualizarea parolei a reusit.") == 0)
        {
            printf("[server]%s\n", raspuns);
            fflush(stdout);            
            afisare = true;
        } else if(strcmp(raspuns, "Utilizator gasit.") == 0)
        {
            printf("[server]%s\n", raspuns);
            fflush(stdout);
            destinatar_gasit = false;

            printf("[client]Introduceti mesajul pe care doriti sa il trimiteti: ");
            fflush(stdout);
            fgets(mesaj, sizeof(mesaj), stdin);
            mesaj[strlen(mesaj)-1] = '\0';

            cod_write = write(server_descriptor, &mesaj, strlen(mesaj));
            if(cod_write == -1)
            {
                perror("[client]Eroare la write() de la client.\n");
                exit(EXIT_FAILURE);
            }

        } else if(strcmp(raspuns, "Utilizatorul nu exista") == 0)
        {
            printf("[server]%s\n", raspuns);
            fflush(stdout);
            destinatar_gasit = true;
        }else if(strcmp(raspuns, "Numele de utilizator introdus nu exista in baza de date a aplicatiei.") == 0)
        {
            printf("[server]Numele de utilizator introdus nu exista in baza de date a aplicatiei.");
        }
        else
        {
            printf("\n%s\n", raspuns);
            fflush(stdout);
        }

    }
}