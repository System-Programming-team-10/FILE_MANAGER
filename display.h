#ifndef DISPLAY_H
# define DISPLAY_H
# include <ncurses.h>
# define MAX_FILES 1024                  // 최대 파일 수 제한
# define PANEL_WIDTH (COLS / 2 - 1)      // 좌측 창 넓이
# define PREVIEW_WIDTH (COLS / 2 - 2)    // 우측 미리보기 창 넓이


void display_files(WINDOW *win, char *files[], int file_count, int highlight, int scroll_offset); //from display.c
void display_ls_file(WINDOW *win, char *files[], int file_count, int highlight, int scroll_offset, int max_display)
int load_files(char *files[]);  //from display.c
void display_preview(WINDOW *preview_win, const char *filename); //from display.c
void do_dir(WINDOW *preview_win,const char *filename)
void do_file(WINDOW *preview_win,const char *filename)
void display_path(WINDOW *path_win);  //from display.c
#endif