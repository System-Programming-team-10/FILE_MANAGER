#include <ncurses.h>
#include <panel.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <signal.h>
#include "display.h"
#include "file.h"
#include "gui.h"


WINDOW *menu_win;
WINDOW *path_win;
WINDOW *left_win;
WINDOW *preview_win;

// 시그널 블로킹/해제를 위한 함수
void block_signals(sigset_t *oldset) {
    sigset_t blockset;
    sigemptyset(&blockset);
    sigaddset(&blockset, SIGINT);  // SIGINT (Ctrl+C) 차단
    if (sigprocmask(SIG_BLOCK, &blockset, oldset) == -1) {
        display_error("sigprocmask - block");
        exit(EXIT_FAILURE);
    }
}

void unblock_signals(sigset_t *oldset) {
    if (sigprocmask(SIG_SETMASK, oldset, NULL) == -1) {
        display_error("sigprocmask - unblock");
        exit(EXIT_FAILURE);
    }
}

int main() {
   
    init_ncurses();

    menu_win = newwin(1, COLS, 0, 0);
    path_win = newwin(3, COLS, LINES - 3, 0);
    left_win = newwin(LINES - 4, PANEL_WIDTH, 1, 1);
    preview_win = newwin(LINES - 4, PREVIEW_WIDTH, 1, PANEL_WIDTH + 2);

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
    int file_count = load_files(files, preview_win);
    int highlight = 0;
    int scroll_offset = 0;

    // 초기 강조 표시
    highlight_window(left_win, 1);  // 좌측 창 강조
    highlight_window(preview_win, 0); // 우측 창 기본 테두리

    display_files(left_win, files, file_count, highlight, scroll_offset);
    display_preview(preview_win, files[highlight]);
    display_path(path_win, preview_win);
    refresh();
    doupdate();

    int ch; //키보드 입력

    highlight_window(left_win, 1);  // 좌측 창 활성화
    highlight_window(preview_win, 0); // 우측 창 비활성화

    int file_flag; // 파일 작업 처리를 위한 flag(0: 초기화, 1: move, 2: copy)

    int help_visible=0; //help 창 표시 상태 (0 : 숨김, 1 : 표시)
    sigset_t oldset;
    while ((ch = getch()) != 27) {  // ESC 키로 종료

        if (help_visible) {
            // Help 창이 열려 있는 상태에서 입력이 들어오면 Help 창을 닫음
            werase(preview_win); // 기존 내용 지우기
            box(preview_win, 0, 0); // 테두리 그리기
            if (file_count > 0) {
                display_preview(preview_win, files[highlight]); // 선택된 파일 내용 출력
            } else {
                mvwprintw(preview_win, 1, 2, "No file selected.");
            }
            wrefresh(preview_win); // 미리보기 창 갱신
            help_visible = 0;
    
            // Help 버튼 색상 원래대로 복원
            mvwchgat(menu_win, 0, 47, 8, A_NORMAL, 1, NULL); // Help 기본 배경 복원
            wrefresh(menu_win); // 메뉴 창 갱신
    
            continue; // 입력 처리를 종료하고 다음 입력 대기
        }

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
            mvwchgat(menu_win, 0, 24, 9, A_NORMAL, 1, NULL);  // Move (M) 기본 배경 복원
            wrefresh(menu_win);

            // 현재 디렉터리 파일 목록을 다시 로드하고 화면 갱신
            file_count = load_files(files, preview_win);
            highlight = 0;  // highlight 초기화
            scroll_offset = 0;  // scrikk_offset 초기화
            display_files(left_win, files, file_count, highlight, scroll_offset);
            display_preview(preview_win, files[highlight]);
            display_path(path_win, preview_win);
            refresh();
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
                if (highlight < 0) highlight = 0; // highlight 경계값 체크
                if (scroll_offset < 0) scroll_offset = 0; // scroll_offset 경계값 체크
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]);
                display_path(path_win, preview_win);
                break;

            case KEY_DOWN:
                if (highlight < file_count - 1) highlight++;
                if (highlight >= scroll_offset + getmaxy(left_win) - 3) scroll_offset++;
                display_files(left_win, files, file_count, highlight, scroll_offset);
                if (highlight >= file_count) highlight = file_count - 1; // highlight 경계값 체크
                display_preview(preview_win, files[highlight]);
                display_path(path_win, preview_win);
                break;

            case KEY_LEFT:  // 상위 디렉터리로 이동
                chdir("..");
                for (int i = 0; i < file_count; i++) {
                    free(files[i]);
                }
                file_count = load_files(files, preview_win);
                highlight = 0;
                scroll_offset = 0;
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]);
                display_path(path_win, preview_win);
                break;

            case KEY_RIGHT:  // 하위 디렉터리로 이동
                if (strcmp(files[highlight], ".") != 0 && strcmp(files[highlight], "..") != 0) {
                    struct stat file_stat;
                    if (stat(files[highlight], &file_stat) == 0 && S_ISDIR(file_stat.st_mode)) { // 디렉터리인지 확인
                        if (chdir(files[highlight]) == 0) { // 디렉터리 이동 성공
                            for (int i = 0; i < file_count; i++) {
                                free(files[i]);
                            }
                            file_count = load_files(files, preview_win);
                            highlight = 0;
                            scroll_offset = 0;

                            display_files(left_win, files, file_count, highlight, scroll_offset);
                            display_preview(preview_win, files[highlight]); // 현재 디렉터리 내용 표시
                            display_path(path_win, preview_win);
                        } else {    // 디렉터리 이동 실패하면
                            display_error("Failed to change directory.");
                            wrefresh(preview_win);
                        }
                    } else {    //루트 디렉터리면

                        display_error("Not a directory : %s",files[highlight]);
                        wrefresh(preview_win);
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
                refresh();


                for (int i = 0; i < file_count; i++) {
                    free(files[i]);
                }   
                file_count = load_files(files, preview_win);


                highlight = 0;  // 강조 표시 초기화
                scroll_offset = 0;  // 스크롤 초기화
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]); // 미리보기 창 갱신

                
                break;

            case 'd':  // Delete
                file_flag = 3; // Delete 상태 활성화
                filename = files[highlight];
                 // 시그널 차단
                block_signals(&oldset);
                remove_file(filename); // 파일 삭제
                // 시그널 차단 해제
                unblock_signals(&oldset);
                file_count = load_files(files, preview_win);
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]);
                display_path(path_win, preview_win);

               // Delete 메뉴 배경색 변경
                mvwchgat(menu_win, 0, 12, 9, A_NORMAL, 7, NULL); // Delete (D) 배경 시안으로 변경
                wrefresh(menu_win);

                mvwchgat(menu_win, 0, 12, 9, A_NORMAL, 1, NULL); // Delete 기본 배경 복원
                wrefresh(menu_win);
                refresh();
                break;


            case 'm':  // Move
                file_flag = 1; // Move 상태 활성화
                memset(abs_filepath, 0, PATH_MAX);
                filename = files[highlight];
                strcpy(save_filename, filename);
                resolve_absolute_path(abs_filepath, filename);

                mvwchgat(menu_win, 0, 24, 8, A_NORMAL, 7, NULL); // Move (M) 배경 시안으로 변경
                wrefresh(menu_win);
                refresh();

                for (int i = 0; i < file_count; i++) {
                    free(files[i]);
                }   
                file_count = load_files(files, preview_win);


                highlight = 0;  // 강조 표시 초기화
                scroll_offset = 0;  // 스크롤 초기화
                display_files(left_win, files, file_count, highlight, scroll_offset);
                display_preview(preview_win, files[highlight]); // 미리보기 창 갱신
                break;

            case 'p':  // Paste
                if (file_flag != 0) {
                    memset(abs_dirpath, 0, PATH_MAX);
                    memset(destination, 0, PATH_MAX);
                    get_current_directory(abs_dirpath,PATH_MAX);
                    strcat(destination, abs_dirpath);
                    strcat(destination, "/");
                    strcat(destination, save_filename);
                    if (strcmp(destination, abs_filepath) != 0) {
                        switch (file_flag) {
                            case 1:
                                // 시그널 차단
                                block_signals(&oldset);
                                move_file(destination, abs_filepath); // Move
                                // 시그널 차단 해제
                                unblock_signals(&oldset);
                                break; 
                            case 2:
                                 // 시그널 차단
                                block_signals(&oldset);
                                cp_file(destination, abs_filepath); // Copy
                                // 시그널 차단 해제
                                unblock_signals(&oldset);
                                break; 
                        }
                    }
                    file_count = load_files(files, preview_win);
                    display_files(left_win, files, file_count, highlight, scroll_offset);
                    display_preview(preview_win, files[highlight]);
                    display_path(path_win, preview_win);

                    file_flag = 0; // Paste 완료 후 상태 초기화

                    mvwchgat(menu_win,0,1,8,A_NORMAL,7,NULL);
                    wrefresh(menu_win);

                    mvwchgat(menu_win, 0, 1, 8, A_NORMAL, 1, NULL); // Copy 기본 배경 복원
                    mvwchgat(menu_win, 0, 24, 8, A_NORMAL, 1, NULL); // Move 기본 배경 복원
                    mvwchgat(menu_win,0,1,8,A_NORMAL,1,NULL);
                    wrefresh(menu_win);
                    refresh();

                
                } else {
                    display_error( "No file to paste");  // 에러 메시지 출력
                
                    break;
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
                struct stat file_stat;
                if (stat(files[highlight], &file_stat) == 0) { // 파일 상태 확인
                    if (S_ISDIR(file_stat.st_mode)) { // 디렉터리인 경우 Tab 동작 무시
                        display_error("Error : Cannot open directories with Tab.");
                        wrefresh(preview_win);
                        break;
                    } else { // 일반 파일인 경우 more 기능 실행
                        highlight_window(left_win, 0);  // 좌측 창 비활성화
                        highlight_window(preview_win, 1); // 우측 창 활성화
                        more(preview_win, files[highlight]);
                        highlight_window(left_win, 1);  // 좌측 창 활성화
                        highlight_window(preview_win, 0); // 우측 창 비활성화
                    }
                } else {
                    display_error("Error : accwssing file : %s",files[highlight]);
                    wrefresh(preview_win);
                }
                break;
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