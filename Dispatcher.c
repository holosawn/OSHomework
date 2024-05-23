#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Sabit değerlerin tanımlanması
#define RAM_SIZE 2048
#define CPU1_ALLOCATED_RAM 512
#define CPU2_QUEUE2_QUANTUM 8
#define CPU2_QUEUE3_QUANTUM 16
#define MAX_PROCESSES 500

// Process yapısı
typedef struct {
    char process_number[5];
    int arrival_time;
    int priority;
    int burst_time;
    int ram;
    int cpu;
} Process;

// Kuyruk düğümü yapısı
typedef struct Node {
    Process process;
    struct Node *next;
} Node;

// Kuyruk yapısı
typedef struct {
    Node *front;
    Node *rear;
} Queue;

// CPU yapısı
typedef struct {
    int remaining_busy_time;
    int available_rate;
    Process *current_process;
    Queue *current_queue;
} Cpu;

void initQueue(Queue *q) {
    q->front = q->rear = NULL;
}

int isEmpty(Queue *q) {
    return q->front == NULL;
}

void enqueue(Queue *q, Process process) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->process = process;
    newNode->next = NULL;
    if (isEmpty(q)) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

Process dequeue(Queue *q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        exit(1);
    }
    Node *temp = q->front;
    Process process = temp->process;
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    free(temp);
    return process;
}

Process peek(Queue *q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        exit(1);
    }
    return q->front->process;
}

// Reads the given file and parses processes
void readInputFile(const char *filename, Process processes[], int *process_count) {
    // open file
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Cannot open file %s\n", filename);
        exit(1);
    }
    // Read each line of the file and parse processes information
    char line[100];
    *process_count = 0;
    while (fgets(line, sizeof(line), file)) {
        Process process;
        sscanf(line, "%[^,], %d, %d, %d, %d, %d",
               process.process_number,
               &process.arrival_time,
               &process.priority,
               &process.burst_time,
               &process.ram,
               &process.cpu);
        processes[(*process_count)++] = process;
    }
    // Close file
    fclose(file);
}

void writeToFile(FILE *output_file, const char *text) {
    if (output_file != NULL) {
        fprintf(output_file, "%s", text);
    } else {
        printf("Error: File is not open.\n");
    }
}

// Complete process on CPU1
void completeProcessOnCPU1(Cpu *cpu, int *related_ram) {
    cpu->current_process = NULL;
    cpu->available_rate = 100;
    *related_ram = CPU1_ALLOCATED_RAM;
}

// Complete process on CPU2
void completeProcessOnCPU2(Cpu *cpu, int *related_ram) {
    cpu->current_process = NULL;
    cpu->available_rate = 100;
    *related_ram = RAM_SIZE - CPU1_ALLOCATED_RAM;
}

