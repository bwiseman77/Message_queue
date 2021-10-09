/* shell.c
 * Demonstration of multiplexing I/O with a background thread also printing.
 **/

#include "mq/thread.h"
#include "mq/client.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>

#include<ncurses.h>

WINDOW *new_window(WINDOW *, int, int);

/* Constants */

#define BACKSPACE   127

/* global */


int Line_Number = 0;
char CLEAR[BUFSIZ] = "                                                                                   "; 

/* Threads */

void *background_thread(void *arg) {

	MessageQueue* mq = (MessageQueue*)arg;
	while (!mq_shutdown(mq)){
		char *message = mq_retrieve(mq);
		if (message){
			int row, col;

			getmaxyx(stdscr, row, col);

			if(Line_Number >= row - 3) {
				clear();
				Line_Number = 0;
				mvprintw(row-2, 0, "Input: \n");

			}
			if(strstr(message, mq->name)) {
				mvprintw(Line_Number++, 0, "%s\n", message);
			} else {
				mvprintw(Line_Number++, (col - strlen(message)), "%s\n", message);
			}
			mvprintw(row-1, 0, "%s", CLEAR);
			mvprintw(row-1, 0, "%s", "");
			refresh();
		}

		free(message);
	}
	return NULL;
}

/* Main Execution */

int main(int argc, char *argv[]) {


    /* Foreground Thread */
    char   input_buffer[BUFSIZ] = "";
    //size_t input_index = 0;

	char* name = getenv("USER");
	char* host = "localhost";
	char* port = "9620";

	
	if (argc > 1) { host = argv[1]; }
	if (argc > 2) { port = argv[2]; }
	if (argc > 3) { name = argv[3]; }
	if (!name)    { name = "echo_client_test"; }

	MessageQueue* mq = mq_create(name, host, port);
	mq_start(mq);
	initscr();

    /* Background Thread */
    Thread background;
    thread_create(&background, NULL, background_thread, (void*)mq);


	while (!mq_shutdown(mq)){
		//fgets(input_buffer, BUFSIZ, stdin);
		int row, col;
		getmaxyx(stdscr, row, col);
		col++;
		mvprintw(row-1, 0, "%s", CLEAR);
		//refresh();
		mvprintw(row-2, 0, "Input: \n");
		getstr(input_buffer);
		char* first = strtok(input_buffer, " \n");
		char* second = strtok(NULL, " \n");

		if (strcmp(first, "/exit") == 0){
			mq_stop(mq);
			break;
		}
		if (strcmp(first, "/quit") == 0){
			mq_stop(mq);
			break;
		}
		if (strcmp(first, "sub") == 0){
			mq_subscribe(mq, second);
			continue;
		}
		if (strcmp(first, "unsub") == 0){
			mq_unsubscribe(mq, second);
			continue;
		}
		if (strcmp(first, "pub") == 0){
			char header[BUFSIZ];
			char *body = strtok(NULL, "\n");
		   	sprintf(header, "(%s): %s", mq->name, body);

			mq_publish(mq, second, header);
			continue;
		}
	}

	mq_delete(mq);
	endwin();

    return 0;
}

