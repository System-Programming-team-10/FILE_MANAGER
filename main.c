#include <ncurses.h>
#include<panel.h>
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
    mvwprintw(menu_win, 0, 1, "Copy (C)   Delete(D)   Move (M)   Paste (p)   Help (H)");
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

    //WINDOW *help_win; // 도움말 창
    int help_visible=0; //help 창 표시 상태 (0 : 숨김, 1 : 표시)

    while ((ch = getch()) != 27) {  // ESC 키로 종료

        int is_arrow_key = (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_LEFT || ch == KEY_RIGHT);
        int is_paste_key = (ch == 'p');

        // Copy 상태 해제 조건
        if (file_flag == 2 && !is_arrow_key && !is_paste_key && ch != 'c') {
            file_flag = 0;
            mvwchgat(menu_win, 0, 1, 9, A_NORMAL, 1, NULL);  // Copy (C) 기본 배경 복원
            wrefresh(menu_win);
        }

        // Move 상태 해제 조건
        if (file_flag == 1 && !is_arrow_key && !is_paste_key && ch != 'm') {
            file_flag = 0;
            mvwchgat(menu_win, 0, 21, 9, A_NORMAL, 1, NULL);  // Move (M) 기본 배경 복원
            wrefresh(menu_win);
        }

        // Delete 상태 해제 조건
        if (file_flag == 3 && !is_arrow_key && ch != 'd') {
            file_flag = 0;
            mvwchgat(menu_win, 0, 11, 9, A_NORMAL, 1, NULL);  // Delete (D) 기본 배경 복원
            wrefresh(menu_win);
        }

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

            case 'c':  // Copy
                file_flag = 2; // Copy 상태 활성화
                memset(abs_filepath, 0, PATH_MAX);
                filename = files[highlight];
                strcpy(save_filename, filename);
                resolve_absolute_path(abs_filepath, filename);

                // Copy 메뉴 배경색 변경
                mvwchgat(menu_win, 0, 1, 8, A_NORMAL, 7, NULL); // Copy (C) 배경 시안으로 변경
                wrefresh(menu_win);
                break;

            case 'd':  // Delete
                file_flag = 3; // Delete 상태 활성화
                filename = files[highlight];
                remove_file(filename); // 파일 삭제
                file_count = load_files(files);
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]);
                display_path(path_win);

               // Delete 메뉴 배경색 변경
                mvwchgat(menu_win, 0, 12, 9, A_NORMAL, 7, NULL); // Delete (D) 배경 시안으로 변경
                wrefresh(menu_win);
                break;


            case 'm':  // Move
                file_flag = 1; // Move 상태 활성화
                memset(abs_filepath, 0, PATH_MAX);
                filename = files[highlight];
                resolve_absolute_path(abs_filepath, filename);

                mvwchgat(menu_win, 0, 24, 8, A_NORMAL, 7, NULL); // Move (M) 배경 시안으로 변경
                wrefresh(menu_win);
                break;

            case 'p':  // Paste
                if (file_flag != 0) {
                    memset(abs_dirpath, 0, PATH_MAX);
                    memset(destination, 0, PATH_MAX);
                    get_current_directory(abs_dirpath);
                    strcat(destination, abs_dirpath);
                    strcat(destination, "/");
                    strcat(destination, save_filename);

                    if (strcmp(destination, abs_filepath) != 0) {
                        switch (file_flag) {
                            case 1: move_file(destination, abs_filepath); break;  // Move
                            case 2: cp_file(destination, abs_filepath); break;    // Copy
                        }
                    }
                    file_count = load_files(files);
                    display_files(left_win, files, file_count, highlight, scroll_offset);
                    display_preview(preview_win, files[highlight]);
                    display_path(path_win);

                    file_flag = 0; // Paste 완료 후 상태 초기화
                    mvwchgat(menu_win, 0, 1, 8, A_NORMAL, 1, NULL); // Copy 기본 배경 복원
                    mvwchgat(menu_win, 0, 24, 8, A_NORMAL, 1, NULL); // Move 기본 배경 복원
                    wrefresh(menu_win);
                }
                break;

            case 'h': // Help 창 토글
                if (!help_visible) {
                    // 미리보기 창에 Help 내용 출력
                    werase(preview_win); // 기존 내용 지우기
                    box(preview_win, 0, 0); // 테두리 그리기
                    mvwprintw(preview_win, 1, 2, " Help Menu ");
                    mvwprintw(preview_win, 3, 2, "This is the help screen for your File Manager.");
                    mvwprintw(preview_win, 5, 2, "Key Commands:");
                    mvwprintw(preview_win, 6, 4, "C: Copy the selected file");
                    mvwprintw(preview_win, 7, 4, "D: Delete the selected file");
                    mvwprintw(preview_win, 8, 4, "M: Move the selected file");
                    mvwprintw(preview_win, 9, 4, "P: Paste the copied/moved file");
                    mvwprintw(preview_win, 10, 4, "Arrow Keys: Navigate through the files");
                    mvwprintw(preview_win, 11, 4, "H: Toggle this Help menu");
                    mvwprintw(preview_win, 12, 4, "ESC: Exit the program");
                    wrefresh(preview_win); // 미리보기 창 갱신
                    help_visible = 1;

                    mvwchgat(menu_win, 0, 47, 8, A_NORMAL, 7, NULL); // Move (M) 배경 시안으로 변경
                    wrefresh(menu_win); // 메뉴 창 갱신
                } else {
                    // 미리보기 창 원래 내용 복원
                    werase(preview_win); // 기존 내용 지우기
                    box(preview_win, 0, 0); // 테두리 그리기
                    if (file_count > 0) {
                        display_preview(preview_win, files[highlight]); // 선택된 파일 내용 출력
                    } else {
                        mvwprintw(preview_win, 1, 2, "No file selected.");
                    }
                    wrefresh(preview_win); // 미리보기 창 갱신
                    help_visible = 0;

                    mvwchgat(menu_win, 0, 47, 8, A_NORMAL, 1, NULL); // Move 기본 배경 복원
                    wrefresh(menu_win); // 메뉴 창 갱신
                }
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