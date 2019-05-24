/* 
   Server for MyPong project, CSCI 335.001
   By Lawrence Wilkes
   This is the server. It receives two incoming connections from clients,
   and tells them how to blit their screens. All scoring/session-handling/collison detection
   is done on this server. If the game ends normally, the final score is printed to the highscores table.
*/

#include <arpa/inet.h> 
#include <sys/socket.h> 
#include <setjmp.h>
#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <math.h>
#include <netdb.h> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "csapp.h"

#define NROWS 20
#define NCOLS 50
#define X0 0
#define Y0 0

char *game(int p1fd, int p2fd, jmp_buf env);
void sigpipe_handler(int sig);
jmp_buf env;

int main(int argc, char ** argv)
{

    char * listen_port = "8289";
    int listenfd, connfd, connfd2;
    FILE * highscorefd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    
    listenfd = Open_listenfd(listen_port);
    
    
    while (1) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGPIPE);

    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) != 0) { //reset sig handler
        perror("sigprocmask");
        exit(1);
    }

    signal(SIGPIPE, sigpipe_handler);
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    connfd2 = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    char * scores = game(connfd, connfd2, env); //game loop
    Close(connfd);
    Close(connfd2);
    printf("Score: %s\n", scores);
    //write to highscore table
    highscorefd = fopen("score.txt", "w");
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(highscorefd, "Game on %d-%d-%d at %d:%d : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, scores);
    fclose(highscorefd);
    }
    return 0;
}

char *game(int p1fd, int p2fd, jmp_buf env) {
    char recbuff[255];
    char recbuff2[255];
    char sendbuff[255];
    memset(recbuff, 0, 255);
    memset(recbuff2, 0, 255);
    memset(sendbuff, 0, 255);
    int ballx, bally, p1input, p2input, i;
    int p1paddle = 5;
    int p2paddle = 5;
    int p1score = 0;
    int p2score = 0; //game start values
    srand(time(NULL));

    double ballspeedx = (((double) random() / (double) RAND_MAX) - .5);
    ballspeedx += copysign(0.5, ballspeedx); //add .5 if positive, sub .5 if negative
    double ballspeedy = copysign(.75, (double) random() - ((double) RAND_MAX / 2)); //randomly .75 or -.75
    double realballx = (double) NROWS / 2;
    double realbally = (double) NCOLS / 2;

    while (1) {
        recv(p1fd, &recbuff, 255, 0);
        recv(p2fd, &recbuff2, 255, 0);

        if (setjmp(env) != 0) {
            printf("Jumped!\n");
            char output[20];
            sprintf(output, "%d | %d", p1score, p2score);
            char * o = output;
            return o;
        }
        
        sscanf(recbuff, "%d", &p1input);
        sscanf(recbuff2, "%d", &p2input);
        
        //game switch
        switch(p1input) {
            case -1:
                break;
            case 65: //arrow up
            case 119: //W key
                if (p1paddle > X0) { //ensure top does not clip offscreen
                    p1paddle--;
                }
                break;
            case 66: //arrow down
            case 115: //S key
                if (p1paddle < X0 + NROWS - 6) { //ensure bottom does not clip
                    p1paddle++;
                }
                break;
        }

        switch(p2input) {
            case -1:
                break;
            case 65: //arrow up
            case 119: //W key
                if (p2paddle > X0) { //ensure top does not clip offscreen
                    p2paddle--;
                }
                break;
            case 66: //arrow down
            case 115: //S key
                if (p2paddle < X0 + NROWS - 6) { //ensure bottom does not clip
                    p2paddle++;
                }
                break;
        }
        
        realballx += ballspeedx;
        realbally += ballspeedy;
        ballx = (int)round(realballx);
        bally = (int)round(realbally);

        if (bally <= Y0+3) { //player 1's side of the court
            if (ballx > p1paddle - 1 && ballx < p1paddle + 5 ) { //paddle is wide to compensate for rounding weirdness
                ballspeedy = -ballspeedy;
                ballspeedy *= 1.2;
            } else {
                //reinitialize ball, add to p2 score
                ballspeedy = copysign(.75, (double) random() - ((double) RAND_MAX / 2)); //randomly .75 or -.75
                ballspeedx = (((double) random() / (double) RAND_MAX) * 2) - 1; //map a random number between 1 and -1
                realballx = (double) NROWS / 2;
                realbally = (double) NCOLS / 2;
                p2score++;
            }
        }
        if (bally >= Y0 + NCOLS-3) { //player 2's side
            if (ballx > p2paddle - 1 && ballx < p2paddle + 5 ) {
                ballspeedy = -ballspeedy;
                ballspeedy *= 1.2;
            } else {
                //reinitialize ball, add to p1 score
                ballspeedy = copysign(.75, (double) random() - ((double) RAND_MAX / 2)); //randomly .75 or -.75
                ballspeedx = (((double) random() / (double) RAND_MAX) * 2) - 1; //map a random number between 1 and -1
                realballx = (double) NROWS / 2;
                realbally = (double) NCOLS / 2;
                p1score++;
            }
        }
        if (ballx <= X0+1 || ballx >= X0 + NROWS-2) { //top and bottom
            ballspeedx = -ballspeedx;
        }
        
        //testing
        //ballx = 1;
        //bally = 5;

        sprintf(sendbuff, "%d %d %d %d %d %d %c", ballx, bally, p1paddle, p2paddle,  p1score, p2score, '\0');
        for (i = 0; i > 255 - (long) sizeof(sendbuff); i++) {
            sendbuff[i] = 'x';
        }
        printf("Player 1's Message: ");
        printf("%s\n", recbuff);
        printf("Player 2's Message: ");
        printf("%s\n", recbuff2);
        int err = send(p1fd, &sendbuff, 255, 0);
        err |= send(p2fd, &sendbuff, 255, 0);
        if (err > 0)
            printf("Message sent to client: ");
            printf("%s\n", sendbuff);
            memset(sendbuff, 0, 255);
        if (err < 0)
            perror("Error sending message to client");
    }
}

void sigpipe_handler(int sig){
    printf("Longjumping\n");
    longjmp(env, 1);
    printf("Control never reaches here\n");
}