// Allocates necessary ram and cpu and assign process to cpu
void startNewProcessOnCPU(Cpu *cpu, Process process, int *related_ram, Queue *current_queue) {
    cpu->current_process = malloc(sizeof(Process)); // Allocate memory for the new process
    if (cpu->current_process == NULL) {
        // handle fail
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    *(cpu->current_process) = process;
    cpu->available_rate = 100 - process.cpu;
    cpu->remaining_busy_time = process.burst_time;
    *related_ram -= process.ram;
    cpu->current_queue = current_queue;
}

// Allocates necessary ram and cpu and assign process to cpu according to according quantum
void startNewProcessOnCPUWithQuantum(Cpu *cpu, Process process, int *related_ram, int quantum, Queue *current_queue) {
    cpu->current_process = malloc(sizeof(Process));// Allocate memory for the new process
    if (cpu->current_process == NULL) {        
        // handle fail
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    *(cpu->current_process) = process;
    cpu->available_rate = 100 - process.cpu;
    cpu->remaining_busy_time = quantum;
    *related_ram -= process.ram;
    cpu->current_queue = current_queue;
}

// Comparator function for SJF
int compareBurstTime(const void* a, const void* b) {
    Process* p1 = (Process*)a;
    Process* p2 = (Process*)b;
    return p1->burst_time - p2->burst_time;
}

// Function to sort the queue according to SJF
void sortQueueByBurstTime(Queue* q) {
    if (isEmpty(q)) {
        return;
    }

    // Count the number of nodes
    int count = 0;
    Node* temp = q->front;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }

    // Create array of processes
    Process* arr = (Process*)malloc(count * sizeof(Process));
    temp = q->front;
    for (int i = 0; i < count; i++) {
        arr[i] = temp->process;
        temp = temp->next;
    }

    // sort array by burst time
    qsort(arr, count, sizeof(Process), compareBurstTime);

    // Recreate the queue with sorted processes
    initQueue(q);
    for (int i = 0; i < count; i++) {
        enqueue(q, arr[i]);
    }

    free(arr);
}

// Iterates through processes and enqueues them if there are available recources . Also sorts cpu2_que1 according to SJF
void checkAndEnqueueProcess(Process *processes, int process_count, int current_time, Queue *cpu1_queue, Queue *cpu2_que1, Queue *cpu2_que2, Queue *cpu2_que3, int *cpu2_allocated_ram, FILE *output_file) {
    char buffer[256];
    for (int i = 0; i < process_count; i++) {

        // printf("\n\npeek of que3 %d \n\n", peek(cpu2_que3).process_number);
        // Check if recources has arrived on the curren time
        if (processes[i].arrival_time == current_time) {

            // Check if processes meant for cpu1
            if (processes[i].priority == 0) {

                // Check if there are enough processes
                if (processes[i].ram <= CPU1_ALLOCATED_RAM) {
                    enqueue(cpu1_queue, processes[i]);
                    snprintf(buffer, sizeof(buffer), "Process %s is queued to be assigned to CPU-1.\n", processes[i].process_number);
                    printf("%s", buffer);
                    writeToFile(output_file, buffer);
                } else {
                    snprintf(buffer, sizeof(buffer), "Process %s cannot be allocated due to insufficient RAM.\n", processes[i].process_number);
                    printf("%s", buffer);
                    writeToFile(output_file, buffer);
                }
            } else {
                // Check if ther process can be allocated to cpu2
     
                printf("\n\n %s %d and needed: %d \n\n", processes[i].process_number, *cpu2_allocated_ram, processes[i].ram);

                if (processes[i].ram <= *cpu2_allocated_ram) {

                    // Check the priority and enqueue to the appropriate queue
                    if (processes[i].priority == 1) {
                        enqueue(cpu2_que1, processes[i]);

                        // sort the queue
                        sortQueueByBurstTime(cpu2_que1);
                        snprintf(buffer, sizeof(buffer), "Process %s is placed in the que1 queue to be assigned to CPU-2.\n", processes[i].process_number);
                        printf("%s", buffer);
                        writeToFile(output_file, buffer);
                    } else if (processes[i].priority == 2) {
                        enqueue(cpu2_que2, processes[i]);
                        snprintf(buffer, sizeof(buffer), "Process %s is placed in the que2 queue to be assigned to CPU-2.\n", processes[i].process_number);
                        printf("%s", buffer);
                        writeToFile(output_file, buffer);
                    } else {
                        enqueue(cpu2_que3, processes[i]);
                        snprintf(buffer, sizeof(buffer), "Process %s is placed in the que3 queue to be assigned to CPU-2.\n", processes[i].process_number);
                        printf("%s", buffer);
                        writeToFile(output_file, buffer);
                    }
                } else {
                    // Log if ram is insufficient
                    snprintf(buffer, sizeof(buffer), "Process %s cannot be allocated due to insufficient RAM.\n", processes[i].process_number);
                    printf("%s", buffer);
                    writeToFile(output_file, buffer);
                }
            }
        }
    }
}

// If its restricted by quantum time reenqueues the process else executes process completion function
void handleCpu2ProcessCompletion(Cpu *cpu2, int *cpu2_allocated_ram, bool *is_cpu2_quantum_restricted, FILE *output_file) {
    char buffer[256];

    // Check if its restricted
    if (*is_cpu2_quantum_restricted) {
        // Allocate memory for the copy process
        Process *copy = (Process *)malloc(sizeof(Process));
        if (copy == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        *copy = *(cpu2->current_process);// Copy contents

        // Reenqueue
        enqueue(cpu2->current_queue, *copy);
        snprintf(buffer, sizeof(buffer), "The operation of process %s run until the defined quantum time and is queued again because the process is not completed.\n", cpu2->current_process->process_number);

        *is_cpu2_quantum_restricted = false;
    } else {
        snprintf(buffer, sizeof(buffer), "The operation of process %s is completed and terminated.\n", cpu2->current_process->process_number);
    }

    printf("%s", buffer);
    writeToFile(output_file, buffer);
    // handle process completion
    completeProcessOnCPU2(cpu2, cpu2_allocated_ram);
}

// Abstraction of process assign and logging
void assignProcessToCpu(Cpu *cpu, Queue *queue, int *allocated_ram, FILE *output_file, const char *cpu_label) {
    char buffer[256];

    //Dequeue process and start new process on cpu
    Process p = dequeue(queue);
    startNewProcessOnCPU(cpu, p, allocated_ram, queue);
    // Log actions
    snprintf(buffer, sizeof(buffer), "Process %s is assigned to %s.\n", p.process_number, cpu_label);
    printf("%s", buffer);
    writeToFile(output_file, buffer);
}

// Abstraction of process assign and logging with quantum restriction
void assignProcessToCpuWithQuantum(Cpu *cpu, Queue *queue, int *allocated_ram, int quantum, bool *is_cpu_quantum_restricted, FILE *output_file) {
    char buffer[256];

    Process p = dequeue(queue);
    // Check if it should be reenqueued
    if (p.burst_time > quantum) {
        p.burst_time -= quantum;

        // If it should be restricted in future sets the related variable true
        if (p.burst_time > quantum) {
            *is_cpu_quantum_restricted = true;
        }
        // Start new process with quantum restriction
        startNewProcessOnCPUWithQuantum(cpu, p, allocated_ram, quantum, queue);
    } else {
        // Start new process without quantum restriction
        startNewProcessOnCPU(cpu, p, allocated_ram, queue);
    }

    snprintf(buffer, sizeof(buffer), "Process %s is assigned to CPU.\n", p.process_number);
    printf("%s", buffer);
    writeToFile(output_file, buffer);
}

// Scheduler processes to two CPUs with different priority queues
void scheduler(Process processes[], int process_count) {
    FILE *output_file = fopen("output.txt", "w"); // Open file
    char buffer[100];

    // Initialize queues for CPU1 and CPU2
    Queue cpu1_queue, cpu2_que1, cpu2_que2, cpu2_que3;
    Cpu cpu1 = {0, 100, NULL};
    Cpu cpu2 = {0, 100, NULL};

    char cpu1_string[1000];
    char cpu2_que1_string[1000];
    char cpu2_que2_string[1000];
    char cpu2_que3_string[1000];

    cpu1_string[0] = '\0';
    cpu2_que1_string[0] = '\0';
    cpu2_que2_string[0] = '\0';
    cpu2_que3_string[0] = '\0';

    // Init queues
    initQueue(&cpu1_queue);
    initQueue(&cpu2_que1);
    initQueue(&cpu2_que2);
    initQueue(&cpu2_que3);

    int total_cpu2_ram = RAM_SIZE - CPU1_ALLOCATED_RAM;
    int total_cpu1_ram = CPU1_ALLOCATED_RAM;

    // Ram allocation for CPUs
    int cpu2_allocated_ram = RAM_SIZE - CPU1_ALLOCATED_RAM;
    int cpu1_allocated_ram = CPU1_ALLOCATED_RAM;

    // Initialize current time and quantum restriction flag for CPU-2
    int current_time = 0;
    bool is_cpu2_quantum_restricted = false;

    while (1) {
        int all_done = 0;

        // Check for new arrivals and enqueue them to the appropriate queues
        checkAndEnqueueProcess(processes, process_count, current_time, &cpu1_queue, &cpu2_que1, &cpu2_que2, &cpu2_que3, &total_cpu2_ram, output_file);

        // Check if CPU-1 completed its process
        if (cpu1.current_process != NULL && cpu1.remaining_busy_time == 0) {
            snprintf(buffer, sizeof(buffer), "Process %s is completed and terminated.\n", cpu1.current_process->process_number);
            printf("%s", buffer);
            writeToFile(output_file, buffer);
            completeProcessOnCPU1(&cpu1, &cpu1_allocated_ram);
        }

        // Check if CPU-2 completed its process
        if (cpu2.current_process != NULL && cpu2.remaining_busy_time == 0) {
            handleCpu2ProcessCompletion(&cpu2, &cpu2_allocated_ram, &is_cpu2_quantum_restricted, output_file);
        }

        // Assign process to CPU1 if its available and there is any process waiting
        if (!isEmpty(&cpu1_queue) && cpu1.current_process == NULL) {
            snprintf(buffer, sizeof(buffer), "%s-", peek(&cpu1_queue).process_number);
            strcat(cpu1_string, buffer);
            assignProcessToCpu(&cpu1, &cpu1_queue, &cpu1_allocated_ram, output_file, "CPU-1");
        }

        // Assign process to CPU-2 if its available and there is any process waiting
        if (!isEmpty(&cpu2_que1) && cpu2.current_process == NULL) {
            snprintf(buffer, sizeof(buffer), "%s-", peek(&cpu2_que1).process_number);
            strcat(cpu2_que1_string, buffer);
            assignProcessToCpu(&cpu2, &cpu2_que1, &cpu2_allocated_ram, output_file, "CPU-2");
        }

        // If cpu2_que1 is emptied assign process to CPU-2 from cpu2_que2
         else if (!isEmpty(&cpu2_que2) && cpu2.current_process == NULL) {
            snprintf(buffer, sizeof(buffer), "%s-", peek(&cpu2_que2).process_number);
            strcat(cpu2_que2_string, buffer);
            assignProcessToCpuWithQuantum(&cpu2, &cpu2_que2, &cpu2_allocated_ram, CPU2_QUEUE2_QUANTUM, &is_cpu2_quantum_restricted, output_file);
        } 

        // If cpu2_que1 and cpu_que2 is emptied assign process to CPU-2 from cpu2_que3
        else if (!isEmpty(&cpu2_que3) && cpu2.current_process == NULL) {
            snprintf(buffer, sizeof(buffer), "%s-", peek(&cpu2_que3).process_number);
            strcat(cpu2_que3_string, buffer);
            assignProcessToCpuWithQuantum(&cpu2, &cpu2_que3, &cpu2_allocated_ram, CPU2_QUEUE3_QUANTUM, &is_cpu2_quantum_restricted, output_file);
        }

        

        // Check if there is no process waiting and all processes arrived 
        if (isEmpty(&cpu1_queue) && isEmpty(&cpu2_que1)  && isEmpty(&cpu2_que2) && isEmpty(&cpu2_que3) )
        {
            for (int i = 0; i < process_count; i++) {
            if (processes[i].arrival_time < current_time -1) {
                all_done = 1;
                break;
            }
        }
        }
        


        writeToFile(output_file, "\n");

        // Decrement the busy time if they are busy with any process
        current_time += 1;
        if (cpu1.remaining_busy_time > 0) cpu1.remaining_busy_time--;
        if (cpu2.remaining_busy_time > 0) cpu2.remaining_busy_time--;

        if (all_done) {
            break;
        }
    }

    // Remove last chars
    cpu1_string[strlen(cpu1_string) - 1] = '\0';
    cpu2_que1_string[strlen(cpu2_que1_string) - 1] = '\0';
    cpu2_que2_string[strlen(cpu2_que2_string) - 1] = '\0';
    cpu2_que3_string[strlen(cpu2_que3_string) - 1] = '\0';

    // Write the final order of process execution in each queue 
    fprintf(output_file, "\nCPU-1 que(priority-0) (FCFS) -> %s\n", cpu1_string);
    fprintf(output_file, "CPU-2 que1(priority-1) (SJF) -> %s\n", cpu2_que1_string);
    fprintf(output_file, "CPU-2 que2(priority-2) (RR-q8) -> %s\n", cpu2_que2_string);
    fprintf(output_file, "CPU-2 que3(priority-3) (RR-q16) -> %s\n", cpu2_que3_string);

    printf( "\nCPU-1 que(priority-0) (FCFS) -> %s\n", cpu1_string);
    printf( "CPU-2 que1(priority-1) (SJF) -> %s\n", cpu2_que1_string);
    printf( "CPU-2 que2(priority-2) (RR-q8) -> %s\n", cpu2_que2_string);
    printf( "CPU-2 que3(priority-3) (RR-q16) -> %s\n", cpu2_que3_string);

    fclose(output_file);
}

// Ana fonksiyon
int main(int argc, char *argv[]) {
    // Check if correct number of command-line arguments provided
    if (argc != 2) {
        printf("Usage: %s input.txt\n", argv[0]);
        return 1;
    }

    // Array to store processes
    Process processes[MAX_PROCESSES];
    int process_count;

    // Read input file and parse processes
    readInputFile(argv[1], processes, &process_count);

    // Start scheduling processes
    scheduler(processes, process_count);

    return 0;
}