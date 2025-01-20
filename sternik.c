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

void captain_process() {
    int boat1_priority_msgid, boat1_msgid, bridge1_msgid, barriers_1;
    struct passenger pass;

    // Create message queues
    boat1_priority_msgid = msgget(MOLO_QUEUE_P1_KEY, IPC_CREAT | 0666);
    if (boat1_priority_msgid == -1) {
        perror("Failed to create b1_p");
        exit(EXIT_FAILURE);
    }

    boat1_msgid = msgget(MOLO_QUEUE_1_KEY, IPC_CREAT | 0666);
    if (boat1_msgid == -1) {
        perror("Failed to create b1");
        exit(EXIT_FAILURE);
    }

    bridge1_msgid = msgget(BRIDGE_QUEUE_1_KEY, IPC_CREAT | 0666);
    if (bridge1_msgid == -1) {
        perror("Failed to create bridge1");
        exit(EXIT_FAILURE);
    }

    barriers_1 = semget(123, 2, 0600 | IPC_CREAT);
    if (barriers_1 == -1) {
        perror("Failed to create barriers");
        exit(EXIT_FAILURE);
    }

    if (semctl(barriers_1, 0, SETVAL, 5) == -1) {
        perror("Failed to set first barrier");
        exit(EXIT_FAILURE);
    }
    if (semctl(barriers_1, 1, SETVAL, 15) == -1) {
        perror("Failed to set second barrier");
        exit(EXIT_FAILURE);
    }

   while (1) {

        if (msgrcv(boat1_priority_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
            if (errno == ENOMSG) {
                //printf("No messages in boat1_priority_msgid.\n");
            } else {
                perror("msgrcv failed");
                exit(EXIT_FAILURE);
            }
        } else
            printf("I am c1 and see %d on molo\n", pass.pid_p);

        if (msgrcv(boat1_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
            if (errno == ENOMSG) {
                //printf("No messages in boat1_priority_msgid.\n");
            } else {
                perror("msgrcv failed");
                exit(EXIT_FAILURE);
            }
        } else
            printf("I am c1 and see %d on molo\n", pass.pid_p);
    }

}
void captain_process2() {
    int boat2_priority_msgid, boat2_msgid, bridge2_msgid, barriers_2;
    struct passenger pass;

    boat2_priority_msgid = msgget(MOLO_QUEUE_P2_KEY, IPC_CREAT | 0666);
    if (boat2_priority_msgid == -1) {
        perror("Failed to create b2_p");
        exit(EXIT_FAILURE);
    }

    boat2_msgid = msgget(MOLO_QUEUE_2_KEY, IPC_CREAT | 0666);
    if (boat2_msgid == -1) {
        perror("Failed to create b2");
        exit(EXIT_FAILURE);
    }

    bridge2_msgid = msgget(BRIDGE_QUEUE_2_KEY, IPC_CREAT | 0666);
    if (bridge2_msgid == -1) {
        perror("Failed to create bridge2");
        exit(EXIT_FAILURE);
    }

    barriers_2 = semget(123, 2, 0600 | IPC_CREAT);
    if (barriers_2 == -1) {
        perror("Failed to create barriers");
        exit(EXIT_FAILURE);
    }

    if (semctl(barriers_2, 0, SETVAL, 5) == -1) {
        perror("Failed to set first barrier");
        exit(EXIT_FAILURE);
    }

    if (semctl(barriers_2, 1, SETVAL, 15) == -1) {
        perror("Failed to set second barrier");
        exit(EXIT_FAILURE);
    }

    while (1) {

        if (msgrcv(boat2_priority_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
            if (errno == ENOMSG) {
                //printf("No messages in boat1_priority_msgid.\n");
            } else {
                perror("msgrcv failed");
                exit(EXIT_FAILURE);
            }
        } else
            printf("I am c2 and see %d on molo\n", pass.pid_p);

        if (msgrcv(boat2_msgid, & pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
            if (errno == ENOMSG) {
                //printf("No messages in boat1_priority_msgid.\n");
            } else {
                perror("msgrcv failed");
                exit(EXIT_FAILURE);
            }
        } else {
            printf("I am c2 and see %d on molo\n", pass.pid_p);
        }
    }

}

int main() {
    printf("Captain process started.");
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: captain of boat 1
        captain_process();
    } else if (pid > 0) {
        captain_process2();
    } else if (pid < 0) {
        perror("Failed to fork passenger process");
        exit(EXIT_FAILURE);
    }
    return 0;
}