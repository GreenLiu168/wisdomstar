/**
 * Raspberry Pi Camera Info Display with UI
 * 
 * 此程序在 Raspberry Pi Linux 環境下使用 C 語言 (通過 C++ OpenCV)
 * 透過 NCurses UI 顯示攝像頭資訊和實時預覽
 * 
 * 編譯指令:
 *   gcc -o camera_info camera_info.c `pkg-config --cflags --libs opencv4` -lncurses -lstdc++
 * 
 * 依賴安裝:
 *   sudo apt-get update
 *   sudo apt-get install libopencv-dev libncurses5-dev pkg-config
 *   sudo apt-get install libcamera-dev (如果需要 libcamera 支持)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#define MAX_CAMERAS 10
#define INFO_LINES 20

// 攝像頭資訊結構體
typedef struct {
    int device_id;
    char device_path[64];
    char driver_name[32];
    char card_name[64];
    char bus_info[64];
    char version[16];
    int width;
    int height;
    int fps;
    char formats[256];
} CameraInfo;

// 全局變量
CameraInfo cameras[MAX_CAMERAS];
int camera_count = 0;
cv::VideoCapture cap;
int current_camera = 0;
bool show_preview = true;

// 顏色對定義
#define COLOR_HEADER 1
#define COLOR_INFO 2
#define COLOR_WARNING 3
#define COLOR_SUCCESS 4

/**
 * 初始化 NCurses UI
 */
void init_ui() {
    initscr();
    clear();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    // 檢查並初始化顏色
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(COLOR_HEADER, COLOR_CYAN, -1);
        init_pair(COLOR_INFO, COLOR_WHITE, -1);
        init_pair(COLOR_WARNING, COLOR_YELLOW, -1);
        init_pair(COLOR_SUCCESS, COLOR_GREEN, -1);
    }
}

/**
 * 結束 NCurses UI
 */
void end_ui() {
    endwin();
}

/**
 * 獲取攝像頭的詳細資訊
 */
int get_camera_info(const char* device_path, CameraInfo* info) {
    int fd;
    struct v4l2_capability cap;
    struct v4l2_input input;
    struct v4l2_fmtdesc fmt;
    struct v4l2_frmsizeenum frmsize;
    struct v4l2_frmivalenum frmival;
    
    fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // 獲取基本能力
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        close(fd);
        return -1;
    }
    
    strncpy(info->driver_name, (char*)cap.driver, sizeof(info->driver_name) - 1);
    info->driver_name[sizeof(info->driver_name) - 1] = '\0';
    
    strncpy(info->card_name, (char*)cap.card, sizeof(info->card_name) - 1);
    info->card_name[sizeof(info->card_name) - 1] = '\0';
    
    strncpy(info->bus_info, (char*)cap.bus_info, sizeof(info->bus_info) - 1);
    info->bus_info[sizeof(info->bus_info) - 1] = '\0';
    
    snprintf(info->version, sizeof(info->version), "%d.%d.%d",
             (cap.version >> 16) & 0xFF,
             (cap.version >> 8) & 0xFF,
             cap.version & 0xFF);
    
    // 獲取支持的格式
    char formats[512] = "";
    fmt.index = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int format_count = 0;
    
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0) {
        if (format_count > 0) {
            strncat(formats, ", ", sizeof(formats) - strlen(formats) - 1);
        }
        strncat(formats, (char*)fmt.description, sizeof(formats) - strlen(formats) - 1);
        fmt.index++;
        format_count++;
        if (format_count >= 5) break; // 限制顯示的格式數量
    }
    
    if (format_count == 0) {
        strcpy(formats, "Unknown");
    } else if (format_count > 5) {
        strncat(formats, "...", sizeof(formats) - strlen(formats) - 1);
    }
    
    strncpy(info->formats, formats, sizeof(info->formats) - 1);
    info->formats[sizeof(info->formats) - 1] = '\0';
    
    // 嘗試打開攝像頭獲取分辨率和 FPS
    cv::VideoCapture test_cap(device_path);
    if (test_cap.isOpened()) {
        info->width = (int)test_cap.get(cv::CAP_PROP_FRAME_WIDTH);
        info->height = (int)test_cap.get(cv::CAP_PROP_FRAME_HEIGHT);
        info->fps = (int)test_cap.get(cv::CAP_PROP_FPS);
        test_cap.release();
    } else {
        info->width = 0;
        info->height = 0;
        info->fps = 0;
    }
    
    close(fd);
    return 0;
}

