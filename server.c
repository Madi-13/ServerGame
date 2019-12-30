#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#define N 20

char map[20][20];

static struct sembuf sop_lock[2] = {
    0, 0, 0, // РѕР¶РёРґР°С‚СЊ РѕР±РЅСѓР»РµРЅРёСЏ
    0, 1, 0 // СѓРІРµР»РёС‡РёС‚СЊ РЅР° РµРґРёРЅРёС†Сѓ
};

static struct sembuf sop_unlock[1] = {
    0, -1, 0
};

typedef struct data {
    char action;
    int x, y;
    int id;
} Data;

void set_map(Data pos_list[], int n) {
    for(int i = 1; i <= n; i++) {
        map[pos_list[i].y][pos_list[i].x] = '*';
    }
}

void print_map() {
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            putchar(map[i][j]);
        }
        puts("");
    }
}

int char_to_digit(char ch) {
    return ch - '0';
}

int load_map() {
    int sum_points = 0;
    int fd = open("map.txt", O_RDWR);
    char ch;
    for(int i = 0; i <= N; i++) {
        for(int j = 0; j <= N; j++) {
            read(fd, &ch, 1);
            if (ch != '\n') {
                map[i][j] = ch;
                if (ch >= '1' && ch <= '9') {
                    sum_points += char_to_digit(ch);
                }
            }
        }
    }
    return sum_points;
}

void send_new_map(int *client_socket, int n) {
    for (int i = 1; i <= n; i++) { // РѕС‚РїСЂР°РІРєР° РЅРѕРІС‹С… РєРѕРѕСЂРґРёРЅР°С‚ РІСЃРµРј
        write(client_socket[i], map, 400);
    }
}

void connect_server(int server_socket, int socket_option) {
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option));
    struct sockaddr_in server_address;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);
    server_address.sin_family = AF_INET;
    bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));
    listen(server_socket, 5);
}

void print_points(int * points, int n) {
    puts("Results: ");
    for(int i = 1; i <= n; i++) {
        printf("Player #%d: %d points\n", i, points[i]);
    }
    puts("");
}

void declare_winner(int * points, int n, int * client_socket) {
    int count = 0;
    int max_val = -1, index;
    for(int i = 1; i <= n; i++) {
        if (points[i] > max_val)
            max_val = points[i];
    }
    for(int i = 1; i <= n; i++) {
        if (points[i] == max_val) {
            ++count;
            index = i;
        }
    }
    for (int i = 1; i <= n; i++) { // РѕС‚РїСЂР°РІРєР° РёРЅС„РѕСЂРјР°С†РёРё Рѕ РїРѕР±РµРґРёС‚РµР»Рµ
        map[0][0] = 'e';
        if (count == 1) {
            map[0][1] = 'l'; // loser
            if (index == i) {
                map[0][1] = 'w'; // winner
            }
        } else {
            map[0][1] = 'd'; // draw
        }
        write(client_socket[i], map, 400);
    }
    for(int i = 1; i <= n; i++) {
        close(client_socket[i]);
    }
    puts("The game is over: ");
    if (count > 1) {
        puts("It's draw");
    } else {
        printf("Player #%d is the winner!\n", index);
    }
    exit(0);
}

