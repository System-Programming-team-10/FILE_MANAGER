#include <ncurses.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include "display.h"
#include "file.h"
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
    for(int i=5;i<37;i++)
        mvwprintw(win,1,i," ");
    mvwprintw(win, 1, 37, "Size"); // Size 위치 조정
    //공백 배경색 처리
    for(int i=41;i<getmaxx(win)-20;i++)
        mvwprintw(win,1,i," ");
   
    mvwprintw(win, 1, getmaxx(win) - 20, "Modify time"); // Modify time 위치 오른쪽 정렬
    for(int i=getmaxx(win)-9;i<getmaxx(win);i++) {
        mvwprintw(win,1,i," ");
    }
    wattroff(win, COLOR_PAIR(8)); // 헤더 배경색 해제

    int max_display = getmaxy(win) - 3;

       // 파일 목록 출력
    display_ls_file(win, files, file_count, highlight, scroll_offset,max_display); //함수최적화
    wrefresh(win);  //창 업데이트
}


void display_ls_file(WINDOW *win, char *files[], int file_count, int highlight, int scroll_offset, int max_display)
{
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
            mvwprintw(win, i + 2, 32, "%10lld bytes", (long long)file_stat.st_size); // 파일 크기 위치 조정
            mvwprintw(win, i + 2, getmaxx(win) - 20, "%s", mod_time);             // 수정 시간 오른쪽 끝에 배치

            if (index == highlight) {
                wattroff(win, COLOR_PAIR(7));
            } else {
                wattroff(win, COLOR_PAIR(2) | COLOR_PAIR(3) | COLOR_PAIR(4));
            }
        }
    }
}

