#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#define TAMANHO_FORMIGA   256
#define quadrado_tamanho    31
#define MAX_nome_LENGTH 21
#define MAX_LEADERBOARD_ENTRIES 5
#define MAX_GOTAS 256

typedef struct Formiga {
    Vector2 posicao;
    Vector2 tamanho;
    Vector2 velocidade;
    Color cor;
} Formiga;

typedef struct Migalha {
    Vector2 posicao;
    Vector2 tamanho;
    bool active;
    Color cor;
} Migalha;

typedef struct Gotas {
    Vector2 posicao;
    Vector2 tamanho;
    bool active;
    Color cor;
} Gotas;

typedef struct LeaderboardEntry {
    char nome[MAX_nome_LENGTH];
    int pontos;
    struct LeaderboardEntry* next;
} LeaderboardEntry;

static int largura = 800;
static int altura = 450;

static LeaderboardEntry* leaderboard = NULL;

static int pontos = 0;
static int contador_comida = 0;
static int framesCounter = 0;
static int gameOver = 2;//0:jogo começa |1:apertar enter | 2:insere o nome
static bool pause = false;
static bool gameStart = false;

static char jogador[MAX_nome_LENGTH];

static int ContadorGotas = 0; // Adiciona a definição da variável ContadorGotas


static Migalha migalha = { 0 };
static Formiga formiga[TAMANHO_FORMIGA] = { 0 };
static Gotas gotas[TAMANHO_FORMIGA] = { 0 };
static Vector2 Posicao_Formiga[TAMANHO_FORMIGA] = { 0 };
static bool allowMove = false;
static Vector2 offset = { 0 };
static int Tamanho = 0;

static void InitGame(void);
static void UpdateGame(void);
static void DrawGame(void);
static void UpdateDrawFrame(void);
static void InitGotas(void);
static void Updategotas(void);
static void CheckObstacleCollision(void);
void AddLeaderboardEntry(const char* nome, int pontos);
void DrawLeaderboard(void);
void SaveLeaderboard(void);
void LoadLeaderboard(void);
void NomeJogador();

int main(void)
{
    InitWindow(largura, altura, "Formiguita");
    SetWindowPosition(0, 0);
    InitGame();
    LoadLeaderboard();

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
    CloseWindow();
    return 0;
}

void AddLeaderboardEntry(const char* nome, int pontos)
{
    LeaderboardEntry* newEntry = (LeaderboardEntry*)malloc(sizeof(LeaderboardEntry));
    if (newEntry == NULL) {
        printf("Erro ao alocar memória para nova entrada do leaderboard.\n");
        return;
    }

    strncpy(newEntry->nome, nome, MAX_nome_LENGTH - 1);
    newEntry->nome[MAX_nome_LENGTH - 1] = '\0'; // Garantir que a string está terminada em null
    newEntry->pontos = pontos;
    newEntry->next = NULL;

    if (leaderboard == NULL || leaderboard->pontos < pontos) {
        newEntry->next = leaderboard;
        leaderboard = newEntry;
    }
    else {
        LeaderboardEntry* current = leaderboard;
        while (current->next != NULL && current->next->pontos >= pontos) {
            current = current->next;
        }
        newEntry->next = current->next;
        current->next = newEntry;
    }

    // Limita o número de entradas no leaderboard
    LeaderboardEntry* current = leaderboard;
    int count = 1;
    while (current->next != NULL && count < MAX_LEADERBOARD_ENTRIES) {
        current = current->next;
        count++;
    }
    if (current->next != NULL) {
        LeaderboardEntry* temp = current->next;
        current->next = NULL;
        while (temp != NULL) {
            LeaderboardEntry* toDelete = temp;
            temp = temp->next;
            free(toDelete);
        }
    }
}


void InitGame(void)
{
    framesCounter = 0;
    gameOver = 2;
    pause = false;
    Tamanho = 1;
    allowMove = false;
    pontos = 0;

    // Resetar o tamanho do mapa ao tamanho original
    largura = 800;
    altura = 450;
    SetWindowSize(largura, altura);

    offset.x = largura % quadrado_tamanho;
    offset.y = altura % quadrado_tamanho;

    contador_comida = 0;

    Color CorCabeca = BLACK;
    Color CorCorpo = (Color){ 0, 0, 0, 200 };

    Vector2 initialposicao = { offset.x / 2, offset.y / 2 };

    for (int i = 0; i < TAMANHO_FORMIGA; i++) {
        formiga[i].posicao = initialposicao;
        formiga[i].tamanho = (Vector2){ quadrado_tamanho, quadrado_tamanho };
        formiga[i].velocidade = (Vector2){ quadrado_tamanho, 0 };
        formiga[i].cor = (i == 0) ? CorCabeca : CorCorpo;
    }

    memset(Posicao_Formiga, 0, sizeof(Posicao_Formiga));

    migalha.tamanho = (Vector2){ quadrado_tamanho, quadrado_tamanho };
    migalha.cor = (Color){ 205, 133, 63, 200 };
    migalha.active = false;

    InitGotas();
}



