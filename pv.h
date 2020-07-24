/* name:			pv.h	
 * purpose: 		p & v operations  
 * author:			HX
 * date:			
 * last modified:	13082000
 */
int getsem(int key, int semval);

void rmsem(int semid);

void p(int semid);	// SEM_UNDO version

void v(int semid);	// SEM_UNDO version

void p0(int semid); // no SEM_UNDO version

void v0(int semid);	// no SEM_UNDO version
