#ifndef GUI_H
# define GUI_H
# define PANEL_WIDTH (COLS / 2 - 1)      // 좌측 창 넓이
# define PREVIEW_WIDTH (COLS / 2 - 2)    // 우측 미리보기 창 넓이

extern WINDOW *menu_win;
extern WINDOW *path_win;
extern WINDOW *left_win;
extern WINDOW *preview_win;

extern WINDOW *file_menu_panel;
extern WINDOW *edit_menu_panel;
extern WINDOW *view_menu_panel;
extern WINDOW *help_menu_panel;

void highlight_window(WINDOW *win, int is_selected);
void init_ncurses(); //from gui.c
void close_ncurses(); //from gui.c
void toggle_menu_panel(WINDOW *menu_panel, int *visible, const char *title); //from gui.c
#endif