void InitGotas(void)
{
    for (int i = 0; i < MAX_GOTAS; i++) {
        gotas[i].active = false;
        gotas[i].tamanho = (Vector2){ quadrado_tamanho, quadrado_tamanho };
        gotas[i].cor = BLUE;
    }
    ContadorGotas = 0;
}

void UpdateDrawFrame(void)
{
    UpdateGame();
    DrawGame();
}

void UpdateGame(void)
{
    if (gameOver == 2) {
        if (IsKeyPressed(KEY_ENTER) && strlen(jogador) > 0 && jogador[0] != ' ') {
            gameOver = 0;
        }
        else if (strlen(jogador) >= 20) {
            return;
        }
        else if (IsKeyPressed(KEY_SPACE) && strlen(jogador) < 1) {
            return;  // Evita adicionar espaço no início
        }
        else if (IsKeyPressed(KEY_BACKSPACE) && strlen(jogador) > 0) {
            jogador[strlen(jogador) - 1] = '\0';
        }
        else {
            NomeJogador(); // Funciona mas adiciona if por if
        }
    }
    else if (!gameStart) // Adicione esta condição
    {
        if (IsKeyPressed(KEY_ENTER))
        {
            gameStart = true;
        }
    }
    else if (gameOver == 0)
    {
        if (IsKeyPressed('P')) pause = !pause;

        if (!pause)
        {
            //Controles
            if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) && (formiga[0].velocidade.x == 0) && allowMove)
            {
                formiga[0].velocidade = (Vector2){ quadrado_tamanho, 0 };
                allowMove = false;
            }
            if ((IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) && (formiga[0].velocidade.x == 0) && allowMove)
            {
                formiga[0].velocidade = (Vector2){ -quadrado_tamanho, 0 };
                allowMove = false;
            }
            if ((IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) && (formiga[0].velocidade.y == 0) && allowMove)
            {
                formiga[0].velocidade = (Vector2){ 0, -quadrado_tamanho };
                allowMove = false;
            }
            if ((IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) && (formiga[0].velocidade.y == 0) && allowMove)
            {
                formiga[0].velocidade = (Vector2){ 0, quadrado_tamanho };
                allowMove = false;
            }

            // Atualizar a posição da cauda da cobra
            if (Tamanho > 0) {
                formiga[Tamanho].posicao = Posicao_Formiga[Tamanho - 1];
            }

            // Movimentação
            for (int i = 0; i < Tamanho; i++) Posicao_Formiga[i] = formiga[i].posicao;

            if ((framesCounter % 5) == 0)
            {
                for (int i = 0; i < Tamanho; i++)
                {
                    if (i == 0)
                    {
                        formiga[0].posicao.x += formiga[0].velocidade.x;
                        formiga[0].posicao.y += formiga[0].velocidade.y;
                        allowMove = true;
                    }
                    else formiga[i].posicao = Posicao_Formiga[i - 1];
                }
            }

            // Wall behaviour
            if (((formiga[0].posicao.x) > (largura - offset.x)) ||
                ((formiga[0].posicao.y) > (altura - offset.y)) ||
                (formiga[0].posicao.x < 0) || (formiga[0].posicao.y < 0))
            {
                gameOver = 1;
                AddLeaderboardEntry(jogador, pontos);
                SaveLeaderboard();
            }

            // Collision with yourself
            for (int i = 1; i < Tamanho; i++)
            {
                if ((formiga[0].posicao.x == formiga[i].posicao.x) && (formiga[0].posicao.y == formiga[i].posicao.y)) gameOver = 1;
            }

            // migalha posicao calculation
            if (!migalha.active)
            {
                migalha.active = true;
                bool validposicao = false;
                while (!validposicao) {
                    migalha.posicao = (Vector2){ GetRandomValue(0, (largura / quadrado_tamanho) - 1) * quadrado_tamanho + offset.x / 2, GetRandomValue(0, (altura / quadrado_tamanho) - 1) * quadrado_tamanho + offset.y / 2 };
                    validposicao = true;

                    // Verifica se a comida está na mesma posição que qualquer parte da cobra
                    for (int i = 0; i < Tamanho; i++) {
                        if (migalha.posicao.x == formiga[i].posicao.x && migalha.posicao.y == formiga[i].posicao.y) {
                            validposicao = false;
                            break;
                        }
                    }

                    // Verifica se a comida está na mesma posição que qualquer obstáculo
                    for (int i = 0; i < ContadorGotas; i++) {
                        if (migalha.posicao.x == gotas[i].posicao.x && migalha.posicao.y == gotas[i].posicao.y) {
                            validposicao = false;
                            break;
                        }
                    }
                }
            }

            // Collision
            if ((formiga[0].posicao.x < (migalha.posicao.x + migalha.tamanho.x) && (formiga[0].posicao.x + formiga[0].tamanho.x) > migalha.posicao.x) &&
                (formiga[0].posicao.y < (migalha.posicao.y + migalha.tamanho.y) && (formiga[0].posicao.y + formiga[0].tamanho.y) > migalha.posicao.y))
            {
                Tamanho += 1;
                migalha.active = false;
                contador_comida++; // Incrementa o contador de comidas
                pontos += 10; // Incrementa o pontos

                // Atualiza o título da janela com o novo tamanho da cobra
                char windowTitle[50];
                sprintf(windowTitle, "cobrita");
                SetWindowTitle(windowTitle);

                // Verifica se a cobra consumiu 3 comidas e se o jogo já se expandiu 15 vezes
                if (contador_comida == 3 && pontos <= 450)
                {
                    largura += 2 * quadrado_tamanho; // Aumenta a largura do mapa em 2 unidades
                    altura += 2 * quadrado_tamanho; // Aumenta a altura do mapa em 2 unidades
                    SetWindowSize(largura, altura); // Ajusta o tamanho da janela
                    contador_comida = 0; // Reseta o contador de comidas
                }
            }

            // Garante que sempre haja uma comida ativa no mapa
            if (!migalha.active)
            {
                migalha.active = true;
                bool validposicao = false;
                while (!validposicao) {
                    migalha.posicao = (Vector2){ GetRandomValue(0, (largura / quadrado_tamanho) - 1) * quadrado_tamanho + offset.x / 2, GetRandomValue(0, (altura / quadrado_tamanho) - 1) * quadrado_tamanho + offset.y / 2 };
                    validposicao = true;

                    // Verifica se a comida está na mesma posição que qualquer parte da cobra
                    for (int i = 0; i < Tamanho; i++) {
                        if (migalha.posicao.x == formiga[i].posicao.x && migalha.posicao.y == formiga[i].posicao.y) {
                            validposicao = false;
                            break;
                        }
                    }

                    // Verifica se a comida está na mesma posição que qualquer obstáculo
                    for (int i = 0; i < ContadorGotas; i++) {
                        if (migalha.posicao.x == gotas[i].posicao.x && migalha.posicao.y == gotas[i].posicao.y) {
                            validposicao = false;
                            break;
                        }
                    }
                }
            }

            Updategotas();
            CheckObstacleCollision();

            framesCounter++;
        }
    }
    else
    {
        if (IsKeyPressed(KEY_ENTER))
        {
            InitGame();
            gameOver = 0;
            gameStart = false;
        }
    }
}



