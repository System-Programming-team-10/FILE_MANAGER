#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

// 파일 또는 디렉터리를 삭제하는 함수
void remove_file(const char* filepath) {
    struct dirent* entry; 
    struct stat info;
    DIR* dir = opendir(filepath); 

    if (dir == NULL) { // 디렉터리가 아니라 파일일 경우, unlink()로 삭제
        if (unlink(filepath) == -1) { 
            perror("unlink");
        }
        return;
    }

    // 디렉터리 내부 파일 및 하위 디렉터리를 순회
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
            remove_file(fullpath); // 하위 디렉터리를 재귀적으로 삭제
        } else { 
            if (unlink(fullpath) == -1) { 
                perror("unlink");
            }
        }
    }

    closedir(dir); // 디렉터리 닫기

    if (rmdir(filepath) == -1) { 
        perror("rmdir");
    }
}

// 파일 또는 디렉터리를 이동하는 함수
void move_file(const char* destination, const char* filepath) {
    if (rename(filepath, destination) == 0) { 
        // 파일 또는 디렉터리를 간단히 이동
        return;
    }

    struct stat info;
    if (stat(filepath, &info) == -1) { 
        perror("stat");
        return;
    }

    if (S_ISDIR(info.st_mode)) { 
        DIR* dir = opendir(filepath); 
        if (dir == NULL) { 
            perror("opendir");
            return;
        }

        // 이동 대상에 디렉터리 생성
        if (mkdir(destination, 0777) == -1 && errno != EEXIST) { 
            perror("mkdir");
            closedir(dir);
            return;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) { 
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { 
                continue;
            }

            char srcPath[1024], destPath[1024];
            snprintf(srcPath, sizeof(srcPath), "%s/%s", filepath, entry->d_name);
            snprintf(destPath, sizeof(destPath), "%s/%s", destination, entry->d_name);

            move_file(destPath, srcPath); // 하위 파일 및 디렉터리 재귀적으로 이동
        }

        closedir(dir); 
        rmdir(filepath); // 원본 디렉터리 삭제
    } else { 
        if (rename(filepath, destination) == -1) { 
            perror("rename");
        }
    }
}

// 파일 또는 디렉터리를 복사하는 함수
void cp_file(const char* destination, const char* filepath) {
    struct stat info;
    if (stat(filepath, &info) == -1) { 
        perror("stat");
        return;
    }

    if (S_ISDIR(info.st_mode)) { 
        DIR* dir = opendir(filepath); 
        if (dir == NULL) { 
            perror("opendir");
            return;
        }

        // 복사 대상에 디렉터리 생성
        if (stat(destination, &info) == -1) { 
            if (mkdir(destination, 0777) == -1) { 
                perror("mkdir");
                closedir(dir);
                return;
            }
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) { 
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { 
                continue;
            }

            char srcPath[1024], destPath[1024];
            snprintf(srcPath, sizeof(srcPath), "%s/%s", filepath, entry->d_name);
            snprintf(destPath, sizeof(destPath), "%s/%s", destination, entry->d_name);

            cp_file(destPath, srcPath); // 하위 파일 및 디렉터리 재귀적으로 복사
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

// 테스트를 위한 메인 함수
int main() {
    // 테스트를 위해 디렉터리와 파일 생성
    mkdir("testdir", 0777);
    FILE* f = fopen("testdir/testfile.txt", "w");
    fprintf(f, "Test content");
    fclose(f);

    const char* sourcePath = "testdir";
    const char* destinationPath = "destination";

    printf("Copying file or directory:\n");
    cp_file(destinationPath, sourcePath); // 디렉터리 복사

    printf("\nMoving file or directory:\n");
    move_file("new_destination", sourcePath); // 디렉터리 이동

    printf("\nDeleting file or directory:\n");
    remove_file(destinationPath); // 디렉터리 삭제

    return 0;
}
