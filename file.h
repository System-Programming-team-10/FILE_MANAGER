#ifndef FILE_H
# define FILE_H
void paste(); //from file.c
void remove_file(const char* filepath); //from file.c
void move_file(const char* destination, const char* filepath); //from file.c
void cp_file(const char* destination, const char* filepath); //from file.c
void resolve_absolute_path(char* absolute_path, const char* filename);
void get_current_directory(char* absolute_path);
#endif