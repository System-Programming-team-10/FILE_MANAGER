#include <ncurses.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#define MAX_FILES 1024                  // 최대 파일 수 제한
#define PANEL_WIDTH (COLS / 2 - 1)      // 좌측 창 넓이
#define PREVIEW_WIDTH (COLS / 2 - 2)    // 우측 미리보기 창 넓이

int show_hidden_files = 0;          // 숨김 파일 표시 여부
int preview_scroll_offset = 0;      // 미리보기 창 scroll offset
int menu_visible[4] = {0, 0, 0, 0}; // 메뉴 패널 표시 유무 설정

WINDOW *menu_win;
WINDOW *path_win;
WINDOW *left_win;
WINDOW *preview_win;

WINDOW *file_menu_panel;
WINDOW *edit_menu_panel;
WINDOW *view_menu_panel;
WINDOW *help_menu_panel;


// init_ncurses
void init_ncurses() {
    initscr();  //ncurses 시작
    noecho();   // 에코 해제 -> 키 입력해도 화면에 보이지 않게
    cbreak();      // 키보드 입력 버퍼링 해제
    curs_set(0);    // 커서 숨김
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);  // 모든 마우스 입력 감지
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);   // 기본 배경
    init_pair(2, COLOR_CYAN, COLOR_BLACK);   // 디렉터리 색상
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // 일반 파일 색상
    init_pair(4, COLOR_GREEN, COLOR_BLACK);  // 실행 파일 색상
    init_pair(7, COLOR_WHITE, COLOR_CYAN);   // 선택된 항목의 배경과 텍스트 색상
    init_pair(8, COLOR_WHITE, COLOR_BLUE);   // 헤더 배경색 (헤더 구분을 위한 색상
}

void close_ncurses() {
 // window 삭제 및 ncurses 조욜
    delwin(menu_win);
    delwin(left_win);
    delwin(preview_win);
    delwin(path_win);
    delwin(file_menu_panel);
    delwin(edit_menu_panel);
    delwin(view_menu_panel);
    delwin(help_menu_panel);
    endwin();

}


// highlight : 현재 선택된 항목 -> files 배열에서 현재 선택된 파일의 인덱스
// 파일 목록을 표시하는 함수 , scroll_offset : 스크롤 위치 조정 -> 시작 인덱스 조절
void display_files(WINDOW *win, char *files[], int file_count, int highlight, int scroll_offset) {
    werase(win);    //창 지우기
    box(win, 0, 0); // 창 테두리 그리기


    // 좌측 창 최상단 바(이름, 사이즈, 수정시간) 배경색 설정
    wattron(win, COLOR_PAIR(8)); // 헤더 배경색 설정
    mvwprintw(win, 1, 1, "Name");
    //공백 배경색 처리
    for(int i=2;i<37;i++)
        mvwprintw(win,1,i," ");
    mvwprintw(win, 1, 37, "Size"); // Size 위치 조정
    //공백 배경색 처리
    for(int i=38;i<getmaxx(win)-20;i++)
        mvwprintw(win,1,i," ");
   
    mvwprintw(win, 1, getmaxx(win) - 20, "Modify time"); // Modify time 위치 오른쪽 정렬
    wattroff(win, COLOR_PAIR(8)); // 헤더 배경색 해제

    int max_display = getmaxy(win) - 3;

    // 파일 목록 출력
    for (int i = 0; i < max_display && i + scroll_offset < file_count; i++) {
        int index = i + scroll_offset;
        int line_width = getmaxx(win) - 2;

        // 선택된 항목의 배경색을 변경
        if (index == highlight) {
            wattron(win, COLOR_PAIR(7));
            for (int j = 0; j < line_width; j++) {
                mvwaddch(win, i + 2, j + 1, ' ');
            }
            wattroff(win, COLOR_PAIR(7));
        }

        struct stat file_stat;
        //파일 정보 가져오기
        if (stat(files[index], &file_stat) == 0) {
            char mod_time[20];
            strftime(mod_time, 20, "%Y-%m-%d %H:%M", localtime(&file_stat.st_mtime));

            if (index == highlight) {
                wattron(win, COLOR_PAIR(7));    //선택 항목 배경색 변경
            } else {
                if (S_ISDIR(file_stat.st_mode)) {
                    wattron(win, COLOR_PAIR(2));    // 디렉터리 색상
                } else if (file_stat.st_mode & S_IXUSR) {
                    wattron(win, COLOR_PAIR(4));    // binary file 색상
                } else {
                    wattron(win, COLOR_PAIR(3));    // 일반 파일 색상
                }
            }

            // 파일명, 사이즈, 수정 시간 출력 위치 조정
            mvwprintw(win, i + 2, 1, "%-25s", files[index]);                   // 파일 이름
            mvwprintw(win, i + 2, 37, "%10lld bytes", (long long)file_stat.st_size); // 파일 크기 위치 조정
            mvwprintw(win, i + 2, getmaxx(win) - 20, "%s", mod_time);             // 수정 시간 오른쪽 끝에 배치

            if (index == highlight) {
                wattroff(win, COLOR_PAIR(7));
            } else {
                wattroff(win, COLOR_PAIR(2) | COLOR_PAIR(3) | COLOR_PAIR(4));
            }
        }
    }
    wrefresh(win);  //창 업데이트
}


