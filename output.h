
#ifndef VANEDITOR_OUTPUT_H
#define VANEDITOR_OUTPUT_H

#include "VanEditor.h"
#include "append_buffer.h"
#include "row_operations.h"

/*** output ***/
/* 处理输出相关事件 */

/** Editor **/

/**
 * 绘制 ~ / 版本信息 / 文本数据
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

/** Status Bar **/

void EditorDrawStatusBar(struct abuf *ab) {
    abAppend(ab, "\x1b[7m", 4);

    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                       E.file_name ? E.file_name : "[No Name]", E.num_rows,
                       E.dirty ? "(modify)" : "");
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
    abAppend(ab, E.statusmsg, msglen);
}

void EditorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

#endif //VANEDITOR_OUTPUT_H
