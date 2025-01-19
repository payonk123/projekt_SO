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

// Define the passenger structure
struct passenger {
    long int mtype; // Message type
    pid_t pid_p;    // Process ID of the passenger
    int age;        // Age of the passenger
    bool discount50; 
};

// Define the ticket structure
struct ticket {
    long int mtype; // Message type
    int assigned_boat; // Assigned boat number (1 or 2)
    int price; //if pass.another = 0 20 if pass.another = 1 10
};


void captain_process() {
    printf("HALO??");
    int boat1_priority_msgid, boat1_msgid, boat2_priority_msgid, boat2_msgid;
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
    while(1){

    if (msgrcv(boat1_priority_msgid, &pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
        if (errno == ENOMSG) {
        //printf("No messages in boat1_priority_msgid.\n");
    } else {
        perror("msgrcv failed");
        exit(EXIT_FAILURE);
    }
    }
    else
        printf("I see %d on molo\n", pass.pid_p);

    if (msgrcv(boat1_msgid, &pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
        if (errno == ENOMSG) {
        //printf("No messages in boat1_priority_msgid.\n");
    } else {
        perror("msgrcv failed");
        exit(EXIT_FAILURE);
        }
    }
    else
        printf("I see %d on molo\n", pass.pid_p);

    if (msgrcv(boat2_priority_msgid, &pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
        if (errno == ENOMSG) {
        //printf("No messages in boat1_priority_msgid.\n");
    } else {
        perror("msgrcv failed");
        exit(EXIT_FAILURE);
    }
    }
    else
        printf("I see %d on molo\n", pass.pid_p);

    if (msgrcv(boat2_msgid, &pass, sizeof(pass) - sizeof(long int), 0, IPC_NOWAIT) == -1) {
        if (errno == ENOMSG) {
        //printf("No messages in boat1_priority_msgid.\n");
    } else {
        perror("msgrcv failed");
        exit(EXIT_FAILURE);
    }
    }
    else{
        printf("I see %d on molo\n", pass.pid_p);
    }
    }

} 

int main() {
    printf("Captain process started.");
    captain_process();
    return 0;
}