void Updategotas(void)
{
    if (framesCounter % (60 * 2) == 0) {
        for (int i = 0; i < MAX_GOTAS; i++) {
            if (!gotas[i].active) {
                bool validposicao = false;
                while (!validposicao) {
                    gotas[i].posicao = (Vector2){
                        GetRandomValue(0, (largura / quadrado_tamanho) - 1) * quadrado_tamanho + offset.x / 2,
                        GetRandomValue(0, (altura / quadrado_tamanho) - 1) * quadrado_tamanho + offset.y / 2
                    };
                    validposicao = true;

                    // Verifica se o obstáculo está na mesma posição que a comida
                    if (gotas[i].posicao.x == migalha.posicao.x && gotas[i].posicao.y == migalha.posicao.y) {
                        validposicao = false;
                    }

                    // Verifica se o obstáculo está na mesma posição que qualquer parte da cobra
                    for (int j = 0; j < Tamanho; j++) {
                        if (gotas[i].posicao.x == formiga[j].posicao.x && gotas[i].posicao.y == formiga[j].posicao.y) {
                            validposicao = false;
                            break;
                        }
                    }
                }
                gotas[i].active = true;
                ContadorGotas++;
                break;
            }
        }
    }
}


void CheckObstacleCollision(void)
{
    for (int i = 0; i < ContadorGotas; i++) {
        if (gotas[i].active &&
            (formiga[0].posicao.x == gotas[i].posicao.x) &&
            (formiga[0].posicao.y == gotas[i].posicao.y)) {
            gameOver = 1;
            AddLeaderboardEntry(jogador, pontos);
            SaveLeaderboard();
        }
    }
}

