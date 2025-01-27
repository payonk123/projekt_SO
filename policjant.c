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

// Receiving pids of processes to kill from these queues:
#define CAPTAIN_QUEUE_KEY 1313
#define PASS_EX_QUEUE_KEY 1616
#define CASHIER_EX_QUEUE_KEY 1919

// Time for all trips (how much time from T_p to T_k) 
#define Tp_Tk 120


// Processes to kill:
struct captain {
    long int mtype;
    pid_t pid_c;
    int which_c; //1 or 2
};

struct parent_of_all{
    long int mtype;
    pid_t pid;
};

struct cashier {
    long int mtype; 
    pid_t pid;
};

int main(){
    printf("Policeman process started\n");
    int captain_msgid, cashier_ex_msgid, pass_ex_msgid;
    int c1, c2;
    struct captain captain;
    struct parent_of_all parent;
    struct cashier cashier;

    captain_msgid = msgget(CAPTAIN_QUEUE_KEY, IPC_CREAT | 0666);
    if (captain_msgid == -1) {
        perror("Failed to create POLICEMAN queue");
        exit(EXIT_FAILURE);
    }

    cashier_ex_msgid = msgget(CASHIER_EX_QUEUE_KEY, IPC_CREAT | 0666);
    if (cashier_ex_msgid == -1) {
        perror("Failed to create POLICEMAN queue");
        exit(EXIT_FAILURE);
    }

    pass_ex_msgid = msgget(PASS_EX_QUEUE_KEY, IPC_CREAT | 0666);
    if (pass_ex_msgid == -1) {
        perror("Failed to create POLICEMAN queue");
        exit(EXIT_FAILURE);
    }

    // Wait for the end of the day to send signal to captain
    sleep(Tp_Tk);

    if (msgrcv(captain_msgid, &captain, sizeof(captain) - sizeof(long int), 0, 0) == -1) {
        perror("failed");
        exit(EXIT_FAILURE);
    }

    // Kill captains
    printf("Policeman received captain PID: %d\n", captain.pid_c);
    // There are two captains so we have to identify them to send proper signals: SIGUSR1 or SIGUSR2
    if(captain.which_c == 1)
        c1 = captain.pid_c;
    else
        c2 = captain.pid_c;

    if (msgrcv(captain_msgid, &captain, sizeof(captain) - sizeof(long int), 0, 0) == -1) {
        perror("failed");
        exit(EXIT_FAILURE);
    }

    printf("Policeman received captain PID: %d\n", captain.pid_c);
    if(captain.which_c == 1)
        c1 = captain.pid_c;
    else
        c2 = captain.pid_c;

    if (kill(c1, SIGUSR1) == -1) {
        perror("Failed to send signal");
        exit(EXIT_FAILURE);
    }

    if (kill(c2, SIGUSR2) == -1) {
        perror("Failed to send signal");
        exit(EXIT_FAILURE);
    }

    // killing cashier and passeners based on received pids:

    if (msgrcv(pass_ex_msgid, &parent, sizeof(parent) - sizeof(long int), 0, 0) == -1) {
        perror("failed");
        exit(EXIT_FAILURE);
    }

    if (kill(parent.pid, SIGINT) == -1) {
        perror("Failed to send signal killing passangers");
        exit(EXIT_FAILURE);
    }

    if (msgrcv(cashier_ex_msgid, &cashier, sizeof(cashier) - sizeof(long int), 0, 0) == -1) {
        perror("failed");
        exit(EXIT_FAILURE);
    }

    if (kill(cashier.pid, SIGINT) == -1) {
        perror("Failed to send signal killing passangers");
        exit(EXIT_FAILURE);
    }

    // cleaning up resources when everyone is gone

    msgctl(cashier_ex_msgid, IPC_RMID, NULL);
    msgctl(pass_ex_msgid, IPC_RMID, NULL);
    msgctl(captain_msgid, IPC_RMID, NULL);


    exit(1);
}