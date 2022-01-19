#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <GL/glut.h>
#include <math.h>

/* --- 便利な関数 ------------------------------ */
// n1進数からn2進数に変換
float nAryConvertor(float src, float n1, float n2)
{
    return src * (n2 / n1);
}
/* --------------------------------------------- */

/* --- コンソール ------------------------------ */
// コンソールのバッファーの数
#define CONSOLE_BUF_NUM 3
#define CONSOLE_STR_LEN_MAX 40
// コンソールバッファーの配列
char console_bufs[CONSOLE_BUF_NUM][CONSOLE_STR_LEN_MAX];

// コンソールバッファーにメッセージを設定
bool setConsoleBuffer(int idx, char *data)
{
    // 格納先の行が存在しない場合
    if (idx < 0 || idx > CONSOLE_BUF_NUM - 1)
        return false;
    // メッセージが長すぎる場合
    if (strlen(data) > CONSOLE_STR_LEN_MAX)
        return false;

    strcpy(console_bufs[idx], data);
    glutPostRedisplay();

    return true;
}
/* --------------------------------------------- */

/* --- 駒 -------------------------------------- */
// 駒のない状態
#define EMPTY 'e'
// 黒
#define BLACK 'b'
// 白
#define WHITE 'w'
// 黒のターンか白のターンか
char now_piece;
// 相手の駒を取得
char getEnemyPiece(char my_piece)
{
    return (my_piece == BLACK) ? WHITE : BLACK;
}
/* --------------------------------------------- */

/* --- ターン ---------------------------------- */
// 現在のターン
int turn_count = 0;
// 最高到達ターン
int max_reach_turn_count = 0;
/* --------------------------------------------- */

/* --- 盤面 ------------------------------------ */
// 盤面のマス数
#define SQ_NUM 8
// 盤面
char board[SQ_NUM + 2][SQ_NUM + 2];
// 盤面上の位置を示す
typedef struct
{
    int row;
    int col;
} SqVec2;

// 盤面の記録を保持しておく変数
char board_history[SQ_NUM * SQ_NUM + 1][SQ_NUM][SQ_NUM];
// 駒を置いた位置の記録を保持しておく変数
SqVec2 place_history[SQ_NUM * SQ_NUM];

// 盤面を初期化
void initBoard(char board_arg[][SQ_NUM + 2])
{
    // 全て駒のない状態にする
    for (int row = 0; row < SQ_NUM + 2; row++)
        for (int col = 0; col < SQ_NUM + 2; col++)
            board_arg[row][col] = EMPTY;

    // 初期配置になるように駒を配置
    board_arg[SQ_NUM / 2][SQ_NUM / 2] = BLACK;
    board_arg[SQ_NUM / 2][SQ_NUM / 2 + 1] = WHITE;
    board_arg[SQ_NUM / 2 + 1][SQ_NUM / 2] = WHITE;
    board_arg[SQ_NUM / 2 + 1][SQ_NUM / 2 + 1] = BLACK;

    // 初期盤面を記録
    for (int row = 0; row < SQ_NUM; row++)
        for (int col = 0; col < SQ_NUM; col++)
            board_history[0][row][col] = board_arg[row + 1][col + 1];
}
// 盤面をコピー
void copyBoard(char board_result[][SQ_NUM + 2], char board_src[][SQ_NUM + 2])
{
    for (int row = 0; row < SQ_NUM + 2; row++)
        for (int col = 0; col < SQ_NUM + 2; col++)
            board_result[row][col] = board_src[row][col];
}
// 盤面と駒を置いた位置を記録
void recordHistory(SqVec2 place_pos)
{
    for (int row = 0; row < SQ_NUM; row++)
    {
        for (int col = 0; col < SQ_NUM; col++)
        {
            board_history[turn_count + 1][row][col] = board[row + 1][col + 1];
            place_history[turn_count] = place_pos;
        }
    }
}
/* --------------------------------------------- */

