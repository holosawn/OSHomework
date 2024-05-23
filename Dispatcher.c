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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s input.txt\n", argv[0]);
        return 1;
    }

    Process processes[MAX_PROCESSES];
    int process_count;
    readInputFile(argv[1], processes, &process_count);

    // Print the read processes
    for (int i = 0; i < process_count; i++) {
        printf("Process Number: %s, Arrival Time: %d, Priority: %d, Burst Time: %d, RAM: %d, CPU: %d\n",
               processes[i].process_number,
               processes[i].arrival_time,
               processes[i].priority,
               processes[i].burst_time,
               processes[i].ram,
               processes[i].cpu);
    }
    
    return 0;
}

