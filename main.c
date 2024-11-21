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

    const char* filename;
    char abs_filepath[PATH_MAX]={0};
    char abs_dirpath[PATH_MAX]={0};
    char destination[PATH_MAX]={0};
    char save_filename[256]={0};
    wbkgd(menu_win, COLOR_PAIR(1));
    mvwprintw(menu_win, 0, 1, "File (F1)   Edit (F2)   View (F3)   Help (F4)");
    refresh();
    wrefresh(menu_win);

   
    wbkgd(path_win, COLOR_PAIR(1));

    

    char *files[MAX_FILES];
    int file_count = load_files(files);
    int highlight = 0;
    int scroll_offset = 0;

    // 초기 강조 표시
    highlight_window(left_win, 1);  // 좌측 창 강조
    highlight_window(preview_win, 0); // 우측 창 기본 테두리

    display_files(left_win, files, file_count, highlight, scroll_offset);
    display_preview(preview_win, files[highlight]);
    display_path(path_win);
    refresh();
    doupdate();

    int ch; //키보드 입력

    //추가함
    highlight_window(left_win, 1);  // 좌측 창 활성화
    highlight_window(preview_win, 0); // 우측 창 비활성화

    int file_flag; // 파일 작업 처리를 위한 flag(0: 초기화, 1: move, 2: copy)

    while ((ch = getch()) != 27) {  // ESC 키로 종료
        switch (ch) {
            
            case KEY_UP:
                if (highlight > 0) highlight--;
                if (highlight < scroll_offset) scroll_offset--;
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]);
                display_path(path_win);
                break;

            case KEY_DOWN:
                if (highlight < file_count - 1) highlight++;
                if (highlight >= scroll_offset + getmaxy(left_win) - 3) scroll_offset++;
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]);
                display_path(path_win);
                break;

            case KEY_LEFT:  // 상위 디렉터리로 이동
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

            case KEY_RIGHT:  // 하위 디렉터리로 이동
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

            case 'd':
                filename = files[highlight];
                remove_file(filename);
                file_count = load_files(files);
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]);
                display_path(path_win);
                break;
            
            case 'm':  // Move
                memset(abs_filepath, 0, PATH_MAX); // abs_filepath 초기화
                file_flag = 1;
                filename = files[highlight];
                resolve_absolute_path(abs_filepath, filename);
                break;

            case 'c':  // Copy
                memset(abs_filepath, 0, PATH_MAX); // abs_filepath 초기화
                file_flag = 2;
                filename = files[highlight];
                strcpy(save_filename,filename);
                resolve_absolute_path(abs_filepath, filename);
                break;

            case 'p':
                //get destination
                memset(abs_dirpath, 0, PATH_MAX); 
                memset(destination, 0, PATH_MAX);
                get_current_directory(abs_dirpath);
                strcat(destination, abs_dirpath);
                strcat(destination, "/");
                strcat(destination, save_filename);
                
                if(strcmp(destination,abs_filepath)==0)
                    break;
                switch (file_flag) {
                    case 1:  // Move
                        move_file(destination, abs_filepath);
                        break;
                    case 2:  // Copys
                        cp_file(destination, abs_filepath);
                        break;
                }
                file_count = load_files(files);
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]);
                display_path(path_win);
                file_flag = 0; // Reset flag
                break;
            case '\t':  // Tab 키로 우측 창으로 전환
                highlight_window(left_win, 0);  // 좌측 창 비활성화
                highlight_window(preview_win, 1); // 우측 창 활성화
                more(preview_win, files[highlight]);
                highlight_window(left_win, 1);  // 좌측 창 활성화
                highlight_window(preview_win, 0); // 우측 창 비활성화
                break;

            // 추가 키 처리
        }
    highlight_window(left_win, 1);
    
    refresh();
    doupdate();
    }
    for (int i = 0; i < file_count; i++) 
        free(files[i]);
    close_ncurses();
   
    return 0;
}