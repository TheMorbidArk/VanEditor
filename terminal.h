#ifndef VANEDITOR_TERMINAL_H
#define VANEDITOR_TERMINAL_H

#include "VanEditor.h"

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

#endif //VANEDITOR_TERMINAL_H
