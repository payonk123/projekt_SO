#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>


#define MOLO_QUEUE_P1_KEY 1111
#define MOLO_QUEUE_1_KEY 2222
#define MOLO_QUEUE_P2_KEY 3333
#define MOLO_QUEUE_2_KEY 4444

#define N1 3
#define N2 3
#define T1 5
#define T2 6
#define K 2
#define Tk 60


#define BRIDGE_QUEUE_1_KEY 5555
#define BRIDGE_QUEUE_2_KEY 6666
#define BOAT_QUEUE_1_KEY 1212
#define BOAT_QUEUE_2_KEY 3434
#define RETURNING_QUEUE_1_KEY 7777
#define RETURNING_QUEUE_2_KEY 8888

#define CHILD_QUEUE_KEY 9999

#define CAPTAIN_QUEUE_KEY 1313

//zmienna globalna 'sail' gdy sygnal to sail = 0
int sail = 1;
int nr;

// Define the passenger structure
struct passenger {
    long int mtype; // Message type
    pid_t pid_p;    // Process ID of the passenger
    int age;        // Age of the passenger
    bool discount50;   // czy uprawniony do znizki - gdy jechal tak gdy pierwszy raz to nir
    int child_age;
};
// Define the ticket structure
struct ticket {
    long int mtype; // Message type
    int assigned_boat; // Assigned boat number (1 or 2)
    int price; //if pass.another = 0 20 if pass.another = 1 10
};

struct captain {
    long int mtype;
    pid_t pid_c;
    int which_c; //1 or 2
};

// Obsługa sygnałów policjanta
void handle_signal(int signal) {
    if (signal == SIGUSR1 && nr == 1) {
        printf("Boat 1 stops sailing if possible\n");
        sail = 0;
    }
    else if (signal == SIGUSR2 && nr == 2){
        printf("Boat 2 stops sailing if possible\n");
        sail = 0;
    }
    else
        perror("Failed to handle signal");
}


