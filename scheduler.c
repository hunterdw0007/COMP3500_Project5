/*
 * COMP 3500: Project 5 Scheduling
 * Hunter Westerlund
 * Version 1.0  11/28/2021
 *
 * I used the class slides and the Project 5 description to develop this solution
 * 
 * This source code shows how to conduct separate compilation.
 *
 * How to compile using Makefile?
 * $make
 *
 * How to manually compile?
 * $gcc -c open.c
 * $gcc -c read.c
 * $gcc -c print.c
 * $gcc open.o read.o print.o scheduler.c -o scheduler
 *
 * How to run?
 * Case 1: no argument. Sample usage is printed
 * $./scheduler
 * Usage: scheduler <file_name>
 *
 * Case 2: file doesn't exist.
 * $./scheduler file1
 * File "file1" doesn't exist. Please try again...
 *
 * Case 3: Input file
 * $./scheduler task.list
 * data in task.list is printed below...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheduler.h"
#include "print.h"
#include "open.h"
#include "read.h"


stat_info_t stats;
u_int idle_time = 0;
u_int quantum;

int main( int argc, char *argv[] )  {
    char *file_name; /* file name from the commandline */
    FILE *fp; /* file descriptor */
    task_t TaskArr[MAX_TASK_NUM];

    int error_code;
    u_int i;
    u_int count;
    u_int quantum = -1;

    // Input Validation
    // Checking if the correct number of arguments are input
    if (argc <= 2) {
        printf("Usage: scheduler task_list_file [FCFS|RR|SRTF] [time_quantum]");
        return EXIT_FAILURE;
    }
    //Making sure that RR has a time quantum argument
    if (strcmp(argv[2], "RR") == 0) {
        if (argc == 4) {
            printf("time_quantum is set to %s\n", argv[3]);
            quantum = atoi(argv[3]);
        }
        else {
            printf("Please enter time_quantum for the RR policy!\n");
            return EXIT_FAILURE;
        }
    }

    error_code = open_file(argv[1], &fp);
    if (error_code == 1)
        return EXIT_FAILURE;

    // prints out the tasks that are to be run then waits for user input
    printf("Scheduling policy: %s\n", argv[2]);
    read_file(fp, TaskArr, &count);
    print_task_list(TaskArr, count);

    // this is to ensure boolean checks in the policy functions succeed without any problems
    for (int i = 0; i < count; i++) {
        TaskArr[i].queued = 0;
        TaskArr[i].null = 0;
        TaskArr[i].doneThisRound = 0;
    }

    printf("==================================================================\n");

    //task master chooses which type of policy to use and then calls it
    task_master(TaskArr, count, argv[2], quantum);

    fclose(fp);
    return EXIT_SUCCESS;
}

void task_master(task_t task_list[], int size, char *procedure, u_int quantum) {
    if (strcmp(procedure, "FCFS") == 0) {
        fcfs_policy(task_list, size);
    }
    else if (strcmp(procedure, "RR") == 0) {
        rr_policy(task_list, size, quantum);
    }
    else if (strcmp(procedure, "SRTF") == 0) {
        srtf_policy(task_list, size);
    }
    // Input is invalid, print string instructing the user
    else {
        printf("No valid policy was provided. Choices are FCFS, RR, and SRTF.");
        return;
    }
    // print the data from stats
    printf("============================================================\n");
    printf("Average waiting time:    %u\n", stats.WaitingAverage);
    printf("Average response time:   %u\n", stats.ResponseAverage);
    printf("Average turnaround time: %u\n", stats.TurnaroundAverage);
    printf("Average CPU usage:       %.2f%%\n", stats.CPUUsage); // 2 decimal places format
    printf("============================================================\n");
}

