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
  printf("Passenger process is ending...\n");
  kill(0, SIGINT);
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


// This is only needed for killing all passengers and stopping them from spawning
struct parent_of_all{
    long int mtype;
    pid_t pid;
};

// With this queue we will send passenger's details to cashier
#define PASSENGER_QUEUE_KEY 1234
// With this queue we will receive ticket details
#define CASHIER_QUEUE_KEY 5678
// With this queue we will send passenger process pid to a policeman
#define PASS_EX_QUEUE_KEY 1616
// With this queue we will manage passengers on molo that has already been on a trip today and want to get to boat 1
#define MOLO_QUEUE_P1_KEY 1111
// With this queue we will manage passengers on molo that hasn't been on a trip today and want to get to boat 1
#define MOLO_QUEUE_1_KEY 2222
// With this queue we will manage passengers on molo that has already been on a trip today and want to get to boat 2
#define MOLO_QUEUE_P2_KEY 3333
// With this queue we will manage passengers on molo that hasn't been on a trip today and want to get to boat 2
#define MOLO_QUEUE_2_KEY 4444
// With this queue we will be able to identify passengers returning from the trip on boat 1 for a little while
#define RETURNING_QUEUE_1_KEY 7777
// With this queue we will be able to identify passengers returning from the trip on boat 2 for a little while
#define RETURNING_QUEUE_2_KEY 8888

void passenger_process() {
    int passenger_msgid; 
    int cashier_msgid;
    // flag is a semaphore that informs us whether boat is sailing or not
    int boat1_priority_msgid, boat1_msgid, boat2_priority_msgid, boat2_msgid, flag;
    int returning1_msgid;
    int returning2_msgid;
    struct passenger pass;
    struct ticket ticket;

    // Reseed the random number generator in the child process to ensure unique seeds
    srand(time(NULL)+getpid()); // Unique seed combining time and PID for randomness
    // This generally makes each of our passengers unique as each seed is unique thanks to pid uniqueness

    // I will assume about 1/3 of passengers will bring a child with them
    int child_rand = rand()%3;
    if(child_rand == 0) {
        pass.child_age = rand()%14+1; // child has age for 1 to 14 
        pass.age = rand()%63+18; // Adult has to be at least 18
    }
    else {
        pass.child_age = -1; // childless passenger
        pass.age = rand() % 66 + 15; // passenger from 15 can sail without adults, passengers are from 1 to 80
    }
    

    // Get message queue ID for passenger
    // The queues are created by captain
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
    pass.pid_p = getpid();
    pass.mtype = pass.pid_p;
    pass.discount50 = 0; // No discount for the first trip
    // Variable  'go' is for simulating passenger's decision to go on another boat trip
    int go = 0; 

    do {

    // Send passenger details to cashier
    if (msgsnd(passenger_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
        perror("Failed to send passenger details");
        exit(EXIT_FAILURE);
    }

    // Receive ticket from the cashier
    if (msgrcv(cashier_msgid, &ticket, sizeof(ticket) - sizeof(long int), pass.pid_p, 0) == -1) {
        perror("Failed to receive ticket");
        exit(EXIT_FAILURE);
    }

    printf("Passenger (PID: %d, Age: %d) assigned to boat %d.\n", pass.pid_p, pass.age, ticket.assigned_boat);

    // deciding in which line to stand based on ticket details:

    if(ticket.assigned_boat==1 && pass.discount50 == 0){
    if (msgsnd(boat1_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
        perror("Failed to send passenger details");
        exit(EXIT_FAILURE);
    }}

    if(ticket.assigned_boat==1 && pass.discount50 == 1){ 
    if (msgsnd(boat1_priority_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
        perror("Failed to send passenger details");
        exit(EXIT_FAILURE);
    }}

    if(ticket.assigned_boat==2 && pass.discount50 == 0){
    if (msgsnd(boat2_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
        perror("Failed to send passenger details");
        exit(EXIT_FAILURE);
    }}

    if(ticket.assigned_boat==2 && pass.discount50 == 1){ 
    if (msgsnd(boat2_priority_msgid, &pass, sizeof(pass) - sizeof(long int), 0) == -1) {
        perror("Failed to send passenger details");
        exit(EXIT_FAILURE);
    }}

    // Creating semaphore that will inform passenger if they can get on the boat
    flag = semget(10+ticket.assigned_boat, 1, 0600 | IPC_CREAT);
    if (flag == -1) {
        perror("Failed to create barriers");
        exit(EXIT_FAILURE);
    }

    struct passenger ghost;

    // Getting details of passengers who returned from the trip

    if(ticket.assigned_boat==1){
        if (msgrcv(returning1_msgid, &ghost, sizeof(pass) - sizeof(long int), pass.pid_p, 0) == -1) {
            perror("Failed to receive ticket");
            exit(EXIT_FAILURE);
        }
        else{
            printf("\nPassenger %d is ordered to leave\n", getpid());
        }
    }

    else if(ticket.assigned_boat==2){
        if (msgrcv(returning2_msgid, &ghost, sizeof(pass) - sizeof(long int), pass.pid_p, 0) == -1) {
            perror("Failed to receive ticket");
            exit(EXIT_FAILURE);
        }
        else{
            printf("\nPassenger %d is ordered to leave\n", getpid());
        }
    }
    
    // Waiting for all passengers to exit the boat
    while(1){
        int num = semctl(flag, 0, GETVAL);
            if (num == -1) {
                perror("Semaphore read error");
                exit(EXIT_FAILURE);
            }
        if(num==0)
            break;
    }

    printf("Passenger %d ended their trip!\n", pass.pid_p);

    go = rand() % 4; // I will assume chances of someone going for a second trip are 25%
    if(go == 3){
        pass.discount50 = 1;
        printf("I want to go on another trip (I am %d)\n", pass.pid_p);
    }
    } while(go == 3); // you are still the same passenger, but you need to go through the same process
    // of receiving the ticket again


    exit(EXIT_SUCCESS); // kills passenger
}


// The parent spawns passengers - children are passengers
void spawn_passengers() {
    while (1) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process: Passenger
            passenger_process();
        } 
        else if (pid < 0) {
            perror("Failed to fork passenger process");
            exit(EXIT_FAILURE);
        }
        usleep((rand()%10 + 1)*500000); // spawn passengers in random time intervals
    }
}

int main(int argc, char *argv[]) {

    signal(SIGINT, handle_signal);

    // For sending passenger's parent details to policeman who will end all processes
    struct parent_of_all parent;

    parent.mtype=1;
    parent.pid=getpid();

    // Creating queue with which we will send pid of a parent process to policeman

    int pass_ex_msgid;

    pass_ex_msgid = msgget(PASS_EX_QUEUE_KEY, 0666);
    if (pass_ex_msgid == -1) {
        perror("Failed to create queue POLPASS");
        exit(EXIT_FAILURE);
    }

    // Sending the pid of a parent process to policeman
    if (msgsnd(pass_ex_msgid, &parent, sizeof(parent) - sizeof(long int), 0) == -1) {
        perror("Failed to send parent's pid");
        exit(EXIT_FAILURE);
    }
    
    printf("Passenger process manager started. Spawning passengers...\n");

    spawn_passengers();

    return 0;
}
