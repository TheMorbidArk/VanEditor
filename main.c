/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

typedef struct termios termios;

/*** define ***/

#define CTRL_KEY(k) ((k) & 0x1f)
#define VanEditor_VERSION "0.0.1"

enum EditorKey {    //特殊按键Index
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

/*** data ***/

typedef struct editorConfig {
    int cx, cy;  //光标位置

    //窗口大小
    int screen_rows;
    int screen_cols;

    termios orig_termios;
} editorConfig;

editorConfig E;

/*** terminal ***/

/**
 * 输出错误信息并结束进程
 * @param s:char* 错误信息
 */
void die(const char *s) {

    //异常退出前清屏
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

/**
 * 退出时恢复终端原模式
 */
void DisableRawMode() {
    //设置终端标志位，同时监听是否产生错误
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

/**
 * 启用编辑模式
 */
void EnableRawMode() {
    // 获取终端初试情况，同时监听是否产生错误
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        die("tcgetattr");
    atexit(DisableRawMode);

    // 终端对象
    termios raw;
    // 获取终端标志位
    tcgetattr(STDIN_FILENO, &raw);

    // 设置标志位
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // VMIN 值设置 read() 返回之前所需的最小输入字节数
    // 我们将其设置为 0，以便 read() 在有任何输入要读取时立即返回。
    // VTIME 值设置在 read() 返回之前等待的最长时间
    // 它以十分之一秒为单位，因此我们将其设置为 1/10 秒，即 100 毫秒
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    //设置终端标志位，同时监听是否产生错误
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

/**
 * 获取终端的大小
 * @param rows 行
 * @param cols 列
 * @return -1/0 获取失败返回-1，成功返回0
 */
int GetWindowSize(int *rows, int *cols) {
    struct winsize win_size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win_size) == -1 || win_size.ws_col == 0) {
        return -1;
    } else {
        *rows = win_size.ws_row;
        *cols = win_size.ws_col;
        return 0;
    }
}

/*** append buffer ***/

typedef struct abuf {
    char *b;
    int len;
} abuf;

#define ABUF_INIT {NULL, 0}

/**
 * 将字符串 s 附加到 abuf.b
 * @param ab:abuf* abuf对象
 * @param s:const char* 将附加到 abuf.b 的字符串
 * @param len:int 字符串 s 的长度
 */
void abAppend(abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);
    if (new == NULL) return;

    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

/**
 * 释放 abuf 使用的内存
 * @param ab
 */
void abFree(struct abuf *ab) {
    free(ab->b);
}

/*** input ***/

/**
 * 返回 input 字符，返回键盘按下的字符
 * @return ch:char 输入的字符
 */
int EditorReadKey() {
    int num_read;
    char ch;
    while ((num_read = read(STDIN_FILENO, &ch, 1) != 1)) {
        if (num_read == -1 && errno != EAGAIN)
            die("read");
    }
    if (ch == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1':
                            return HOME_KEY;
                        case '3':
                            return DEL_KEY;
                        case '4':
                            return END_KEY;
                        case '5':
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                        case '7':
                            return HOME_KEY;
                        case '8':
                            return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A':
                        return ARROW_UP;
                    case 'B':
                        return ARROW_DOWN;
                    case 'C':
                        return ARROW_RIGHT;
                    case 'D':
                        return ARROW_LEFT;
                    case 'H':
                        return HOME_KEY;
                    case 'F':
                        return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
            }
        }
        return '\x1b';
    } else {
        return ch;
    }
}

/**
 * 根据 key 的值移动光标位置
 * @param key:int -> enum EditorKey
 */
void EditorMoveCursor(int key) {
    switch (key) {
        case ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            }
            break;
        case ARROW_RIGHT:
            if (E.cx != E.screen_cols - 1) {
                E.cx++;
            }
            break;
        case ARROW_UP:
            if (E.cy != 0) {
                E.cy--;
            }
            break;
        case ARROW_DOWN:
            if (E.cy != E.screen_rows - 1) {
                E.cy++;
            }
            break;
    }
}

/**
 * 等待按键并处理
 * 检测关键Key是否被按下
 * 当按下 CTRL+Q 退出程序
 */
void EditorProcessKeys() {
    int c = EditorReadKey();
    switch (c) {
        //CTRL+q
        case CTRL_KEY('q'):
            //退出前清屏
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case HOME_KEY:
            E.cx = 0;
            break;
        case END_KEY:
            E.cx = E.screen_cols - 1;
            break;

        case PAGE_UP:
        case PAGE_DOWN: {
            int times = E.screen_rows;
            while (times--)
                EditorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
        }
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            EditorMoveCursor(c);
            break;
    }
}

/*** output ***/

/**
 * 绘制 ~ 和版本信息
 */
void EditorDrawRows(abuf *ab) {
    for (int y = 0; y < E.screen_rows; y++) {
        if (y == E.screen_rows / 3) {
            char welcome[80];
            int welcomelen = snprintf(welcome, sizeof(welcome),
                                      "Van Editor -- version %s", VanEditor_VERSION);
            if (welcomelen > E.screen_cols) welcomelen = E.screen_cols;
            int padding = (E.screen_cols - welcomelen) / 2;
            if (padding) {
                abAppend(ab, "~", 1);
                padding--;
            }
            while (padding--) abAppend(ab, " ", 1);
            abAppend(ab, welcome, welcomelen);
        } else {
            abAppend(ab, "~", 1);
        }
        abAppend(ab, "\x1b[K", 3);
        if (y < E.screen_rows - 1) {
            abAppend(ab, "\r\n", 2);
        }
    }
}

/**
 * 清除屏幕,并将光标移动到屏幕左上角
 */
void EditorRefreshScreen() {
    abuf ab = ABUF_INIT;
    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);
    EditorDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/*** init ***/

void InitEditor() {
    E.cx = 0;
    E.cy = 0;
    if (GetWindowSize(&E.screen_rows, &E.screen_cols) == -1) die("getWindowSize");
}

int main() {
    EnableRawMode();
    InitEditor();

    while (1) {
        EditorRefreshScreen();
        EditorProcessKeys();
    }
    return 0;
}