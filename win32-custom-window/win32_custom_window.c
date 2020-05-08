/*
** Win32 Custom Window Example Program
**  v1.2.1 - April 30th 2020
**  by Allen Webster allenwebster@4coder.net
**
** public domain example program
** NO WARRANTY IMPLIED; USE AT YOUR OWN RISK
**
**
** Tested on: Windows 7 aero, Windows 7 classic, Windows 10
**
**
** This is an example only, it is designed for:
**  Easy copy-pasting the main components
**  Learn how and why it works
**  Build, step through, modify, experiment
**
** This example is not designed for use as a library/engine/framework etc.
**
**
**  This example *does show* how to create a win32 window where the client area
** covers the whole area of the window and is an axis aligned rectangle. It shows
** how to maintain normal window resizing, moving, and positioning shortcuts like
** WinKey + Arrows for such a window. And it shows how to handle widgets that are
** placed inside the caption area of the window.
**
**  This example *does not show* non-rectangular windows or transparency. It does
** not show a fully abstracted win32 main loop for an application or game. It does
** not show graphics context setup.
**
**
** Most important points:
**  CustomBorderWindowProc
**  CreateWindowW
**
**
** Required windows libraries:
**  User32.lib UxTheme.lib
** Pragmas versions:
**  #pragma comment(lib, "User32.lib")
**  #pragma comment(lib, "UxTheme.lib")
**
** Libraries only for rendering, not required:
** Gdi32.lib Dwmapi.lib
*/

#include <Windows.h>
#include <Windowsx.h>

// only for vsync, not necessary in copies:
#include <dwmapi.h>

// only for logging, not necessary in copies:
#include <stdio.h>

////////////////////////////////

// For simplicity this example statically defines the border and caption width.
// These determine the size of the area where resizing and moving the window
// is possible. There is no reason in principle why these have to be statically
// determined or why these would be the only options in setting up the border
// shape.

int caption_width = 30;
int border_width = 10;

////////////////////////////////

// These fascilitate communication between the message handling loop and the
// update function.

// The application has a way to report widgets that overlap the caption of the
// window so that clicking them will not move the window. A detailed discussion
// of this problem can be found at WM_NCHITTEST.

// With 100% control of how the border looks it is nice to be able to determine
// if the window is active so that it's border can respond with some kind of
// visual que to being inactive.

// Besides those points the details of this aren't specific to a custom window
// they are just mimicking a simple system layer <-> application layer interface.

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

// A handy helper for this example

int
HitTest(int x, int y, RECT rect){
    return((rect.left <= x && x < rect.right) && (rect.top <= y && y < rect.bottom));
}

////////////////////////////////

// Implementation for the pseudo system layer.

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

