#include <ncurses.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include "display.h"
#include "file.h"
#include "gui.h"
int menu_visible[4] = {0, 0, 0, 0}; // 메뉴 패널 표시 유무 설정

WINDOW *menu_win;
WINDOW *path_win;
WINDOW *left_win;
WINDOW *preview_win;

WINDOW *file_menu_panel;
WINDOW *edit_menu_panel;
WINDOW *view_menu_panel;
WINDOW *help_menu_panel;


int main() {
   
    init_ncurses();

    menu_win = newwin(1, COLS, 0, 0);
    path_win = newwin(2, COLS, LINES - 3, 0);
    left_win = newwin(LINES - 6, PANEL_WIDTH, 1, 1);
    preview_win = newwin(LINES - 6, PREVIEW_WIDTH, 1, PANEL_WIDTH + 2);
    file_menu_panel = newwin(5, 20, 2, 1);
    edit_menu_panel = newwin(5, 20, 2, 22);
    view_menu_panel = newwin(5, 20, 2, 43);
    help_menu_panel = newwin(5, 20, 2, 64);


    
    wbkgd(menu_win, COLOR_PAIR(1));
    mvwprintw(menu_win, 0, 1, "File (F1)   Edit (F2)   View (F3)   Help (F4)");
    refresh();
    wrefresh(menu_win);

   
    wbkgd(path_win, COLOR_PAIR(1));

    

    char *files[MAX_FILES];
    int file_count = load_files(files);
    int highlight = 0;
    int scroll_offset = 0;

    display_files(left_win, files, file_count, highlight, scroll_offset);
    display_preview(preview_win, files[highlight]);
    display_path(path_win);
    refresh();
    doupdate();

    int ch; //키보드 입력
    int max_display = getmaxy(left_win) - 2;

    // loop
    while ((ch = getch()) != 27) {  // ESC 키로 종료
        switch (ch) {
            case KEY_UP:    //방향키(상)
                if (highlight > 0) highlight--;
                if (highlight < scroll_offset) scroll_offset--;
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]);
                break;

            case KEY_DOWN:  //방향키(하)
                if (highlight < file_count - 1) highlight++;
                if (highlight >= scroll_offset + max_display) scroll_offset++;
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]);
                break;

            case KEY_LEFT:  //방향키(좌)
                chdir("..");
                for (int i = 0; i < file_count; i++) {
                    free(files[i]);
                }
                file_count = load_files(files);
                highlight = 0;
                scroll_offset = 0;
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]);
                display_path(path_win);
                break;

            case KEY_RIGHT: //방향키(우)
                if (strcmp(files[highlight], ".") != 0 && strcmp(files[highlight], "..") != 0) {
                    struct stat file_stat;
                    stat(files[highlight], &file_stat);
                    if (S_ISDIR(file_stat.st_mode)) {
                        chdir(files[highlight]);
                        for (int i = 0; i < file_count; i++) {
                            free(files[i]);
                        }
                        file_count = load_files(files);
                        highlight = 0;
                        scroll_offset = 0;
                        display_files(left_win, files, file_count, highlight, scroll_offset);
                        display_preview(preview_win, files[highlight]);
                        display_path(path_win);
                    }
                }
                break;

            case KEY_F(1):  // F1
                toggle_menu_panel(file_menu_panel, &menu_visible[0], "File");
                break;

            case KEY_F(2):  // F2
                toggle_menu_panel(edit_menu_panel, &menu_visible[1], "Edit");
                break;

            case KEY_F(3):  // F3
                toggle_menu_panel(view_menu_panel, &menu_visible[2], "View");
                break;

            case KEY_F(4):  // F4
                toggle_menu_panel(help_menu_panel, &menu_visible[3], "Help");
                break;
        }
        refresh();
        doupdate();
    }


    for (int i = 0; i < file_count; i++) 
        free(files[i]);

    close_ncurses();
   
    return 0;
}