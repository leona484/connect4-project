//sarina hemmat
//40437555
//hello
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_ROWS 11
#define MAX_COLS 11
#define EMPTY '.'
#define PLAYER1 'X'
#define PLAYER2 'O'
#define MAX_MOVES 200
#define DB_FILE "games.db"
#define MAX_NAME 50

char board[MAX_ROWS][MAX_COLS];
int R, C;

typedef struct {
    char board[MAX_ROWS][MAX_COLS];
    int R, C;
} GameState;

typedef int (*MoveFn)(const GameState *st, void *ctx);

typedef struct {
    MoveFn move;
    void *ctx;
    char token;
} Player;

typedef struct {
    char name[MAX_NAME];
    int moves[MAX_MOVES];
    char tokens[MAX_MOVES];
    int totalMoves;
    int R, C;
    char result[50];
} SavedGame;

typedef struct {
    int moves[MAX_MOVES];
    int total;
    int current;
    int playerNum;
} FileCtx;

void initBoard() {
    int r, c;
    for (r = 0; r < R; r++)
        for (c = 0; c < C; c++)
            board[r][c] = EMPTY;
}

void showBoard() {
    int r, c;
    printf("\n");
    for (r = 0; r < R; r++) {
        printf("| ");
        for (c = 0; c < C; c++)
            printf("%c ", board[r][c]);
        printf("|\n");
    }
    printf("+-");
    for (c = 0; c < C; c++)
        printf("--");
    printf("+\n  ");
    for (c = 0; c < C; c++)
        printf("%d ", c + 1);
    printf("\n\n");
}

int dropPiece(int col, char piece) {
    int r;
    for (r = R - 1; r >= 0; r--) {
        if (board[r][col] == EMPTY) {
            board[r][col] = piece;
            return r;
        }
    }
    return -1;
}

void undoPiece(int row, int col) {
    board[row][col] = EMPTY;
}

int countDir(int r, int c, int dr, int dc, char piece) {
    int count = 0;
    while (r >= 0 && r < R && c >= 0 && c < C && board[r][c] == piece) {
        count++;
        r += dr;
        c += dc;
    }
    return count;
}

int checkWin(int row, int col, char piece) {
    int total;
    total = countDir(row, col, 0, 1, piece) + countDir(row, col, 0, -1, piece) - 1;
    if (total >= 4) return 1;
    total = countDir(row, col, 1, 0, piece) + countDir(row, col, -1, 0, piece) - 1;
    if (total >= 4) return 1;
    total = countDir(row, col, -1, 1, piece) + countDir(row, col, 1, -1, piece) - 1;
    if (total >= 4) return 1;
    total = countDir(row, col, -1, -1, piece) + countDir(row, col, 1, 1, piece) - 1;
    if (total >= 4) return 1;
    return 0;
}

int checkDraw() {
    int c;
    for (c = 0; c < C; c++)
        if (board[0][c] == EMPTY)
            return 0;
    return 1;
}

int isColValid(int col) {
    if (col < 0 || col >= C) return 0;
    if (board[0][col] != EMPTY) return 0;
    return 1;
}

int canWinCol(int col, char piece) {
    int row = dropPiece(col, piece);
    if (row == -1) return 0;
    int win = checkWin(row, col, piece);
    undoPiece(row, col);
    return win;
}

int humanMove(const GameState *st, void *ctx) {
    int playerNum = *(int*)ctx;
    int col;
    while (1) {
        printf("Player %d, enter column (1-%d): ", playerNum, C);
        if (scanf("%d", &col) != 1) {
            scanf("%*[^\n]");
            printf("Invalid input!\n");
            continue;
        }
        col--;
        if (!isColValid(col)) {
            printf("Invalid column! Try again.\n");
            continue;
        }
        return col;
    }
}

int fileMove(const GameState *st, void *ctx) {
    FileCtx *fc = (FileCtx*)ctx;
    int col;
    while (1) {
        if (fc->current >= fc->total) {
            printf("File ended! Player %d enter column (1-%d): ", fc->playerNum, C);
            scanf("%d", &col);
            col--;
        } else {
            col = fc->moves[fc->current];
            fc->current++;
            printf("Player %d (file) plays column %d\n", fc->playerNum, col + 1);
        }
        if (!isColValid(col)) {
            printf("Invalid move (col %d)! Skipping.\n", col + 1);
            continue;
        }
        return col;
    }
}

