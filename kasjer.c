#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

int passenger_msgid, cashier_msgid;

void handle_signal(int signal) {
  printf("Cashier process ends\n");

  msgctl(passenger_msgid, IPC_RMID, NULL);
  msgctl(cashier_msgid, IPC_RMID, NULL);
  exit(1);
}

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

struct cashier {
    long int mtype; // Message type
    pid_t pid; // Process ID of the cashier
};

// Define message queue keys
#define PASSENGER_QUEUE_KEY 1234 // With this queue cashier will receive information about passengers
#define CASHIER_QUEUE_KEY 5678 // With this queue cashier will send information about assigned ticket and boat 
#define CASHIER_EX_QUEUE_KEY 1919
// Price of the Ticket without a discount
#define TICKET 20.0

void cashier_process() {

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
        // Confirm receiving passenger's detail 
        printf("Cashier received passenger (PID: %d, Age: %d).\n", pass.pid_p, pass.age);

        // Assign boat based on age and whether passenger is with a child
        ticket.mtype = pass.pid_p; // Send ticket to the specific passenger
        if (pass.age > 70 || pass.child_age != -1) {
            ticket.assigned_boat = 2; // Assign boat 2 due to age > 70
        } 
        else {
            ticket.assigned_boat = (rand() % 2) + 1; // Assign random boat (1 or 2)
        }
        
        ticket.price = TICKET; 

        if (pass.child_age != -1 && pass.child_age > 2) {
            ticket.price *= 2; // If passenger's child is three or more, both need to pay for ticket
        } // There is no reason to add 'if' for parents with children aged 1 or 2 as they pay their_ticket + 0

        // If passenger has been on the boat before the ticket is 50% cheaper
        if (pass.discount50 == 1) {
            ticket.price /= 2;
        }
            


        // Send ticket to passenger
        if (msgsnd(cashier_msgid, &ticket, sizeof(ticket) - sizeof(long int), 0) == -1) {
            perror("Failed to send ticket");
            exit(EXIT_FAILURE);
        }

        printf("Ticket for passenger (PID: %d) sent. Assigned boat: %d. Age of passenger's child is: %d. Ticket's price: %d\n "
        , pass.pid_p, ticket.assigned_boat, pass.child_age, ticket.price);
    }
}

int main() {

    signal(SIGINT, handle_signal);

    srand(time(NULL)); // Initialize random number generator - for boat selection
    struct cashier cashier;
    cashier.mtype=1;
    cashier.pid=getpid(); // Get cashier's pid for policeman to stop process when needed

    int cashier_ex_msgid; // Initialize queue with which we will send cashier's pid to policeman


    // Create this queue
    cashier_ex_msgid = msgget(CASHIER_EX_QUEUE_KEY, 0666);
    if (cashier_ex_msgid == -1) {
        perror("Failed to create queue POLPASS");
        exit(EXIT_FAILURE);
    }

    // Send cashier's pid to the policeman
    if (msgsnd(cashier_ex_msgid, &cashier, sizeof(cashier) - sizeof(long int), 0) == -1) {
        perror("Failed to send parent's pid");
        exit(EXIT_FAILURE);
    }

    printf("Cashier process started.\n");
    cashier_process();
    return 0;
}
