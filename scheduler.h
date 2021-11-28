/*
 * COMP 3500: Project 5 Scheduling
 * Hunter Westerlund
 * Version 1.0  11/28/2021
 *
 * This source code shows how to conduct separate compilation.
 *
 * scheduler.h: The header file of scheduler.c
 */
#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#define MAX_TASK_NUM 32

typedef unsigned int u_int;

typedef struct task_info { // each task has many features 
    u_int pid; // process id
    u_int arrival_time;
    u_int burst_time; // run time
    u_int remaining_time;
    u_int queued; // indicates a task is in the ready queue
    u_int start_time;
    u_int finish_time; // finish time
    u_int null; // indicates a task has finished, i.e. 'removed' from ready queue
    u_int doneThisRound; //used for round robin
} task_t;

typedef struct stat_info { // statistical analysis
    u_int WaitingAverage;
    u_int TurnaroundAverage;
    u_int ResponseAverage;
    double CPUUsage;
} stat_info_t;

// TaskList is the waiting list, size is the size of waiting list, Policy is the policy passed in, and TimeQuantum is for round robin
void task_master(task_t TaskList[], int size, char *Policy, u_int TimeQuantum);
// chooses the correct policy function to call

//TaskArr is the waiting list, count is the size of waiting list
void fcfs_policy(task_t TaskArr[], u_int count);
//implements first come first serve, i.e. tasks are processed in order of arrival

//TaskArr is the waiting list, count is the size of waiting list
void srtf_policy(task_t TaskArr[], u_int count);
//implements shortest remaining time first, i.e. remaining time is processed on ready queue before a task is chosen

//TaskArr is the waiting list, count is the size of waiting list, TimeQuantum is length of equal time slices
void rr_policy(task_t TaskArr[], u_int count, u_int TimeQuantum);
//implements round robin, allows for time slices so that response time is minimized

#endif