// The WindowProc callback does most of the work for a custom boredred window.
// The messages that start with "WM_NC" deal with processing the non-client
// area of the window. The main goal is to leave window moving and resizing
// up to the OS while taking control of the rendering and border widgets.
LRESULT
CustomBorderWindowProc(HWND   hwnd,
                       UINT   uMsg,
                       WPARAM wParam,
                       LPARAM lParam){
    LRESULT result = 0;
    switch (uMsg){
        // The WM_NCCALCSIZE is used to establish the mapping from the window's
        // non-client area to the client area. The non-client area is passed to
        // this code through a pointer in lParam that points to the RECT type.
        // Then this code indicates the mapping to a client area by modifying
        // that RECT.
        
        // The WM_NCCALCSIZE documentation mentions a circumstance where wParam
        // is true and another where wParam is false, and the possibility that
        // lParam points to the type NCCALCSIZE_PARAMS. It turns out that the
        // first member of NCCALCSIZE_PARAMS is a RECT, and that if the rest
        // of the NCCALCSIZE_PARAMS structure is ignored then this message
        // behaves the same way in eitehr case. NCCALCSIZE_PARAMS offers
        // additional features that are not not used here.
        
        // The main strategy is to leave the client area the same as the
        // non-client area by not modifying the RECT at all. This way the whole
        // window will be considered client area and the rendering of the border
        // is just the same as rendering anything else inside the window.
        
        // However there is a circumstance when this strategy doesn't work.
        // When a window is maximized the OS will automatically allow the
        // window's area hang a little outside of the monitor. When a window
        // uses the default border this makes it so that the maximized window
        // doesn't show it's border. This behavior can't be disabled but
        // it is possible nullify the effect by querying how wide the overhang
        // area will be and pushing the non-client area in by that amount.
        case WM_NCCALCSIZE:
        {
            MARGINS m = { 0, 0, 0, 0 };

            RECT* r = (RECT*)lParam;
            // A convenient function for checking if a window is maximized.
            if (IsZoomed(hwnd)){
                int x_push_in = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                int y_push_in = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                r->top    += x_push_in;
                r->left   += y_push_in;
                r->right  -= x_push_in;
                r->bottom -= y_push_in;

                m.cxLeftWidth = m.cxRightWidth = x_push_in;
                m.cyTopHeight = m.cyBottomHeight = y_push_in;
            }

            BOOL enabled;
            DwmIsCompositionEnabled(&enabled);
            if (enabled)
            {
                DwmExtendFrameIntoClientArea(hwnd, &m);
            }

        }break;

        // this fixes margins for maximized window when dwm compositor is enabled
        case WM_DWMCOMPOSITIONCHANGED:
        {
            MARGINS m = { 0, 0, 0, 0 };

            if (IsZoomed(hwnd))
            {
                LONG bx = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                LONG by = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);

                m.cxLeftWidth = m.cxRightWidth = bx - 1;
                m.cyTopHeight = m.cyBottomHeight = by - 1;
            }
            DwmExtendFrameIntoClientArea(hwnd, &m);
        } break;

        
        // The WM_NCACTIVATE message is sent to a window before the WM_ACTIVATE
        // message. When a window uses the default border this message is used
        // to repaint the border to indicate the change of status to active or
        // inactive. This would paint over the custom window border. This code
        // simply overrides that behavior with tracking the window state. It is
        // still necessary to return true from this message so that normal window
        // activation handling continues and so that the WM_ACTIVATE message is
        // still sent.
        
        // The documentation for WM_NCACTIVATE says without explanation:
        //  "If the window is minimized when this message is received, the
        //   application should pass the message to the DefWindowProc function"
        // This code follows this instruction, but for extra caution passes
        // -1 instead of lParam as the docs suggest this will prevent it from
        // rendering.
        case WM_NCACTIVATE:
        {
            result = 1;
            window_is_active = wParam;
            // A convenient function for checking if a window is minimized.
            if (IsIconic(hwnd)){
                result = DefWindowProcW(hwnd, uMsg, wParam, -1);
            }
        }break;
        
        // The WM_NCHITTEST message is used to map points on the window to
        // a symbol indicating the "behavior" of that point. The possible
        // "behaviors" include resizing, moving, clicking close, minimize,
        // and maximize buttons, being outside of the window, and being
        // inside the client area.
        
        // In some versions of Windows using the behaviors for the various
        // buttons causes unwanted rendering for the buttons, so those
        // are not used in this custom window border. Luckily there is not
        // much difficulty in recreating the buttons.
        
        // There is one tricky problem with handling this message. The problem
        // occurs if your custom border embeds any widgets along the area that
        // would be considered the "caption" of the window which really means
        // the area that can be grabbed to move the window around. For example
        //  the close, minimize, and maximize buttons are widgets embedded in
        // the caption. Generally any widget that overlaps any part of the
        // area that would be for moving the window.
        //
        // When a point is inside an embedded widget this message should return
        // HTCLIENT so that normal mouse events are sent to the application layer.
        // The problem is that the application is going to be in charge of placing
        // those buttons, so there must be a strategy for getting that information
        // back to this message which expects an immediate response, not a response
        // later on in the frame.
        
        // Some ideas of strategies to consider:
        
        // 1. Just don't have any embedded widgets.
        //
        // > This works but it does mean you're officially giving up on replacing
        //  the normal close, minimize, maximize buttons.
        
        // 2. Only allow embedded widgets that immediately move the window when
        //    clicked so that there is no chance of accidentally moving the
        //    window when trying to use the widgets.
        //
        // > This works but is very restricting, and requires buttons that
        //  respond on mouse down immediately instead of mouse up.
        
        // 3. Pre-define the embedded widget layout and have both the application
        //    layer and platform layer depend on this pre-defined layout rule.
        //
        // > This works but it does couple the platform layer to things that could
        //  have been purely application controlled concepts.
        
        // 4. Have the application layer set a "widget rectangle list" when it
        //   updates and always use the most recent one
        //
        // > This works but it is also the most work to set up in the platform
        //  layer and requires the update function to do new work too.
        
        // Certainly there are more possible strategies, this list is just mean
        // to offer a few ideas, mostly to clarify the nature of the problem.
        
        // This example is using strategy 4.
        
        case WM_NCHITTEST:
        {
            POINT pos;
            pos.x = GET_X_LPARAM(lParam);
            pos.y = GET_Y_LPARAM(lParam);
            
            // Make sure the point is inside of the window
            RECT frame_rect;
            GetWindowRect(hwnd, &frame_rect);
            if (!HitTest(pos.x, pos.y, frame_rect)){
                result = HTNOWHERE;
            }
            
            else{
                
                RECT rect;
                GetClientRect(hwnd, &rect);
                ScreenToClient(hwnd, &pos);
                
                // Check each border
                int l = 0;
                int r = 0;
                int b = 0;
                int t = 0;
                if (!IsZoomed(hwnd)){
                    if (rect.left <= pos.x && pos.x < rect.left + border_width){
                        l = 1;
                    }
                    if (rect.right - border_width <= pos.x && pos.x < rect.right){
                        r = 1;
                    }
                    if (rect.bottom - border_width <= pos.y && pos.y < rect.bottom){
                        b = 1;
                    }
                    if (rect.top <= pos.y && pos.y < rect.top + border_width){
                        t = 1;
                    }
                }
                
                // If the point is in two borders, use the corresponding corner resize.
                // If the point is in just one border, use the corresponding side resize.
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
                
                // Here the point must be further inside the window than the resize
                // borders, so the final options are the window moving area (caption)
                // and the client area.
                else{
                    if (rect.top <= pos.y && pos.y < rect.top + caption_width){
                        result = HTCAPTION;
                        // Check the application defined widget areas
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
        
        // This is only included to emphasize that there is no reason to handle
        // any of these WM_NC* messages. They give the window the opportunity
        // to handle mouse events in the non-client area before generating the
        // equivalent client area versions. If WM_NCHITTEST returns HTCLIENT
        // then the default handling for these is to generate the client area
        // version of the message. If WM_NCHITTEST generates one of the symbols,
        // HTLEFT, HTTOP, ... HTTOPLEFT ... HTCAPTION, then it determined that
        // the OS should provide default handling for a resize or move operation.
        // In either case, these should just do their default behavior.
#if 0
        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP:
        case WM_NCLBUTTONDBLCLK:
        case WM_NCMBUTTONDBLCLK:
        case WM_NCMBUTTONDOWN:
        case WM_NCMBUTTONUP:
        case WM_NCMOUSEHOVER:
        case WM_NCMOUSELEAVE:
        case WM_NCMOUSEMOVE:
        case WM_NCRBUTTONDBLCLK:
        case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP:
        case WM_NCXBUTTONDBLCLK:
        case WM_NCXBUTTONDOWN:
        case WM_NCXBUTTONUP:
        {
            result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }break;
#endif
        
        case WM_CLOSE:
        {
            keep_running = 0;
        }break;
        
        case WM_SIZING:
        {
            InvalidateRect(hwnd, 0, 0);
        }break;
        
        // It's always a good idea to do *something* with the WM_PAINT message
        // so that your window looks better when resizing, but in this case it's
        // especially important. Remember your code creates the border graphics
        // now, so if you don't render anything when the window is reszing then
        // there will be nothing at all to indicate that the window is changing
        // size until the resize finishes.
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
    
    // With CustomBorderWindowProc done, creating the window is not
    // that much more work, and is almost identical to creating a
    // default window.
    
#define WINDOW_CLASS L"MainWindow"
    
    // Nothing about the window class differs from a default window.
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
    
    // One key aspect is making sure these styles are present:
    //  WS_SIZEBOX, WS_MAXIMIZEBOX, WS_MINIMIZEBOX, WS_CAPTION, WM_SYSMENU
    // Because these styles are needed for making shortcuts and
    // default resize and moving behavior work, even when the border
    // itself is overriden. These are exactly equivalent to the combined style:
    //  WS_OVERLAPPEDWINDOW
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
    
    // The other key aspect is SetWindowTheme. Using SetWindowTheme with single
    // spaces effectively disables an theme on the window. Doing this is critical
    // for removing special shapes, shading, etc. from the default window on
    // any given version of Windows.
    SetWindowTheme(hwnd, L" ", L" ");
    
    
    // A convenient way to get vsync since this example doesn't have a more
    // sophistcated graphics context.
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
        
        // Personal preference - I like to put anything that resizes the window
        // from my code after the main update so that I can assume that the
        // size never changes during the update. This also happens to be a
        // convenient way to make sure I immediately redo my layout and render
        // after a size change.
        
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
        
        // This can be whatever vsync or frame rate limiting method you'd
        // like or none at all.
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

// The implementation of the application tick function which renders the border,
// inside of the window, and embedded widgets, and processes input for both the
// border and the interior through a single input system.

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

