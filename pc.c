#include <stdio.h>
#include <errno.h>
#include <sys/sem.h>
#include <stdlib.h>
#include "pv.h"

// get semapore id
int getsem(int key, int semval){
    int semid;
    union semun{
	int val;
	struct semid_ds *buff;
	unsigned short int *array; 
    }arg;

    // create semaphore if it doesn't exist else returns its id
    semid = semget((key_t)key, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(semid<0 && errno==EEXIST){
	if((semid = semget((key_t)key, 1, 0666))!=-1)return semid; // ady exist 
	perror("semget() failed");
	exit(1);
    }
    if(semid<0){ perror("cannot get semid"); exit(1); }

    // set the semaphore value 
    arg.val = semval; 
    if(semctl(semid, 0, SETVAL, arg)<0){ perror("semctl() failed"); exit(2); } // IPC_SETVAL to set

    return semid;
}


void rmsem(int semid){
    union semun{
	int val;
	struct semid_ds* buff;
	unsigned short int *array;
    }arg;

    if(semctl(semid, 0, IPC_RMID, arg)<0){ perror("semtcl() removed failed"); exit(1); } // IPC_RMID to remove
}

// p operation - SEM_UNDO
void p(int semid){
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = -1; // p operation
    sb.sem_flg = SEM_UNDO;

    if(semop(semid, &sb, 1)<0){ perror("semop in p failed"); exit(1); }
}

// p operation - no SEM_UNDO
void p0(int semid){
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = -1; // p operation
    sb.sem_flg = 0;

    if(semop(semid, &sb, 1)<0){ perror("semop in p failed"); exit(1); }
}

// v operation - SEM_UNO
void v(int semid){
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = 1; // v operation
    sb.sem_flg = SEM_UNDO;

    if(semop(semid, &sb, 1)<0){ perror("semop in v failed"); exit(1); }
}


// v operation - no SEM_UNO
void v0(int semid){
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = 1; // v operation
    sb.sem_flg = 0;

    if(semop(semid, &sb, 1)<0){ perror("semop in v failed"); exit(1); }
}