/**
 * 掃描系統中的攝像頭設備
 */
int scan_cameras() {
    camera_count = 0;
    
    // 檢查常見的攝像頭設備路徑
    const char* device_paths[] = {
        "/dev/video0",
        "/dev/video1",
        "/dev/video2",
        "/dev/video3",
        "/dev/video4",
        "/dev/video5",
        "/dev/video6",
        "/dev/video7",
        NULL
    };
    
    for (int i = 0; device_paths[i] != NULL && camera_count < MAX_CAMERAS; i++) {
        if (access(device_paths[i], F_OK) == 0) {
            cameras[camera_count].device_id = camera_count;
            strncpy(cameras[camera_count].device_path, device_paths[i], 
                    sizeof(cameras[camera_count].device_path) - 1);
            
            if (get_camera_info(device_paths[i], &cameras[camera_count]) == 0) {
                camera_count++;
            }
        }
    }
    
    return camera_count;
}

/**
 * 顯示主界面
 */
void display_ui() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    
    clear();
    
    // 繪製邊框
    box(stdscr, 0, 0);
    
    // 標題
    attron(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvprintw(0, (cols - 40) / 2, " Raspberry Pi Camera Info ");
    attroff(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    
    // 分隔線
    mvhline(2, 1, ACS_HLINE, cols - 2);
    
    // 攝像頭列表
    attron(A_BOLD);
    mvprintw(3, 2, "Available Cameras: %d", camera_count);
    attroff(A_BOLD);
    
    for (int i = 0; i < camera_count; i++) {
        if (i == current_camera) {
            attron(A_REVERSE);
        }
        mvprintw(5 + i, 4, "[%d] %s - %s", 
                 i, 
                 cameras[i].device_path,
                 cameras[i].card_name);
        if (i == current_camera) {
            attroff(A_REVERSE);
        }
    }
    
    // 分隔線
    if (camera_count > 0) {
        mvhline(5 + camera_count + 1, 1, ACS_HLINE, cols - 2);
        
        // 當前選中攝像頭的詳細資訊
        CameraInfo* cam = &cameras[current_camera];
        
        int info_start = 5 + camera_count + 3;
        attron(A_BOLD);
        mvprintw(info_start, 2, "Camera Details:");
        attroff(A_BOLD);
        
        attron(COLOR_PAIR(COLOR_INFO));
        mvprintw(info_start + 1, 4, "Device Path : %s", cam->device_path);
        mvprintw(info_start + 2, 4, "Driver      : %s", cam->driver_name);
        mvprintw(info_start + 3, 4, "Card Name   : %s", cam->card_name);
        mvprintw(info_start + 4, 4, "Bus Info    : %s", cam->bus_info);
        mvprintw(info_start + 5, 4, "Version     : %s", cam->version);
        mvprintw(info_start + 6, 4, "Resolution  : %dx%d", cam->width, cam->height);
        mvprintw(info_start + 7, 4, "FPS         : %d", cam->fps);
        
        // 自動換行顯示格式資訊
        mvprintw(info_start + 8, 4, "Formats     : ");
        int format_line = info_start + 8;
        int format_col = 16;
        char* format_ptr = cam->formats;
        int len = strlen(format_ptr);
        int max_col = cols - 6;
        
        while (*format_ptr) {
            if (format_col >= max_col) {
                format_col = 16;
                format_line++;
                mvprintw(format_line, 4, "                ");
            }
            addch(*format_ptr);
            format_ptr++;
            format_col++;
        }
        
        attroff(COLOR_PAIR(COLOR_INFO));
        
        // 預覽窗口提示
        if (show_preview && cap.isOpened()) {
            attron(COLOR_PAIR(COLOR_SUCCESS));
            mvprintw(rows - 4, 2, "[P] Toggle Preview | [Q] Quit | Preview: ON");
            attroff(COLOR_PAIR(COLOR_SUCCESS));
            
            // 在右下角顯示小預覽（文本模式）
            // 注意：實際的視頻預覽需要在單獨的窗口或使用 OpenCV 的 GUI
        } else {
            attron(COLOR_PAIR(COLOR_WARNING));
            mvprintw(rows - 4, 2, "[P] Toggle Preview | [Q] Quit | Preview: OFF");
            attroff(COLOR_PAIR(COLOR_WARNING));
        }
    } else {
        attron(COLOR_PAIR(COLOR_WARNING));
        mvprintw(5, 4, "No cameras found!");
        mvprintw(6, 4, "Please connect a camera and try again.");
        attroff(COLOR_PAIR(COLOR_WARNING));
        
        mvprintw(rows - 4, 2, "[R] Rescan | [Q] Quit");
    }
    
    // 底部說明
    mvhline(rows - 2, 1, ACS_HLINE, cols - 2);
    attron(A_DIM);
    mvprintw(rows - 1, 2, "↑↓: Select Camera | R: Rescan | P: Preview | Q: Quit");
    attroff(A_DIM);
    
    refresh();
}

/**
 * 顯示視頻預覽（使用 OpenCV）
 */
void show_video_preview() {
    if (!cap.isOpened()) {
        return;
    }
    
    cv::Mat frame;
    char key = 0;
    
    // 創建一個命名窗口
    cv::namedWindow("Camera Preview", cv::WINDOW_AUTOSIZE);
    
    while (key != 'q' && key != 27) { // ESC
        cap >> frame;
        
        if (!frame.empty()) {
            // 在幀上顯示資訊
            char info[128];
            snprintf(info, sizeof(info), "Cam %d: %s", 
                     current_camera, 
                     cameras[current_camera].card_name);
            cv::putText(frame, info, cv::Point(10, 30),
                       cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
            
            cv::imshow("Camera Preview", frame);
        }
        
        // 等待 10ms 檢查按鍵
        key = cv::waitKey(10);
        
        // 如果按下 'q' 或 ESC，退出預覽
        if (key == 'q' || key == 27) {
            break;
        }
    }
    
    cv::destroyWindow("Camera Preview");
}

/**
 * 打開選定的攝像頭
 */
bool open_camera(int index) {
    if (index < 0 || index >= camera_count) {
        return false;
    }
    
    if (cap.isOpened()) {
        cap.release();
    }
    
    cap.open(cameras[index].device_path);
    return cap.isOpened();
}

/**
 * 主函數
 */
int main(int argc, char* argv[]) {
    int ch;
    
    printf("Raspberry Pi Camera Info Display\n");
    printf("================================\n\n");
    
    // 初始化 UI
    init_ui();
    
    // 掃描攝像頭
    scan_cameras();
    
    // 如果有攝像頭，打開第一個
    if (camera_count > 0) {
        open_camera(current_camera);
    }
    
    // 主循環
    while (1) {
        display_ui();
        
        ch = getch();
        
        switch (ch) {
            case KEY_UP:
            case 'k':
            case 'K':
                if (current_camera > 0) {
                    current_camera--;
                    if (cap.isOpened()) {
                        cap.release();
                    }
                    open_camera(current_camera);
                }
                break;
                
            case KEY_DOWN:
            case 'j':
            case 'J':
                if (current_camera < camera_count - 1) {
                    current_camera++;
                    if (cap.isOpened()) {
                        cap.release();
                    }
                    open_camera(current_camera);
                }
                break;
                
            case 'r':
            case 'R':
                // 重新掃描攝像頭
                if (cap.isOpened()) {
                    cap.release();
                }
                scan_cameras();
                if (camera_count > 0) {
                    current_camera = 0;
                    open_camera(current_camera);
                }
                break;
                
            case 'p':
            case 'P':
                // 切換預覽
                if (cap.isOpened()) {
                    end_ui(); // 暫時關閉 ncurses
                    show_video_preview();
                    init_ui(); // 重新初始化 ncurses
                } else {
                    beep();
                }
                break;
                
            case 'q':
            case 'Q':
                // 退出
                if (cap.isOpened()) {
                    cap.release();
                }
                end_ui();
                printf("\nExiting...\n");
                return 0;
                
            default:
                break;
        }
    }
    
    // 清理
    if (cap.isOpened()) {
        cap.release();
    }
    end_ui();
    
    return 0;
}
