#include <ncurses.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include "display.h"

int show_hidden_files = 0;          // 숨김 파일 표시 여부
int preview_scroll_offset = 0;      // 미리보기 창 scroll offset



// highlight : 현재 선택된 항목 -> files 배열에서 현재 선택된 파일의 인덱스
// 파일 목록을 표시하는 함수 , scroll_offset : 스크롤 위치 조정 -> 시작 인덱스 조절
void display_files(WINDOW *win, char *files[], int file_count, int highlight, int scroll_offset) {
    werase(win);    //창 지우기
    box(win, 0, 0); // 창 테두리 그리기


    // 좌측 창 최상단 바(이름, 사이즈, 수정시간) 배경색 설정
    wattron(win, COLOR_PAIR(8)); // 헤더 배경색 설정
    mvwprintw(win, 1, 1, "Name");
    //공백 배경색 처리
    for(int i=2;i<37;i++)
        mvwprintw(win,1,i," ");
    mvwprintw(win, 1, 37, "Size"); // Size 위치 조정
    //공백 배경색 처리
    for(int i=38;i<getmaxx(win)-20;i++)
        mvwprintw(win,1,i," ");
   
    mvwprintw(win, 1, getmaxx(win) - 20, "Modify time"); // Modify time 위치 오른쪽 정렬
    wattroff(win, COLOR_PAIR(8)); // 헤더 배경색 해제

    int max_display = getmaxy(win) - 3;

    // 파일 목록 출력
    for (int i = 0; i < max_display && i + scroll_offset < file_count; i++) {
        int index = i + scroll_offset;
        int line_width = getmaxx(win) - 2;

        // 선택된 항목의 배경색을 변경
        if (index == highlight) {
            wattron(win, COLOR_PAIR(7));
            for (int j = 0; j < line_width; j++) {
                mvwaddch(win, i + 2, j + 1, ' ');
            }
            wattroff(win, COLOR_PAIR(7));
        }

        struct stat file_stat;
        //파일 정보 가져오기
        if (stat(files[index], &file_stat) == 0) {
            char mod_time[20];
            strftime(mod_time, 20, "%Y-%m-%d %H:%M", localtime(&file_stat.st_mtime));

            if (index == highlight) {
                wattron(win, COLOR_PAIR(7));    //선택 항목 배경색 변경
            } else {
                if (S_ISDIR(file_stat.st_mode)) {
                    wattron(win, COLOR_PAIR(2));    // 디렉터리 색상
                } else if (file_stat.st_mode & S_IXUSR) {
                    wattron(win, COLOR_PAIR(4));    // binary file 색상
                } else {
                    wattron(win, COLOR_PAIR(3));    // 일반 파일 색상
                }
            }

            // 파일명, 사이즈, 수정 시간 출력 위치 조정
            mvwprintw(win, i + 2, 1, "%-25s", files[index]);                   // 파일 이름
            mvwprintw(win, i + 2, 37, "%10lld bytes", (long long)file_stat.st_size); // 파일 크기 위치 조정
            mvwprintw(win, i + 2, getmaxx(win) - 20, "%s", mod_time);             // 수정 시간 오른쪽 끝에 배치

            if (index == highlight) {
                wattroff(win, COLOR_PAIR(7));
            } else {
                wattroff(win, COLOR_PAIR(2) | COLOR_PAIR(3) | COLOR_PAIR(4));
            }
        }
    }
    wrefresh(win);  //창 업데이트
}


// 디렉토리 파일 목록 로드 함수
int load_files(char *files[]) {
    DIR *dir;
    struct dirent *entry;
    int file_count = 0;

    dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        if (!show_hidden_files && entry->d_name[0] == '.') {
            continue;
        }
        files[file_count] = strdup(entry->d_name);
        file_count++;
    }
    closedir(dir);

    return file_count;
}

// 파일 미리보기 기능
void display_preview(WINDOW *preview_win, const char *filename) {
    werase(preview_win);
    box(preview_win, 0, 0);

    struct stat file_stat;
    if (stat(filename, &file_stat) == 0) {

        if (S_ISDIR(file_stat.st_mode)) {   // 디렉터리 미리보기
            DIR *dir = opendir(filename);
            if (dir) {  
                struct dirent *entry;
                int line_num = 1;
                mvwprintw(preview_win, 1, 1, "[Directory Contents]");
                while ((entry = readdir(dir)) != NULL && line_num < getmaxy(preview_win) - 2) {
                    if (entry->d_name[0] == '.' && !show_hidden_files) continue;
                    mvwprintw(preview_win, ++line_num, 1, "%s", entry->d_name);
                }
                closedir(dir);
            } else {
                mvwprintw(preview_win, 1, 1, "Cannot open directory.");
            }
        } else {    //디렉터리 아닐 떄
            FILE *file = fopen(filename, "r");
            if (file != NULL) {
                char line[PREVIEW_WIDTH - 2];
                int line_num = 1 + preview_scroll_offset;
                while (fgets(line, sizeof(line), file) != NULL && line_num < getmaxy(preview_win) - 2) {
                    int line_length = strlen(line);
                    if (line[line_length - 1] == '\n') line[line_length - 1] = '\0';

                    int x = 1;
                    for (int i = 0; i < line_length; i++) {
                        if (x >= PREVIEW_WIDTH - 1) {
                            x = 1;
                            line_num++;
                        }
                        mvwaddch(preview_win, line_num, x++, line[i]);
                    }
                    line_num++;
                }
                fclose(file);
            } else {
                mvwprintw(preview_win, 1, 1, "Cannot open file.");
            }
        }
    }
    wrefresh(preview_win);
}

// 경로 표시
void display_path(WINDOW *path_win) {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    werase(path_win);
    box(path_win, 0, 0);
    mvwprintw(path_win, 1, 1, "Current Path: %s", cwd);
    wrefresh(path_win);
}
