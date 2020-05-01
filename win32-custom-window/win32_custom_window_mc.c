/*
** Win32 Custom Window Example Program
**  v1.1 - April 30th 2020
**  by Allen Webster allenwebster@4coder.net
**
** public domain example program
** NO WARRANTY IMPLIED; USE AT YOUR OWN RISK
**
** Minimal comments version
**
*/

#include <Windows.h>
#include <Windowsx.h>
#include <dwmapi.h>
#include <stdio.h>

////////////////////////////////

int caption_width = 30;
int border_width = 10;

////////////////////////////////

int  WindowIsActive(void);
void StopRunning(void);
void Minimize(HWND hwnd);
void ToggleMaximize(HWND hwnd);
void EmbeddedWidgetRect(RECT rect);

typedef enum{
    InputEventKind_MouseLeftPress,
    InputEventKind_MouseLeftRelease,
} Input_Event_Kind;

typedef struct Input_Event Input_Event;
struct Input_Event{
    Input_Event *next;
    Input_Event_Kind kind;
};

typedef struct Input Input;
struct Input{
    Input_Event *first_event;
    Input_Event *last_event;
    int mouse_x;
    int mouse_y;
    int left;
};

void UpdateAndRender(HWND hwnd, Input *input);

////////////////////////////////

int
HitTest(int x, int y, RECT rect){
    return((rect.left <= x && x < rect.right) && (rect.top <= y && y < rect.bottom));
}

////////////////////////////////

int keep_running = 0;
int window_is_active = 1;

int minimize_at_end_of_update = 0;
int toggle_maximize_at_end_of_update = 0;

int
WindowIsActive(void){
    return(window_is_active);
}
void
StopRunning(void){
    keep_running = 0;
}
void
Minimize(HWND hwnd){
    minimize_at_end_of_update = 1;
}
void
ToggleMaximize(HWND hwnd){
    toggle_maximize_at_end_of_update = 1;
}

#define MAX_EMBEDDED_WIDGETS 128
int embedded_widget_count = 0;
RECT embedded_widget_rect[MAX_EMBEDDED_WIDGETS];
void
EmbeddedWidgetRect(RECT rect){
    if (embedded_widget_count < MAX_EMBEDDED_WIDGETS){
        embedded_widget_rect[embedded_widget_count] = rect;
        embedded_widget_count += 1;
    }
}

////////////////////////////////

#define MAX_INPUT_EVENT_COUNT 128
int input_event_cursor = 0;
Input_Event input_event_memory[MAX_INPUT_EVENT_COUNT];
Input input;

Input_Event*
PushEvent(void){
    Input_Event *result = 0;
    if (input_event_cursor < MAX_INPUT_EVENT_COUNT){
        result = &input_event_memory[input_event_cursor];
        input_event_cursor += 1;
        if (input.first_event == 0){
            input.first_event = result;
        }
        if (input.last_event != 0){
            input.last_event->next = result;
        }
        input.last_event = result;
    }
    return(result);
}

