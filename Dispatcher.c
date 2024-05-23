#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define RAM_SIZE 2048
#define CPU1_ALLOCATED_RAM 512
#define CPU2_QUEUE2_QUANTUM 8
#define CPU2_QUEUE3_QUANTUM 16
#define MAX_PROCESSES 500

typedef struct {
    char process_number[5];
    int arrival_time;
    int priority;
    int burst_time;
    int ram;
    int cpu;
} Process;

typedef struct Node {
    Process process;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *rear;
} Queue;

typedef struct {
    int remaining_busy_time;
    int available_rate;
    Process *current_process ;
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

void readInputFile(const char *filename, Process processes[], int *process_count) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Cannot open file %s\n", filename);
        exit(1);
    }

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

    fclose(file);
}

void writeToFile(FILE *output_file, const char *text) {
    if (output_file != NULL) {
        fprintf(output_file, "%s", text);
    } else {
        printf("Error: File is not open.\n");
    }
}

void completeProcessOnCPU1(Cpu *cpu, int *related_ram) {
    cpu->current_process = NULL;
    cpu->available_rate = 100;
    *related_ram = CPU1_ALLOCATED_RAM;
}

void completeProcessOnCPU2(Cpu *cpu, int *related_ram) {
    cpu->current_process = NULL;
    cpu->available_rate = 100;
    *related_ram = RAM_SIZE - CPU1_ALLOCATED_RAM;
}

