
#ifndef VANEDITOR_VANEDITOR_H
#define VANEDITOR_VANEDITOR_H

/*** includes ***/
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdarg.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)
#define VanEditor_VERSION "0.0.1"
#define KILO_TAB_STOP 4
#define KILO_QUIT_TIMES 3


/*** data ***/
typedef struct termios termios;

enum EditorKey {    //特殊按键Index
    BACKSPACE = 127,
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

typedef struct erow {
    //行文本数据
    int size;
    char *chars;

    //渲染文本数据
    int rsize;
    char *render;
} erow;

typedef struct EditorConfig {
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

    int dirty;

    //状态栏数据
    char statusmsg[80];
    time_t statusmsg_time;

    termios orig_termios;
} EditorConfig;

//全局变量
EditorConfig E;

//前置声明
void EditorSetStatusMessage(const char *fmt, ...);
void EditorRefreshScreen();
int EditorReadKey();
char *EditorPrompt(char *prompt);

#endif //VANEDITOR_VANEDITOR_H
