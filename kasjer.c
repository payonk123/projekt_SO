#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

void handle_signal(int signal) {
  printf("Cashier process ends\n");
  exit(1);
}

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

struct cashier {
    long int mtype; 
    pid_t pid;
};

// Define message queue keys
#define PASSENGER_QUEUE_KEY 1234
#define CASHIER_QUEUE_KEY 5678
#define CASHIER_EX_QUEUE_KEY 1919

void cashier_process() {
    int passenger_msgid, cashier_msgid;
    struct passenger pass;
    struct ticket ticket;

    // Create message queues
    passenger_msgid = msgget(PASSENGER_QUEUE_KEY, IPC_CREAT | 0666);
    if (passenger_msgid == -1) {
        perror("Failed to create passenger queue");
        exit(EXIT_FAILURE);
    }

    cashier_msgid = msgget(CASHIER_QUEUE_KEY, IPC_CREAT | 0666);
    if (cashier_msgid == -1) {
        perror("Failed to create cashier queue");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Receive passenger details
        if (msgrcv(passenger_msgid, &pass, sizeof(pass) - sizeof(long int), 0, 0) == -1) {
            perror("Failed to receive passenger details");
            exit(EXIT_FAILURE);
        }

        printf("Cashier received passenger (PID: %d, Age: %d).\n", pass.pid_p, pass.age);

        // Assign boat based on age
        ticket.mtype = pass.pid_p; // Send ticket to the specific passenger
        if (pass.age > 70) {
            ticket.assigned_boat = 2; // Assign boat 2
        } else {
            ticket.assigned_boat = (rand() % 2) + 1; // Assign random boat (1 or 2)
        }
        if (pass.discount50 == 0) {
            pass.discount50 = 1; // next time will be another
            ticket.price = 20;
        } else {
            ticket.price = 10; // 50% discount for passengers sailing on repeat
        }


        // Send ticket to passenger
        if (msgsnd(cashier_msgid, &ticket, sizeof(ticket) - sizeof(long int), 0) == -1) {
            perror("Failed to send ticket");
            exit(EXIT_FAILURE);
        }

        printf("Ticket for passenger (PID: %d) sent. Assigned boat: %d.\n", pass.pid_p, ticket.assigned_boat);
    }
}

int main() {

    signal(SIGINT, handle_signal);

    srand(time(NULL)); // Initialize random number generator
    struct cashier cashier;
    cashier.mtype=1;
    cashier.pid=getpid();

    int cashier_ex_msgid;

    cashier_ex_msgid = msgget(CASHIER_EX_QUEUE_KEY, 0666);
    if (cashier_ex_msgid == -1) {
        perror("Failed to create queue POLPASS");
        exit(EXIT_FAILURE);
    }

    if (msgsnd(cashier_ex_msgid, &cashier, sizeof(cashier) - sizeof(long int), 0) == -1) {
        perror("Failed to send parent's pid");
        exit(EXIT_FAILURE);
    }

    printf("Cashier process started.\n");
    cashier_process();
    return 0;
}
