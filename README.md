# Raspberry Pi Camera Info Display

在 Raspberry Pi Linux 環境下使用 C 語言透過 UI 顯示攝像頭資訊的程式。

## 功能特點

- 📷 **自動掃描攝像頭**: 檢測系統中所有可用的攝像頭設備 (`/dev/video*`)
- 📊 **詳細資訊顯示**: 顯示攝像頭的驅動、名稱、總線信息、版本等
- 🎬 **分辨率與 FPS**: 獲取並顯示支持的分辨率和幀率
- 🎨 **彩色 NCurses UI**: 美觀的終端界面，支持顏色和高亮
- ▶️ **實時預覽**: 使用 OpenCV 顯示攝像頭實時視頻預覽
- ⌨️ **鍵盤控制**: 支持上下鍵切換攝像頭、重新掃描、預覽等功能

## 系統要求

- Raspberry Pi (或其他 Linux 系統)
- Raspberry Pi Camera Module 或 USB 攝像頭
- Raspbian/Raspberry Pi OS 或其他 Linux 發行版

## 依賴安裝

### 方法 1: 使用 Makefile (推薦)

```bash
make install-deps
```

### 方法 2: 手動安裝

```bash
sudo apt-get update
sudo apt-get install libopencv-dev libncurses5-dev pkg-config
```

## 編譯

### 使用 Makefile

```bash
make
```

### 手動編譯

```bash
g++ -Wall -Wextra -O2 `pkg-config --cflags opencv4` \
    -o camera_info camera_info.c \
    `pkg-config --libs opencv4` -lncurses -lstdc++
```

如果 `pkg-config opencv4` 不可用，嘗試使用 `opencv`:

```bash
g++ -Wall -Wextra -O2 `pkg-config --cflags opencv` \
    -o camera_info camera_info.c \
    `pkg-config --libs opencv` -lncurses -lstdc++
```

## 使用方法

### 運行程序

```bash
./camera_info
```

**注意**: 可能需要 root 權限訪問攝像頭：

```bash
sudo ./camera_info
```

或者將用戶添加到 video 組：

```bash
sudo usermod -aG video $USER
# 然後重新登錄
```

### 鍵盤操作

| 按鍵 | 功能 |
|------|------|
| `↑` / `k` | 選擇上一個攝像頭 |
| `↓` / `j` | 選擇下一個攝像頭 |
| `R` | 重新掃描攝像頭 |
| `P` | 開啟/關閉視頻預覽 |
| `Q` | 退出程序 |

### 預覽模式

按下 `P` 鍵後會進入視頻預覽模式：
- 在彈出的窗口中查看實時視頻
- 視頻上會顯示當前攝像頭信息
- 按 `q` 或 `ESC` 返回主界面

## 程序界面示例

```
┌─────────────────────────────────────────┐
│     Raspberry Pi Camera Info            │
├─────────────────────────────────────────┤
│                                         │
│ Available Cameras: 1                    │
│                                         │
│  [0] /dev/video0 - imx219               │
│                                         │
├─────────────────────────────────────────┤
│                                         │
│ Camera Details:                         │
│   Device Path : /dev/video0             │
│   Driver      : unicam                  │
│   Card Name   : imx219                  │
│   Bus Info    : platform:fe801000.csi   │
│   Version     : 5.10                    │
│   Resolution  : 1920x1080               │
│   FPS         : 30                      │
│   Formats     : YU12, RGB3, BGR3, ...   │
│                                         │
│ [P] Toggle Preview | [Q] Quit | Prev:ON │
│                                         │
├─────────────────────────────────────────┤
│ ↑↓: Select Camera | R: Rescan | Q: Quit │
└─────────────────────────────────────────┘
```

## 支持的攝像頭

- Raspberry Pi Camera Module V1 (OV5647)
- Raspberry Pi Camera Module V2 (IMX219)
- Raspberry Pi High Quality Camera (IMX477)
- Raspberry Pi Camera Module 3 (IMX708)
- 大多數 USB UVC 兼容攝像頭

## 技術細節

### 使用的庫

- **OpenCV**: 視頻捕獲和處理
- **NCurses**: 終端 UI 渲染
- **V4L2 (Video4Linux2)**: Linux 視頻設備 API

### 架構

程序主要由以下模塊組成：

1. **攝像頭掃描**: 遍歷 `/dev/video*` 設備
2. **信息獲取**: 通過 V4L2 ioctl 獲取設備信息
3. **UI 渲染**: 使用 NCurses 繪製界面
4. **視頻預覽**: 使用 OpenCV 顯示實時視頻

### 數據結構

```c
typedef struct {
    int device_id;          // 設備 ID
    char device_path[64];   // 設備路徑 (/dev/video0)
    char driver_name[32];   // 驅動名稱
    char card_name[64];     // 攝像頭名稱
    char bus_info[64];      // 總線信息
    char version[16];       // 驅動版本
    int width;              // 分辨率寬度
    int height;             // 分辨率高度
    int fps;                // 幀率
    char formats[256];      // 支持的格式列表
} CameraInfo;
```

## 故障排除

### 問題：找不到攝像頭

**解決方案**:
1. 確認攝像頭已正確連接
2. 檢查設備是否存在：`ls -la /dev/video*`
3. 確認用戶有權限：`ls -l /dev/video0`
4. 將用戶加入 video 組：`sudo usermod -aG video $USER`

### 問題：編譯錯誤 "opencv2/opencv.hpp: No such file or directory"

**解決方案**:
```bash
sudo apt-get install libopencv-dev
```

### 問題：編譯錯誤 "ncurses.h: No such file or directory"

**解決方案**:
```bash
sudo apt-get install libncurses5-dev
```

### 問題：運行時提示 "Permission denied"

**解決方案**:
```bash
sudo ./camera_info
# 或
sudo usermod -aG video $USER
# 然後重新登錄
```

### 問題：預覽窗口無法打開

**解決方案**:
- 確保在 X11 環境或支持 GUI 的環境中運行
- 如果使用 SSH，需要啟用 X11 轉發：`ssh -X user@raspberrypi`
- 或設置 DISPLAY 變量：`export DISPLAY=:0`

## 許可證

本項目代碼可供自由使用和修改。

## 貢獻

歡迎提交問題報告和功能建議！

## 相關資源

- [Raspberry Pi 官方文檔](https://www.raspberrypi.org/documentation/)
- [OpenCV 文檔](https://docs.opencv.org/)
- [NCurses 編程指南](https://tldp.org/LDP/lpg/NCURSES-Programming-HOWTO.html)
- [V4L2 規範](https://linuxtv.org/downloads/v4l-dvb-apis/)
