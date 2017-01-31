/**
 * Joseph McGill
 * Fall 2015
 *
 * Program that dynamically allocates and manages simulated memory and
 * reports statistics on how it ran. Used to understand how operating systems
 * allocate and manage memory.
 **/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#define LONG 2000

/* Event struct */
typedef struct Event {
    int time;
    int type;
} Event;

/* Node struct for priority queue */
typedef struct Node Node;
struct Node {
    Event* event;
    Node* next;
} *head, *tail;

/* Function prototypes */
void enq(Event* e);
void deq();
Node* front();
int process_events();
int allocate(int i);
void release(int address);
int size(int header);
int preuse(int header);
int use(int header);
void combine_blocks(int left, int right);
bool is_following_free(int address);
bool is_prev_free(int address);
void statistics(int now, int allocations, int releases);

/* Global variables */
int FREE_START;
int memory[LONG];
bool first_release;

/* main */
int main() {

    /* seed random number generator with the system's current time */
    srand(time(NULL));

    /* initialize the head and tail nodes of the priority queue */
    Node* head = (Node*)malloc(sizeof(Node));
    Node* tail = (Node*)malloc(sizeof(Node));

    /* set FREE_START to 0, the first free block */
    FREE_START = 0;
    first_release = false;

    /* initialize all elements of memory to 0 to avoid avoid
     * random number errors */
    int i;
    for (i = 0; i < LONG; i++) {
        memory[i] = 0;
    }

    /* declares sentinel for free list */
    memory[0] = 0;
    memory[1] = 4;
    memory[2] = 4;
    memory[3] = 0;
    memory[LONG - 1] = 1;

    /* declares the initial free block */
    memory[4] = 4*(LONG - 5) + 2*0 + 0;
    memory[5] = 0;
    memory[6] = 0;
    memory[LONG - 2] = LONG - 5;

    process_events();

    return 0;
}

/* Function to add an element to the priority queue */
void enq(Event* e) {

    /* create a new node to insert */
    Node* temp = (Node*)malloc(sizeof(Node));
    temp->event = e;
    temp->next = NULL;

    /* if the queue is empty, insert temp at the head */
    if (head == NULL && tail == NULL) {

        head = tail = temp;
        return;
    }

    /* get the time of temp for comparison */
    int time = temp->event->time;

    /* get the time of the head node for comparison */
    Node* current_node = head;
    int current_time = current_node->event->time;

    /* find the position to insert the node, then insert it */
    if (time > 0 && time > current_time) {

        /* if temp should go immediately after the head node */
        if (current_node->next != NULL) {

            current_time = current_node->next->event->time;
        }

        /* find the location that temp should go */
        while (time > current_time && current_node->next != NULL) {
           current_node = current_node->next;

            if (current_node->next != NULL) {

                current_time = current_node->next->event->time;
            } else  break;

        }

        /* place temp at its location */
        if (current_node->next != NULL && current_node != tail) {

            Node* temp_node = current_node->next;
            current_node->next = temp;
            temp->next = temp_node;
        } else {

            tail->next = temp;
            tail = temp;
        }

    } else  {

        /* make temp the new head */
        temp->next = head;
        head = temp;
    }
}

/* Function to remove the first element from the priority queue */
void deq() {

    Node* temp = head;

    /* return if the queue is already empty */
    if (head == NULL) {
        return;
    }

    /* remove the first element of the queue */
    if (head == tail) {
        head = tail = NULL;
    } else {
        head = head->next;
    }

    /* free the dequeued event */
    free(temp);
}

/* Function to get the head of the queue */
Node* front() {

    /* if the list is empty, return NULL */
    if(head == NULL) {
        return NULL;
    }

    /* return the head of the queue */
    return head;
}


