#include <stdio.h>
#include <errno.h>
#include <sys/sem.h>
#include <stdlib.h>

#include "pv.h"
#include "status.h"

// get semapore id
int getsem(int key, int semval) {
    int semid;
    
    union semun{
    	int val;
    	struct semid_ds *buff;
	    unsigned short int *array; 
    }arg;

    // create semaphore if it doesn't exist else returns its id
    semid = semget((key_t)key, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(semid<0 && errno==EEXIST){
    	if((semid = semget((key_t)key, 1, 0666))!=-1) return semid; // ady exist 
    	exitStatus("getsem(): semget() failed", 2);
    }
    if(semid<0) exitStatus("getsem(): cannot get semid", 2);

    // set the semaphore value 
    arg.val = semval; 
    if(semctl(semid, 0, SETVAL, arg)<0) 
        exitStatus("getesm(): semctl() failed", 2); // IPC_SETVAL to set

    return semid;
}

void rmsem(int semid) {
    union semun{
    	int val;
    	struct semid_ds* buff;
    	unsigned short int *array;
    }arg;

    if(semctl(semid, 0, IPC_RMID, arg)<0) 
        exitStatus("rmsem(): semtcl() removed failed", 2); // IPC_RMID to remove
}

// p operation - SEM_UNDO
void p(int semid){
    struct sembuf sb;
    sb.sem_num  = 0;
    sb.sem_op   = -1; // p operation
    sb.sem_flg  = SEM_UNDO;

    if(semop(semid, &sb, 1)<0)
        exitStatus("p(): semop in p failed", 2);
}

// p operation - no SEM_UNDO
void p0(int semid){
    struct sembuf sb;
    sb.sem_num  = 0;
    sb.sem_op   = -1; // p operation
    sb.sem_flg  = 0;

    if(semop(semid, &sb, 1)<0)
        exitStatus("p0(): semop in p failed", 2);
}

// v operation - SEM_UNO
void v(int semid){
    struct sembuf sb;
    sb.sem_num  = 0;
    sb.sem_op   = 1; // v operation
    sb.sem_flg  = SEM_UNDO;

    if(semop(semid, &sb, 1)<0)
        exitStatus("v(): semop in v failed", 2);
}


// v operation - no SEM_UNO
void v0(int semid){
    struct sembuf sb;
    sb.sem_num  = 0;
    sb.sem_op   = 1; // v operation
    sb.sem_flg  = 0;

    if(semop(semid, &sb, 1)<0)
        exitStatus("v0(): semop in v failed", 2); 
}

