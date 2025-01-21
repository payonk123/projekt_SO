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

#define MOLO_QUEUE_P1_KEY 1111
#define MOLO_QUEUE_1_KEY 2222
#define MOLO_QUEUE_P2_KEY 3333
#define MOLO_QUEUE_2_KEY 4444

#define N1 15
#define N2 20
#define T1 10
#define T2 15
#define K 5

#define BRIDGE_QUEUE_1_KEY 5555
#define BRIDGE_QUEUE_2_KEY 6666

// Define the passenger structure
struct passenger {
    long int mtype; // Message type
    pid_t pid_p; // Process ID of the passenger
    int age; // Age of the passenger
    bool discount50;
};

// Define the ticket structure
struct ticket {
    long int mtype; // Message type
    int assigned_boat; // Assigned boat number (1 or 2)
    int price; //if pass.another = 0 20 if pass.another = 1 10
};

void captain_process(int nr) {
    int boat_priority_msgid, boat_msgid, bridge_msgid, barriers;
    struct passenger pass;

    int key_MQ;
    int key_MPQ;
    int key_BQ;
    int N;
    int T;

    if(nr==1){
        key_MQ = MOLO_QUEUE_1_KEY;
        key_MPQ = MOLO_QUEUE_P1_KEY;
        key_BQ = BRIDGE_QUEUE_1_KEY;
        N = N1;
        T = T1; 
    }
    else{
        key_MQ = MOLO_QUEUE_2_KEY;
        key_MPQ = MOLO_QUEUE_P2_KEY;
        key_BQ = BRIDGE_QUEUE_2_KEY;
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

    barriers = semget(123+nr, 2, 0600 | IPC_CREAT);
    if (barriers == -1) {
        perror("Failed to create barriers");
        exit(EXIT_FAILURE);
    }

    //while dla rejsu

    while (1) { //pętla rejsu - zaladunek i rozladunek odbywaja sie w petli

        if (semctl(barriers, 0, SETVAL, K) == -1) { //for now K = 5
            perror("Failed to set first barrier");
            exit(EXIT_FAILURE);
        }
        if (semctl(barriers, 1, SETVAL, N) == -1) { // for now N = 15
            perror("Failed to set second barrier");
            exit(EXIT_FAILURE);
        }

        while (1) { //when no signal from police - else get everyone out and kill them
            // semctl should i read another (how many on brigde + on boat)

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

            if (msgrcv(boat_priority_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
                if (errno == ENOMSG) {
                    if (msgrcv(boat_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
                        if (errno == ENOMSG) {
                            //printf("No messages in boat1_priority_msgid.\n");
                        } else {
                            perror("msgrcv failed");
                            exit(EXIT_FAILURE);
                        }
                    } else
                        passengerSpotted = true;
                }
            } else
                passengerSpotted = true;

            if (!passengerSpotted)
                continue;

            printf("I am c%d and see %d on molo\n", nr, pass.pid_p);
            sleep(1);
            struct sembuf opening0 = {0, -1, 0};
            if (semop(barriers, & opening0, 1) == -1) {
                perror("Cannot get to the brigde! \n");
                exit(EXIT_FAILURE);
            }

            if (msgsnd(bridge_msgid, & pass, sizeof(pass) - sizeof(long int), 0) == -1) {
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
                printf("\n\nPassenger got on brigde %d safely %d places left\n\n", nr, num + num2 - K);

            // now get off of the bridge to the boat

            struct sembuf closing0 = {0, 1, 0};
            if (semop(barriers, & closing0, 1) == -1) {
                perror("Cannot get to the brigde! \n");
                exit(EXIT_FAILURE);
            }

            struct sembuf opening1 = {1, -1, 0};
            if (semop(barriers, & opening1, 1) == -1) {
                perror("Cannot get to the boat! \n");
                exit(EXIT_FAILURE);
            }

        }

        sleep(1);
        printf("\n\nBoat %d is sailing...\n\n", nr);
        sleep(T);
        printf("\n\nBoat %d returned :) now passengers get out ;v;\n\n", nr);

        while (1) { //when no signal from police - else get everyone out and kill them
            // semctl should i read another (how many on brigde + on boat)

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

            if (num + num2 == 20) {
                printf("Each passenger left the boat and the brigde %d", nr);
                break;
            } //as long as there are still some passengers...
            struct sembuf leaving1 = {1, 1, 0};
            if (semop(barriers, & leaving1, 1) == -1) {
                perror("Cannot get to the brigde! \n");
                exit(EXIT_FAILURE);
            }

            if (msgsnd(bridge_msgid, & pass, sizeof(pass) - sizeof(long int), 0) == -1) {
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
                printf("\n\nPassenger got off the brigde %d safely %d have to get out\n\n",nr, (N+K) - (num + num2));
            sleep(1);
            // now get off of the bridge to the boat

            struct sembuf entering0 = {0, -1, 0};
            if (semop(barriers, & entering0, 1) == -1) {
                perror("Cannot get to the brigde! \n");
                exit(EXIT_FAILURE);
            }

            struct sembuf leaving0 = {0, 1, 0};
            if (semop(barriers, & leaving0, 1) == -1) {
                perror("Cannot get to the boat! \n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main() {
    printf("Captain process started.\n");
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: captain of boat 1
        captain_process(1);
    } else if (pid > 0) {
        captain_process(2);
    } else if (pid < 0) {
        perror("Failed to fork passenger process");
        exit(EXIT_FAILURE);
    }
    return 0;
}