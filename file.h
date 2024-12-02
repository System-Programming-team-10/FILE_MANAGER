#ifndef FILE_H
# define FILE_H
void remove_file(const char* filepath);
void move_file(const char* destination, const char* filepath);
void cp_file(const char* destination, const char* filepath);
void get_current_directory(char *buf, size_t size);
void resolve_absolute_path(char* absolute_path, const char* filename);
#endif