void fcfs_policy(task_t TaskArr[], u_int count) {
    u_int ResponseAverage = 0;
    u_int CPUUsage = 0;
    u_int CurrentTime = 0;
    u_int WaitingAverage = 0;
    u_int TurnaroundAverage = 0;

    u_int ReadySize = 0;
    u_int FinishSize = 0;
    task_t ReadyArr[MAX_TASK_NUM];
    task_t FinishArr[MAX_TASK_NUM];

    u_int index = -1;

    u_int CanContinue = 1;
    
    while (CanContinue == 1) {
        for (int i = 0; i < count; i++) { // this is for handling tasks as they 'arrive'
            if (TaskArr[i].queued == 0 && TaskArr[i].arrival_time <= CurrentTime) {
                TaskArr[i].remaining_time = TaskArr[i].burst_time;
                ReadyArr[ReadySize] = TaskArr[i];
                ReadySize++;
                TaskArr[i].queued = 1; // queued field indicates a task has 'arrived'
            } else {
                continue;
            }
        }
        
        for (int i = 0; i < ReadySize; i++) { // for FCFS, the first non-null task in the array is chosen
            if (ReadyArr[i].null == 0) { // null is a field for determining when a task is finished
                index = i;
                break;
            }
        }
        if (index == -1 || ReadyArr[index].null == 1) { // this is to check if computer is idle
            CurrentTime++;
            idle_time++;
            continue;
        } 

        if (ReadyArr[index].remaining_time == ReadyArr[index].burst_time) { // set burst time
            ReadyArr[index].start_time = CurrentTime;
        }

        while (ReadyArr[index].remaining_time != 0) { // the process continues to run until finished
            printf("<time %u> process %u is running ...\n", CurrentTime, ReadyArr[index].pid);
            ReadyArr[index].remaining_time--;
            CurrentTime++;
            if (ReadyArr[index].remaining_time == 0) {
                printf("<time %u> process %u is finished ...\n", CurrentTime, ReadyArr[index].pid);
                ReadyArr[index].finish_time = CurrentTime; // change finish_time field to CurrentTime
                FinishArr[FinishSize] = ReadyArr[index];
                FinishSize++;
                ReadyArr[index].null = 1; // task is null to indicate it shouldnt be selected to run
                break;
            }
        }
        // count if there is anything in the waiting/ready queue
        u_int placeholder = 0;
        for (int i = 0; i < count; i++) {
            if (ReadyArr[i].null == 0 || TaskArr[i].queued == 0) {
                placeholder++;
            }
        }
        // if this is zero then both queues are empty and thus the program can end
        if (placeholder == 0) {
            CanContinue = 0;
        }
    }
    // Calculating totals for the stats which will later be divided by count to get the final average
    for (int i = 0; i < count; i++) {
        u_int response = ReadyArr[i].start_time - ReadyArr[i].arrival_time;
        u_int waiting_time = ReadyArr[i].finish_time - ReadyArr[i].arrival_time - ReadyArr[i].burst_time;
        u_int trnd = ReadyArr[i].finish_time - ReadyArr[i].arrival_time;
        stats.ResponseAverage += response;
        stats.WaitingAverage += waiting_time;
        stats.TurnaroundAverage += trnd;
    }
    // Stats are saved here to be printed after execution
    double inverse_usage = (double)(idle_time/CurrentTime) * 100;
    stats.CPUUsage = 100-inverse_usage;
    stats.ResponseAverage /= count;
    stats.WaitingAverage /= count;
    stats.TurnaroundAverage /= count;
}

void srtf_policy(task_t TaskArr[], u_int count) {
    u_int ResponseAverage = 0;
    u_int CPUUsage = 0;
    u_int CurrentTime = 0;
    u_int WaitingAverage = 0;
    u_int TurnaroundAverage = 0;

    u_int ReadySize = 0;
    u_int FinishSize = 0;
    task_t ReadyArr[MAX_TASK_NUM];
    task_t FinishArr[MAX_TASK_NUM];

    u_int index = -1;

    u_int CanContinue = 1;
    
    while (CanContinue == 1) {
        for (int i = 0; i < count; i++) { // deals with newly arrived tasks
            if (TaskArr[i].queued == 0 && TaskArr[i].arrival_time <= CurrentTime) {
                TaskArr[i].remaining_time = TaskArr[i].burst_time;
                ReadyArr[ReadySize] = TaskArr[i];
                ReadySize++;
                TaskArr[i].queued = 1;
            } else {
                continue;
            }
        }
        u_int shortest_remaining = INT32_MAX;
        u_int shortest_remaining_index = -1; // different from FCFS, must choose shortest remaining time
        for (int i = 0; i < ReadySize; i++) { // at the end of this for block, the shortest remaining time index will be saved
            if (ReadyArr[i].null == 0 && ReadyArr[i].remaining_time < shortest_remaining) {
                shortest_remaining_index = i;
                shortest_remaining = ReadyArr[i].remaining_time;
            } 
            
            if (shortest_remaining_index == -1) {
                break;
            }
            else {
                index = shortest_remaining_index;
            }
        }
        if (index == -1 || ReadyArr[index].null == 1) { // checks if CPU is idle
            CurrentTime++;
            idle_time++;
            continue;
        }

        if (ReadyArr[index].remaining_time == ReadyArr[index].burst_time) { //set start time
            ReadyArr[index].start_time = CurrentTime;
        }

        while (ReadyArr[index].remaining_time != 0) { // task continues until finished
            printf("<time %u> process %u is running ...\n", CurrentTime, ReadyArr[index].pid);
            ReadyArr[index].remaining_time--;
            CurrentTime++;
            if (ReadyArr[index].remaining_time == 0) {
                printf("<time %u> process %u is finished ...\n", CurrentTime, ReadyArr[index].pid);
                ReadyArr[index].finish_time = CurrentTime;
                FinishArr[FinishSize] = ReadyArr[index];
                FinishSize++;
                ReadyArr[index].null = 1; // 'removed' from ready queue
                break;
            }
        }

        u_int placeholder = 0; //determines if work is still to be done
        for (int i = 0; i < count; i++) {
            if (ReadyArr[i].null == 0 || TaskArr[i].queued == 0) {
                placeholder++;
            }
        }
        if (placeholder == 0) {
            CanContinue = 0;
        }
    }

    for (int i = 0; i < count; i++) { //statistical analysis
        u_int response = ReadyArr[i].start_time - ReadyArr[i].arrival_time;
        u_int waiting_time = ReadyArr[i].finish_time - ReadyArr[i].arrival_time - ReadyArr[i].burst_time;
        u_int trnd = ReadyArr[i].finish_time - ReadyArr[i].arrival_time;
        stats.ResponseAverage += response;
        stats.WaitingAverage += waiting_time;
        stats.TurnaroundAverage += trnd;
    }
    double divide = (double)(idle_time/CurrentTime) * 100;
    stats.CPUUsage = 100-divide;
    stats.ResponseAverage /= count;
    stats.WaitingAverage /= count;
    stats.TurnaroundAverage /= count;
}

