#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SIZE 9

// Структура для хранения состояния игры
typedef struct {
    char board[SIZE];  // Игровое поле
    sem_t *sem_player1; // Семофор для синхронизации игрока 1
    sem_t *sem_player2; // Семофор для синхронизации игрока 2
    int turn; // Чей ход (1 для игрока 1, 2 для игрока 2)
} GameState;

// Инициализация игры
void init_game(GameState *game) {
    for (int i = 0; i < SIZE; i++) {
        game->board[i] = ' ';
    }
    game->turn = 1;
}

// Отображение игрового поля
void display_board(GameState *game) {
    printf("\n");
    for (int i = 0; i < SIZE; i++) {
        printf(" %c ", game->board[i]);
        if (i % 3 == 2) {
            printf("\n");
        } else {
            printf("|");
        }
    }
    printf("\n");
}

// Проверка победителя
int check_winner(GameState *game) {
    // Проверяем горизонтали, вертикали и диагонали
    for (int i = 0; i < 3; i++) {
        if (game->board[i * 3] == game->board[i * 3 + 1] && game->board[i * 3 + 1] == game->board[i * 3 + 2] && game->board[i * 3] != ' ') {
            return 1;
        }
        if (game->board[i] == game->board[i + 3] && game->board[i + 3] == game->board[i + 6] && game->board[i] != ' ') {
            return 1;
        }
    }
    if (game->board[0] == game->board[4] && game->board[4] == game->board[8] && game->board[0] != ' ') {
        return 1;
    }
    if (game->board[2] == game->board[4] && game->board[4] == game->board[6] && game->board[2] != ' ') {
        return 1;
    }
    return 0;
}

// Основная функция для игрока
void play_game(GameState *game, int player_num) {
    int pos;
    char mark = (player_num == 1) ? 'X' : 'O';

    while (1) {
        if (game->turn != player_num) {
            // Ожидание своего хода
            continue;
        }

        display_board(game);

        // Получение хода от игрока
        printf("Player %d (%c), enter your move (0-8): ", player_num, mark);
        scanf("%d", &pos);

        if (pos < 0 || pos >= SIZE || game->board[pos] != ' ') {
            printf("Invalid move, try again.\n");
            continue;
        }

        game->board[pos] = mark;
        game->turn = (player_num == 1) ? 2 : 1;

        // Проверка на победу
        if (check_winner(game)) {
            display_board(game);
            printf("Player %d (%c) wins!\n", player_num, mark);
            break;
        }
    }
}

int main() {
    // Создание общедоступной памяти
    int shm_fd = shm_open("/tictactoe", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd == -1) {
        perror("Error creating shared memory");
        exit(1);
    }
    ftruncate(shm_fd, sizeof(GameState));
    GameState *game = mmap(NULL, sizeof(GameState), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) {
        perror("Error mapping shared memory");
        exit(1);
    }

    // Создание семафоров
    game->sem_player1 = sem_open("/sem_player1", O_CREAT, S_IRUSR | S_IWUSR, 0);
    game->sem_player2 = sem_open("/sem_player2", O_CREAT, S_IRUSR | S_IWUSR, 0);

    if (game->sem_player1 == SEM_FAILED || game->sem_player2 == SEM_FAILED) {
        perror("Error creating semaphores");
        exit(1);
    }

    init_game(game);
    pid_t pid = fork();

    if (pid == 0) {
        // Процесс для игрока 1 (крестики)
        play_game(game, 1);
    } else {
        // Процесс для игрока 2 (нолики)
        play_game(game, 2);
    }

    // Очистка ресурсов
    sem_close(game->sem_player1);
    sem_close(game->sem_player2);
    shm_unlink("/tictactoe");
    sem_unlink("/sem_player1");
    sem_unlink("/sem_player2");

    return 0;
}
