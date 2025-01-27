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

// With this queue we will manage passengers on molo that has already been on a trip today and want to get to boat 1
#define MOLO_QUEUE_P1_KEY 1111
// With this queue we will manage passengers on molo that hasn't been on a trip today and want to get to boat 1
#define MOLO_QUEUE_1_KEY 2222
// With this queue we will manage passengers on molo that has already been on a trip today and want to get to boat 2
#define MOLO_QUEUE_P2_KEY 3333
// With this queue we will manage passengers on molo that hasn't been on a trip today and want to get to boat 2
#define MOLO_QUEUE_2_KEY 4444

#define N1 8 // Boat's 1 capacity
#define N2 10 // Boat's 2 capacity
#define T1 5 // Trip's 1 time 
#define T2 6 // Trip's 2 time 
#define K 5 // Bridge's capacity
#define Tp_Tk 120 //Time for all the trips

// With these queues we will send passenger's details through bridges and boats
#define BRIDGE_QUEUE_1_KEY 5555
#define BRIDGE_QUEUE_2_KEY 6666
#define BOAT_QUEUE_1_KEY 1212
#define BOAT_QUEUE_2_KEY 3434
// With this queue we will be able to send passengers returning from the trip on boat 1
#define RETURNING_QUEUE_1_KEY 7777
// With this queue we will be able to send passengers returning from the trip on boat 2
#define RETURNING_QUEUE_2_KEY 8888
// This is a very special queue for situation where the capacity is an odd number
// and adult with a child won't fit in. This will make them most prioritised for the next trip
#define CHILD_QUEUE_KEY 9999

// With this queue we will send pid to policeman
#define CAPTAIN_QUEUE_KEY 1313

// When there is signal from captain sail turns to 0
int sail = 1;
// Boat identification
int nr;

struct passenger {
    long int mtype; // Message type
    pid_t pid_p;    // Process ID of the passenger
    int age;        // Age of the passenger
    bool discount50;   // 50% discount for passengers sailing two times or more this day
    int child_age; // When passenger with no child child_age = -1
};

struct ticket {
    long int mtype; // Message type
    int assigned_boat; // Assigned boat number (1 or 2)
    int price; // Based on discount and child's age
};

struct captain {
    long int mtype;
    pid_t pid_c;
    int which_c; //which captain 1 or 2
};

