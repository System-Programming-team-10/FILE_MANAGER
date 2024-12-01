#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h> // For PATH_MAX
#include <errno.h>
#include <ncurses.h>
#include "file.h"
#include "gui.h"


// 파일 또는 디렉터리를 삭제하는 함수
void remove_file(const char* filepath, WINDOW* preview_win) {
    struct dirent* entry; 
    struct stat info;
    DIR* dir = opendir(filepath); 

    if (dir == NULL) { // 디렉터리가 아니라 파일일 경우, unlink()로 삭제
        if (unlink(filepath) == -1) {
            mvwprintw(preview_win, 1, 1, "unlink fail %s", filepath);
            wrefresh(preview_win);
        }
        return;
    }

    // 디렉터리 내부 파일 및 하위 디렉터리를 순회
    while ((entry = readdir(dir)) != NULL) {
        char fullpath[PATH_MAX]; 
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { 
            continue;
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", filepath, entry->d_name);

        if (stat(fullpath, &info) == -1) { 
            mvwprintw(preview_win, 1, 1, "stat fail: %s", fullpath);
            wrefresh(preview_win);
            continue;
        }

        if (S_ISDIR(info.st_mode)) { 
            remove_file(fullpath, preview_win); // 하위 디렉터리를 재귀적으로 삭제
        } else { 
            if (unlink(fullpath) == -1) { 
                mvwprintw(preview_win, 1, 1, "unlink fail: %s", fullpath);
                wrefresh(preview_win);
                continue;
            }
        }
    }

    closedir(dir); // 디렉터리 닫기

    if (rmdir(filepath) == -1) { 
        mvwprintw(preview_win, 1, 1, "rmdir fail: %s", filepath);
        wrefresh(preview_win);
    }
}

// 파일 또는 디렉터리를 이동하는 함수
void move_file(const char* destination, const char* filepath, WINDOW* preview_win) {
    // filepath가 destination의 상위 디렉터리인 경우 수행하지 않음
    size_t parent_len = strlen(filepath);
    if (strncmp(filepath, destination, parent_len) == 0) {
        mvwprintw(preview_win, 5, 1, "destination이 filepath의 하위 디렉터리입니다.");
        wrefresh(preview_win);
        return;
    }
    if (rename(filepath, destination) == -1) {

        display_error(menu_win,"rename fail : %s -> %s (Erorr : %s)",filepath,destination,strerror(errno));
       // mvwprintw(preview_win, 1, 1, "rename fail: %s -> %s (Error: %s)", filepath, destination, strerror(errno));

        wrefresh(preview_win);
    }
}

void cp_file(const char* destination, const char* filepath, WINDOW* preview_win) {
    struct stat src_info, dest_info;

    // 원본 파일 상태 확인
    if (stat(filepath, &src_info) == -1) {
        display_error(menu_win, "stat fail: %s", filepath);
        wrefresh(preview_win);
        return;
    }

    char real_src[PATH_MAX];
    char real_dest[PATH_MAX];

    // 원본과 대상의 절대 경로 가져오기
    if (realpath(filepath, real_src) == NULL) {
        display_error(menu_win, "Error resolving source path: %s", filepath);
        wrefresh(preview_win);
        return;
    }
    if (realpath(destination, real_dest) == NULL) {
        display_error(menu_win, "Error resolving destination path: %s", destination);
        wrefresh(preview_win);
        return;
    }

    // 복사 대상이 원본 디렉터리의 하위 경로인지 확인
    size_t src_len = strlen(real_src);
    if (strncmp(real_dest, real_src, src_len) == 0 && real_dest[src_len] == '/') {
        display_error(menu_win, "Destination is a subdirectory of the source directory.");
        wrefresh(preview_win);
        return;
    }

    // 동일한 파일인지 확인
    if (strcmp(filepath, destination) == 0) {
        display_error(menu_win, "Source and destination are the same: %s", filepath);
        wrefresh(preview_win);
        return;
    }

    // 복사 대상에 동일한 파일이 존재하는지 확인
    if (stat(destination, &dest_info) == 0) {
        if (S_ISDIR(dest_info.st_mode)) {
            display_error(menu_win, "Error: Directory already exists at destination");
        } else {
            display_error(menu_win, "Error: File already exists at destination");
        }
        wrefresh(preview_win);
        return;
    }

    // 디렉터리 복사
    if (S_ISDIR(src_info.st_mode)) {
        DIR* dir = opendir(filepath);
        if (dir == NULL) {
            display_error(menu_win, "Cannot open directory: %s", filepath);
            wrefresh(preview_win);
            return;
        }

        // 복사 대상 디렉터리 생성
        if (mkdir(destination, src_info.st_mode & 0777) == -1) {
            display_error(menu_win, "Cannot create directory: %s", destination);
            wrefresh(preview_win);
            closedir(dir);
            return;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // 하위 디렉터리 및 파일 경로 생성
            char srcPath[PATH_MAX], destPath[PATH_MAX];
            snprintf(srcPath, sizeof(srcPath), "%s/%s", filepath, entry->d_name);
            snprintf(destPath, sizeof(destPath), "%s/%s", destination, entry->d_name);

            cp_file(destPath, srcPath, preview_win); // 하위 파일 및 디렉터리 재귀적으로 복사
        }

        closedir(dir);
    } else { // 파일 복사
        FILE *src, *dest;

        // 원본 파일 열기
        src = fopen(filepath, "rb");
        if (src == NULL) {
            display_error(menu_win, "Cannot open source file: %s", filepath);
            wrefresh(preview_win);
            return;
        }

        // 대상 파일 열기
        dest = fopen(destination, "wb");
        if (dest == NULL) {
            display_error(menu_win, "Cannot open destination file: %s", destination);
            wrefresh(preview_win);
            fclose(src);
            return;
        }

        char buffer[1024];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
            fwrite(buffer, 1, bytes, dest);
        }

        fclose(src);
        fclose(dest);
    }
}