/* Function to process the events in the priority queue */
int process_events() {

    /* create the first event */
    Event event = (Event) {.time = 0, .type = -1};
    enq(&event);

    Node* temp_node;

    /* variables to hold the current time, type,
     * and address of the allocation */
    int now, type, address;

    /* variables to hold the random numbers generated for an allocation */
    int s, t, T;

    /* variables for counting the number of allocations and releases */
    int allocations = 0;
    int releases = 0;

    /* loop until 4000 allocations have been processed */
    while(allocations < 4000) {
        /* get the head of the queue */
        temp_node = front();
        Node* test = front();

        /* process event if a node exists */
        if (temp_node != NULL) {

            /* generate random numbers for the event */
            s = (rand() % 100) + 1;
            t = (rand() % 250) + 1;
            T = (rand() % 60)  + 1;

            now = temp_node->event->time;
            type = temp_node->event->type;

            deq(); //might cause error when destroying pointer

            if (type == -1) {

                address = allocate(s);

                /* exit program if memory could not be allocated */
                if (address == -1) {

                    printf("Request of size %d could not be honored\n", s);
                    int free_size = 0;
                    int counter = 0;
                    int address = 4;

                    do {

                        /* print the size of the free blocks */
                        if (memory[address] % 2 == 0) {

                            free_size = size(memory[address]);
                            counter = counter + 1;
                            printf("Size of free block %d: %d",
                                    counter, free_size);
                        }

                        address = address + size(memory[address]);
                    } while (address != LONG - 1);

                    return -1;
                }

                /* create a release event for the allocation */
                Event* event1 = (Event*)malloc(sizeof(Event));
                event1->time = now + t;
                event1->type = address;
                enq(event1);

                /* create a new allocation event */
                Event* event2 = (Event*)malloc(sizeof(Event));
                event2->time = now + T;
                event2->type = -1;
                enq(event2);

                /* increment the number of allocations */
                allocations++;

                /* print statistics after every 50 allocations */
                if (allocations % 50 == 0) {
                    /* print stats */
                    statistics(now, allocations, releases);
                }

            } else {

                /* release the memory at 'type' */
                release(type);

                /* increment the number of releases */
                releases++;
            }
        }  else return 0;


        /* print the details of the first 40 releases */
        if (releases < 40) {

            if (type == -1) {
                //allocate

                printf("Allocation of size %d at time %d to location %d\n",
                        s, now, address);
            } else {
                //release

                printf("Release memory at location %d at time %d\n", type, now);
            }
        }

    }

    /* return an error if somehow you make it out of the loop */
    return -1;

}

/* Function to allocate memory to an event */
int allocate(int i) {

    /* variables */
    int temp_start = FREE_START;
    int alloc_size, free_size;
    int k = i;

    /* round up to 3 if the requested size is less than 3 */
    if (k < 3) k = 3;

    /* loop until partition is found, or list is completely searched */
    do {

        /* get the size of the current free block */
        free_size = size(memory[temp_start]);

        /* allocate 1 more than the size needed to make room for the header */
        alloc_size = k + 1;

        /* if a large enough parition is found, allocate it */
        if (alloc_size <= free_size) {

            /* if the free block left over is smaller than k + 5,
             * allocate the entire block */
            if((free_size - alloc_size) <  4) {

                /* next to previous, previous to next */
                memory[memory[temp_start + 2] + 1] = memory[temp_start + 1];
                memory[memory[temp_start + 1] + 2] = memory[temp_start + 2];

                /* set info of free block to zero */
                memory[temp_start + 1] = 0;
                memory[temp_start + 2] = 0;
                memory[temp_start + free_size - 1] = 0;

                /* change the starting position of the free block to the next
                 * available free block */
                FREE_START = memory[temp_start + 1];

                /* change the preuse bit of the next block to 1, unless it
                 * is the last number in the array */
                if ((temp_start + free_size) != (LONG - 1)) {
                    memory[temp_start + free_size] =
                                        memory[temp_start + free_size] + 2;
                }

                /* set the header of the newly allocated block */
                memory[temp_start] = 4*(free_size) + 2*1 + 1;

                /* return the address of the newly allocated block */
                return temp_start;

            } else {

                /* set start of new partition */
                memory[temp_start + alloc_size] = 4*(free_size - alloc_size)
                                                + 2*1 + 0;

                /* set f link of new partition */
                memory[temp_start + alloc_size + 1] = memory[temp_start + 1];
                memory[memory[temp_start + 2] + 1] = temp_start + alloc_size;

                /* set b link of new partition */
                memory[temp_start + alloc_size + 2] = memory[temp_start + 2];
                memory[memory[temp_start + 1] + 2] = temp_start + alloc_size;

                /* set size of new partition */
                memory[temp_start + free_size - 1] = free_size - alloc_size;

                /* set header of allocated partition */
                memory[temp_start] = 4*alloc_size + 2*1 + 1;

                FREE_START = temp_start + alloc_size;
                return temp_start;
            }

        } else {

            /* check next free partition */
            temp_start = memory[temp_start + 1];
        }
    } while (temp_start != FREE_START);

    /* terminate if partition could not be allocated */
    return -1;
}

/* Function to release the memory starting at the address provided */
void release(int address) {
    int temp;

    /* handle special first release case */
    if (!first_release) {

        temp = FREE_START;
        first_release = true;
    } else temp = memory[2];

    /* add free block to the free list  */
    memory[2] = address;
    memory[address + 1] = 0;
    memory[address + 2] = temp;
    memory[temp + 1] = address;
    memory[address] = memory[address] - 1;

    /* set the size of the new free block */
    memory[address + size(memory[address]) - 1] = size(memory[address]);


    /* change the preuse bit of the following block to 0 */
    if (memory[address + size(memory[address])] != 1999) {

        memory[address + size(memory[address])] =
                                   memory[address + size(memory[address])] - 2;
    }

    /* combine with following block if free */
    if (is_following_free(address)) {


        /* get the next address, then combine the two blocks */
        int next_address = address + size(memory[address]);
        combine_blocks(address, next_address);
    }

    /* combine with previous block if free */
    if (is_prev_free(address)) {

        /* get the previous address, then combine the two blocks */
        int prev_address =  address - memory[address - 1];
        combine_blocks(prev_address, address);
    }

    return;
}