FileCtx loadFile(int playerNum) {
    FileCtx fc;
    fc.total = 0;
    fc.current = 0;
    fc.playerNum = playerNum;
    char filename[100];
    printf("Enter filename for Player %d: ", playerNum);
    scanf("%s", filename);
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        printf("Could not open '%s'! Will ask manually.\n", filename);
        return fc;
    }
    int col;
    while (fscanf(f, "%d", &col) == 1) {
        fc.moves[fc.total] = col - 1;
        fc.total++;
    }
    fclose(f);
    printf("Loaded %d moves.\n", fc.total);
    return fc;
}

int aiEasy(const GameState *st, void *ctx) {
    int col;
    do { col = rand() % C; } while (!isColValid(col));
    return col;
}

int aiMedium(const GameState *st, void *ctx) {
    char myToken = *(char*)ctx;
    int c;
    for (c = 0; c < C; c++)
        if (isColValid(c) && canWinCol(c, myToken))
            return c;
    int col;
    do { col = rand() % C; } while (!isColValid(col));
    return col;
}

int aiHard(const GameState *st, void *ctx) {
    char myToken = *(char*)ctx;
    char oppToken = (myToken == PLAYER1) ? PLAYER2 : PLAYER1;
    int c;
    for (c = 0; c < C; c++)
        if (isColValid(c) && canWinCol(c, myToken))
            return c;
    for (c = 0; c < C; c++)
        if (isColValid(c) && canWinCol(c, oppToken))
            return c;
    int mid = C / 2;
    if (isColValid(mid)) return mid;
    int col;
    do { col = rand() % C; } while (!isColValid(col));
    return col;
}

void saveGame(SavedGame *sg) {
    char name[MAX_NAME];
    printf("Enter a name to save this game: ");
    scanf(" %[^\n]", name);
    strcpy(sg->name, name);
    sg->R = R;
    sg->C = C;
    FILE *f = fopen(DB_FILE, "a");
    if (f == NULL) {
        printf("Could not save game!\n");
        return;
    }
    fprintf(f, "GAME %s\n", sg->name);
    fprintf(f, "SIZE %d %d\n", sg->R, sg->C);
    fprintf(f, "RESULT %s\n", sg->result);
    fprintf(f, "MOVES %d\n", sg->totalMoves);
    int i;
    for (i = 0; i < sg->totalMoves; i++)
        fprintf(f, "%d %c\n", sg->moves[i] + 1, sg->tokens[i]);
    fprintf(f, "END\n");
    fclose(f);
    printf("Game saved as '%s'!\n", name);
}

void replayGame() {
    FILE *f = fopen(DB_FILE, "r");
    if (f == NULL) {
        printf("No saved games found!\n");
        return;
    }
    char line[200];
    char names[50][MAX_NAME];
    int count = 0;
    while (fgets(line, sizeof(line), f)) {
        char gameName[MAX_NAME];
        if (sscanf(line, "GAME %[^\n]", gameName) == 1)
            strcpy(names[count++], gameName);
    }
    fclose(f);
    if (count == 0) {
        printf("No saved games!\n");
        return;
    }
    printf("\nSaved games:\n");
    int i;
    for (i = 0; i < count; i++)
        printf("%d- %s\n", i + 1, names[i]);
    printf("Choose a game (0 to cancel): ");
    int choice;
    scanf("%d", &choice);
    if (choice < 1 || choice > count) return;

    char target[MAX_NAME];
    strcpy(target, names[choice - 1]);

    f = fopen(DB_FILE, "r");
    SavedGame sg;
    sg.totalMoves = 0;
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        char gameName[MAX_NAME];
        if (sscanf(line, "GAME %[^\n]", gameName) == 1 && strcmp(gameName, target) == 0) {
            found = 1;
            int r2, c2;
            fgets(line, sizeof(line), f);
            sscanf(line, "SIZE %d %d", &r2, &c2);
            sg.R = r2; sg.C = c2;
            fgets(line, sizeof(line), f);
            sscanf(line, "RESULT %[^\n]", sg.result);
            int total;
            fgets(line, sizeof(line), f);
            sscanf(line, "MOVES %d", &total);
            sg.totalMoves = total;
            int j;
            for (j = 0; j < total; j++) {
                int col; char tok;
                fscanf(f, "%d %c\n", &col, &tok);
                sg.moves[j] = col - 1;
                sg.tokens[j] = tok;
            }
            break;
        }
    }
    fclose(f);
    if (!found) { printf("Game not found!\n"); return; }

    R = sg.R; C = sg.C;
    initBoard();
    printf("\nReplaying: %s | Result: %s\n", target, sg.result);
    showBoard();
    int j;
    for (j = 0; j < sg.totalMoves; j++) {
        printf("Press Enter for next move...");
        getchar(); getchar();
        dropPiece(sg.moves[j], sg.tokens[j]);
        printf("Move %d: %c plays column %d\n", j + 1, sg.tokens[j], sg.moves[j] + 1);
        showBoard();
    }
    printf("Replay done! Result: %s\n", sg.result);
}

