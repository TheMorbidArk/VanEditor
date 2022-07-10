/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdarg.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

typedef struct termios termios;

/*** define ***/

#define CTRL_KEY(k) ((k) & 0x1f)
#define VanEditor_VERSION "0.0.1"
#define KILO_TAB_STOP 4

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

typedef struct erow {
    //行文本数据
    int size;
    char *chars;

    //渲染文本数据
    int rsize;
    char *render;
} erow;

typedef struct editorConfig {
    int cx, cy;  //光标位置
    int rx;
    int rowoff;
    int coloff;

    //窗口大小
    int screen_rows;
    int screen_cols;

    //文本数据
    int num_rows;    //行数
    erow *row;       //文本信息
    char *file_name;  //文件名

    //状态栏数据
    char statusmsg[80];
    time_t statusmsg_time;

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

/*** row operations ***/

int EditorRowCxToRx(erow *row, int cx) {
    int rx = 0;
    for (int j = 0; j < cx; j++) {
        if (row->chars[j] == '\t')
            rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
        rx++;
    }
    return rx;
}

void EditorUpdateRow(erow *row) {//渲染字符
    int tab_nums = 0;
    for (int j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tab_nums++;
    free(row->render);
    row->render = malloc(row->size + tab_nums * (KILO_TAB_STOP - 1) + 1);
    int index = 0;
    for (int j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[index++] = ' ';
            while (index % KILO_TAB_STOP != 0) row->render[index++] = ' ';
        } else {
            row->render[index++] = row->chars[j];
        }
    }
    row->render[index] = '\0';
    row->rsize = index;
}

void EditorAppendRow(char *s, int len) {//读取字符,写入E.row
    E.row = realloc(E.row, sizeof(erow) * (E.num_rows + 1));
    int at = E.num_rows;
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    EditorUpdateRow(&E.row[at]);
    E.num_rows++;
}

/*** file i/o ***/

void EditorOpen(char *filename) {//读取文件内容
    free(E.file_name);
    E.file_name = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        EditorAppendRow(line, linelen);
    }
    free(line);
    fclose(fp);
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
    erow *row = (E.cy >= E.num_rows) ? NULL : &E.row[E.cy];

    switch (key) {
        case ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->size) {
                E.cx++;
            } else if (row && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;
        case ARROW_UP:
            if (E.cy != 0) {
                E.cy--;
            }
            break;
        case ARROW_DOWN:
            if (E.cy < E.num_rows) {
                E.cy++;
            }
            break;
    }

    row = (E.cy >= E.num_rows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
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
            if (E.cy < E.num_rows)
                E.cx = E.row[E.cy].size;
            break;

        case PAGE_UP:
        case PAGE_DOWN: {
            if (c == PAGE_UP) {
                E.cy = E.rowoff;
            } else if (c == PAGE_DOWN) {
                E.cy = E.rowoff + E.screen_rows - 1;
                if (E.cy > E.num_rows) E.cy = E.num_rows;
            }

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
        int filerow = E.rowoff + y;
        if (filerow >= E.num_rows) {
            if (E.num_rows == 0 && y == E.screen_rows / 3) {
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
        } else {
            int len = E.row[filerow].rsize - E.coloff;
            if (len < 0) len = 0;
            if (len > E.screen_cols) len = E.screen_cols;

            abAppend(ab, &E.row[filerow].render[E.coloff], len);
        }
        abAppend(ab, "\x1b[K", 3);

        abAppend(ab, "\r\n", 2);
    }
}

/**
 * 检查光标是否移出可见窗口
 * 如果是，调整 E.rowoff 使光标刚好在可见窗口内
 */
void EditorScroll() {
    E.rx = 0;
    if (E.cy < E.num_rows) {
        E.rx = EditorRowCxToRx(&E.row[E.cy], E.cx);
    }

    if (E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if (E.cy >= E.rowoff + E.screen_rows) {
        E.rowoff = E.cy - E.screen_rows + 1;
    }
    if (E.rx < E.coloff) {
        E.coloff = E.rx;
    }
    if (E.rx >= E.coloff + E.screen_cols) {
        E.coloff = E.rx - E.screen_cols + 1;
    }
}

void EditorDrawStatusBar(struct abuf *ab) {
    abAppend(ab, "\x1b[7m", 4);

    char status[80],rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines",
                       E.file_name ? E.file_name : "[No Name]", E.num_rows);
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
                        E.cy + 1, E.num_rows);
    if (len > E.screen_cols)
        len = E.screen_cols;
    abAppend(ab, status, len);

    while (len < E.screen_cols) {
        if (E.screen_cols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void EditorDrawMessageBar(struct abuf *ab) {
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(E.statusmsg);
    if (msglen > E.screen_cols) msglen = E.screen_cols;
    //if (msglen && time(NULL) - E.statusmsg_time < 5)
        abAppend(ab, E.statusmsg, msglen);
}

void EditorSetStatusMessage(const char *fmt, ...) {
    snprintf(E.statusmsg, sizeof(E.statusmsg), fmt);
}

/**
 * 清除屏幕,并将光标移动到屏幕左上角
 */
void EditorRefreshScreen() {
    EditorScroll();

    abuf ab = ABUF_INIT;
    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    EditorDrawRows(&ab);
    EditorDrawStatusBar(&ab);
    EditorDrawMessageBar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/*** init ***/

void InitEditor() {
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.num_rows = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.row = NULL;
    E.file_name = NULL;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;
    if (GetWindowSize(&E.screen_rows, &E.screen_cols) == -1)
        die("getWindowSize");
    E.screen_rows -= 2;
}

int main(int argc, char *argv[]) {
    EnableRawMode();
    InitEditor();
    if (argc >= 2) {
        EditorOpen(argv[1]);
    }

    EditorSetStatusMessage("HELP: Ctrl-Q = quit");

    while (1) {
        EditorRefreshScreen();
        EditorProcessKeys();
    }
    return 0;
}