// Handling signals 
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
    // This is an approximate amount od time after which we will sail even with only 1 passenger
    int part = Tp_Tk/2;

    captain.mtype = 1;
    captain.pid_c = getpid();
    captain.which_c = nr;

    // Creating queue to send policeman a pid 
    captain_msgid = msgget(CAPTAIN_QUEUE_KEY, 0666);
    if (captain_msgid == -1) {
        perror("Failed to create queue POLCAP");
        exit(EXIT_FAILURE);
    }

    // Sending policeman pid
    if (msgsnd(captain_msgid, &captain, sizeof(captain) - sizeof(long int), 0) == -1) {
        perror("Failed to send captain's pid");
        exit(EXIT_FAILURE);
    }

    printf("wyslalem %d\n\n", captain.pid_c);

    // I want one function that will work for both captains
    // But program has to know which is which so there are general 
    // variables with keys to c1's or c2's queues or time intervals and semaphore based on nr 
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


    // Create (lots of) message  queues
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

    // Create group of 2 semaphores - one for brigde entry and one for boat entry
    barriers = semget(123+nr, 2, 0600 | IPC_CREAT);
    if (barriers == -1) {
        perror("Failed to create barriers");
        exit(EXIT_FAILURE);
    }

    // Create a semaphore for a passenger to know if trip is finished and a boat is empty
    flag = semget(10+nr, 1, 0600 | IPC_CREAT);
    if (flag == -1) {
        perror("Failed to create barriers");
        exit(EXIT_FAILURE);
    }
    //while for a single trip
    time_t start_time;
    while (sail) { 
        // variable for semaphore to be changed two times if passenger is with a child or once if not
        int sem_num;
        // new time check for each trip
        start_time = time(NULL);
        
        // barrier before bridge - size K
        if (semctl(barriers, 0, SETVAL, K) == -1) { 
            perror("Failed to set first barrier");
            exit(EXIT_FAILURE);
        }
        // barrier before boat - size Ni
        if (semctl(barriers, 1, SETVAL, N) == -1) { 
            perror("Failed to set second barrier");
            exit(EXIT_FAILURE);
        }

        while (sail) { //when no signal from police sail - else get everyone out and kill them
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

            if (num + num2 == K) //no places for passengers left! K places is a must (bridgehas to be empty)
                break;

            bool passengerSpotted = false;

            struct passenger pass;
            // check if there is an adult with a child that couldn't get on the boat previously
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
            // if there are no passengers with children in super priorirty queue check normal priority and non-priority
            if (!passengerSpotted && msgrcv(boat_priority_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
                if (errno == ENOMSG) {
                    if (msgrcv(boat_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
                        if (errno == ENOMSG) {
                            //printf("No messages");
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
                // lack of passengers is suspicious - check how much time has passed
                // if (a) passenger(s) are/is waiting you can sail
                if ((difftime(time(NULL), start_time) >= part) && (num + num2 != K+N)) {
                    printf("So much time has passed I am starting new course.\n");
                    break;
                }
                else
                    continue;
            }
            printf("I am c%d and see %d on molo\n", nr, pass.pid_p);
            sleep(1);
            // We need to check if passenger and a child will fit in
            sem_num = 1;
            if(pass.child_age != -1){
                if(num + num2 > K+1){
                    printf("Passenger with a child can get on board!\n");
                    sem_num = 2;
                }
                else {
                    printf("Lack of space for passenger with a child. But they receive a special priority!\n");
                    if (msgsnd(child_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
                        perror("Failed to get passenger with child on child_queue");
                        exit(EXIT_FAILURE);
                    }
                    else {
                        // I need to avoid a situation where multiple parents and children
                        // are blocking the trip - after a while we will sail anyway
                        // but an adult with child will get the super priority 
                        if ((difftime(time(NULL), start_time) >= part) && (num + num2 != K+N)) {
                            printf("So much time has passed I am starting new course.\n");
                            break;
                        }
                        else
                            continue;
                    }
                }
            }

            // let passenger on the brigde
            struct sembuf opening0 = {0, -sem_num, 0};
            if (semop(barriers, &opening0, 1) == -1) {
                perror("Cannot get to the brigde 2! \n");
                exit(EXIT_FAILURE);
            }
            // send passenger's details on brigde
            if (msgsnd(bridge_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
                perror("Failed to get passenger on the brigde");
                exit(EXIT_FAILURE);
            }
            // receive passenger's details from bridge
            if (msgrcv(bridge_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
                if (errno == ENOMSG) {
                    //printf("No messages in boat1_priority_msgid.\n");
                } else {
                    perror("msgrcv failed");
                    exit(EXIT_FAILURE);
                }
            } else
                printf("\n\nPassenger %d got on brigde %d safely %d places left\n\n",pass.pid_p, nr, (num + num2 - K)-1);

            struct sembuf closing0 = {0, sem_num, 0};
            // get off the bridge
            if (semop(barriers, &closing0, 1) == -1) {
                perror("Cannot get off brigde! \n");
                exit(EXIT_FAILURE);
            }
            
            struct sembuf opening1 =  {1, -sem_num, 0};
            // try to get on the boat and stay here for the trip
            if (semop(barriers, & opening1, 1) == -1) {
                perror("Cannot get to the boat! \n");
                exit(EXIT_FAILURE);
            }
            // sending information to boat queue - we need to identify the passenger
            if (msgsnd(boat_queue, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
                perror("Failed to send passenger that enters the boat");
                exit(EXIT_FAILURE);
            }


        }
        // Passenger can't get off or on the boat it is sailing now
        if (semctl(flag, 0, SETVAL, 1) == -1) { 
            perror("Failed to set a flag");
            exit(EXIT_FAILURE);
        }
        // Simulating a trip
        if (sail){
            sleep(1);
            printf("\n\nBoat %d is sailing...\n\n", nr);
            sleep(T);
            printf("\n\nBoat %d returned :)\n\n", nr);
        }

        while (1) { // we will eventually make passengers leave no matter what so while(1)
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

            if (num + num2 == N+K) { // only free places = boat is empty
                printf("Each passenger left the boat and the brigde %d\n", nr);
                break;
            } 

            if (msgrcv(boat_queue, &pass, sizeof(pass) - sizeof(long int), 0, 0) == -1) {
                perror("Failed to redirect passenger from boat");
                exit(EXIT_FAILURE);
            }
            // we need to know if passenger is with child to manage semaphores correctly
            if(pass.child_age != -1)
                sem_num = 2;
            else
                sem_num = 1;

            struct sembuf leaving1 = {1, sem_num, 0};

            // managing unloading similarily to loading the passengers on boat

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
                
            printf("\n\nPassenger got off the brigde %d safely %d have to get out\n\n", nr, ((N+K) - (num + num2)-1));
            sleep(1);

            struct sembuf entering0 = {0, -sem_num, 0};
            if (semop(barriers, & entering0, 1) == -1) {
                perror("Cannot get on the barrier! \n");
                exit(EXIT_FAILURE);
            }

            struct sembuf leaving0 = {0, sem_num, 0};

            if (semop(barriers, & leaving0, 1) == -1) {
                perror("Cannot get on land\n");
                exit(EXIT_FAILURE);
            }

            printf("Passenger %d needs to get off of the boat\n", pass.pid_p);

            if (msgsnd(returning_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
                perror("Failed to get passenger on the brigde");
                exit(EXIT_FAILURE);
            }

        }
            // This semaphore informes passenger if the trip is over AND everyone got out 
            struct sembuf opening3 = {0, -1, 0};
            if (semop(flag, &opening3, 1) == -1) {
                perror("Cannot diminish semaphore \n");
                exit(EXIT_FAILURE);
            }

    }

    printf("Boat %d definitely ended sailing for today\n", nr);
    // cleaning up resources
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
        // Parent process: captain of boat 2
        nr = 2;
        captain_process(2);
    } else if (pid < 0) {
        perror("Failed to fork passenger process");
        exit(EXIT_FAILURE);
    }
    return 0;
}