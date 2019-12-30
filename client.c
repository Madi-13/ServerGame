#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <locale.h>
#include <ncurses.h>
#include <fcntl.h>
#include <stdlib.h>

int N = 20;
char map[20][20];

typedef struct data {
    char action;
    int x, y;
} Data;

int game_start();
void game_end();

void game_end_check() {
    if (map[0][0] == 'e') {
        game_end();
        if (map[0][1] == 'l') {
            puts("You lost ");
        } else if (map[0][1] == 'w') {
            puts("You won the game! ");
        } else {
            puts("It's draw ");
        }
        exit(0);
    }
}

void set_map() {
    game_end_check();
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            mvaddch(i, j , map[i][j]);
        }
    }
    refresh();
}

int game_start() {
    setlocale (LC_ALL, " ");
	if (!initscr ())
	    return 1;
    cbreak();
    noecho();
    nonl();
    meta(stdscr, TRUE);
    intrflush (stdscr, FALSE );
    keypad(stdscr, TRUE);
    set_map();
	if (has_colors ()) {
		start_color ();
		init_pair(1 , COLOR_WHITE , COLOR_BLUE );
	}
	attrset (COLOR_PAIR(1));
	bkgdset (COLOR_PAIR (1));
	clear();
    return 0;
}

void game_end() {
    bkgdset (COLOR_PAIR(0));
	clear();
	refresh();
	endwin();
}



void launch_game(int server_socket) {
    int caret_x = 5, caret_y = 5;
    Data position;
    read(server_socket, map, 400); // РЎС‡РёС‚С‹РІР°РЅРёРµ РЅР°С‡Р°Р»СЊРЅС‹С… РїРѕР·РёС†РёР№
	int flag = 1, c;
	game_start();
	nodelay(stdscr, TRUE);
	const int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);
    set_map();
    refresh();
	while (flag) {
        int read_ans = read(server_socket, map, 400);
        if (read_ans > 0) {
            set_map();
            refresh();
        }
        if (read_ans == 0) {
            break;
        }
		c = getch();
		switch (c) {
		    case 033:
			    flag = 0;
			    break;
	    	case KEY_UP:
                position.action = 'U';
		        write(server_socket, &position, sizeof(Data));
		    	break;
		    case KEY_DOWN:
                position.action = 'D';
		        write(server_socket, &position, sizeof(Data));
		    	break;
		    case KEY_LEFT:
                position.action = 'L';
		        write(server_socket, &position, sizeof(Data));
                break;
		    case KEY_RIGHT:
                position.action = 'R';
		        write(server_socket, &position, sizeof(Data));
		    	break;
		}
	}
}

void connect_server() {
    char ip[] = "127.0.0.1";
    short port = 8080;
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    int socket_option = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option));
    struct hostent *host = gethostbyname(ip);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    memcpy(&server_address.sin_addr, host->h_addr_list[0], sizeof(server_address));
    if (connect(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("fail");
    }
    launch_game(server_socket);
}

int main () {
    connect_server();
    game_end();
    return 0;
}