LRESULT
CustomBorderWindowProc(HWND   hwnd,
                       UINT   uMsg,
                       WPARAM wParam,
                       LPARAM lParam){
    LRESULT result = 0;
    switch (uMsg){
        case WM_NCCALCSIZE:
        {
            RECT* r = (RECT*)lParam;
            if (IsZoomed(hwnd)){
                int x_push_in = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                int y_push_in = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                r->top    += x_push_in;
                r->left   += y_push_in;
                r->right  -= x_push_in;
                r->bottom -= y_push_in;
            }
        }break;
        
        case WM_NCACTIVATE:
        {
            result = 1;
            window_is_active = wParam;
            if (IsIconic(hwnd)){
                result = DefWindowProcW(hwnd, uMsg, wParam, -1);
            }
        }break;
        
        case WM_NCHITTEST:
        {
            POINT pos;
            pos.x = GET_X_LPARAM(lParam);
            pos.y = GET_Y_LPARAM(lParam);
            RECT frame_rect;
            GetWindowRect(hwnd, &frame_rect);
            
            if (!HitTest(pos.x, pos.y, frame_rect)){
                result = HTNOWHERE;
            }
            
            else{
                
                // Resize
                int l = 0;
                int r = 0;
                int b = 0;
                int t = 0;
                if (!IsZoomed(hwnd)){
                    if (frame_rect.left <= pos.x && pos.x < frame_rect.left + border_width){
                        l = 1;
                    }
                    if (frame_rect.right - border_width <= pos.x && pos.x < frame_rect.right){
                        r = 1;
                    }
                    if (frame_rect.bottom - border_width <= pos.y && pos.y < frame_rect.bottom){
                        b = 1;
                    }
                    if (frame_rect.top <= pos.y && pos.y < frame_rect.top + border_width){
                        t = 1;
                    }
                }
                if (l){
                    if (t){
                        result = HTTOPLEFT;
                    }
                    else if (b){
                        result = HTBOTTOMLEFT;
                    }
                    else{
                        result = HTLEFT;
                    }
                }
                else if (r){
                    if (t){
                        result = HTTOPRIGHT;
                    }
                    else if (b){
                        result = HTBOTTOMRIGHT;
                    }
                    else{
                        result = HTRIGHT;
                    }
                }
                else if (t){
                    result = HTTOP;
                }
                else if (b){
                    result = HTBOTTOM;
                }
                
                // Inside
                else{
                    if (frame_rect.top <= pos.y && pos.y < frame_rect.top + caption_width){
                        result = HTCAPTION;
                        ScreenToClient(hwnd, &pos);
                        for (int i = 0; i < embedded_widget_count; i += 1){
                            if (HitTest(pos.x, pos.y, embedded_widget_rect[i])){
                                result = HTCLIENT;
                                break;
                            }
                        }
                    }
                    else{
                        result = HTCLIENT;
                    }
                }
            }
            
        }break;
        
        case WM_CLOSE:
        {
            keep_running = 0;
        }break;
        
        case WM_SIZING:
        {
            InvalidateRect(hwnd, 0, 0);
        }break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            embedded_widget_count = 0;
            UpdateAndRender(hwnd, 0);
            EndPaint(hwnd, &ps);
        }break;
        
        case WM_LBUTTONDOWN:
        {
            Input_Event *event = PushEvent();
            if (event != 0){
                event->kind = InputEventKind_MouseLeftPress;
            }
        }break;
        
        case WM_LBUTTONUP:
        {
            Input_Event *event = PushEvent();
            if (event != 0){
                event->kind = InputEventKind_MouseLeftRelease;
            }
        }break;
        
        default:
        {
            result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }break;
    }
    return(result);
}

