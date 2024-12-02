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
#include "display.h"

// 파일 또는 디렉터리를 삭제하는 함수
void remove_file(const char* filepath) {
    struct dirent* entry; 
    struct stat info;
    DIR* dir = opendir(filepath); 

    if (dir == NULL) { // 디렉터리가 아니라 파일일 경우, unlink()로 삭제
        if (unlink(filepath) == -1) {
            display_error("unlink fail %s", filepath);
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
            display_error("stat fail: %s", filepath);
            continue;
        }

        if (S_ISDIR(info.st_mode)) { 
            remove_file(fullpath); // 하위 디렉터리를 재귀적으로 삭제
        } else { 
            if (unlink(fullpath) == -1) { 
                display_error("unlink fail: %ss", filepath);
                continue;
            }
        }
    }

    closedir(dir); // 디렉터리 닫기

    if (rmdir(filepath) == -1) { 
        display_error("rmdir fail: %s", filepath);
    }
}

// 파일 또는 디렉터리를 이동하는 함수
void move_file(const char* destination, const char* filepath) {
    // filepath가 destination의 상위 디렉터리인 경우 수행하지 않음
    size_t parent_len = strlen(filepath);
    if (strncmp(filepath, destination, parent_len) == 0) {
        display_error("The destination is a subdirectory of this file path");
        return;
    }
    if (rename(filepath, destination) == -1) {

        display_error("rename fail : %s -> %s (Erorr : %s)",filepath,destination,strerror(errno));
    }
}

void cp_file(const char* destination, const char* filepath) {
    struct stat src_info;

    // 원본 파일 상태 확인
    if (stat(filepath, &src_info) == -1) {
        display_error("stat fail: %s", filepath);
        return;
    }
    
    size_t parent_len = strlen(filepath);
    if (strncmp(filepath, destination, parent_len) == 0) {
        display_error("The destination is a subdirectory of this file path");
        return;
    }

    // 복사 로직 시작
    if (S_ISDIR(src_info.st_mode)) {
        // 디렉터리 복사 로직
        DIR* dir = opendir(filepath);
        if (dir == NULL) {
            display_error("Cannot open directory: %s", filepath);
            return;
        }

        // 복사 대상 디렉터리 생성
        if (mkdir(destination, src_info.st_mode & 0777) == -1 && errno != EEXIST) {
            display_error( "Cannot create directory: %s", destination);
            closedir(dir);
            return;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char srcPath[PATH_MAX], destPath[PATH_MAX];
            snprintf(srcPath, sizeof(srcPath), "%s/%s", filepath, entry->d_name);
            snprintf(destPath, sizeof(destPath), "%s/%s", destination, entry->d_name);

            cp_file(destPath, srcPath);
        }

        closedir(dir);
    } else {
        // 일반 파일 복사 로직
        FILE *src = fopen(filepath, "rb");
        if (!src) {
            display_error( "Cannot open source file: %s", filepath);
            return;
        }

        FILE *dest = fopen(destination, "wb");
        if (!dest) {
            display_error("Cannot open destination file: %s", destination);
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



void get_current_directory(char *buf, size_t size) {
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
            display_error( "Can not stat: current working directory");
        }
        if( stat("..", &parent_stat) != 0){
            display_error("Can not stat: parent directory");
        }

        // 루트 디렉터리인지 확인
        if (current_stat.st_ino == parent_stat.st_ino && current_stat.st_dev == parent_stat.st_dev) {
            break;
        }

        // 상위 디렉터리로 이동
        DIR *parent = opendir("..");
        if (!parent) {
            display_error( "Can not opendir: parent directory");
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

void resolve_absolute_path(char* absolute_path, const char* filename) {
    char current_dir[PATH_MAX-1] = {0};
    char full_path[PATH_MAX] = {0};

    // 현재 디렉터리를 구함
    get_current_directory(current_dir, PATH_MAX);

    // 파일 경로 조합
    snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, filename);

    // 파일이 존재하는지 확인
    if (access(full_path, F_OK) == 0) {
        strcpy(absolute_path, full_path);
    } else {
        display_error("File not found : %s",filename);
        
        absolute_path[0] = '\0';
    }
}