void startNewProcessOnCPU(Cpu *cpu, Process process, int *related_ram, Queue *current_queue) {
    cpu->current_process = malloc(sizeof(Process)); // Allocate memory for the new process
    if (cpu->current_process == NULL) {
        // Handle memory allocation failure
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    *(cpu->current_process) = process; // Copy the contents of the dequeued process
    cpu->available_rate = 100 - process.cpu;
    cpu->remaining_busy_time = process.burst_time;
    *related_ram -= process.ram;
    cpu->current_queue = current_queue;
}

void startNewProcessOnCPUWithQuantum(Cpu *cpu, Process process, int *related_ram, int quantum, Queue *current_queue) {  
    cpu->current_process = malloc(sizeof(Process)); // Allocate memory for the new process
    if (cpu->current_process == NULL) {
        // Handle memory allocation failure
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    *(cpu->current_process) = process; // Copy the contents of the dequeued process
    cpu->available_rate = 100 - process.cpu;
    cpu->remaining_busy_time = quantum;
    *related_ram -= process.ram;
    cpu->current_queue = current_queue;
}

// Comparator function for SJF (Shortest Job First)
int compareBurstTime(const void* a, const void* b) {
    Process* p1 = (Process*)a;
    Process* p2 = (Process*)b;
    return p1->burst_time - p2->burst_time;
}

// Function to sort the queue using SJF algorithm
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

    // Create an array of processes
    Process* arr = (Process*)malloc(count * sizeof(Process));
    temp = q->front;
    for (int i = 0; i < count; i++) {
        arr[i] = temp->process;
        temp = temp->next;
    }

    // Sort the array by burst time
    qsort(arr, count, sizeof(Process), compareBurstTime);

    // Recreate the queue with sorted processes
    initQueue(q);
    for (int i = 0; i < count; i++) {
        enqueue(q, arr[i]);
    }

    free(arr);
}

void checkAndEnqueueProcess(Process *processes, int process_count, int current_time, Queue *cpu1_queue, Queue *cpu2_que1, Queue *cpu2_que2, Queue *cpu2_que3, int *cpu2_allocated_ram, FILE *output_file) {
    char buffer[256];
    for (int i = 0; i < process_count; i++) {
        if (processes[i].arrival_time == current_time) {

            if (processes[i].priority == 0) {
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
                if (processes[i].ram <= *cpu2_allocated_ram) {
                    if (processes[i].priority == 1) {
                        enqueue(cpu2_que1, processes[i]);
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
                    snprintf(buffer, sizeof(buffer), "Process %s cannot be allocated due to insufficient RAM.\n", processes[i].process_number);
                    printf("%s", buffer);
                    writeToFile(output_file, buffer);
                }
            }
        }
    }
}

void handleCpu2ProcessCompletion(Cpu *cpu2, int *cpu2_allocated_ram, bool *is_cpu2_quantum_restricted, FILE *output_file) {
    char buffer[256];

    if (*is_cpu2_quantum_restricted) {
        Process *copy = (Process *)malloc(sizeof(Process));  // Allocate memory for the new process
        if (copy == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        *copy = *(cpu2->current_process); // Copy the contents of the current process

        enqueue(cpu2->current_queue, *copy);
        snprintf(buffer, sizeof(buffer), "The operation of process %s run until the defined quantum time and is queued again because the process is not completed.\n", cpu2->current_process->process_number);

        *is_cpu2_quantum_restricted = false;
    } else {
        snprintf(buffer, sizeof(buffer), "The operation of process %s is completed and terminated.\n", cpu2->current_process->process_number);
    }

    printf("%s", buffer);
    writeToFile(output_file, buffer);
    completeProcessOnCPU2(cpu2, cpu2_allocated_ram);
}

void assignProcessToCpu(Cpu *cpu, Queue *queue, int *allocated_ram, FILE *output_file, const char *cpu_label) {
    char buffer[256];

    Process p = dequeue(queue);
    startNewProcessOnCPU(cpu, p, allocated_ram, queue);

    snprintf(buffer, sizeof(buffer), "Process %s is assigned to %s.\n", p.process_number, cpu_label);
    printf("%s", buffer);
    writeToFile(output_file, buffer);
}

void assignProcessToCpuWithQuantum(Cpu *cpu, Queue *queue, int *allocated_ram, int quantum, bool *is_cpu_quantum_restricted, FILE *output_file) {
    char buffer[256];

    Process p = dequeue(queue);

    if (p.burst_time > quantum) {
        p.burst_time -= quantum;
        if (p.burst_time > quantum) {
            *is_cpu_quantum_restricted = true;
        }
        startNewProcessOnCPUWithQuantum(cpu, p, allocated_ram, quantum, queue);
    } else {
        startNewProcessOnCPU(cpu, p, allocated_ram, queue);
    }

    snprintf(buffer, sizeof(buffer), "Process %s is assigned to CPU.\n", p.process_number);
    printf("%s", buffer);
    writeToFile(output_file, buffer);
}


void scheduler(Process processes[], int process_count) {
    FILE *output_file = fopen("output.txt", "w"); // Open file for writing
    char buffer[100];
    Queue cpu1_queue, cpu2_que1, cpu2_que2, cpu2_que3;
    Cpu cpu1 = {0, 100, NULL} ;
    Cpu cpu2 = {0, 100, NULL} ;

    initQueue(&cpu1_queue);
    initQueue(&cpu2_que1);
    initQueue(&cpu2_que2);
    initQueue(&cpu2_que3);

    int cpu2_allocated_ram = RAM_SIZE - CPU1_ALLOCATED_RAM;
    int cpu1_allocated_ram = CPU1_ALLOCATED_RAM;
    int current_time = 0;
    bool is_cpu2_quantum_restricted = false; 
 
    while (1) {
        int all_done = 1;

        // Check for new arrivals
        checkAndEnqueueProcess(processes, process_count, current_time, &cpu1_queue, &cpu2_que1, &cpu2_que2, &cpu2_que3, &cpu2_allocated_ram, output_file);
        // Process queues

        if (cpu1.current_process != NULL && cpu1.remaining_busy_time == 0){

            snprintf(buffer, sizeof(buffer),"Process %s is completed and terminated.\n", cpu1.current_process->process_number);
            printf("%s", buffer);
            writeToFile(output_file,buffer);
            completeProcessOnCPU1(&cpu1, &cpu1_allocated_ram);
        }
        if (cpu2.current_process != NULL && cpu2.remaining_busy_time == 0){
            handleCpu2ProcessCompletion(&cpu2, &cpu2_allocated_ram, &is_cpu2_quantum_restricted, output_file);
        }

        if (!isEmpty(&cpu1_queue) && cpu1.current_process == NULL){
            assignProcessToCpu(&cpu1, &cpu1_queue, &cpu1_allocated_ram, output_file, "CPU-1");
        } 

        if (!isEmpty(&cpu2_que1) && cpu2.current_process == NULL){
            assignProcessToCpu(&cpu2, &cpu2_que1, &cpu2_allocated_ram, output_file, "CPU-2");
        } 
        else if (!isEmpty(&cpu2_que2) && cpu2.current_process == NULL){
            assignProcessToCpuWithQuantum(&cpu2, &cpu2_que2, &cpu2_allocated_ram, CPU2_QUEUE2_QUANTUM, &is_cpu2_quantum_restricted, output_file);
        } 
        else if (!isEmpty(&cpu2_que3) && cpu2.current_process == NULL){
            assignProcessToCpu(&cpu1, &cpu2_que3, &cpu2_allocated_ram, output_file, "CPU-2");
        }

        for (int i = 0; i < process_count; i++) {
            if ((processes[i].arrival_time + processes[i].burst_time) > current_time ) {
                all_done = 0;
                break;
            }
        }

        writeToFile(output_file, "\n");

        current_time += 1;
        if (cpu1.remaining_busy_time > 0) cpu1.remaining_busy_time--;
        if (cpu2.remaining_busy_time > 0) cpu2.remaining_busy_time--;
        
        if (all_done) {
            break;
        }

    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s input.txt\n", argv[0]);
        return 1;
    }

    Process processes[MAX_PROCESSES];
    int process_count;
    readInputFile(argv[1], processes, &process_count);

    scheduler(processes, process_count);

    return 0;
}
