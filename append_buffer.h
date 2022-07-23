

#ifndef VANEDITOR_APPEND_BUFFER_H
#define VANEDITOR_APPEND_BUFFER_H

/*** append buffer ***/
/* 字符串缓冲区 */

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


#endif //VANEDITOR_APPEND_BUFFER_H