void captain_process() {
    int boat_priority_msgid, boat_msgid, bridge_msgid, barriers, returning_msgid, flag, boat_queue, captain_msgid, child_msgid;
    struct passenger pass;
    struct captain captain;
    int half = Tk/2;

    captain.mtype = 1;
    captain.pid_c = getpid();
    captain.which_c = nr;

    captain_msgid = msgget(CAPTAIN_QUEUE_KEY, 0666);
    if (captain_msgid == -1) {
        perror("Failed to create queue POLCAP");
        exit(EXIT_FAILURE);
    }

    if (msgsnd(captain_msgid, &captain, sizeof(captain) - sizeof(long int), 0) == -1) {
        perror("Failed to send captain's pid");
        exit(EXIT_FAILURE);
    }

    printf("wyslalem %d\n\n", captain.pid_c);


    int key_MQ;
    int key_MPQ;
    int key_BQ;
    int key_RQ;
    int key_BOATQ;
    int N;
    int T;

    if(nr==1){
        key_MQ = MOLO_QUEUE_1_KEY;
        key_MPQ = MOLO_QUEUE_P1_KEY;
        key_BQ = BRIDGE_QUEUE_1_KEY;
        key_RQ = RETURNING_QUEUE_1_KEY;
        key_BOATQ = BOAT_QUEUE_1_KEY;    
        N = N1;
        T = T1; 
    }
    else{
        key_MQ = MOLO_QUEUE_2_KEY;
        key_MPQ = MOLO_QUEUE_P2_KEY;
        key_BQ = BRIDGE_QUEUE_2_KEY;
        key_RQ = RETURNING_QUEUE_2_KEY;
        key_BOATQ = BOAT_QUEUE_2_KEY; 
        N = N2;
        T = T2; 
    };


    // Create message queues
    boat_priority_msgid = msgget(key_MPQ, IPC_CREAT | 0666);
    if (boat_priority_msgid == -1) {
        perror("Failed to create queue");
        exit(EXIT_FAILURE);
    }

    boat_msgid = msgget(key_MQ, IPC_CREAT | 0666);
    if (boat_msgid == -1) {
        perror("Failed to create queue");
        exit(EXIT_FAILURE);
    }

    bridge_msgid = msgget(key_BQ, IPC_CREAT | 0666);
    if (bridge_msgid == -1) {
        perror("Failed to create bridge1");
        exit(EXIT_FAILURE);
    }

    returning_msgid = msgget(key_RQ, IPC_CREAT | 0666);
    if (returning_msgid == -1) {
        perror("Failed to create queue");
        exit(EXIT_FAILURE);
    }

    boat_queue = msgget(key_BOATQ, IPC_CREAT | 0666);
    if (boat_queue == -1) {
        perror("Failed to create queue");
        exit(EXIT_FAILURE);
    }

    child_msgid = msgget(CHILD_QUEUE_KEY, IPC_CREAT | 0666);
    if (child_msgid == -1) {
        perror("Failed to create queue");
        exit(EXIT_FAILURE);
    }

    barriers = semget(123+nr, 2, 0600 | IPC_CREAT);
    if (barriers == -1) {
        perror("Failed to create barriers");
        exit(EXIT_FAILURE);
    }

    flag = semget(10+nr, 1, 0600 | IPC_CREAT);
    if (flag == -1) {
        perror("Failed to create barriers");
        exit(EXIT_FAILURE);
    }
    //while dla rejsu
    time_t start_time;
    while (sail) { //pętla rejsu - zaladunek i rozladunek odbywaja sie w petli
        int decNo;

        start_time = time(NULL);
        //przy kazdym nowym zaladunku sprawdz czas, nadaj mu nowa wartosc
        
        if (semctl(barriers, 0, SETVAL, K) == -1) { //for now K = 5
            perror("Failed to set first barrier");
            exit(EXIT_FAILURE);
        }
        if (semctl(barriers, 1, SETVAL, N) == -1) { // for now N = 15
            perror("Failed to set second barrier");
            exit(EXIT_FAILURE);
        }

        while (sail) { //when no signal from police - else get everyone out and kill them
            int num = semctl(barriers, 0, GETVAL);
            if (num == -1) {
                perror("Semaphore read error");
                exit(EXIT_FAILURE);
            }
            int num2 = semctl(barriers, 1, GETVAL);
            if (num2 == -1) {
                perror("Semaphore read error");
                exit(EXIT_FAILURE);
            }

            if (num + num2 == K) //no places for passengers left!
                break;

            bool passengerSpotted = false;

            struct passenger pass;
            //nowy priorytet - child_queue, sciagnij z child _queue TYLKO gdy jest miejsce na dwoch pass
            //jesli nie ma miejsca na dwa lub child_queue puste to:
            if(nr == 2 && num + num2 > K+1){
                if(msgrcv(child_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1){
                    if (errno != ENOMSG){
                        perror("msgrcv failed");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                    passengerSpotted = true;
            }

            if (!passengerSpotted && msgrcv(boat_priority_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
                if (errno == ENOMSG) {
                    if (msgrcv(boat_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
                        if (errno == ENOMSG) {
                            //printf("No messages in boat1_priority_msgid.\n");
                        } 
                        else {
                            perror("msgrcv failed");
                            exit(EXIT_FAILURE);
                        }
                    } 
                    else
                        passengerSpotted = true;
                }
            } 
            else
                passengerSpotted = true;

            if (!passengerSpotted){
                //przy kazdym braku pasazera sprawdz roznice czasow (teraz-poczatek zaladunku)
                //jesli ten czas to wiecej niz 1/2 Tk i sa jacys pasazerowie to break czyli plyniemy
                //jesli ten czas to wiecej niz 1/2 Tk i nie ma pasazerow to continue (czekamy na pasazerow)
                //nie ma pasazerow gdy num2+num == K+N wiec luz
                if ((difftime(time(NULL), start_time) >= half) && (num + num2 != K+N)) {
                    printf("So much time has passed I am starting new course.\n");
                    break;
                }
                else
                    continue;
            }
            printf("I am c%d and see %d on molo\n", nr, pass.pid_p);
            sleep(1);
            //jesli ma dziecko to SPRAWDZ CZY SIE ZMIESCI NA LODKE
            //jesli sie miesci na lodke to: {0, -2, 0}
            //jesli sie nie zmiesci to wyslij do kolejki child queue i kontynuuj
            
            decNo = 1;
            if(pass.child_age != -1){
                if(num + num2 > K+1){
                    printf("Mozemy wpuscic rodzica z dzieckiem!\n");
                    decNo = 2;
                }
                else {
                    printf("Brak miejsca, zapraszam do osobnej kolejki!\n");
                    if (msgsnd(child_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
                        perror("Failed to get passenger with child on child_queue");
                        exit(EXIT_FAILURE);
                    }
                    else {
                        //przy kazdym braku pasazera sprawdz roznice czasow (teraz-poczatek zaladunku)
                        //jesli ten czas to wiecej niz 1/2 Tk i sa jacys pasazerowie to break czyli plyniemy
                        //jesli ten czas to wiecej niz 1/2 Tk i nie ma pasazerow to continue (czekamy na pasazerow)
                        //nie ma pasazerow gdy num2+num == K+N wiec luz
                        if ((difftime(time(NULL), start_time) >= half) && (num + num2 != K+N)) {
                            printf("So much time has passed I am starting new course.\n");
                            break;
                        }
                        else
                            continue;
                    }
                }
            }

            struct sembuf opening0 = {0, -decNo, 0};
            if (semop(barriers, &opening0, 1) == -1) {
                perror("Cannot get to the brigde 2! \n");
                exit(EXIT_FAILURE);
            }

            if (msgsnd(bridge_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
                perror("Failed to get passenger on the brigde");
                exit(EXIT_FAILURE);
            }

            if (msgrcv(bridge_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
                if (errno == ENOMSG) {
                    //printf("No messages in boat1_priority_msgid.\n");
                } else {
                    perror("msgrcv failed");
                    exit(EXIT_FAILURE);
                }
            } else
                printf("\n\nPassenger %d got on brigde %d safely %d places left\n\n",pass.pid_p, nr, (num + num2 - K)-1);

            struct sembuf closing0 = {0, decNo, 0};

            if (semop(barriers, &closing0, 1) == -1) {
                perror("Cannot get off brigde! \n");
                exit(EXIT_FAILURE);
            }
            
            struct sembuf opening1 =  {1, -decNo, 0};

            if (semop(barriers, & opening1, 1) == -1) {
                perror("Cannot get to the boat! \n");
                exit(EXIT_FAILURE);
            }
            if (msgsnd(boat_queue, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
                perror("Failed to send passenger that enters the boat");
                exit(EXIT_FAILURE);
            }


        }
        //you can't get off the boat it is sailing now
        if (semctl(flag, 0, SETVAL, 1) == -1) { 
            perror("Failed to set a flag");
            exit(EXIT_FAILURE);
        }

        if (sail){
            sleep(1);
            printf("\n\nBoat %d is sailing...\n\n", nr);
            sleep(T);
            printf("\n\nBoat %d returned :) now passengers get out ;v;\n\n", nr);
        }

        while (1) { //rozladunek odbedzie sie niewazne co wiec 1 a nie sail
            
            
            int num = semctl(barriers, 0, GETVAL);
            if (num == -1) {
                perror("Semaphore read error");
                exit(EXIT_FAILURE);
            }
            int num2 = semctl(barriers, 1, GETVAL);
            if (num2 == -1) {
                perror("Semaphore read error");
                exit(EXIT_FAILURE);
            }

            if (num + num2 == N+K) {
                printf("Each passenger left the boat and the brigde %d\n", nr);
                break;
            } //as long as there are still some passengers...

            if (msgrcv(boat_queue, &pass, sizeof(pass) - sizeof(long int), 0, 0) == -1) {
                perror("Failed to redirect passenger from boat");
                exit(EXIT_FAILURE);
            }

            if(pass.child_age != -1)
                decNo = 2;
            else
                decNo = 1;

            struct sembuf leaving1 = {1, decNo, 0};

            if (semop(barriers, &leaving1, 1) == -1) {
                perror("Cannot get to the brigde! \n");
                exit(EXIT_FAILURE);
            }


            if (msgsnd(bridge_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
                perror("Failed to get passenger on the brigde");
                exit(EXIT_FAILURE);
            }


            if (msgrcv(bridge_msgid, &pass, sizeof(pass) - sizeof(long int), 0, 0) == -1) {
                perror("msgrcv failed");
                exit(EXIT_FAILURE);
            }
                
            printf("\n\nPassenger got off the brigde %d safely %d have to get out\n\n", nr, (N+K) - (num + num2));
            sleep(1);

            struct sembuf entering0 = {0, -decNo, 0};
            if (semop(barriers, & entering0, 1) == -1) {
                perror("Cannot get on the barrier! \n");
                exit(EXIT_FAILURE);
            }

            struct sembuf leaving0 = {0, decNo, 0};

            if (semop(barriers, & leaving0, 1) == -1) {
                perror("Cannot get on land\n");
                exit(EXIT_FAILURE);
            }

            printf("Nadalem rozkaz wyjscia pasazerowi %d\n", pass.pid_p);

            if (msgsnd(returning_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
                perror("Failed to get passenger on the brigde");
                exit(EXIT_FAILURE);
            }

        }

            struct sembuf opening3 = {0, -1, 0};
            if (semop(flag, &opening3, 1) == -1) {
                perror("Cannot diminish semaphore \n");
                exit(EXIT_FAILURE);
            }

    }

    printf("Boat %d definitely ended sailing for today\n", nr);
    //kill all passengers when signal from police received
    semctl(barriers, 2, IPC_RMID);
    semctl(flag, 1, IPC_RMID);

    msgctl(boat_priority_msgid, IPC_RMID, NULL);
    msgctl(boat_msgid, IPC_RMID, NULL);
    msgctl(bridge_msgid, IPC_RMID, NULL);
    msgctl(returning_msgid, IPC_RMID, NULL);
    msgctl(boat_queue, IPC_RMID, NULL);
    msgctl(child_msgid, IPC_RMID, NULL);

    exit(1);
}

int main() {

    if(N1<K || N2<K){
        printf("Condition K<N not met - captain process ends\n");
        exit(EXIT_FAILURE);
    }
    printf("Captain process started.\n");

    signal(SIGUSR1, handle_signal);
    signal(SIGUSR2, handle_signal);

    pid_t pid = fork();
    if (pid == 0) {
        // Child process: captain of boat 1
        nr = 1;
        captain_process(1);
    } else if (pid > 0) {
        nr = 2;
        captain_process(2);
    } else if (pid < 0) {
        perror("Failed to fork passenger process");
        exit(EXIT_FAILURE);
    }
    return 0;
}