/* --- 駒を置く処理, 裏返し処理 ---------------- */
// 特定のマスで特定の方向に裏返せる駒の数を取得(裏返す場合は裏返し処理も行う)
int getReverseNumOneWay(char board_arg[][SQ_NUM + 2], SqVec2 sq_pos, char my_piece, SqVec2 way, bool do_reverse)
{
    // すでに駒がある場合
    if (board_arg[sq_pos.row + 1][sq_pos.col + 1] != EMPTY)
        return 0;

    char enemy_piece = getEnemyPiece(my_piece);

    // 特定方向の駒が相手の駒でないとき
    if (board_arg[sq_pos.row + 1 + way.row][sq_pos.col + 1 + way.col] != enemy_piece)
        return 0;

    // 裏返せる個数を数える
    int reverse_num = 1;
    while (true)
    {
        // 駒が置かれていないとき
        if (board_arg[(sq_pos.row + 1) + way.row * (reverse_num + 1)][(sq_pos.col + 1) + way.col * (reverse_num + 1)] == EMPTY)
        {
            reverse_num = 0;
            break;
        }

        // 自分の駒のとき
        if (board_arg[(sq_pos.row + 1) + way.row * (reverse_num + 1)][(sq_pos.col + 1) + way.col * (reverse_num + 1)] == my_piece)
            break;

        // 相手の駒のとき
        reverse_num++;
    }

    // 裏返し処理
    if (reverse_num > 0 && do_reverse)
    {
        // 裏返し処理
        for (int i = 1; i <= reverse_num; i++)
            board_arg[sq_pos.row + 1 + way.row * i][sq_pos.col + 1 + way.col * i] = my_piece;
    }

    return reverse_num;
}
// 特定のマスで裏返せる方向数を取得
int placePiece(char board_arg[][SQ_NUM + 2], SqVec2 sq_pos, char my_piece, bool do_place)
{
    // すでに駒がある場合
    if (board_arg[sq_pos.row + 1][sq_pos.col + 1] != EMPTY)
        return 0;

    int reverse_way_num = 0;

    // 裏返せる方向数を調べる
    SqVec2 way;
    for (way.row = -1; way.row <= 1; way.row++)
    {
        for (way.col = -1; way.col <= 1; way.col++)
        {
            if (way.col == 0 && way.row == 0)
                continue;
            if (getReverseNumOneWay(board_arg, sq_pos, my_piece, way, do_place) != 0)
                reverse_way_num++;
        }
    }

    // 駒を置く処理
    if (reverse_way_num > 0 && do_place)
        board_arg[sq_pos.row + 1][sq_pos.col + 1] = now_piece;

    return reverse_way_num;
}
// 盤面上で駒を置けるマスの数を取得
int getPlaceSqNum(char board_arg[][SQ_NUM + 2], char my_piece)
{
    int place_sq_num = 0;
    SqVec2 sq_pos;

    // 駒を置ける(=裏返せる)マスの数を調べる
    for (sq_pos.row = 0; sq_pos.row < SQ_NUM; sq_pos.row++)
        for (sq_pos.col = 0; sq_pos.col < SQ_NUM; sq_pos.col++)
            if (placePiece(board_arg, sq_pos, my_piece, false) > 0)
                place_sq_num++;

    return place_sq_num;
}
/* --------------------------------------------- */

/* --- 駒を置けるマスを記録 -------------------- */
// 駒を置けるマスの一覧を保持
SqVec2 place_sqs[SQ_NUM * SQ_NUM];
// 駒を置けるマスの数
int place_sq_num;
// 盤面上で駒を置けるマスを記録
void recordPlaceSq(char my_piece)
{
    place_sq_num = 0;
    SqVec2 sq_pos;

    for (sq_pos.row = 0; sq_pos.row < SQ_NUM; sq_pos.row++)
    {
        for (sq_pos.col = 0; sq_pos.col < SQ_NUM; sq_pos.col++)
        {
            if (placePiece(board, sq_pos, my_piece, false) > 0)
            {
                place_sqs[place_sq_num] = sq_pos;
                place_sq_num++;
            }
        }
    }
}
/* --------------------------------------------- */