void get_current_directory(char *buf, size_t size, WINDOW* preview_win) {
    struct stat current_stat, parent_stat, root_stat;
    char temp[PATH_MAX];  // 경로를 저장할 임시 버퍼
    char *ptr = temp + PATH_MAX - 1;  // 버퍼의 끝부터 시작

    *ptr = '\0';  // 문자열의 끝 표시

    if (stat("/", &root_stat) != 0) {
            getcwd(buf, size);
            sleep(3);
            return;
        }
    while (1) {
        // 현재 디렉터리와 상위 디렉터리의 inode 정보를 가져옴
        if (stat(".", &current_stat) != 0) {
            mvwprintw(preview_win, 1, 1, "Can not stat: current working directory");
            wrefresh(preview_win);
        }
        if( stat("..", &parent_stat) != 0){
            mvwprintw(preview_win, 1, 1, "Can not stat: parent directory");
            wrefresh(preview_win);
        }

        // 루트 디렉터리인지 확인
        if (current_stat.st_ino == parent_stat.st_ino && current_stat.st_dev == parent_stat.st_dev) {
            break;
        }

        // 상위 디렉터리로 이동
        DIR *parent = opendir("..");
        if (!parent) {
            mvwprintw(preview_win, 1, 1, "Can not opendir: parent directory");
            wrefresh(preview_win);
        }

        struct dirent *entry;
        while ((entry = readdir(parent)) != NULL) {
            if (entry->d_ino == current_stat.st_ino) {
                size_t len = strlen(entry->d_name);
                ptr -= len + 1;
                if (ptr < temp) {
                    closedir(parent);
                    //error  // 경로가 버퍼를 초과함
                }
                memcpy(ptr, "/", 1);
                memcpy(ptr + 1, entry->d_name, len);
                break;
            }
        }
        closedir(parent);
        chdir("..");  // 상위 디렉터리로 이동
    }
    // 루트 디렉터리 처리
    if (ptr == temp + PATH_MAX - 1) {
        ptr--;
        *ptr = '/';
    }

    // 결과를 사용자 제공 버퍼로 복사
    size_t len = temp + PATH_MAX - ptr;
    if (len > size) {
        //error// 버퍼가 충분하지 않음
    }
    memcpy(buf, ptr, len);

    chdir(buf);
}

void resolve_absolute_path(char* absolute_path, const char* filename, WINDOW* preview_win) {
    char current_dir[PATH_MAX-1] = {0};
    char full_path[PATH_MAX] = {0};

    // 현재 디렉터리를 구함
    get_current_directory(current_dir, PATH_MAX, preview_win);

    // 파일 경로 조합
    snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, filename);

    // 파일이 존재하는지 확인
    if (access(full_path, F_OK) == 0) {
        strcpy(absolute_path, full_path);
    } else {
        display_error(menu_win,"File not found : %s",filename);
        //mvwprintw(preview_win, 1, 2, "File not found: %s\n", filename);
        wrefresh(preview_win);
        absolute_path[0] = '\0';
    }
}