void DrawGame(void)
{
    BeginDrawing();
    ClearBackground((Color) { 210, 180, 140, 200 });
    char atualizandoNome[MAX_nome_LENGTH];
    strcpy(atualizandoNome, jogador);
    //TELA EXTRA
    if (gameOver == 2) {
        DrawText("INSERIR NOME :", largura / 2 - MeasureText("INSERIR NOME :", 20) / 2, altura / 2 - 100, 20, BLACK);
        DrawText(atualizandoNome, largura / 2 - MeasureText(atualizandoNome, 20) / 2, altura / 2 - 60, 20, BLACK);
    }
    else if (!gameStart) // Adicione esta condição
    {
        DrawText("Aperte [ENTER] para iniciar", largura / 2 - MeasureText("Aperte [ENTER] para iniciar", 20) / 2, altura / 2, 20, BLACK);
    }
    else if (gameOver == 0)
    {
        // Desenhar formiga
        for (int i = 0; i < Tamanho; i++)
        {
            DrawRectangleV(formiga[i].posicao, formiga[i].tamanho, formiga[i].cor);
        }

        // Desenhar migalha
        if (migalha.active)
        {
            DrawRectangleV(migalha.posicao, migalha.tamanho, migalha.cor);
        }

        // Desenhar gotas
        for (int i = 0; i < MAX_GOTAS; i++)
        {
            if (gotas[i].active)
            {
                DrawRectangleV(gotas[i].posicao, gotas[i].tamanho, gotas[i].cor);
            }
        }

        DrawText(TextFormat("pontos: %04i", pontos), 10, 10, 20, DARKGRAY);
        if (pause) DrawText("GAME PAUSED", largura / 2 - MeasureText("GAME PAUSED", 40) / 2, altura / 2 - 40, 40, GRAY);
    }
    else
    {
        char strpontos[4];
        char buffer[12] = "Pontos: ";
        sprintf(strpontos, "%d", pontos);
        strcat(buffer, strpontos);
        DrawText("GAME OVER", largura / 2 - MeasureText("GAME OVER", 40) / 2, altura / 2 - 200, 40, BLACK);
        DrawText(buffer, largura / 2 - MeasureText(buffer, 30) / 2, altura / 2 - 150, 30, BLACK);
        DrawText("Aperte [ENTER] para jogar novamente", largura / 2 - MeasureText("Aperte [ENTER] para jogar novamente", 20) / 2, altura / 2 - 110, 20, GRAY);
        DrawLeaderboard(); // Exibe o leaderboard
    }



    EndDrawing();
}

void DrawLeaderboard(void)
{
    int y = altura / 2 - 60;
    DrawText("LEADERBOARD", largura / 2 - MeasureText("LEADERBOARD", 25) / 2, altura / 2 - 80, 20, DARKGRAY);
    LeaderboardEntry* current = leaderboard;
    while (current != NULL) {
        DrawText(TextFormat("%s - %d", current->nome, current->pontos), largura / 2 - 100, y, 20, DARKGRAY);
        y += 30;
        current = current->next;
    }
}

void SaveLeaderboard(void)
{
    FILE* file = fopen("leaderboard.txt", "w"); // Abre o arquivo no modo escrita (assegura que mesmo que o arquivo não exista, ele seja criado
    if (file == NULL) {
        printf("Erro ao abrir o arquivo para salvar o leaderboard.\n");
        return;
    }

    LeaderboardEntry* current = leaderboard;
    while (current != NULL) {
        fprintf(file, "%s %d\n", current->nome, current->pontos);
        current = current->next;
    }

    fclose(file);
}

void LoadLeaderboard(void)
{
    FILE* file = fopen("leaderboard.txt", "r"); // Abre o arquivo no modo leitura
    if (file == NULL) { // Verifica se o arquivo existe
        printf("Arquivo de leaderboard não encontrado. Criando um novo.\n");
        return;
    }

    char nome[MAX_nome_LENGTH];
    int pontos;

    while (fscanf(file, "%20s %d", nome, &pontos) == 2) {  // loop se mantém enquanto tiver tanto nome quanto pontos para serem lidos
        AddLeaderboardEntry(nome, pontos); // adicionar a entrada lida (nome e pontuação) ao leaderboard.
    }

    fclose(file); // Fecha o arquivo
}

void NomeJogador() {
    int key = GetKeyPressed();  // Tecla apertada
    for (int key = KEY_A; key <= KEY_Z; key++) {
        if (IsKeyPressed(key)) {  // Verifica se a tecla foi apertada
            char letter[2] = { (char)(key - KEY_A + 'a'), '\0' };  // Tecla -> Letra
            strcat(jogador, letter);  // Adiciona as letras ao nome do jogador
        }
    }
}