/* --- 人間側の駒を置く処理 -------------------- */
// 候補のインデックス
int now_cand_idx;
// 候補のマスの変更
void setNowCandIdx(bool is_next)
{
    now_cand_idx = (is_next) ? (now_cand_idx + 1) % place_sq_num : (now_cand_idx + place_sq_num - 1) % place_sq_num;
}
// 盤面上に駒を置く（人間）
void placePieceByHuman()
{
    SqVec2 place_pos = place_sqs[now_cand_idx];

    placePiece(board, place_pos, now_piece, true);
    recordHistory(place_pos);
}
/* --------------------------------------------- */

/* --- コンピューター側の駒を置く処理 ---------- */
// 評価値を取得
int eval(char board_arg[][SQ_NUM + 2], char my_piece)
{
    // 自分側の評価値
    int my_val = 0;
    // 相手側の評価値
    int enemy_val = 0;

    // 評価値表
    int val_table[SQ_NUM][SQ_NUM] = {
        {120, -20, 20, 5, 5, 20, -20, 120},
        {-20, -40, -5, -5, -5, -5, -40, -20},
        {20, -5, 15, 3, 3, 15, -5, 20},
        {5, -5, 3, 3, 3, 3, -5, 5},
        {5, -5, 3, 3, 3, 3, -5, 5},
        {20, -5, 15, 3, 3, 15, -5, 20},
        {-20, -40, -5, -5, -5, -5, -40, -20},
        {120, -20, 20, 5, 5, 20, -20, 120}};

    for (int row = 0; row < SQ_NUM; row++)
    {
        for (int col = 0; col < SQ_NUM; col++)
        {
            char sq_piece = board_arg[row + 1][col + 1];

            if (sq_piece == my_piece)
                my_val += val_table[row][col];
            else if (sq_piece == getEnemyPiece(my_piece))
                enemy_val += val_table[row][col];
        }
    }

    return my_val - enemy_val;
}
// 最善の置き場所(=自分が置いたときに相手が最も不利になる場所)のインデックスを取得
int getGreatestCandIdx()
{
    int greatest_pos_idx = -1;
    int min_next_val = 100000;

    for (int idx = 0; idx < place_sq_num; idx++)
    {
        char board_tmp[SQ_NUM + 2][SQ_NUM + 2];
        copyBoard(board_tmp, board);

        SqVec2 place_pos = place_sqs[idx];
        placePiece(board_tmp, place_pos, now_piece, true);

        int next_val = eval(board_tmp, getEnemyPiece(now_piece));
        if (next_val < min_next_val)
        {
            greatest_pos_idx = idx;
            min_next_val = next_val;
        }
    }

    return greatest_pos_idx;
}
// 盤面上に駒を置く（コンピューター）
void placePieceByCom()
{
    int greatest_pos_idx = getGreatestCandIdx();
    if (greatest_pos_idx == -1)
        return;

    SqVec2 place_pos = place_sqs[greatest_pos_idx];

    placePiece(board, place_pos, now_piece, true);
    recordHistory(place_pos);
}
/* --------------------------------------------- */

/* --- ゲームステータス ------------------------ */
// 準備中
#define PREPARING 'p'
// ゲーム中
#define GAMING 'g'
// ゲーム終了
#define FINISHED 'f'
// ゲームステータスを保持
char game_status = PREPARING;
/* --------------------------------------------- */

/* --- プレイヤー ------------------------------ */
// 人間
#define HUMAN 'h'
// コンピューター
#define COM 'c'
// 先攻のプレイヤー
#define FIRST_ATTACKER COM
// 後攻のプレイヤー
#define SECOND_ATTACKER HUMAN
// プレイヤーを取得
char getPlayer(char my_piece)
{
    return (my_piece == BLACK) ? FIRST_ATTACKER : SECOND_ATTACKER;
}
/* --------------------------------------------- */