// 디렉토리 파일 목록 로드 함수
int load_files(char *files[]) {
    DIR *dir;
    struct dirent *entry;
    int file_count = 0;

    dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        if (!show_hidden_files && entry->d_name[0] == '.') {
            continue;
        }
        files[file_count] = strdup(entry->d_name);
        file_count++;
    }
    closedir(dir);

    return file_count;
}

// 파일 미리보기 기능
void display_preview(WINDOW *preview_win, const char *filename) {
    werase(preview_win);
    box(preview_win, 0, 0);

    struct stat file_stat;
    if (stat(filename, &file_stat) == 0) {

        if (S_ISDIR(file_stat.st_mode)) {   // 디렉터리 미리보기
            DIR *dir = opendir(filename);
            if (dir) {  
                struct dirent *entry;
                int line_num = 1;
                mvwprintw(preview_win, 1, 1, "[Directory Contents]");
                while ((entry = readdir(dir)) != NULL && line_num < getmaxy(preview_win) - 2) {
                    if (entry->d_name[0] == '.' && !show_hidden_files) continue;
                    mvwprintw(preview_win, ++line_num, 1, "%s", entry->d_name);
                }
                closedir(dir);
            } else {
                mvwprintw(preview_win, 1, 1, "Cannot open directory.");
            }
        } else {    //디렉터리 아닐 떄
            FILE *file = fopen(filename, "r");
            if (file != NULL) {
                char line[PREVIEW_WIDTH - 2];
                int line_num = 1 + preview_scroll_offset;
                while (fgets(line, sizeof(line), file) != NULL && line_num < getmaxy(preview_win) - 2) {
                    int line_length = strlen(line);
                    if (line[line_length - 1] == '\n') line[line_length - 1] = '\0';

                    int x = 1;
                    for (int i = 0; i < line_length; i++) {
                        if (x >= PREVIEW_WIDTH - 1) {
                            x = 1;
                            line_num++;
                        }
                        mvwaddch(preview_win, line_num, x++, line[i]);
                    }
                    line_num++;
                }
                fclose(file);
            } else {
                mvwprintw(preview_win, 1, 1, "Cannot open file.");
            }
        }
    }
    wrefresh(preview_win);
}

// 경로 표시
void display_path(WINDOW *path_win) {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    werase(path_win);
    box(path_win, 0, 0);
    mvwprintw(path_win, 1, 1, "Current Path: %s", cwd);
    wrefresh(path_win);
}

// 특정 메뉴 패널을 토글하는 함수
void toggle_menu_panel(WINDOW *menu_panel, int *visible, const char *title) {
    if (*visible) {
        werase(menu_panel);
        *visible = 0;
    } else {
        wclear(menu_panel);
        box(menu_panel, 0, 0);
        mvwprintw(menu_panel, 1, 1, "[%s Menu Panel]", title);
        mvwprintw(menu_panel, 2, 1, "Option 1");
        mvwprintw(menu_panel, 3, 1, "Option 2");
        mvwprintw(menu_panel, 4, 1, "Option 3");
        *visible = 1;
    }
    wrefresh(menu_panel);
}

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