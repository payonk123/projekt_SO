#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <sys/sem.h>
#include <signal.h>

void handle_signal(int signal) {
  printf("Passenger ends.\n");
  kill(0, SIGINT);
  exit(1);
}

// Define the passenger structure
struct passenger {
    long int mtype; // Message type
    pid_t pid_p;    // Process ID of the passenger
    int age;        // Age of the passenger
    bool discount50;   // czy uprawniony do znizki - gdy jechal tak gdy pierwszy raz to nir
};

// Define the ticket structure
struct ticket {
    long int mtype; // Message type
    int assigned_boat; // Assigned boat number (1 or 2)
};

struct parent_of_all{
    long int mtype;
    pid_t pid;
};

// Define message queue keys
#define PASSENGER_QUEUE_KEY 1234
#define CASHIER_QUEUE_KEY 5678

#define PASS_EX_QUEUE_KEY 1616

#define MOLO_QUEUE_P1_KEY 1111
#define MOLO_QUEUE_1_KEY 2222
#define MOLO_QUEUE_P2_KEY 3333
#define MOLO_QUEUE_2_KEY 4444
#define RETURNING_QUEUE_1_KEY 7777
#define RETURNING_QUEUE_2_KEY 8888

void passenger_process() {
    int passenger_msgid; //queue of passengers that want to get a ticket
    int cashier_msgid;
    int boat1_priority_msgid, boat1_msgid, boat2_priority_msgid, boat2_msgid, flag;
    int returning1_msgid;
    int returning2_msgid;
    struct passenger pass;
    struct ticket ticket;

    // Reseed the random number generator in the child process to ensure unique seeds
    srand(time(NULL)+getpid()); // Unique seed combining time and PID for randomness

    // Generate a random age for the passenger
    int age = rand() % 66 + 15; // Generate random age (15 to 80)

    // Get message queue ID for passenger
    passenger_msgid = msgget(PASSENGER_QUEUE_KEY, 0666);
    if (passenger_msgid == -1) {
        perror("Failed to get passenger queue");
        exit(EXIT_FAILURE);
    }

    cashier_msgid = msgget(CASHIER_QUEUE_KEY, 0666);
    if (cashier_msgid == -1) {
        perror("Failed to get cashier queue");
        exit(EXIT_FAILURE);
    }

    boat1_priority_msgid = msgget(MOLO_QUEUE_P1_KEY, 0666);
    if (boat1_priority_msgid == -1) {
        perror("Failed to create passenger queue");
        exit(EXIT_FAILURE);
    }

    boat1_msgid = msgget(MOLO_QUEUE_1_KEY, 0666);
    if (boat1_msgid == -1) {
        perror("Failed to create passenger queue");
        exit(EXIT_FAILURE);
    }

     boat2_priority_msgid = msgget(MOLO_QUEUE_P2_KEY, 0666);
    if (boat2_priority_msgid == -1) {
        perror("Failed to create passenger queue");
        exit(EXIT_FAILURE);
    }

    boat2_msgid = msgget(MOLO_QUEUE_2_KEY, 0666);
    if (boat2_msgid == -1) {
        perror("Failed to create passenger queue");
        exit(EXIT_FAILURE);
    }

    returning1_msgid = msgget(RETURNING_QUEUE_1_KEY, 0666);
    if (returning1_msgid == -1) {
        perror("Failed to create queue");
        exit(EXIT_FAILURE);
    }
    returning2_msgid = msgget(RETURNING_QUEUE_2_KEY, 0666);
    if (returning2_msgid == -1) {
        perror("Failed to create queue");
        exit(EXIT_FAILURE);
    }

    // Fill passenger data
    //pass.mtype = 1; // Arbitrary type for passenger messages
    pass.pid_p = getpid();
    pass.mtype = pass.pid_p;
    pass.age = age;
    pass.discount50 = 0;
    int go = 0;

    do {

    // Send passenger details to cashier
    if (msgsnd(passenger_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
        perror("Failed to send passenger details");
        exit(EXIT_FAILURE);
    }

    if (msgrcv(cashier_msgid, &ticket, sizeof(ticket) - sizeof(long int), pass.pid_p, 0) == -1) {
        perror("Failed to receive ticket");
        exit(EXIT_FAILURE);
    }

    printf("Passenger (PID: %d, Age: %d) assigned to boat %d.\n", pass.pid_p, age, ticket.assigned_boat);


    if(ticket.assigned_boat==1 && pass.discount50 == 0){
    if (msgsnd(boat1_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
        perror("Failed to send passenger details");
        exit(EXIT_FAILURE);
    }}

    if(ticket.assigned_boat==1 && pass.discount50 == 1){ //go == 3 ?
    if (msgsnd(boat1_priority_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
        perror("Failed to send passenger details");
        exit(EXIT_FAILURE);
    }}

    if(ticket.assigned_boat==2 && pass.discount50 == 0){
    if (msgsnd(boat2_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
        perror("Failed to send passenger details");
        exit(EXIT_FAILURE);
    }}

    if(ticket.assigned_boat==2 && pass.discount50 == 1){ //go == 3 ?
    if (msgsnd(boat2_priority_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
        perror("Failed to send passenger details");
        exit(EXIT_FAILURE);
    }}

    flag = semget(10+ticket.assigned_boat, 1, 0600 | IPC_CREAT);
    if (flag == -1) {
        perror("Failed to create barriers");
        exit(EXIT_FAILURE);
    }

    struct passenger ghost;

    if(ticket.assigned_boat==1){
        if (msgrcv(returning1_msgid, &ghost, sizeof(pass) - sizeof(long int), pass.pid_p, 0) == -1) {
            perror("Failed to receive ticket");
            exit(EXIT_FAILURE);
        }
        else{
            printf("\nPasazer %d dostal rozkaz wyjscia\n", getpid());
        }
    }

    else if(ticket.assigned_boat==2){
        if (msgrcv(returning2_msgid, &ghost, sizeof(pass) - sizeof(long int), pass.pid_p, 0) == -1) {
            perror("Failed to receive ticket");
            exit(EXIT_FAILURE);
        }
        else{
            printf("\nPasazer %d dostal rozkaz wyjscia\n", getpid());
        }
    }
    
    while(1){
        int num = semctl(flag, 0, GETVAL);
            if (num == -1) {
                perror("Semaphore read error");
                exit(EXIT_FAILURE);
            }
        if(num==0)
            break;
    }

    printf("Pasazer %d zakonczyl rejs!\n", pass.pid_p);

    go = rand() % 4;
    if(go == 3)
    printf("ja chce jeszcze raz! (jestem %d)\n", pass.pid_p);
    } while(go == 3);


    exit(EXIT_SUCCESS);
}

void spawn_passengers() {
    while (1) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process: Passenger
            passenger_process();
        } else if (pid < 0) {
            perror("Failed to fork passenger process");
            exit(EXIT_FAILURE);
        }
        //usleep((rand()%10 + 1)*750000); // spawn passengers in random time intervals
        sleep(5);
    }
}

int main(int argc, char *argv[]) {

    signal(SIGINT, handle_signal);

    struct parent_of_all parent;

    parent.mtype=1;
    parent.pid=getpid();

    int pass_ex_msgid;

    pass_ex_msgid = msgget(PASS_EX_QUEUE_KEY, 0666);
    if (pass_ex_msgid == -1) {
        perror("Failed to create queue POLPASS");
        exit(EXIT_FAILURE);
    }

    if (msgsnd(pass_ex_msgid, &parent, sizeof(parent) - sizeof(long int), 0) == -1) {
        perror("Failed to send parent's pid");
        exit(EXIT_FAILURE);
    }
    
    printf("Passenger process manager started. Spawning passengers...\n");

    spawn_passengers();

    return 0;
}