void syncState(GameState *st) {
    int r, c;
    st->R = R; st->C = C;
    for (r = 0; r < R; r++)
        for (c = 0; c < C; c++)
            st->board[r][c] = board[r][c];
}

void playGame(Player players[2]) {
    initBoard();
    showBoard();
    GameState st;
    int turn = 0;
    SavedGame sg;
    sg.totalMoves = 0;
    while (1) {
        syncState(&st);
        int col = players[turn].move(&st, players[turn].ctx);
        char piece = players[turn].token;
        int row = dropPiece(col, piece);
        if (row == -1) {
            printf("Column %d is full!\n", col + 1);
            continue;
        }
        sg.moves[sg.totalMoves] = col;
        sg.tokens[sg.totalMoves] = piece;
        sg.totalMoves++;
        showBoard();
        if (checkWin(row, col, piece)) {
            printf("*** Player %d (%c) wins! ***\n", turn + 1, piece);
            sprintf(sg.result, "Player %d (%c) wins", turn + 1, piece);
            saveGame(&sg);
            return;
        }
        if (checkDraw()) {
            printf("*** It's a draw! ***\n");
            sprintf(sg.result, "Draw");
            saveGame(&sg);
            return;
        }
        turn = 1 - turn;
    }
}

void getBoardSize() {
    printf("Enter number of rows (4-11): ");
    scanf("%d", &R);
    if (R < 4 || R >= 12) { printf("Invalid!\n"); getBoardSize(); return; }
    printf("Enter number of columns (4-11): ");
    scanf("%d", &C);
    if (C < 4 || C >= 12) { printf("Invalid!\n"); getBoardSize(); return; }
}

int getDifficulty() {
    int d;
    printf("Choose AI difficulty:\n1- Easy\n2- Medium\n3- Hard\nChoice: ");
    scanf("%d", &d);
    if (d < 1 || d > 3) { printf("Invalid!\n"); return getDifficulty(); }
    return d;
}

int getMenuChoice() {
    int choice;
    printf("\n=== Connect Four ===\n");
    printf("1- Human vs Human\n");
    printf("2- Human vs Computer\n");
    printf("3- Human vs File\n");
    printf("4- File vs File\n");
    printf("5- Replay a saved game\n");
    printf("6- Exit\n");
    printf("Choose: ");
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > 6) {
        scanf("%*[^\n]");
        printf("Invalid!\n");
        return getMenuChoice();
    }
    return choice;
}

int main() {
    srand(time(NULL));

    while (1) {
        int choice = getMenuChoice();

        if (choice == 6) {
            printf("Goodbye!\n");
            return 0;
        }
        if (choice == 5) {
            replayGame();
            continue;
        }

        getBoardSize();

        Player players[2];
        players[0].token = PLAYER1;
        players[1].token = PLAYER2;

        static int p1num = 1, p2num = 2;
        static FileCtx fc1, fc2;
        static char aiToken2 = PLAYER2;

        if (choice == 1) {
            players[0].move = humanMove; players[0].ctx = &p1num;
            players[1].move = humanMove; players[1].ctx = &p2num;
        } else if (choice == 2) {
            players[0].move = humanMove; players[0].ctx = &p1num;
            int diff = getDifficulty();
            if (diff == 1) { players[1].move = aiEasy;   players[1].ctx = &aiToken2; }
            if (diff == 2) { players[1].move = aiMedium; players[1].ctx = &aiToken2; }
            if (diff == 3) { players[1].move = aiHard;   players[1].ctx = &aiToken2; }
        } else if (choice == 3) {
            players[0].move = humanMove; players[0].ctx = &p1num;
            fc2 = loadFile(2);
            players[1].move = fileMove; players[1].ctx = &fc2;
        } else {
            fc1 = loadFile(1); players[0].move = fileMove; players[0].ctx = &fc1;
            fc2 = loadFile(2); players[1].move = fileMove; players[1].ctx = &fc2;
        }

        playGame(players);
    }

    return 0;
}
