# Boat trip simulation

The project's aim is to simulate cruises on two boats.

The project implements all of its objectives through the following programs:

### Passenger Program
- Generates passengers aged 1 to 80, with or without children
- After receiving a ticket from the cashier, the passenger joins the appropriate queue – priority if it's the passenger's next cruise, or regular if not.
- After the cruise, the passenger decides whether to take another cruise – if not, the process ends for that particular passenger. If so, they must return to the cashier for a ticket and a new boat assignment. Then, he or she buys a ticket with a 50% discount.

### Cashier Program
- The cashier's task is to assign a ticket for the appropriate boat and price to the passenger.

- If the cashier notices that a passenger has a 50% discount, they reduce the ticket price.

- The cashier will always issue passengers older than 70 and those with children a ticket for boat number 2.

### Skipper Program
- The skipper is responsible for loading, sailing, and unloading passengers. They allow passengers onto the boat so that there are never more than K people on the bridge and no more than N people in the boat.

- The skipper ensures that those who come for a repeat trip on a given day board the boat without waiting in line (they initially invite passengers standing in line with priority for the cruise).

- They end and start cruises at the appropriate times. The cruise ends when the skipper receives a signal from a police officer. Additionally, the cruise starts even when the boat is not full, if a significant amount of time has passed and there is at least one passenger on the boat.

### Policeman program
- sends a SIGUSR1 signal to the skipper from boat 1 and a SIGUSR2 signal to the skipper from boat 2, so that the cruises end before the Tk time.
- terminates all processes and then terminates itself, forcing the
deletion of all used structures (semaphores and message queues).


## Role of each IPC resource.

#### Queues and semaphores in the passenger-cashier relationship
- The passenger_msgid queue is used to send passenger information to the cashier so that they can assign the appropriate ticket ( with price and assigned boat) to the passenger
- The cashier_msgid queue is the queue through which the cashier assigns the appropriate ticket to a specific passenger.
#### Queues and signals in the passenger-skipper relationship
- boat1_priority_msgid queue carries information about a passenger aged 15 to 70 years who intends to take another cruise on boat 1.
- boat1_msgid queue carries information about a passenger aged 15 to 70 years who intends to take their first cruise on boat 1.
- boat2_priority_msgid queue carries information about a passenger intending to take a repeat cruise on boat 2 – this may include passengers aged 71 to 80 years and passengers with children.
- boat2_msgid queue carries information about a passenger aged 15 to 70 years who intends to take their first cruise on boat 2 – it can accommodate passengers aged 71 to 80, as well as passengers with children.
The skipper receives the information sent via queues by the passenger and then manages the distribution and loading order of passengers.

Four queues are necessary to resolve the issue of boarding a passenger who has already been on a trip, bypassing the regular queue.

- The returning1_msgid queue is responsible for receiving information from the skipper of boat 1 about passengers who have completed their trip, allowing them to decide only after the trip ends whether they want to go on another trip (the passenger selection is randomly selected using the go variable; if the passenger decides to go on another trip, they are entitled to a discount until the end of the day).
- The returning2_msgid queue performs the same task as returning1_msgid, but for boat 2.
- The flag is a semaphore that is set to 1 (in the skipper's program) at the start of the trip to prevent a passenger already on a trip from trying to buy a ticket for another trip. The end of the trip, meaning the boat is fully unloaded, leads to the passenger leaving the semaphore – then they can go buy a ticket.

The returning1_msgid, returning2_msgid, and flag semaphores act as a defense mechanism against passenger entrapment on the bridge – the passenger must wait until they first disembark themselves, and then until everyone else disembarks, in order to potentially go through the process of purchasing a new ticket and boarding again (thus bypassing the queue).
#### Queues and semaphores in the skipper-skipper relationship
- The bridge_msgid queue carries information about passengers that are currently on the bridge.
- The boat_queue queue carries information about passengers that are currently on the boat.
- A group of two semaphores (separate for each skipper) constitutes two barriers - the first before entering the bridge, the second before entering the boat.
- The child_msgid queue is a special queue with the highest priority for passengers with children, which addresses the problem of only one seat being available for such a passenger.
#### Queues in the policeman - skipper/cashier/passenger relationship
- the queues captain_msgid, cashier_ex_msgid, pass_ex_msgid are only for the policeman to know the PIDs of the processes that must end before Tk - only then is it possible for him to send signals.

## Execution

The project uses Linux libraries, so to start the program you need to compile and execute it on Linux or WSL using these commands:
```bash
gcc -o cashier kasjer.c
gcc -o skipper sternik.c
gcc -o policeman policjant.c
gcc -o passenger pasazer.c

./policeman & ./cashier & ./skipper & ./passenger
```