// 디렉토리 파일 목록 로드 함수
int load_files(char *files[],WINDOW* preview_win) {
    DIR *dir;
    struct dirent *entry;
    int file_count = 0;

    dir = opendir(".");
    if (dir == NULL) {
        mvwprintw(preview_win, 1, 1, "Can not opendir: current working directory");
        wrefresh(preview_win);
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
// 파일 미리보기 기능 -> display_ls_files를 사용해서 바꾸고 안에 있는 케이스 바꿔야 됨
void display_preview(WINDOW *preview_win, const char *filename) {
    werase(preview_win);
    box(preview_win, 0, 0);

    struct stat file_stat;
    if (stat(filename, &file_stat) == 0) {

        if (S_ISDIR(file_stat.st_mode)) {   // 디렉터리 미리보기
            do_dir(preview_win,filename);  
        } else {    // 파일 미리보기 (텍스트 또는 바이너리)
            do_file(preview_win, filename);
        }
    }
    wrefresh(preview_win);
}

void do_dir(WINDOW *preview_win,const char *filename)
{
    DIR *dir = opendir(filename);
            if (dir) {  
                struct dirent *entry;
                int line_num = 1;

                wattron(preview_win, COLOR_PAIR(5)); // 헤더(title) 배경색 설정
                mvwprintw(preview_win, 1, 1, "[Directory: %s]", filename);
                wattroff(preview_win, COLOR_PAIR(5)); // 헤더 배경색 해제

                while ((entry = readdir(dir)) != NULL && line_num < getmaxy(preview_win) - 2) {
                    if (entry->d_name[0] == '.' && !show_hidden_files) continue;

                    // 파일의 전체 경로를 구성하여 파일 타입을 확인
                    char full_path[PATH_MAX];
                    snprintf(full_path, sizeof(full_path), "%s/%s", filename, entry->d_name);
                    struct stat entry_stat;
                    stat(full_path, &entry_stat);

                    // 파일 유형에 따라 색상 설정
                    if (S_ISDIR(entry_stat.st_mode)) {
                        wattron(preview_win, COLOR_PAIR(2)); // 디렉터리 색상
                    } else if (entry_stat.st_mode & S_IXUSR) {
                        wattron(preview_win, COLOR_PAIR(4)); // 실행 파일 색상
                    } else {
                        wattron(preview_win, COLOR_PAIR(3)); // 일반 파일 색상
                    }

                    // 파일 이름 출력
                    mvwprintw(preview_win, ++line_num, 1, "  %s", entry->d_name);

                    // 색상 해제
                    wattroff(preview_win, COLOR_PAIR(2) | COLOR_PAIR(3) | COLOR_PAIR(4));
                }
                closedir(dir);
            } else {
                mvwprintw(preview_win, 1, 1, "Cannot opendir: %s", filename);
            }
}

void do_file(WINDOW *preview_win,const char *filename)
{
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
    }else {
        mvwprintw(preview_win, 1, 1, "Cannot open file.");
    }
}

void more(WINDOW *preview_win, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        mvwprintw(preview_win, 1, 1, "Cannot fopen: %s",filename);
        wrefresh(preview_win);
        return;
    }

    int row, col;
    getmaxyx(preview_win, row, col); // 창 크기 가져오기
    col -= 2; // 좌우 여백 감안
    row -= 2; // 상하 여백 감안
    char line[col + 1];
    long current_offset = 0; // 현재 오프셋
    long prev_offset = 0;   // 이전 오프셋
    FILE *fp_tty = fopen("/dev/tty", "r"); // for 사용자 입력
    if (fp_tty == NULL) {
        mvwprintw(preview_win, 1, 1, "Cannot open: /dev/tty");
        wrefresh(preview_win);
        fclose(file);
        exit(1);
    }

    while (1) {
        // 창을 지우고 현재 줄부터 출력
        werase(preview_win);
        box(preview_win, 0, 0);
        wattron(preview_win, COLOR_PAIR(9));
        box(preview_win, 0, 0);
        wattroff(preview_win, COLOR_PAIR(9));

        // 파일 포인터를 현재 오프셋으로 이동
        fseek(file, current_offset, SEEK_SET);

        int lines_displayed = 0; // 현재 화면에 출력된 줄 수
        int first_line_length = 0; // 첫 번째 줄의 크기 저장
        while (lines_displayed < row && fgets(line, sizeof(line), file) != NULL) {
            mvwprintw(preview_win, lines_displayed + 1, 1, "%s", line);
            if (lines_displayed == 0) { // 첫 번째 줄에서 크기 계산 -> reply = 1인 경우, 첫번째 줄 지우기 위함
                first_line_length = strlen(line);
            }
            lines_displayed++;
        }

        // 파일 끝에 도달한 경우 종료
        if (feof(file)) {
            break;
        }

        wrefresh(preview_win);

        // 사용자 입력 처리
        int reply = see_more(fp_tty, preview_win, row, col);
        if (reply == 0) { // 종료
            break;
        } else if (reply == row) { // 한 페이지
            prev_offset = current_offset;
            current_offset = ftell(file); // 파일 포인터 이동
        } else if (reply == 1) { // 한 줄
            current_offset = prev_offset + first_line_length; // 첫 번째 줄의 길이를 기준으로 한 줄 뒤로 이동
            prev_offset = current_offset;
        }
    }

    fclose(fp_tty);
    fclose(file);
}

int see_more(FILE* file, WINDOW* preview_win, int row, int col)
{
    char c = 0;
    while ((c = getc(file)) != EOF) {
        if (c == '\t') // 종료
            return 0;
        else if (c == ' ') // 한 페이지 출력
            return 1;
        else if (c == '\r') // 한 줄 출력
            return row;
        else
            continue;
    }
    return 0;
}

// 경로 표시
void display_path(WINDOW *path_win, WINDOW* preview_win) {
    char cwd[PATH_MAX];
    memset(cwd, 0, PATH_MAX); 
    get_current_directory(cwd, PATH_MAX, preview_win);
    werase(path_win);
    box(path_win, 0, 0);
    mvwprintw(path_win, 1, 1, "Current Path: %s", cwd);
    wrefresh(path_win);
}