/* --- ゲーム結果の管理 ------------------------------ */
// 結果の種類
// 黒の勝利
#define BLACK_WIN 'b'
// 黒の勝利
#define WHITE_WIN 'w'
// 引き分け
#define DRAW 'd'
// 盤面上の特定の駒の数を取得
int getPieceNum(char my_piece)
{
    int piece_num = 0;

    for (int row = 1; row < SQ_NUM + 1; row++)
    {
        for (int col = 1; col < SQ_NUM + 1; col++)
        {
            if (board[row][col] == my_piece)
                piece_num++;
        }
    }

    return piece_num;
}
// 勝敗を取得する
char getWinLose()
{
    int black_piece_num = getPieceNum(BLACK);
    int white_piece_num = getPieceNum(WHITE);

    if (black_piece_num == white_piece_num)
        return DRAW;
    if (black_piece_num > white_piece_num)
        return BLACK_WIN;
    return WHITE_WIN;
}
/* --------------------------------------------- */

/* --- ゲームのメイン処理で利用する関数 -------- */
// ターンの変更時の描画
void redrawNextTurn()
{
    // 次のターンが誰かを表示する
    char console_text[CONSOLE_STR_LEN_MAX] = "";
    if (now_piece == BLACK)
        sprintf(console_text, "Next turn(%d / %d) is BLACK.", turn_count + 1, SQ_NUM * SQ_NUM);
    else if (now_piece == WHITE)
        sprintf(console_text, "Next turn(%d / %d) is WHITE.", turn_count + 1, SQ_NUM * SQ_NUM);
    setConsoleBuffer(0, console_text);
}
/* --------------------------------------------- */

/* --- ゲームのメイン処理 ---------------------- */
// ゲームを初期化
void initGame()
{
    // 駒を先攻の駒の種類に初期化
    now_piece = BLACK;

    // 盤面を初期化
    initBoard(board);

    // コンソールのメッセージを初期化
    char console_text[CONSOLE_STR_LEN_MAX] = "";
    sprintf(console_text, "Next turn(%d / %d) is BLACK.", turn_count + 1, SQ_NUM * SQ_NUM);
    setConsoleBuffer(0, console_text);

    // 駒を置けるマスの一覧を初期化
    recordPlaceSq(now_piece);

    // 先攻が人間の場合は駒を置けるマスの候補のインデックスを初期化
    if (getPlayer(now_piece) == HUMAN) 
        now_cand_idx = 0;

    // ゲームを開始させる
    game_status = GAMING;
}

