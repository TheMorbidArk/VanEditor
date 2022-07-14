
#include "VanEditor.h"
#include "terminal.h"
#include "file.h"
#include "input.h"
#include "output.h"

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
    E.dirty = 0;
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

    EditorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit");

    while (1) {
        EditorRefreshScreen();
        EditorProcessKeys();
    }
    return 0;
}