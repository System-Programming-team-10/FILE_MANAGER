#ifndef FILE_H
# define FILE_H
void remove_file(const char* filepath, WINDOW* preview_win);
void move_file(const char* destination, const char* filepath, WINDOW* preview_win);
void cp_file(const char* destination, const char* filepath, WINDOW* preview_win);
void get_current_directory(char *buf, size_t size, WINDOW* preview_win);
void resolve_absolute_path(char* absolute_path, const char* filename, WINDOW* preview_win);
void display_error(WINDOW *menu_win, const char *format, ...);
#endif