// ゲームステータスを考慮して駒を置く
void placePieceProc()
{
    // ゲーム中でない場合
    if (game_status != GAMING)
        return;

    // 駒を置く処理
    if (getPlayer(now_piece) == HUMAN)
        placePieceByHuman();
    else
        placePieceByCom();

    // 駒を置いたことを画面に反映
    glutPostRedisplay();
}
// 次のターンに進められるかを返す
bool canChangeTurn()
{
    // 相手のターンにする
    now_piece = getEnemyPiece(now_piece);
    // 駒を置けるか調べるために, 駒を置けるマスの一覧を更新
    recordPlaceSq(now_piece);

    // パスでない場合
    if (place_sq_num > 0) {
        setConsoleBuffer(1, "");
    }
    // パスの場合（1回連続）
    else
    {
        // 相手のターンにする（2回目）
        now_piece = getEnemyPiece(now_piece);
        // 駒を置けるか調べるために, 駒を置けるマスの一覧を更新
        recordPlaceSq(now_piece);

        // パスでない場合
        if (place_sq_num > 0)
        {
            // パスしたことをコンソールに表示
            if (now_piece == BLACK) 
                setConsoleBuffer(1, "WHITE Passed.");
            else if (now_piece == WHITE) 
                setConsoleBuffer(1, "BLACK Passed.");
        }
        // パスの場合（2回連続）→ 関数の呼び出し元でゲーム終了処理に移る
        else
        {
            setConsoleBuffer(1, "");
            return false;
        }
    }

    return true;
}
// 次のターンに進める
void nextTurnProc()
{
    // 駒を置けるマスの候補のインデックスを初期化
    now_cand_idx = 0;

    // ターンを1増やす
    turn_count++;
    // 最大到達ターンを1増やす
    max_reach_turn_count = turn_count;

    // 次のターンの描画処理
    redrawNextTurn();
}
// ゲームを終了
void finishProc()
{
    // ゲームを終了させる
    game_status = FINISHED;

    // コンソールの更新
    // ゲームが終了したことを表示
    setConsoleBuffer(0, "Finish Game!");
    char output[CONSOLE_STR_LEN_MAX];
    // 盤面上の黒, 白の駒の数を表示
    sprintf(output, "BLACK:%2d, WHITE:%2d", getPieceNum(BLACK), getPieceNum(WHITE));
    setConsoleBuffer(1, output);
    // ゲーム結果を表示
    char win_lose = getWinLose();
    if (win_lose == BLACK_WIN)
        setConsoleBuffer(2, "BLACK Win.");
    else if (win_lose == WHITE_WIN)
        setConsoleBuffer(2, "WHITE Win.");
    else if (win_lose == DRAW)
        setConsoleBuffer(2, "Draw");
}

// イベントハンドラーで呼ばれる関数
// 処理中かどうか
bool is_processing = false;
// 1ターンの処理を行う
void turnProc()
{
    is_processing = true;

    // ゲームステータスを考慮して駒を置く
    placePieceProc();

    // 次のターンに進めるかゲームを終了する
    bool is_next = canChangeTurn();
    if (is_next)
        nextTurnProc();
    else
        finishProc();
    
    is_processing = false;
}
// ターンを手動で変更
void moveTurn(bool is_go_back)
{
    is_processing = true;

    // ターンを戻したい場合
    if (is_go_back)
    {
        // ターンを戻せない場合
        if (turn_count - 1 < 0) {
            is_processing = false;
            return;
        }
        if (turn_count == 1 && getPlayer(getEnemyPiece(now_piece)) == COM) {
            is_processing = false;
            return;
        }

        // ターンを戻す
        turn_count--;
    }
    // ターンを進めたい場合
    else
    {
        // ターンを進められない場合
        if (turn_count + 1 > max_reach_turn_count) {
            is_processing = false;
            return;
        }

        // ターンを進める
        turn_count++;
    }

    // ターンを変更したことで発生する処理
    // 相手のターンにする
    now_piece = getEnemyPiece(now_piece);

    // 盤面を変更したターンの盤面に更新する
    for (int row = 0; row < SQ_NUM; row++)
        for (int col = 0; col < SQ_NUM; col++)
            board[row + 1][col + 1] = board_history[turn_count][row][col];

    // 変更後のターンがコンピューターのターンだった場合はもう一度同様の処理を行う
    if (getPlayer(now_piece) == COM)
        moveTurn(is_go_back);
    
    // 駒を置けるマスの一覧を更新
    recordPlaceSq(now_piece);
    // 駒を置けるマスの候補のインデックスを初期化
    now_cand_idx = 0;
    
    // 変更後のターンの描画処理
    redrawNextTurn();

    is_processing = false;
}
/* ---------------------------------------- */

