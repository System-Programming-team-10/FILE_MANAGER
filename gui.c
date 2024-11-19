#include <ncurses.h>
#include "gui.h"

// 새로 추가한 함수
void highlight_window(WINDOW *win, int is_selected) {
    if (is_selected) {
        wattron(win, COLOR_PAIR(9));  // 마젠타 테두리
        box(win, 0, 0);
        wattroff(win, COLOR_PAIR(9));
    } else {
        box(win, 0, 0);  // 기본 테두리
    }
    wrefresh(win);
}

// init_ncurses() 수정
void init_ncurses() {
    initscr();  // ncurses 시작
    noecho();   // 에코 해제 -> 키 입력해도 화면에 보이지 않게
    cbreak();   // 키보드 입력 버퍼링 해제
    curs_set(0);    // 커서 숨김
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);  // 모든 마우스 입력 감지
    start_color();

    // 색상 설정
    init_pair(1, COLOR_WHITE, COLOR_BLUE);   // 기본 배경
    init_pair(2, COLOR_CYAN, COLOR_BLACK);  // 디렉터리 색상
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // 일반 파일 색상
    init_pair(4, COLOR_GREEN, COLOR_BLACK);  // 실행 파일 색상
    init_pair(7, COLOR_WHITE, COLOR_CYAN);   // 선택된 항목의 배경과 텍스트 색상
    init_pair(8, COLOR_WHITE, COLOR_BLUE);   // 헤더 배경색
    init_pair(9, COLOR_MAGENTA, COLOR_BLACK); // 선택된 창 테두리 색상 (마젠타)     -- 추가함
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
