/* shell_env.h implementation */
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#include "status.h"
#include "shell_env.h"

// *******************private function declaration begin

/*
* purpose:	    get number of bytes that are imediately available for reading      
* pre:		    nil 
* post:		    nil 
* return:       return number of bytes that are imediately available for reading
*/
int kbhit(void){    
    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);    
    return bytesWaiting;
}

// private function declaration end*******************

int isKeyLock() { return !EXIT_KEY; }; 
void unlockEKey() { EXIT_KEY=1; };
void nextRow(int new_row) { row=new_row; }

// source: https://stackoverflow.com/questions/33025599/move-the-cursor-in-a-c-program
void shellInput (char* input, char* sh_prompt){
    struct termios oldattr, newattr;
    int insert, each, end, to, ch, col, f_row, f_col; //insert mode flag, each as current cursor, end as end of cursor, to as incremental variable for loops, ch as getchar()
    insert=each=end=to=ch=col=0; 
    
    // set terminal
    tcgetattr(STDIN, &oldattr);     // get parameters associated with STDIN
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON|ECHO);  // set to canonical mode - input available line by line and echo input characters  
    tcsetattr(STDIN, TCSANOW, &newattr);    // set parameters associated with STDIN  
    setbuf(stdin, NULL);    // disable buffering for stdin 
    
    strcpy(input," ");  // reset input; minimum 1 char to be copied to ensure input appear  
    printf("\033[%d;%dH", row, col);    // update cursor location 
    printf("%s: ", sh_prompt);  // print shell prompt 
    
    col=strlen(sh_prompt)+3;    // inputs start at the third column after the length of shell prompt 
    printf("\033[%d;%dH", row, col);    // update cursor location 
    f_row=row; f_col=col;  // remember where is user input location 
    
    while((ch=getchar())!='\n'){    // continue until enter is recieved 
        if(isprint(ch)){    // check if character printable 
            if(insert && each<end && end<SIZE-3){  // if recieved INS signal 
                end++;  
                for(to=end; to>=each; to--) input[to+1]=input[to]; // push forward to next char 
                printf("\033[%d;%dH", f_row, f_col);    
                printf("\033[K");
                printf("%s",input);
            }
            printf ("\033[%d;%dH", row, col);   
            printf ("%c", ch);  // printf what has been typed 
            input[each]=ch; // update input 
            each++; // next char 
            if(each>end)end=each;
            col++;  // update column 
            if(each==end)input[each]='\0';  // update string terminator 
            if(each>= SIZE-1){ exitStatus("shellInput(): shell input buffer overloaded",1); break; }
            continue;
        }
        if(ch==BACKSPACE){  // if recived BACKSPACE signal 
            if(each){   // removing each character form current cursor
                each--;col--;   // go back 1 char 
                for(to=each; to<=end; to++)input[to]=input[to+1];
                end--;  // update end cursor 
                printf("\033[%d;%dH", f_row, f_col);
                printf("\033[K");// erase to end of line
                printf("%s", input);    // update input 
                printf("\033[%d;%dH", row, col);
            }
        }
        if(ch==ESC){    // if recieved ESC signal 
            if(!kbhit()) continue;   // continue if number of bytes available for reading is 0 
            ch=getchar();
            if(ch==BRACKETLEFT){    
                ch=getchar();
                if(ch==INSERT){     // if recieved INS signal 
                    ch=getchar();
                    if(ch==TILDE){  // toggle INS 
                        insert=!insert;
                        printf ("\033[%d;%dH", row, col);
                    }
                }
                if(ch==ARROWRIGHT){ // if recieved right arrow signal 
                    if(each<end){   // if current cursor is before end of cursor 
                        printf ( "\033[C"); // cursor right
                        each++; col++;
                    }
                }
                if(ch==ARROWLEFT){  // if recieved left arrow signal 
                    if(each){   // if currnet cursor is not at position 0 
                        printf("\033[D");   // cursor left
                        each--; col--;
                    }
                }
            }
            else ungetc(ch,stdin);  // take single character and shove it back to input stream
        }
    }
    tcsetattr(STDIN, TCSANOW, &oldattr);    // restore terminal
}

//source: https://stackoverflow.com/questions/50884685/how-to-get-cursor-position-in-c-using-ansi-code
int getPos(int *y, int *x) {    
    char buf[SIZE]={0}; // set all to 0 
    int ret, i, pow;
    char ch;
    *y = 0; *x = 0;
    
    struct termios term, restore;
    tcgetattr(0, &term);    // get parameters associated with stdin and assign it with term 
    tcgetattr(0, &restore); // get parameters associated with stdin and assign it with restore  
    term.c_lflag &= ~(ICANON|ECHO); // set to canonical mode - input available line by line and echo input characters 
    tcsetattr(0, TCSANOW, &term);   // change occur imediately 
    
    write(1, "\033[6n", 4); // get cursor position 
    for(i=0, ch=0; ch!='R'&&i<SIZE; i++){   
	ret=read(0, &ch, 1);
        if(!ret) {  // if read one byte failed 
           tcsetattr(0, TCSANOW, &restore); // restore stdin parameters 
           exitStatus("getpos(): error reading response: cursor output position", 1);
           return 1;
        }
        buf[i]=ch;
    }
    if(i>=SIZE) { exitStatus("getPos(): command buffer overloaded", 1); return -1; }
    if(i<2){    // unusual behaviour 
        tcsetattr(0, TCSANOW, &restore);
        return 1;
    }
   
    for(i-=2, pow=1; buf[i]!=';'; i--, pow*=10) *x=*x + ( buf[i]-'0') * pow;    // calculate x-axis position 
    for(i-- , pow=1; buf[i]!='['; i--, pow*=10) *y=*y + ( buf[i]-'0') * pow;    // calculate y-axis position 
    tcsetattr(0, TCSANOW, &restore);    // restore terminal  
    return 0;
}
