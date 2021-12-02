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
        TaskArr[i].TurnTaken = 0;
    }

    printf("==================================================================\n");

    //task master chooses which type of policy to use and then calls it
    TaskMaster(TaskArr, count, argv[2], quantum);

    fclose(fp);
    return EXIT_SUCCESS;
}

void TaskMaster(task_t task_list[], int size, char *procedure, u_int quantum) {
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
    printf("Average waiting time:    %.2f\n", stats.WaitingAverage);
    printf("Average response time:   %.2f\n", stats.ResponseAverage);
    printf("Average turnaround time: %.2f\n", stats.TurnaroundAverage);
    printf("Average CPU usage:       %.2f%%\n", stats.CPUUsage);
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
        // This looks for tasks that have 'arrived'
        for (int i = 0; i < count; i++) {
            if (TaskArr[i].queued == 0 && TaskArr[i].arrival_time <= CurrentTime) {
                TaskArr[i].remaining_time = TaskArr[i].burst_time;
                ReadyArr[ReadySize] = TaskArr[i];
                ReadySize++;
                // setting queued indicates the task has arrived
                TaskArr[i].queued = 1;
            }
        }
        
        // FCFS chooses the first task in the list to execute
        for (int i = 0; i < ReadySize; i++) {
            // Checks if the task has not finished, if not it sets index to the index of that task
            if (ReadyArr[i].null == 0) {
                index = i;
                break;
            }
        }

        // If there are no tasks queued then the cpu is idle
        if (index == -1 || ReadyArr[index].null == 1) {
            CurrentTime++;
            idle_time++;
            continue;
        } 

        // This checks that the task has not yet started and if not then it sets its start time
        if (ReadyArr[index].remaining_time == ReadyArr[index].burst_time) {
            ReadyArr[index].start_time = CurrentTime;
        }

        // This loop runs the task until it is finished
        while (ReadyArr[index].remaining_time != 0) {
            printf("<time %u> process %u is running ...\n", CurrentTime, ReadyArr[index].pid);
            ReadyArr[index].remaining_time--;
            CurrentTime++;
            if (ReadyArr[index].remaining_time == 0) {
                printf("<time %u> process %u is finished ...\n", CurrentTime, ReadyArr[index].pid);
                // The task has finished so set finish_time
                ReadyArr[index].finish_time = CurrentTime;
                FinishArr[FinishSize] = ReadyArr[index];
                FinishSize++;
                // task has finished so null is set
                ReadyArr[index].null = 1;
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
        double response = ReadyArr[i].start_time - ReadyArr[i].arrival_time;
        double waiting_time = ReadyArr[i].finish_time - ReadyArr[i].arrival_time - ReadyArr[i].burst_time;
        double trnd = ReadyArr[i].finish_time - ReadyArr[i].arrival_time;
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
        // This loop gets the newly arrived tasks and adds them to the queue
        for (int i = 0; i < count; i++) {
            if (TaskArr[i].queued == 0 && TaskArr[i].arrival_time <= CurrentTime) {
                TaskArr[i].remaining_time = TaskArr[i].burst_time;
                ReadyArr[ReadySize] = TaskArr[i];
                ReadySize++;
                TaskArr[i].queued = 1;
            }
        }
        // Initially set the shortest remaining to UINT_MAX value
        u_int ShortestRemaining = 4294967295;	
        // This variable keeps track of the stortest remaining task index
        u_int ShortestRemainingIndex = -1;
        // This loop finds the shortest remaining task index
        for (int i = 0; i < ReadySize; i++) {
            if (ReadyArr[i].null == 0 && ReadyArr[i].remaining_time < ShortestRemaining) {
                ShortestRemainingIndex = i;
                ShortestRemaining = ReadyArr[i].remaining_time;
            } 
            
            if (ShortestRemainingIndex == -1) {
                break;
            }
            else {
                index = ShortestRemainingIndex;
            }
        }
        // This checks if the cpu is idle and if so it adds to idle_time
        if (index == -1 || ReadyArr[index].null == 1) {
            CurrentTime++;
            idle_time++;
            continue;
        }

        // Sets the start time of the currently running task
        if (ReadyArr[index].remaining_time == ReadyArr[index].burst_time) {
            ReadyArr[index].start_time = CurrentTime;
        }

        // This runs the task until it is finished
        while (ReadyArr[index].remaining_time != 0) {
            printf("<time %u> process %u is running ...\n", CurrentTime, ReadyArr[index].pid);
            ReadyArr[index].remaining_time--;
            CurrentTime++;
            if (ReadyArr[index].remaining_time == 0) {
                printf("<time %u> process %u is finished ...\n", CurrentTime, ReadyArr[index].pid);
                ReadyArr[index].finish_time = CurrentTime;
                FinishArr[FinishSize] = ReadyArr[index];
                FinishSize++;
                // Sets null so the task is finished
                ReadyArr[index].null = 1;
                break;
            }
        }

        u_int placeholder = 0;
        // This loop checks if there are still tasks ready/waiting
        for (int i = 0; i < count; i++) {
            if (ReadyArr[i].null == 0 || TaskArr[i].queued == 0) {
                placeholder++;
            }
        }
        // If there are no tasks ready/waiting then end execution
        if (placeholder == 0) {
            CanContinue = 0;
        }
    }

    // This calculates the totals for the stats
    for (int i = 0; i < count; i++) {
        double response = ReadyArr[i].start_time - ReadyArr[i].arrival_time;
        double waiting_time = ReadyArr[i].finish_time - ReadyArr[i].arrival_time - ReadyArr[i].burst_time;
        double trnd = ReadyArr[i].finish_time - ReadyArr[i].arrival_time;
        stats.ResponseAverage += response;
        stats.WaitingAverage += waiting_time;
        stats.TurnaroundAverage += trnd;
    }
    // This calculates the percent of time the CPU was being used
    double divide = (double)(idle_time/CurrentTime) * 100;
    stats.CPUUsage = 100-divide;
    // This calculates the averages of the stats
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

    // This variable tracks the quantum
    u_int timeSinceSwitch = 0;

    while (CanContinue == 1) {
        // This loop gets newly arriving tasks
        for (int i = 0; i < count; i++) {
            if (TaskArr[i].queued == 0 && TaskArr[i].arrival_time <= CurrentTime) {
                TaskArr[i].remaining_time = TaskArr[i].burst_time;
                ReadyArr[ReadySize] = TaskArr[i];
                ReadySize++;
                TaskArr[i].queued = 1;
            }
        }

        // This checks if the tasks have been done this 'round' and if they have it 'unlocks' them by zeroing TurnTaken
        if (index >= 0 && index == (ReadySize - 1)) {
            for (int i = 0; i < ReadySize; i++) {
                ReadyArr[i].TurnTaken = 0;
            }
        }
        // This loop gets the first task where TurnTaken is zero and it hasn't finished
        for (int i = 0; i < ReadySize; i++) {
            if (i != index && ReadyArr[i].null == 0 && ReadyArr[i].TurnTaken == 0) {
                index = i;
                ReadyArr[i].TurnTaken = 1;
                break;
            }
        }
        // This checks if there are no tasks running and if so it adds idle time to the CPU
        if (index == -1 || ReadyArr[index].null == 1) {
            CurrentTime++;
            idle_time++;
            continue;
        }

        // If the process has not started yet then set its start time to CurrentTime
        if (ReadyArr[index].remaining_time == ReadyArr[index].burst_time) {
            ReadyArr[index].start_time = CurrentTime;
        }

        // Runs the task for the duration of the time quantum
        while (ReadyArr[index].remaining_time != 0) {
            printf("<time %u> process %u is running ...\n", CurrentTime, ReadyArr[index].pid);
            ReadyArr[index].remaining_time--;
            CurrentTime++;
            // Increments this each time to track the quantum
            timeSinceSwitch++;
            if (ReadyArr[index].remaining_time == 0) {
                printf("<time %u> process %u is finished ...\n", CurrentTime, ReadyArr[index].pid);
                ReadyArr[index].finish_time = CurrentTime;
                FinishArr[FinishSize] = ReadyArr[index];
                FinishSize++;
                ReadyArr[index].null = 1;
                // Resets the quantum because the task has finished
                timeSinceSwitch = 0;
                break;
            }
            // If timeSinceSwitch equals quantum then the task should be changed
            if (timeSinceSwitch == quantum) {
                timeSinceSwitch = 0;
                break;
            }
        }

        // This checks if there are any waiting/ready tasks
        u_int placeholder = 0;
        for (int i = 0; i < count; i++) {
            if (ReadyArr[i].null == 0 || TaskArr[i].queued == 0) {
                placeholder++;
            }
        }
        // If there is nothing waiting/ready then the program can finish execution
        if (placeholder == 0) {
            CanContinue = 0;
        }
    }

    // This calculates the totals for the stats
    for (int i = 0; i < count; i++) {
        double response = ReadyArr[i].start_time - ReadyArr[i].arrival_time;
        double waiting_time = ReadyArr[i].finish_time - ReadyArr[i].arrival_time - ReadyArr[i].burst_time;
        double trnd = ReadyArr[i].finish_time - ReadyArr[i].arrival_time;
        stats.ResponseAverage += response;
        stats.WaitingAverage += waiting_time;
        stats.TurnaroundAverage += trnd;
    }
    // This calculates the total cpu usage
    double divide = (double)(idle_time/CurrentTime) * 100;
    stats.CPUUsage = 100-divide;
    // This divides the stats by count to get the averages
    stats.ResponseAverage /= count;
    stats.WaitingAverage /= count;
    stats.TurnaroundAverage /= count;
}
