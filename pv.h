/*
    pv.h - p & v operations 
*/

int getsem(int key, int semval);

void rmsem(int semid);

void p(int semid);

void v(int semid);

void p0(int semid); 

void v0(int semid);