/* --- ゲームのイベントハンドラー ------- */
// 候補のマスを変更するイベントハンドラー
void setCandHandler(int key)
{
    if (getPlayer(now_piece) == HUMAN)
    {
        switch (key)
        {
        case GLUT_KEY_UP:
            setNowCandIdx(false);
            glutPostRedisplay();
            break;
        case GLUT_KEY_DOWN:
            setNowCandIdx(true);
            glutPostRedisplay();
            break;
        }
    }
}
// 人間のターンのイベントハンドラー
void humanTurnHandler(int key)
{
    switch (key)
    {
    case 13: // Enter
        if (getPlayer(now_piece) == HUMAN && !is_processing)
            turnProc();
        break;
    }
}
// コンピューターのターンのイベントハンドラー
void comTurnHandler()
{
    if (getPlayer(now_piece) == COM && !is_processing)
        turnProc();
}
// ターンを手動で変更するイベントハンドラー
void moveTurnHandler(int key)
{
    if (!is_processing)
    {
        switch (key)
        {
        case GLUT_KEY_LEFT:
            moveTurn(true);
            break;
        case GLUT_KEY_RIGHT:
            moveTurn(false);
            break;
        }
    }
}
/* ---------------------------------------- */

/* --- 基本的な描画 ----------------------- */
// 線を描画
void drawLine(int begin_x, int begin_y, int end_x, int end_y)
{
    glBegin(GL_LINES);
    glVertex2i(begin_x, begin_y);
    glVertex2i(end_x, end_y);
    glEnd();
}
// 塗りつぶされた長方形を描画
void drawFillRect(int left, int top, int right, int bottom)
{
    glBegin(GL_QUADS);
    glVertex2i(left, top);
    glVertex2i(right, top);
    glVertex2i(right, bottom);
    glVertex2i(left, bottom);
    glEnd();
}
// 枠のみの長方形を描画
void drawStrokeRect(int left, int top, int right, int bottom, int line_width)
{
    int line_half_width = line_width / 2;
    for (int i = -line_half_width; i <= line_half_width; i++)
    {
        drawLine(left + i, top - line_half_width, left + i, bottom + line_half_width);
        drawLine(right + i, top - line_half_width, right + i, bottom + line_half_width);
        drawLine(left - line_half_width, top + i, right + line_half_width, top + i);
        drawLine(left - line_half_width, bottom + i, right + line_half_width, bottom + i);
    }
}
// 塗りつぶされた円を描画
void drawCircle(int center_x, int center_y, float round)
{
    int edge_num = 360;
    float angle;

    glBegin(GL_POLYGON);

    for (int i = 0; i < edge_num; i++)
    {
        angle = nAryConvertor(i, edge_num, M_PI * 2);
        glVertex2i(
            center_x + (int)(round * sin(angle)),
            center_y - (int)(round * cos(angle)));
    }

    glEnd();
}
// 文字列を描画
void drawText(char *text, int x, int y)
{
    // 描画位置に移動
    glRasterPos2i(x, y);
    for (int i = 0; i < strlen(text); i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
}
/* ---------------------------------------- */

/* --- アプリ内での描画 ------------------------------- */
// マスの描画サイズ
#define SQ_SIZE 40
// 盤面の描画サイズ
#define BOARD_SIZE SQ_SIZE *SQ_NUM
// コンソールの1行あたりの高さ
#define CONSOLE_BUF_HEIGHT 28
// コンソールの高さ
#define CONSOLE_HEIGHT CONSOLE_BUF_HEIGHT *CONSOLE_BUF_NUM

// 描画処理
void draw()
{
    // 背景を白色でクリア
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(1.0, 1.0, 1.0, 1.0);

    // コンソールを描画
    // コンソールの背景を描画
    glColor3ub(0, 0, 0);
    drawFillRect(0, BOARD_SIZE,
                 BOARD_SIZE, BOARD_SIZE + CONSOLE_HEIGHT);
    // コンソールの文字を描画
    glColor3ub(255, 255, 255);
    for (int row = 0; row < CONSOLE_BUF_NUM; row++)
        drawText(console_bufs[row], 5, BOARD_SIZE + (int)(CONSOLE_BUF_HEIGHT * ((float)row + 0.5)) + 5);

    // 盤面の背景を描画
    glColor3ub(71, 255, 71);
    for (int row = 1; row < SQ_NUM + 1; row++)
        for (int col = 1; col < SQ_NUM + 1; col++)
            drawFillRect(SQ_SIZE * (col - 1), SQ_SIZE * (row - 1), SQ_SIZE * col, SQ_SIZE * row);

    // 候補の背景を描画（駒を置くプレイヤーが人間の場合）
    if (getPlayer(now_piece) == HUMAN)
    {
        for (int i = 0; i < place_sq_num; i++)
        {
            glColor3ub(238, 250, 185);
            drawFillRect(SQ_SIZE * place_sqs[i].col, SQ_SIZE * place_sqs[i].row,
                         SQ_SIZE * (place_sqs[i].col + 1), SQ_SIZE * (place_sqs[i].row + 1));
            glColor3ub(255, 208, 0);

            if (i == now_cand_idx)
                drawStrokeRect(SQ_SIZE * place_sqs[i].col + 2, SQ_SIZE * place_sqs[i].row + 2,
                               SQ_SIZE * (place_sqs[i].col + 1) - 2, SQ_SIZE * (place_sqs[i].row + 1) - 2, 3);
        }
    }

    // 前に置いたマスを描画（1番目のターンでない場合）
    if (turn_count >= 1)
    {
        glColor3ub(5, 197, 255);
        drawStrokeRect(SQ_SIZE * place_history[turn_count - 1].col + 2, SQ_SIZE * place_history[turn_count - 1].row + 2,
                       SQ_SIZE * (place_history[turn_count - 1].col + 1) - 2, SQ_SIZE * (place_history[turn_count - 1].row + 1) - 2, 3);
    }

    // マスの枠線を描画
    glColor3ub(0, 0, 0);
    for (int row = 0; row <= SQ_NUM; row++)
        drawLine(0, SQ_SIZE * row, BOARD_SIZE, SQ_SIZE * row);
    for (int col = 0; col <= SQ_NUM; col++)
        drawLine(SQ_SIZE * col, 0, SQ_SIZE * col, BOARD_SIZE);

    // 駒を描画
    for (int row = 1; row < SQ_NUM + 1; row++)
    {
        for (int col = 1; col < SQ_NUM + 1; col++)
        {
            switch (board[row][col])
            {
            case EMPTY:
                continue;
            case BLACK:
                glColor3ub(0, 0, 0);
                break;
            case WHITE:
                glColor3ub(255, 255, 255);
                break;
            default:
                continue;
            }

            drawCircle((int)(SQ_SIZE * ((float)col - 0.5)), (int)(SQ_SIZE * ((float)row - 0.5)), SQ_SIZE * 0.4);
        }
    }

    glFlush();
    // ダブルバッファリング
    glutSwapBuffers();
}
/* ---------------------------------------- */

/* --- GLUTのコールバック関数 ------------- */
void display()
{
    draw();
}
void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glScaled(1, -1, 1);
    glTranslated(0, -h, 0);
}
void keyboard(unsigned char key, int x, int y)
{
    humanTurnHandler(key);
}
void specialKey(int key, int x, int y)
{
    setCandHandler(key);
    moveTurnHandler(key);
}
void comTurnTimer(int value)
{
    comTurnHandler();
    glutTimerFunc(50, comTurnTimer, 0);
}
/* ---------------------------------------- */

/* --- メイン関数 ------------------------- */
int main(int argc, char *argv[])
{
    // GLUTの初期化
    glutInit(&argc, argv);

    // ウィンドウの初期化
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(SQ_SIZE * SQ_NUM, SQ_SIZE * SQ_NUM + CONSOLE_BUF_HEIGHT * CONSOLE_BUF_NUM);
    glutCreateWindow("Reversi");

    // glutMainLoop関数を実行する際に呼ばれるコールバック関数
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKey);
    glutTimerFunc(50, comTurnTimer, 0);

    // ゲームを初期化
    initGame();

    // アプリのメインループ
    glutMainLoop();

    return 0;
}
/* ---------------------------------------- */