int main(int argc, char ** argv) {
    key_t semkey = ftok("/tmp", 'a');
    if(semkey < 0) {
        perror("fail to create key");
    }
    int semid = semget(semkey, 1, IPC_CREAT | IPC_EXCL | S_IRUSR |
        S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(semid < 0) {
        perror("Fail to create semaphore");
    }
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    int socket_option = 1;
    int new_x, new_y, old_pos_x, old_pos_y;
    int i = 0, val, points[100], n = atoi(argv[1]);
    memset(points, 0, 100);
    int client_socket[100];
    Data new_position;
    connect_server(server_socket, socket_option);
    Data pos_list[100];
    pos_list[1].x = 5;
    pos_list[1].y = 5;
    pos_list[2].x = 2;
    pos_list[2].y = 2;
    pos_list[3].x = 10;
    pos_list[3].y = 10;
    int sum_points = load_map();
    set_map(pos_list, n);
    print_map();
    struct sockaddr_in client[100];
    socklen_t size = sizeof(client[i]);
    for(int i = 1; i <= n; i++) {
        client_socket[i] = accept(server_socket, (struct sockaddr *) &client[i], &size);
        map[pos_list[i].y][pos_list[i].x] = '*';
        write(client_socket[i], map, 400);
    }
    int pipe_channel[2];
    pipe(pipe_channel);
    for(int i = 1; i <= n; i++) {
        if (fork() == 0) {
            Data position;
            const int flags = fcntl(client_socket[i], F_GETFL, 0);
            fcntl(client_socket[i], F_SETFL, flags | O_NONBLOCK);
            while(1) {
                int ans = read(client_socket[i], &position, sizeof(position));
                if(ans > 0) {
                    position.id = i;
                    semop(semid, &sop_lock[0], 2);
                    write(pipe_channel[1], &position, sizeof(Data));
                    semop(semid, &sop_unlock[0], 1);
                }
                if(ans == 0) {
                    exit(0);
                }
            }
        }
    }
    while(1) {
        int ans = read(pipe_channel[0], &new_position, sizeof(Data));
        if (ans > 0) {
            old_pos_x = pos_list[new_position.id].x;
            old_pos_y = pos_list[new_position.id].y;
            new_y = pos_list[new_position.id].y;
            new_x = pos_list[new_position.id].x;
            if (new_position.action == 'U') {
                new_y = pos_list[new_position.id].y - 1;
                if(map[new_y][new_x] != ' ' && map[new_y][new_x] != '*' && map[new_y][new_x] != '#') {
                    points[new_position.id] += char_to_digit(map[new_y][new_x]);
                    sum_points -= char_to_digit(map[new_y][new_x]);
                    print_points(points, n);
                }
                if (map[new_y][new_x] != '*' && map[new_y][new_x] != '#') {
                    pos_list[new_position.id].y--;
                    map[old_pos_y][old_pos_x] = ' ';
                    map[pos_list[new_position.id].y][pos_list[new_position.id].x] = '*';
                }
                send_new_map(client_socket, n);
            }
            if (new_position.action == 'D') {
                new_y = pos_list[new_position.id].y + 1;
                if(map[new_y][new_x] != ' ' && map[new_y][new_x] != '*' && map[new_y][new_x] != '#') {
                    points[new_position.id] += char_to_digit(map[new_y][new_x]);
                    sum_points -= char_to_digit(map[new_y][new_x]);
                    print_points(points, n);
                }
                if (map[new_y][new_x] != '*' && map[new_y][new_x] != '#') {
                    pos_list[new_position.id].y++;
                    map[old_pos_y][old_pos_x] = ' ';
                    map[pos_list[new_position.id].y][pos_list[new_position.id].x] = '*';
                }
                send_new_map(client_socket, n);
            }
            if (new_position.action == 'L') {
                new_x = pos_list[new_position.id].x - 1;
                if(map[new_y][new_x] != ' ' && map[new_y][new_x] != '*' && map[new_y][new_x] != '#') {
                    points[new_position.id] += char_to_digit(map[new_y][new_x]);
                    sum_points -= char_to_digit(map[new_y][new_x]);
                    print_points(points, n);
                }
                if (map[new_y][new_x] != '*' && map[new_y][new_x] != '#') {
                    pos_list[new_position.id].x--;
                    map[old_pos_y][old_pos_x] = ' ';
                    map[pos_list[new_position.id].y][pos_list[new_position.id].x] = '*';
                }
                send_new_map(client_socket, n);
            }
            if (new_position.action == 'R') {
                new_x = pos_list[new_position.id].x + 1;
                if(map[new_y][new_x] != ' ' && map[new_y][new_x] != '*' && map[new_y][new_x] != '#') {
                    points[new_position.id] += char_to_digit(map[new_y][new_x]);
                    sum_points -= char_to_digit(map[new_y][new_x]);
                    print_points(points, n);
                }
                if (map[new_y][new_x] != '*' && map[new_y][new_x] != '#') {
                    pos_list[new_position.id].x++;
                    map[old_pos_y][old_pos_x] = ' ';
                    map[pos_list[new_position.id].y][pos_list[new_position.id].x] = '*';
                }
                send_new_map(client_socket, n);
            }
            if(sum_points == 0) {
                declare_winner(points, n, client_socket);
            }
        }
    }
    for(int i = 1; i <= n; i++) {
        close(client_socket[i]);
    }
    return 0;
}
