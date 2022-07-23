

#ifndef VANEDITOR_FILE_H
#define VANEDITOR_FILE_H

#include "VanEditor.h"
#include "terminal.h"

/*** file i/o ***/

/**
 * 读取文件内容
 * @param filename:char* -> 文件名
 */
void EditorOpen(char *filename) {
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

/**
 * 将 struct erow 类型数据转换为 string(char *)
 * @param buflen:int* -> 缓冲区字符串长度
 * @return buf:char* -> 传入缓冲区的字符串
 */
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

/**
 * 保存文件
 */
void EditorSave() {
    if (E.file_name == NULL) {
        E.file_name = EditorPrompt("Save as: %s (ESC to cancel)");
        if (E.file_name == NULL) {
            EditorSetStatusMessage("Save aborted");
            return;
        }
    }
    
    //读取缓冲区信息
    int len;
    char *buf = EditorRowsToString(&len);
    
    //将数据写入文件,如果失败则在状态栏显示报错信息
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