int
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR     lpCmdLine,
        int       nShowCmd){
    
#define WINDOW_CLASS L"MainWindow"
    
    WNDCLASSW window_class = {0};
    window_class.style         = 0;
    window_class.lpfnWndProc   = CustomBorderWindowProc;
    window_class.hInstance     = hInstance;
    window_class.hCursor       = LoadCursorA(0, IDC_ARROW);
    window_class.lpszClassName = WINDOW_CLASS;
    
    ATOM atom = RegisterClassW(&window_class);
    if (atom == 0){
        fprintf(stderr, "RegisterClassW failed\n");
        exit(1);
    }
    
    DWORD style = WS_OVERLAPPEDWINDOW;
    HWND hwnd = CreateWindowW(WINDOW_CLASS,
                              L"Window Name",
                              style,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              0,
                              0,
                              hInstance,
                              0);
    if (hwnd == 0){
        fprintf(stderr, "CreateWindowExW failed\n");
        exit(1);
    }
    
    SetWindowTheme(hwnd, L" ", L" ");
    
    BOOL composition_enabled;
    if (DwmIsCompositionEnabled(&composition_enabled) != S_OK){
        fprintf(stderr, "DwmIsCompositionEnabled failed\n");
        composition_enabled = 0;
    }
    fprintf(stdout, "Has compositor? %s\n", composition_enabled?"Yes":"No");
    
    ShowWindow(hwnd, SW_SHOW);
    
    keep_running = 1;
    for (;keep_running;){
        input.first_event = 0;
        input.last_event = 0;
        input_event_cursor = 0;
        
        MSG msg;
        
        if (minimize_at_end_of_update){
            ShowWindow(hwnd, SW_MINIMIZE);
        }
        else if (toggle_maximize_at_end_of_update){
            if (IsZoomed(hwnd)){
                ShowWindow(hwnd, SW_RESTORE);
            }
            else{
                ShowWindow(hwnd, SW_MAXIMIZE);
            }
        }
        else{
            GetMessage(&msg, 0, 0, 0);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        for (;PeekMessageW(&msg, 0, 0, 0, PM_REMOVE);){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        POINT mpos;
        GetCursorPos(&mpos);
        ScreenToClient(hwnd, &mpos);
        input.mouse_x = mpos.x;
        input.mouse_y = mpos.y;
        input.left = (GetKeyState(VK_LBUTTON)&(1 << 15));
        
        minimize_at_end_of_update = 0;
        toggle_maximize_at_end_of_update = 0;
        embedded_widget_count = 0;
        UpdateAndRender(hwnd, &input);
        
        if (composition_enabled){
            DwmFlush();
        }
        else{
            Sleep(33);
        }
    }
    
    return(0);
}

////////////////////////////////

int
HasEvent(Input *input, Input_Event_Kind kind){
    int result = 0;
    if (input != 0){
        Input_Event **ptr_to = &input->first_event;
        Input_Event *last = 0;
        for (Input_Event *event = input->first_event, *next = 0;
             event != 0;
             event = next){
            next = event->next;
            if (event->kind == kind){
                result = 1;
                *ptr_to = next;
                break;
            }
            else{
                ptr_to = &event->next;
                last = event;
            }
        }
        input->last_event = last;
    }
    return(result);
}

typedef enum{
    Widget_None,
    Widget_Close,
    Widget_Minimize,
    Widget_Maximize,
    Widget_Slider,
} Widget;

Widget click_started = Widget_None;
int slider_x = 0;
int delta_from_mouse_x = 0;

void
UpdateAndRender(HWND hwnd, Input *input){
    RECT window_rect;
    GetClientRect(hwnd, &window_rect);
    
    RECT inside_rect;
    inside_rect.top    = window_rect.top    + caption_width;
    if (!IsZoomed(hwnd)){
        inside_rect.left   = window_rect.left   + border_width;
        inside_rect.right  = window_rect.right  - border_width;
        inside_rect.bottom = window_rect.bottom - border_width;
    }
    else{
        inside_rect.left   = window_rect.left;
        inside_rect.right  = window_rect.right;
        inside_rect.bottom = window_rect.bottom;
    }
    
    HDC dc = GetDC(hwnd);
    {
        SelectObject(dc, GetStockObject(DC_PEN));
        SelectObject(dc, GetStockObject(DC_BRUSH));
        
        // Window border
        {
            if (WindowIsActive()){
                SetDCPenColor(dc, RGB(255, 200, 0));
                SetDCBrushColor(dc, RGB(255, 200, 0));
            }
            else{
                SetDCPenColor(dc, RGB(128, 100, 0));
                SetDCBrushColor(dc, RGB(128, 100, 0));
            }
            
            Rectangle(dc, window_rect.left, window_rect.top, window_rect.right, inside_rect.top);
            Rectangle(dc, window_rect.left, inside_rect.bottom, window_rect.right, window_rect.bottom);
            Rectangle(dc, window_rect.left, inside_rect.top, inside_rect.left, inside_rect.bottom);
            Rectangle(dc, inside_rect.right, inside_rect.top, window_rect.right, inside_rect.bottom);
        }
        
        // Close button
        if (window_rect.right > 20){
            RECT button;
            button.left   = window_rect.right - 20;
            button.top    = window_rect.top + 10;
            button.right  = window_rect.right - 10;
            button.bottom = window_rect.top + 20;
            
            SetDCPenColor(dc, RGB(180, 0, 0));
            SetDCBrushColor(dc, RGB(180, 0, 0));
            
            EmbeddedWidgetRect(button);
            Rectangle(dc, button.left, button.top, button.right, button.bottom);
            if (input != 0 && HitTest(input->mouse_x, input->mouse_y, button)){
                if (HasEvent(input, InputEventKind_MouseLeftPress)){
                    click_started = Widget_Close;
                }
                if (click_started == Widget_Close && HasEvent(input, InputEventKind_MouseLeftRelease)){
                    StopRunning();
                }
            }
        }
        
        // Minimize button
        if (window_rect.right > 40){
            RECT button;
            button.left   = window_rect.right - 40;
            button.top    = window_rect.top + 10;
            button.right  = window_rect.right - 30;
            button.bottom = window_rect.top + 20;
            
            SetDCPenColor(dc, RGB(0, 0, 180));
            SetDCBrushColor(dc, RGB(0, 0, 180));
            
            EmbeddedWidgetRect(button);
            Rectangle(dc, button.left, button.top, button.right, button.bottom);
            if (input != 0 && HitTest(input->mouse_x, input->mouse_y, button)){
                if (HasEvent(input, InputEventKind_MouseLeftPress)){
                    click_started = Widget_Minimize;
                }
                if (click_started == Widget_Minimize && HasEvent(input, InputEventKind_MouseLeftRelease)){
                    Minimize(hwnd);
                }
            }
        }
        
        // Maximize button
        if (window_rect.right > 60){
            RECT button;
            button.left   = window_rect.right - 60;
            button.top    = window_rect.top + 10;
            button.right  = window_rect.right - 50;
            button.bottom = window_rect.top + 20;
            
            SetDCPenColor(dc, RGB(0, 180, 0));
            SetDCBrushColor(dc, RGB(0, 180, 0));
            
            EmbeddedWidgetRect(button);
            Rectangle(dc, button.left, button.top, button.right, button.bottom);
            if (input != 0 && HitTest(input->mouse_x, input->mouse_y, button)){
                if (HasEvent(input, InputEventKind_MouseLeftPress)){
                    click_started = Widget_Maximize;
                }
                if (click_started == Widget_Maximize && HasEvent(input, InputEventKind_MouseLeftRelease)){
                    ToggleMaximize(hwnd);
                }
            }
        }
        
        // Slider
        if (window_rect.right > 200){
            RECT track;
            track.left   = window_rect.right - 195;
            track.top    = window_rect.top + 14;
            track.right  = window_rect.right - 95;
            track.bottom = window_rect.top + 16;
            
            SetDCPenColor(dc, RGB(0, 0, 0));
            SetDCBrushColor(dc, RGB(0, 0, 0));
            Rectangle(dc, track.left, track.top, track.right, track.bottom);
            
            RECT button;
            button.left   = window_rect.right - 200 + slider_x;
            button.top    = window_rect.top + 10;
            button.right  = window_rect.right - 190 + slider_x;
            button.bottom = window_rect.top + 20;
            
            SetDCPenColor(dc, RGB(100, 200, 200));
            SetDCBrushColor(dc, RGB(100, 200, 200));
            
            EmbeddedWidgetRect(button);
            Rectangle(dc, button.left, button.top, button.right, button.bottom);
            if (input != 0){
                if (HitTest(input->mouse_x, input->mouse_y, button)){
                    if (HasEvent(input, InputEventKind_MouseLeftPress)){
                        click_started = Widget_Slider;
                        delta_from_mouse_x = slider_x - input->mouse_x;
                    }
                }
                if (click_started == Widget_Slider){
                    slider_x = delta_from_mouse_x + input->mouse_x;
                    if (slider_x < 0){
                        slider_x = 0;
                    }
                    if (slider_x > 100){
                        slider_x = 100;
                    }
                }
            }
        }
        
        if (input != 0 && !input->left){
            click_started = Widget_None;
        }
        
        // Inside area
        {
            SetDCPenColor(dc, RGB(127, 127, 127));
            SetDCBrushColor(dc, RGB(127, 127, 127));
            Rectangle(dc, inside_rect.left, inside_rect.top, inside_rect.right, inside_rect.bottom);
        }
    }
    ReleaseDC(hwnd, dc);
}

