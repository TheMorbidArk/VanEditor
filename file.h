//
// Created by 14825 on 2022/7/14.
//

#ifndef VANEDITOR_FILE_H
#define VANEDITOR_FILE_H

#include "VanEditor.h"
#include "terminal.h"

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
        EditorInsertRow(E.num_rows, line, linelen);
    }
    free(line);
    fclose(fp);
    E.dirty = 0;
}

char *EditorRowsToString(int *buflen) {
    int totlen = 0;
    int j;
    for (j = 0; j < E.num_rows; j++)
        totlen += E.row[j].size + 1;
    *buflen = totlen;
    char *buf = malloc(totlen);
    char *p = buf;
    for (j = 0; j < E.num_rows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    return buf;
}

void EditorSave() {
    if (E.file_name == NULL) {
        E.file_name = EditorPrompt("Save as: %s (ESC to cancel)");
        if (E.file_name == NULL) {
            EditorSetStatusMessage("Save aborted");
            return;
        }
    }
    int len;
    char *buf = EditorRowsToString(&len);
    int fd = open(E.file_name, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                E.dirty = 0;
                EditorSetStatusMessage("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }
    free(buf);
    EditorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

#endif //VANEDITOR_FILE_H