void rr_policy(task_t TaskArr[], u_int count, u_int quantum) {
    u_int ResponseAverage = 0;
    u_int CPUUsage = 0;
    u_int CurrentTime = 0;
    u_int WaitingAverage = 0;
    u_int TurnaroundAverage = 0;

    u_int ReadySize = 0;
    u_int FinishSize = 0;
    task_t ReadyArr[MAX_TASK_NUM];
    task_t FinishArr[MAX_TASK_NUM];

    u_int index = -1;

    u_int CanContinue = 1;

    u_int timeSinceSwitch = 0; // implements the quantum

    while (CanContinue == 1) {
        for (int i = 0; i < count; i++) { // deals with new arrivals
            if (TaskArr[i].queued == 0 && TaskArr[i].arrival_time <= CurrentTime) {
                TaskArr[i].remaining_time = TaskArr[i].burst_time;
                ReadyArr[ReadySize] = TaskArr[i];
                ReadySize++;
                TaskArr[i].queued = 1;
            } else {
                continue;
            }
        }

        if (index >= 0 && index == (ReadySize - 1)) { // each go around, the 'locks' on each process is lifted
            for (int i = 0; i < ReadySize; i++) { // this lock is to ensure that the same process is not chosen before each other process goes at least once
                ReadyArr[i].doneThisRound = 0;
            }
        }
        // here is where we choose the next task
        for (int i = 0; i < ReadySize; i++) { // only choose tasks that have not gone that round, i.e. choose a task so that no tasks in the ready array will be skipped
            if (i != index && ReadyArr[i].null == 0 && ReadyArr[i].doneThisRound == 0) {
                index = i;
                ReadyArr[i].doneThisRound = 1;
                break;
            }
        }
        if (index == -1 || ReadyArr[index].null == 1) { // deals with idle CPU time
            CurrentTime++;
            idle_time++;
            continue;
        }

        if (ReadyArr[index].remaining_time == ReadyArr[index].burst_time) { // set start time
            ReadyArr[index].start_time = CurrentTime;
        }

        while (ReadyArr[index].remaining_time != 0) { //task runs during its time slot ... 
            printf("<time %u> process %u is running ...\n", CurrentTime, ReadyArr[index].pid);
            ReadyArr[index].remaining_time--;
            CurrentTime++;
            timeSinceSwitch++; // time builds up until ...
            if (ReadyArr[index].remaining_time == 0) {
                printf("<time %u> process %u is finished ...\n", CurrentTime, ReadyArr[index].pid);
                ReadyArr[index].finish_time = CurrentTime;
                FinishArr[FinishSize] = ReadyArr[index];
                FinishSize++;
                ReadyArr[index].null = 1;
                timeSinceSwitch = 0; // if a task is finished, timeSinceSwitch is reset
                break;
            }
            if (timeSinceSwitch == quantum) { // when timeSinceSwitch is the size of the quantum, it is another task's turn
                timeSinceSwitch = 0; // timeSinceSwitch is reset
                break;
            }
        }

        u_int placeholder = 0; // deals with loop, sees if any tasks are waiting/ready
        for (int i = 0; i < count; i++) {
            if (ReadyArr[i].null == 0 || TaskArr[i].queued == 0) {
                placeholder++;
            }
        }
        if (placeholder == 0) {
            CanContinue = 0;
        }
    }

    for (int i = 0; i < count; i++) { //statistical analysis
        u_int response = ReadyArr[i].start_time - ReadyArr[i].arrival_time;
        u_int waiting_time = ReadyArr[i].finish_time - ReadyArr[i].arrival_time - ReadyArr[i].burst_time;
        u_int trnd = ReadyArr[i].finish_time - ReadyArr[i].arrival_time;
        stats.ResponseAverage += response;
        stats.WaitingAverage += waiting_time;
        stats.TurnaroundAverage += trnd;
    }
    double divide = (double)(idle_time/CurrentTime) * 100;
    stats.CPUUsage = 100-divide;
    stats.ResponseAverage /= count;
    stats.WaitingAverage /= count;
    stats.TurnaroundAverage /= count;
}