/* Function to return the size of a partition */
int size(int header) {
    return (int) header / 4;
}

/* Function to return the preuse bit of a partition */
int preuse(int header) {
    return (int) (header % 4) / 2;
}

/* Function to return the use bit of a partition */
int use(int header) {
    return header % 2;
}

/* Function to determine if a following block is free or not */
bool is_following_free(int address) {

    /* get the following address */
    int following_address = address + size(memory[address]);

    /* return true if following block is free */
    if(memory[following_address + size(memory[following_address]) - 1] ==
       size(memory[following_address])) {
        return true;
    } else return false;
}

/* Function to determine if a previous block is free or not */
bool is_prev_free(int address) {

    /* return true if previous block is free */
    if(memory[address - 1] == size(memory[address - memory[address - 1]])) {
        return true;
    } else return false;
}

/* Function to combine two adjacent free blocks of memory */
void combine_blocks(int left, int right) {

    /* change the f link of the previous partition to the previous of left */
    memory[memory[left + 2] + 1] = memory[left + 1];
    memory[memory[left + 1] + 2] = memory[left + 2];

    /* set f link of new partition */
    memory[left + 1] = memory[right + 1];

    /* set b link of new partition */
    memory[left + 2] = memory[right + 2];

    /* change the f link of previous free block */
    memory[memory[left + 2] + 1] = left;
    memory[memory[left + 1] + 2] = left;

    /* set header of new partition */
    memory[left] = 4*(size(memory[left]) + size(memory[right]))
                 + 2*(preuse(memory[left])) + use(memory[right]);

    /* set size of new partition */
    memory[right + size(memory[right]) - 1] = size(memory[left]);

    /* remove the old partition */
    memory[right] = 0;
    memory[right - 1] = 0;
    memory[right + 1] = 0;
    memory[right + 2] = 0;
}

/* Function to print the statistics of the memory */
void statistics(int now, int allocations, int releases) {

    printf("\nStatistics for %d allocations\n-------------------"
           "--------------\n", allocations);

    /* print the simulated time */
    printf("Time: %d\n", now);

    /* print the number of allocations */
    printf("Allocations: %d\n", allocations);

    /* print the number of releases */
    printf("Releases: %d\n", releases);

    /* variables for storing information on the free and allocated blocks */
    int free_blocks = 0;
    int allocated_blocks = 0;
    int total_free = 0;
    int total_allocated = 0;
    int address = 4;

    /* step through the memory list and determine the number of
     * free and allocated blocks */
    do {

        /* check if the use bit is 1 for allocated or 0 for free */
        if (memory[address] % 2 == 1) {

            allocated_blocks++;
            total_allocated = total_allocated + size(memory[address]);

        } else {

            free_blocks++;
            total_free = total_free + size(memory[address]);
        }

        address = address + size(memory[address]);
    } while (address != LONG - 1);

    /* print the number of free blocks */
    printf("Free blocks: %d\n", free_blocks);

    /* print the number of allocated blocks */
    printf("Allocated blocks: %d\n", allocated_blocks);

    /* print the total size of the free blocks */
    printf("Total size of free blocks: %d\n", total_free);

    /* print the total size of the allocated blocks */
    printf("Total size of allocated blocks: %d\n", total_allocated);

    /* average size of free blocks */
    float free_average = (float) total_free / free_blocks;
    printf("Average size of free blocks: %.2f\n", free_average);

    /* average size of allocated blocks */
    float allocated_average = (float) total_allocated / allocated_blocks;
    printf("Average size of allocated blocks: %.2f\n", allocated_average);

    /*number of requests that could be met for sizes equal to the average
     * allocated block size */
    address = 4;
    int u = allocated_average + 1;
    int k = 0;
    do {
        if (memory[address] % 2 == 0) {
            if (size(memory[address]) >= u) {
                k++;
            }
        }

        address = address + size(memory[address]);
    } while (address != LONG - 1);

    printf("Number of requests that could be met: %d\n", k);

    /* percentage of free memory that could not be occupied */
    float percentage_of_free_memory = 100 * (1 - ((k * allocated_average)
                                      / total_free));
    printf("Percentage of free memory: %.2f\n", percentage_of_free_memory);
}
