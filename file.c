#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

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

        snprintf(fullpath, sizeof(fullpath), "%s/%s", filepath, entry->d_name);

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
            snprintf(srcPath, sizeof(srcPath), "%s/%s", filepath, entry->d_name);
            snprintf(destPath, sizeof(destPath), "%s/%s", destination, entry->d_name);

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
            snprintf(srcPath, sizeof(srcPath), "%s/%s", filepath, entry->d_name);
            snprintf(destPath, sizeof(destPath), "%s/%s", destination, entry->d_name);

            cp_file(destPath, srcPath);  // 재귀 호출
        }

        closedir(dir);
    } else {
        FILE *src, *dest;
        src = fopen(filepath, "rb");
        dest = fopen(destination, "wb");

        if (src == NULL || dest == NULL) {
            perror("File open error");
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
