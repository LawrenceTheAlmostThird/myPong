/* 
   Server for MyPong project, CSCI 335.001
   By Lawrence Wilkes
   This is the client. It connects to the server based on the 
   command line argument, and then it recieves information about the state of the game
   and renders the game on the console. 
   It also sends user input to the server.
 */

#include <curses.h>
#include <unistd.h>
#include <locale.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define NROWS 20
#define NCOLS 50
#define X0 0
#define Y0 0

void render_ball(WINDOW * win, int ballx, int bally);
void render_edges(WINDOW *win);
void render_paddle(WINDOW * win, int paddle_top_height, int player);
void render_scores(WINDOW * win, int p1score, int p2score);
void sigpipe_handler(int sig);
void sigquit_handler(int sig);

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("Usage: %s <host addr in dotted decimal IPV4 format>\n", argv[0]);
        return -1;
    }
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0 ) {
        perror("Error during socket creation: ");
        exit(1);
    }
    
    signal(SIGPIPE, sigpipe_handler);
    
    sigset_t mask;
    struct timespec timeout;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) != 0) { //suspend sigints
        perror("sigprocmask");
        exit(1);
    }

    timeout.tv_sec = 0;
    timeout.tv_nsec = 1000000;


    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));//zero out everything
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8289);

    int err = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    if (!err) {
        //perror("Address translation error");
        if (err == EAI_SYSTEM) {
            fprintf(stderr, "Address translation error:%s\n", strerror(errno));
        } else {
            fprintf(stderr, "Address translation error:%s\n", gai_strerror(err));
        }
        //return;
    }

    printf("Attempting connection to %s\n", argv[1]);
    if(connect(sockfd, &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection error");
        exit(1);
    } else {
        printf("Connection successful.\n");
    }

    setlocale(LC_ALL, "");

    initscr();cbreak();noecho();keypad(stdscr, TRUE); //ncurses initialization

    halfdelay(1); //wait for 1/10 seconds
    WINDOW * win = newwin(NROWS, NCOLS, Y0, X0);
    scrollok(win, FALSE);
    
    
    char sendbuff[255];
    char recbuff[255];
    memset(sendbuff, 0, 255);
    memset(recbuff, 0, 255);
    int p1score;
    int p2score;
    int p1paddletop;
    int p2paddletop;
    int ballx;
    int bally;
    int key;
    int winner = 0;

    int quit = 0;
    while (!quit) { //game loop
        
        if ((key = wgetch(win)) == '\033') {
            wgetch(win); //skip the '[' char
            sprintf(sendbuff, "%d", wgetch(win));
        } else {
        sprintf(sendbuff, "%d", key); //if not an arrow key, write key value
        }
        flushinp(); //wipe out any excess input
        send(sockfd, &sendbuff, 255, 0);
        //printf("%s", sendbuff);
        recv(sockfd, &recbuff, 255, MSG_WAITALL);
        sscanf(recbuff, "%d %d %d %d %d %d", &ballx, &bally, &p1paddletop, &p2paddletop, &p1score, &p2score);

        render_edges(win);
        render_ball(win, ballx, bally);
        render_paddle(win, p1paddletop, 1);
        render_paddle(win, p2paddletop, 2);
        render_scores(win, p1score, p2score);
        
        memset(sendbuff, 0, 255);
        memset(recbuff, 0, 255);
        
        if (sigtimedwait(&mask, NULL, &timeout) > 0) {  //poll for suspended SIGINT signals
            quit = 1; //TODO check that this actually works
        }

        if (p1score >= 5) {
            winner = 1;
            sleep(2);
            quit = 1;
        }
        if (p2score >= 5) {
            winner = 2;
            sleep(2);
            quit = 1;
        }
    }

    sleep(2);
    
    delwin(win);

    endwin(); //back to normal
    if (winner > 0) {
        printf("Player %d wins!\n", winner);

    } else {
        printf("Error! SIGQUIT caught.\n");
    }
    printf("Game is over, thanks for playing!\n");
    return 0;
}

void render_edges(WINDOW * win) {
    wmove(win, Y0, X0);
    
    int i;
    int j;
    for (i = 0; i < NCOLS; i++) {
        waddch(win, '-' | 0);
    }
    for (i = 0; i < NROWS-2; i++) {
        waddch(win, '|' | 0);
        for (j = 0; j < NCOLS-2; j++) {
            waddch(win, ' ' | 0);
        }
        waddch(win, '|' | 0);
    }
    for (i = 0; i < NCOLS; i++) {
        waddch(win, '-' | 0);
    }

    wrefresh(win);
}

void render_ball(WINDOW * win, int ballx, int bally) {
    wmove(win, X0 + ballx, Y0 + bally);
    waddch(win, '+' | 0);
    wrefresh(win);
}

void render_paddle(WINDOW * win, int paddle_top_height, int player) {
    int paddle_col;
    if (player == 1) {
        paddle_col = Y0 + 1;
    } else if (player == 2) {
        paddle_col = Y0+NCOLS-3;
    } else {paddle_col = -2;}
    //blank out the column
    int i;
    wmove(win, X0 + 1, paddle_col);
    for (i = 0; i < NROWS - 1; i++) {
        waddch(win, ' ' | 0);
        waddch(win, ' ' | 0);
        wmove(win, X0 + 1 + i, paddle_col);
    }
    wmove(win, X0 + paddle_top_height+1, paddle_col);
    for (i = 0; i < 5; i++) {
        waddch(win, '[' | 0);
        waddch(win, ']' | 0);

        wmove(win, X0 + paddle_top_height + 1 + i, paddle_col);
    }
    wrefresh(win);
}

void render_scores(WINDOW * win, int p1score, int p2score) {
    char scorebuffer[20];
    sprintf(scorebuffer, "%d | %d", p1score, p2score);
    mvwaddstr(win, X0,  Y0 + (NCOLS / 2) - (strlen(scorebuffer) / 2), scorebuffer);
}

void sigpipe_handler(int sig) {

    endwin();
    fprintf(stderr, "\n");
    fflush(stderr);
    fprintf(stderr, "Error: Disconnected.\n");
    fflush(stderr);
    exit(0);

}
