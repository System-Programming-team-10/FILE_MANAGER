#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h> // For PATH_MAX

// 재귀적으로 파일 및 폴더 삭제
void remove_file(const char* filepath) {
    struct dirent* entry;
    struct stat info;
    DIR* dir = opendir(filepath);

    if (dir == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char fullpath[1024];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (stat(fullpath, &info) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(info.st_mode)) {
            remove_file(fullpath); // 재귀 호출
        } else {
            if (remove(fullpath) == -1) {
                perror("remove");
            }
        }
    }

    closedir(dir);

    if (rmdir(filepath) == -1) {
        perror("rmdir");
    }
}

// 재귀적으로 파일 및 폴더 이동
void move_file(const char* destination, const char* filepath) {
    struct stat info;
    if (stat(filepath, &info) == -1) {
        perror("stat");
        return;
    }

    // 폴더인 경우
    if (S_ISDIR(info.st_mode)) {
        DIR* dir = opendir(filepath);
        if (dir == NULL) {
            perror("opendir");
            return;
        }

        mkdir(destination, 0777);  // 새 경로에 폴더 생성

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char srcPath[1024], destPath[1024];

            move_file(destPath, srcPath);  // 재귀 호출
        }

        closedir(dir);
        rmdir(filepath);
    } else {
        rename(filepath, destination);  // 파일 이동
    }
}

// 재귀적으로 파일 및 폴더 복사
void cp_file(const char* destination, const char* filepath) {
    struct stat info;
    if (stat(filepath, &info) == -1) {
        perror("stat");
        return;
    }
    // 폴더인 경우
    if (S_ISDIR(info.st_mode)) {
        DIR* dir = opendir(filepath);
        if (dir == NULL) {
            perror("opendir");
            return;
        }

        mkdir(destination, 0777);  // 새 경로에 폴더 생성

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char srcPath[1024], destPath[1024];

            cp_file(destPath, srcPath);  // 재귀 호출
        }

        closedir(dir);
    } else {
        FILE *src, *dest;
        src = fopen(filepath, "rb");
        dest = fopen(destination, "wb");

        if (src == NULL || dest == NULL) {
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

void get_current_directory(char* absolute_path) {
    struct stat current_stat, parent_stat;
    DIR* dir;
    struct dirent* entry;

    char saved_dir[PATH_MAX]; // 현재 디렉터리를 저장
    if (getcwd(saved_dir, sizeof(saved_dir)) == NULL) {
        perror("Error saving current directory");
        return;
    }

    // 현재 디렉터리의 inode 정보 가져오기
    if (stat(".", &current_stat) != 0) {
        perror("Error getting current directory info");
        return;
    }

    // 상위 디렉터리로 이동
    if (chdir("..") != 0) {
        perror("Error changing to parent directory");
        return;
    }

    // 상위 디렉터리의 inode 정보 가져오기
    if (stat(".", &parent_stat) != 0) {
        perror("Error getting parent directory info");
        chdir(saved_dir); // 경로 복구
        return;
    }

    // 현재 디렉터리가 루트 디렉터리인지 확인
    if (current_stat.st_ino == parent_stat.st_ino && current_stat.st_dev == parent_stat.st_dev) {
        strcpy(absolute_path, "/");
        chdir(saved_dir); // 경로 복구
        return;
    }

    // 상위 디렉터리에서 현재 디렉터리 이름 찾기
    dir = opendir(".");
    if (dir == NULL) {
        perror("Error opening parent directory");
        chdir(saved_dir); // 경로 복구
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        struct stat entry_stat;
        if (stat(entry->d_name, &entry_stat) != 0)
            continue;
        if (entry_stat.st_ino == current_stat.st_ino && entry_stat.st_dev == current_stat.st_dev) {
            char parent_path[PATH_MAX] = {0};

            // 재귀적으로 부모 디렉터리 경로를 구함
            get_current_directory(parent_path);

            // 절대 경로 조합
            if (strcmp(parent_path, "/") == 0) {
                strcat(absolute_path, "/");
                strcat(absolute_path, entry->d_name);
            } else {
                strcat(absolute_path, parent_path);
                strcat(absolute_path, "/");
                strcat(absolute_path, entry->d_name);
            }

            closedir(dir);
            chdir(saved_dir); // 경로 복구
            return;
        }
    }

    closedir(dir);
    chdir(saved_dir); // 경로 복구
    fprintf(stderr, "Error: Could not resolve directory path.\n");
}
void resolve_absolute_path(char* absolute_path, const char* filename) {
    char current_dir[PATH_MAX] = {0};
    char full_path[PATH_MAX] = {0};

    // 현재 디렉터리를 구함
    get_current_directory(current_dir);

    // 파일 경로 조합
    snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, filename);

    // 파일이 존재하는지 확인
    if (access(full_path, F_OK) == 0) {
        strcpy(absolute_path, full_path);
    } else {
        fprintf(stderr, "File not found: %s\n", filename);
        absolute_path[0] = '\0';
    }
}
