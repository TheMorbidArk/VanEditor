//
// Created by 14825 on 2022/7/14.
//

#ifndef VANEDITOR_EDITOR_OPERATIONS_H
#define VANEDITOR_EDITOR_OPERATIONS_H

#include "VanEditor.h"
#include "row_operations.h"

/*** editor operations ***/
void EditorInsertChar(int c) {
    if (E.cy == E.num_rows) {
        EditorInsertRow(E.num_rows, "", 0);
    }
    EditorRowInsertChar(&E.row[E.cy], E.cx, c);
    E.cx++;
}

void EditorInsertNewline() {
    if (E.cx == 0) {
        EditorInsertRow(E.cy, "", 0);
    } else {
        erow *row = &E.row[E.cy];
        EditorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        row = &E.row[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';
        EditorUpdateRow(row);
    }
    E.cy++;
    E.cx = 0;
}

void EditorDelChar() {
    if (E.cy == E.num_rows) return;
    if (E.cx == 0 && E.cy == 0) return;
    erow *row = &E.row[E.cy];
    if (E.cx > 0) {
        EditorRowDelChar(row, E.cx - 1);
        E.cx--;
    } else {
        E.cx = E.row[E.cy - 1].size;
        EditorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
        EditorDelRow(E.cy);
        E.cy--;
    }
}

#endif //VANEDITOR_EDITOR_OPERATIONS_H
