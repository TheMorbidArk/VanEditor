
#ifndef VANEDITOR_ROW_OPERATIONS_H
#define VANEDITOR_ROW_OPERATIONS_H

#include "VanEditor.h"

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

void EditorInsertRow(int at, char *s, size_t len) {
    if (at < 0 || at > E.num_rows) return;
    E.row = realloc(E.row, sizeof(erow) * (E.num_rows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.num_rows - at));
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    EditorUpdateRow(&E.row[at]);
    E.num_rows++;
    E.dirty++;
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
    E.dirty++;
}

void EditorRowInsertChar(erow *row, int at, int c) {
    if (at < 0 || at > row->size)
        at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    EditorUpdateRow(row);
    E.dirty++;
}

void EditorRowDelChar(erow *row, int at) {
    if (at < 0 || at >= row->size) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    EditorUpdateRow(row);
    E.dirty++;
}

void EditorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
}

void EditorDelRow(int at) {
    if (at < 0 || at >= E.num_rows) return;
    EditorFreeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.num_rows - at - 1));
    E.num_rows--;
    E.dirty++;
}

void EditorRowAppendString(erow *row, char *s, size_t len) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    EditorUpdateRow(row);
    E.dirty++;
}

#endif //VANEDITOR_ROW_OPERATIONS_H
