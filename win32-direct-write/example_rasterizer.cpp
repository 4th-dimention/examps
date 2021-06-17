// DirectWrite rasterization example

#define UNICODE
#include <windows.h>
#include <GL\gl.h>
#include <dwrite_1.h>
#include <assert.h>
#include <malloc.h>
#include <stdint.h>
typedef int32_t bool32;

#include "example_gl_defines.h"

HWND
window_setup(HINSTANCE hInstance);

////////////////////////////////

static int32_t window_width = 800;
static int32_t window_height = 600;
static wchar_t font_path[] = L"C:\\Windows\\Fonts\\arial.ttf";
static float point_size = 12.f;

// This is not a guide in fighting with Windows to let you manage the DPI.  Just leave this at 96
// until you're ready for a whole separate nightmare.
static float dpi = 96.f;

////////////////////////////////

struct AutoReleaserClass{
    IUnknown *ptr_member;
    AutoReleaserClass(IUnknown *ptr){
        ptr_member = ptr;
    }
    ~AutoReleaserClass(){
        if (ptr_member != 0){
            ptr_member->Release();
        }
    }
};
#define DeferRelease(ptr) AutoReleaserClass ptr##_releaser(ptr)
#define DWCheck(error,r)        if ((error) != S_OK){ error = S_OK; r; }
#define DWCheckPtr(error,ptr,r) if ((ptr) == 0 || (error) != S_OK){ error = S_OK; r; }

// Font Data Structure

struct Glyph_Metrics{
    float off_x;
    float off_y;
    float advance;
    float xy_w;
    float xy_h;
    float uv_w;
    float uv_h;
};

struct Baked_Font{
    IDWriteFontFace *face;
    GLuint texture;
    Glyph_Metrics *metrics;
    int32_t glyph_count;
};

////////////////////////////////

static char vert_source[] =
"#version 330\n"
"uniform mat3 pixel_to_normal;\n"
"in vec2 position;\n"
"in vec3 tex_position;\n"
"smooth out vec3 uv;\n"
"void main(){\n"
"    gl_Position.xy = (pixel_to_normal*vec3(position, 1.f)).xy;\n"
"    gl_Position.z = 0.f;\n"
"    gl_Position.w = 1.f;\n"
"    uv = tex_position;\n"
"}\n";

static char frag_source[] = 
"#version 330\n"
"smooth in vec3 uv;\n"
"uniform sampler2DArray tex;\n"
"uniform float M_value_table[7];\n"
"layout(location = 0) out vec4 mask;\n"
"\n"
"void main(){\n"
"vec3 S = texture(tex, uv).rgb;\n"
"int C0 = int(S.r*6 + 0.1); // + 0.1 just incase we have some small rounding taking us below the integer we should be hitting.\n"
"int C1 = int(S.g*6 + 0.1);\n"
"int C2 = int(S.b*6 + 0.1);\n"
"mask.rgb = vec3(M_value_table[C0],\n"
"M_value_table[C1],\n"
"M_value_table[C2]);\n"
"mask.a = 1;\n"
"}\n";

static GLuint uniform_pixel_to_normal;
static GLuint uniform_tex;
static GLuint uniform_M_value_table;

static GLuint attrib_position;
static GLuint attrib_tex_position;

////////////////////////////////

uint32_t
next_power_of_two(uint32_t x){
    if (x == 0){
        return(1);
    }
    else{
        x -= 1;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x += 1;
        return(x);
    }
}

int32_t
round_up(float x){
    int32_t r = (int32_t)x;
    if ((float)r < x){
        r += 1;
    }
    return(r);
}

void
draw_string(Baked_Font font, char *text, int32_t x, int32_t y, float r, float g, float b, float a){
    // Get Index Array
    int32_t length = 0;
    for (; text[length] != 0; length += 1);
    uint16_t *indices = (uint16_t*)malloc(sizeof(uint16_t)*length);
    
    for (int32_t i = 0; i < length; i += 1){
        uint32_t codepoint = (uint32_t)text[i];
        font.face->GetGlyphIndices(&codepoint, 1, &indices[i]);
    }
    
    // Fill Vertices
    int32_t float_per_vertex = 5;
    int32_t byte_per_vertex = float_per_vertex*sizeof(float);
    int32_t vertex_per_character = 6;
    int32_t total_float_count = length*vertex_per_character*float_per_vertex;
    float *vertices = (float*)malloc(sizeof(float)*total_float_count);
    
    {
        float layout_x = (float)x;
        float layout_y = (float)y;
        
        float *vertex = vertices;
        for (int32_t i = 0; i < length; i += 1){
            uint16_t index = indices[i];
            assert(index < font.glyph_count);
            
            float index_f = (float)(index/4);
            float uv_x = 0.5f*(float)((index&1));
            float uv_y = 0.5f*(float)(((index&2) >> 1));
            Glyph_Metrics metrics = font.metrics[index];
            
            for (int32_t j = 0; j < vertex_per_character; j += 1){
                float g_x = layout_x + metrics.off_x;
                float g_y = layout_y + metrics.off_y;
                
                switch (j){
                    case 0:
                    {
                        vertex[0] = g_x;
                        vertex[1] = g_y;
                        vertex[2] = uv_x;
                        vertex[3] = uv_y;
                    }break;
                    case 1:
                    case 3:
                    {
                        vertex[0] = g_x;
                        vertex[1] = g_y + metrics.xy_h;
                        vertex[2] = uv_x;
                        vertex[3] = uv_y + metrics.uv_h;
                    }break;
                    case 2:
                    case 4:
                    {
                        vertex[0] = g_x + metrics.xy_w;
                        vertex[1] = g_y;
                        vertex[2] = uv_x + metrics.uv_w;
                        vertex[3] = uv_y;
                    }break;
                    case 5:
                    {
                        vertex[0] = g_x + metrics.xy_w;
                        vertex[1] = g_y + metrics.xy_h;
                        vertex[2] = uv_x + metrics.uv_w;
                        vertex[3] = uv_y + metrics.uv_h;
                    }break;
                }
                vertex[4] = index_f;
                vertex += float_per_vertex;
            }
            
            layout_x += metrics.advance;
        }
    }
    
    // Compute M Values
    float V = r*0.5f + g + b*0.1875f;
    float M_value_table[7];
    
    static float Cmax_table[] = {
        0.f,
        0.380392157f,
        0.600000000f,
        0.749019608f,
        0.854901961f,
        0.937254902f,
        1.f,
    };
    static float Cmin_table[] = {
        0.f,
        0.166666667f,
        0.333333333f,
        0.500000000f,
        0.666666667f,
        0.833333333f,
        1.f,
    };
    
    static float A = 0.839215686374509f; // 214/255
    static float B = 1.266666666666667f; // 323/255
    float L = (V - A)/(B - A);
    
    M_value_table[0] = 0.f;
    for (int32_t i = 1; i <= 5; i += 1){
        float Cmax = Cmax_table[i];
        float Cmin = Cmin_table[i];
        float M = Cmax + (Cmin - Cmax)*L;
        if (M > Cmax){
            M = Cmax;
        }
        if (M < Cmin){
            M = Cmin;
        }
        M_value_table[i] = M*a;
    }
    M_value_table[6] = a;
    
    // Draw
    glBlendColor(r, g, b, a);
    glBufferData(GL_ARRAY_BUFFER, total_float_count*sizeof(float), vertices, GL_DYNAMIC_DRAW);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, font.texture);
    glUniform1i(uniform_tex, 0);
    glUniform1fv(uniform_M_value_table, 7, M_value_table);
    glVertexAttribPointer(attrib_position, 2, GL_FLOAT, GL_FALSE, byte_per_vertex, 0);
    glVertexAttribPointer(attrib_tex_position, 3, GL_FLOAT, GL_FALSE, byte_per_vertex, (void*)(sizeof(float)*2));
    glDrawArrays(GL_TRIANGLES, 0, vertex_per_character*length);
    
    free(vertices);
    free(indices);
}

void
gl_debug(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam){
    assert(!"Bad OpenGL Call!");
}

int
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
    HWND wnd = window_setup(hInstance);
    
    // OpenGL Setup
    GLuint framebuffer = 0;
    
    {
        // Debug
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, 0, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, 0, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, 0, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);
        glDebugMessageCallback(gl_debug, 0);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        
        // Settings
        glEnable(GL_FRAMEBUFFER_SRGB);
        glEnable(GL_BLEND);
        glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_COLOR);
        
        // sRGB Framebuffer
        GLuint frame_texture = 0;
        glGenTextures(1, &frame_texture);
        glBindTexture(GL_TEXTURE_2D, frame_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame_texture, 0);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        assert(status == GL_FRAMEBUFFER_COMPLETE);
        
        // Shader
        GLuint shader_vert = glCreateShader(GL_VERTEX_SHADER);
        GLuint shader_frag = glCreateShader(GL_FRAGMENT_SHADER);
        
        char *vert_source_localized = vert_source;
        char *frag_source_localized = frag_source;
        
        glShaderSource(shader_vert, 1, &vert_source_localized, 0);
        glShaderSource(shader_frag, 1, &frag_source_localized, 0);
        
        glCompileShader(shader_vert);
        glCompileShader(shader_frag);
        
        GLenum error = glGetError();
        assert(error == GL_NO_ERROR);
        
        GLuint program = glCreateProgram();
        glAttachShader(program, shader_vert);
        glAttachShader(program, shader_frag);
        glLinkProgram(program);
        
        error = glGetError();
        assert(error == GL_NO_ERROR);
        
        glUseProgram(program);
        
        // Uniforms and Attributes
        uniform_pixel_to_normal = glGetUniformLocation(program, "pixel_to_normal");
        uniform_tex             = glGetUniformLocation(program, "tex");
        uniform_M_value_table   = glGetUniformLocation(program, "M_value_table");
        
        attrib_position     = glGetAttribLocation(program, "position");
        attrib_tex_position = glGetAttribLocation(program, "tex_position");
        
        float mat[9];
        mat[0] = 2.f/(float)window_width; mat[3] = 0.f;                        mat[6] = -1.f;
        mat[1] = 0.f;                     mat[4] = -2.f/(float)window_height;  mat[7] =  1.f,
        mat[2] = 0.f;                     mat[5] = 0.f;                        mat[8] =  1.f,
        glUniformMatrix3fv(uniform_pixel_to_normal, 1, GL_FALSE, mat);
        
        // Viewport
        glViewport(0, 0, window_width, window_height);
        
        // Vertex Array Object
        GLuint VAO = 0;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        
        // Data Buffer
        GLuint buffer = 0;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        
        glEnableVertexAttribArray(attrib_position);
        glEnableVertexAttribArray(attrib_tex_position);
    }
    
    // Font Setup
    Baked_Font font = {0};
    
    {
        COLORREF back_color = RGB(0,0,0);
        COLORREF fore_color = RGB(255,255,255);
        
        HRESULT error = 0;
        
        // Factory
        IDWriteFactory *factory = 0;
        error = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&factory);
        DeferRelease(factory);
        DWCheckPtr(error, factory, assert(!"factory"));
        
        // File
        IDWriteFontFile *font_file = 0;
        error = factory->CreateFontFileReference(font_path, 0, &font_file);
        DeferRelease(font_file);
        DWCheckPtr(error, font_file, assert(!"font file"));
        
        // Face
        error = factory->CreateFontFace(DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &font_file, 0, DWRITE_FONT_SIMULATIONS_NONE, &font.face);
        // We don't use DeferRelease because we intend to keep the font face around after the baking process.
        DWCheckPtr(error, font.face, assert(!"font face"));
        
        // Params
        IDWriteRenderingParams *default_rendering_params = 0;
        error = factory->CreateRenderingParams(&default_rendering_params);
        DeferRelease(default_rendering_params);
        DWCheckPtr(error, default_rendering_params, assert(!"rendering params"));
        
        FLOAT gamma = 1.f;
        
        IDWriteRenderingParams *rendering_params = 0;
        error = factory->CreateCustomRenderingParams(gamma,
                                                     default_rendering_params->GetEnhancedContrast(),
                                                     default_rendering_params->GetClearTypeLevel(),
                                                     default_rendering_params->GetPixelGeometry(),
                                                     DWRITE_RENDERING_MODE_NATURAL,
                                                     &rendering_params);
        DeferRelease(rendering_params);
        DWCheckPtr(error, rendering_params, assert(!"rendering params"));
        
        // Interop
        IDWriteGdiInterop *dwrite_gdi_interop = 0;
        error = factory->GetGdiInterop(&dwrite_gdi_interop);
        DeferRelease(dwrite_gdi_interop);
        DWCheckPtr(error, dwrite_gdi_interop, assert(!"gdi interop"));
        
        // Metrics
        DWRITE_FONT_METRICS font_metrics = {0};
        font.face->GetMetrics(&font_metrics);
        
        float pixel_per_em = point_size*(1.f/72.f)*dpi;
        float pixel_per_design_unit = pixel_per_em/((float)font_metrics.designUnitsPerEm);
        
        int32_t raster_target_w = (int32_t)(8.f*((float)font_metrics.capHeight)*pixel_per_design_unit);
        int32_t raster_target_h = (int32_t)(8.f*((float)font_metrics.capHeight)*pixel_per_design_unit);
        float raster_target_x = (float)(raster_target_w/2);
        float raster_target_y = (float)(raster_target_h/2);
        
        assert((float) ((int)(raster_target_x)) == raster_target_x);
        assert((float) ((int)(raster_target_y)) == raster_target_y);
        
        // Glyph Count
        font.glyph_count = font.face->GetGlyphCount();
        
        // Render Target
        IDWriteBitmapRenderTarget *render_target = 0;
        error = dwrite_gdi_interop->CreateBitmapRenderTarget(0, raster_target_w, raster_target_h, &render_target);
        HDC dc = render_target->GetMemoryDC();
        
        // Clear the Render Target
        {
            HGDIOBJ original = SelectObject(dc, GetStockObject(DC_PEN));
            SetDCPenColor(dc, back_color);
            SelectObject(dc, GetStockObject(DC_BRUSH));
            SetDCBrushColor(dc, back_color);
            Rectangle(dc, 0, 0, raster_target_w, raster_target_h);
            SelectObject(dc, original);
        }
        
        // Allocate CPU Side Atlas
        int32_t atlas_w = 4*(int32_t)(((float)font_metrics.capHeight)*pixel_per_design_unit);
        int32_t atlas_h = 4*(int32_t)(((float)font_metrics.capHeight)*pixel_per_design_unit);
        if (atlas_w < 16){
            atlas_w = 16;
        }
        else{
            atlas_w = next_power_of_two(atlas_w);
        }
        if (atlas_h < 256){
            atlas_h = 256;
        }
        else{
            atlas_h = next_power_of_two(atlas_h);
        }
        int32_t atlas_c = (font.glyph_count + 3)/4;
        int32_t atlas_slice_size = atlas_w*atlas_h*3;
        int32_t atlas_memory_size = atlas_slice_size*atlas_c;
        uint8_t *atlas_memory = (uint8_t*)malloc(atlas_memory_size);
        memset(atlas_memory, 0, atlas_memory_size);
        
        // Allocate the Metric Data
        font.metrics = (Glyph_Metrics*)malloc(sizeof(Glyph_Metrics)*font.glyph_count);
        memset(font.metrics, 0, sizeof(Glyph_Metrics)*font.glyph_count);
        
        // Fill the CPU Side Atlas and Metric Data
        for (uint16_t glyph_index = 0; glyph_index < font.glyph_count; glyph_index += 1){
            // Render the Glyph Into the Target
            DWRITE_GLYPH_RUN glyph_run = {0};
            glyph_run.fontFace = font.face;
            glyph_run.fontEmSize = pixel_per_em;
            glyph_run.glyphCount = 1;
            glyph_run.glyphIndices = &glyph_index;
            RECT bounding_box = {0};
            error = render_target->DrawGlyphRun(raster_target_x, raster_target_y,
                                                DWRITE_MEASURING_MODE_NATURAL, &glyph_run, rendering_params, RGB(255, 255, 255), &bounding_box);
            DWCheck(error, continue);
            
            assert(0 <= bounding_box.left);
            assert(0 <= bounding_box.top);
            assert(bounding_box.right <= raster_target_w);
            assert(bounding_box.bottom <= raster_target_h);
            
            // Compute Our Glyph Metrics
            DWRITE_GLYPH_METRICS glyph_metrics = {0};
            error = font.face->GetDesignGlyphMetrics(&glyph_index, 1, &glyph_metrics, false);
            DWCheck(error, continue);
            
            float off_x = (float)bounding_box.left - raster_target_x;
            float off_y = (float)bounding_box.top - raster_target_y;
            float advance = ((float)glyph_metrics.advanceWidth)*pixel_per_design_unit;
            int32_t tex_w = bounding_box.right - bounding_box.left;
            int32_t tex_h = bounding_box.bottom - bounding_box.top;
            
            font.metrics[glyph_index].off_x   = off_x;
            font.metrics[glyph_index].off_y   = off_y;
            font.metrics[glyph_index].advance = (float)round_up(advance);
            font.metrics[glyph_index].xy_w    = (float)tex_w;
            font.metrics[glyph_index].xy_h    = (float)tex_h;
            font.metrics[glyph_index].uv_w    = (float)tex_w/(float)atlas_w;
            font.metrics[glyph_index].uv_h    = (float)tex_h/(float)atlas_h;
            
            // Get the Bitmap
            HBITMAP bitmap = (HBITMAP)GetCurrentObject(dc, OBJ_BITMAP);
            DIBSECTION dib = {0};
            GetObject(bitmap, sizeof(dib), &dib);
            
            // Blit the Bitmap Into Our CPU Side Atlas
            int32_t x_slice_offset = (3*atlas_w/2)*(glyph_index&1);
            int32_t y_slice_offset = (3*atlas_w*atlas_h/2)*((glyph_index&2) >> 1);
            uint8_t *atlas_slice = atlas_memory + atlas_slice_size*(glyph_index/4) + x_slice_offset + y_slice_offset;
            
            {
                assert(dib.dsBm.bmBitsPixel == 32);
                int32_t in_pitch  = dib.dsBm.bmWidthBytes;
                int32_t out_pitch = atlas_w*3;
                uint8_t *in_line  = (uint8_t*)dib.dsBm.bmBits + bounding_box.left*4 + bounding_box.top*in_pitch;
                uint8_t *out_line = atlas_slice;
                for (int32_t y = 0; y < tex_h; y += 1){
                    uint8_t *in_pixel  = in_line;
                    uint8_t *out_pixel = out_line;
                    for (int32_t x = 0; x < tex_w; x += 1){
                        out_pixel[0] = in_pixel[2];
                        out_pixel[1] = in_pixel[1];
                        out_pixel[2] = in_pixel[0];
                        in_pixel += 4;
                        out_pixel += 3;
                    }
                    in_line += in_pitch;
                    out_line += out_pitch;
                }
            }
            
            // Clear the Render Target
            {
                HGDIOBJ original = SelectObject(dc, GetStockObject(DC_PEN));
                SetDCPenColor(dc, back_color);
                SelectObject(dc, GetStockObject(DC_BRUSH));
                SetDCBrushColor(dc, back_color);
                Rectangle(dc,
                          bounding_box.left, bounding_box.top,
                          bounding_box.right, bounding_box.bottom);
                SelectObject(dc, original);
            }
        }
        
        // Allocate and Fill the GPU Side Atlas
        glGenTextures(1, &font.texture);
        glBindTexture(GL_TEXTURE_2D_ARRAY, font.texture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, atlas_w, atlas_h, atlas_c, 0, GL_RGB, GL_UNSIGNED_BYTE, atlas_memory);
        
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // Free CPU Side Atlas
        free(atlas_memory);
        atlas_memory = 0;
    }
    
    int32_t mode = 0;
    bool32 paused = false;
    for (;;){
        MSG msg = {0};
        for (;PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_KEYDOWN){
                if (msg.wParam == VK_SPACE){
                    // Check if this key just got pressed
                    if (((msg.lParam >> 30) & 1) == 0){
                        paused = !paused;
                    }
                }
            }
        }
        
        HDC dc = GetDC(wnd);
        
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        
        enum{
            TB_Black,
            TB_White,
            TB_Red,
            TB_Green,
            TB_Blue,
            TB_Yellow,
            TB_Cyan,
            TB_Purple,
            TB_COUNT,
        };
        
        enum{
            TF_Gray,
            TF_RGB,
            TF_YCP,
            TF_AlphaGray,
            TF_AlphaRGB,
            TF_AlphaYCP,
            TF_COUNT,
        };
        
        int32_t mode_index = mode/16;
        int32_t bmode = (mode_index/TF_COUNT)%TB_COUNT;
        int32_t fmode = mode_index%TF_COUNT;
        
        float pop_r = 0.f;
        float pop_g = 0.f;
        float pop_b = 0.f;
#define SetPopColor(r,g,b) pop_r = (r), pop_g = (g), pop_b = (b)
        switch (bmode){
            case TB_Black:
            {
                glClearColor(0.f, 0.f, 0.f, 1.f);
                SetPopColor(1.f, 1.f, 1.f);
                glClear(GL_COLOR_BUFFER_BIT);
                draw_string(font, "Back = Black", 300, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
            
            case TB_White:
            {
                glClearColor(1.f, 1.f, 1.f, 1.f);
                SetPopColor(0.f, 0.f, 0.f);
                glClear(GL_COLOR_BUFFER_BIT);
                draw_string(font, "Back = White", 300, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
            
            case TB_Red:
            {
                glClearColor(0.5f, 0.f, 0.f, 1.f);
                SetPopColor(0.f, 0.5f, 0.5f);
                glClear(GL_COLOR_BUFFER_BIT);
                draw_string(font, "Back = (.5,0,0)", 300, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
            
            case TB_Green:
            {
                glClearColor(0.f, 0.5f, 0.f, 1.f);
                SetPopColor(0.5f, 0.f, 0.5f);
                glClear(GL_COLOR_BUFFER_BIT);
                draw_string(font, "Back = (0,.5,0)", 300, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
            
            case TB_Blue:
            {
                glClearColor(0.f, 0.f, 0.5f, 1.f);
                SetPopColor(0.5f, 0.5f, 0.f);
                glClear(GL_COLOR_BUFFER_BIT);
                draw_string(font, "Back = (0,0,.5)", 300, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
            
            case TB_Yellow:
            {
                glClearColor(0.5f, 0.5f, 0.f, 1.f);
                SetPopColor(0.f, 0.f, 0.5f);
                glClear(GL_COLOR_BUFFER_BIT);
                draw_string(font, "Back = (.5,.5,0)", 300, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
            
            case TB_Cyan:
            {
                glClearColor(0.f, 0.5f, 0.5f, 1.f);
                SetPopColor(0.5f, 0.f, 0.f);
                glClear(GL_COLOR_BUFFER_BIT);
                draw_string(font, "Back = (0,.5,.5)", 300, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
            
            case TB_Purple:
            {
                glClearColor(0.5f, 0.f, 0.5f, 1.f);
                SetPopColor(0.f, 0.5f, 0.f);
                glClear(GL_COLOR_BUFFER_BIT);
                draw_string(font, "Back = (.5,0,.5)", 300, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
        }
        
        switch (fmode){
            case TF_Gray:
            {
                for (int32_t i = 1; i <= 6; i += 1){
                    float v = (i - 1)/5.f;
                    draw_string(font, "DirectWrite rasterizer testing", 50, i*80 + 40, v, v, v, 1.f);
                }
                draw_string(font, "Fore = Grays", 550, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
            
            case TF_RGB:
            {
                for (int32_t i = 0; i < 3; i += 1){
                    float v[3] = {0.f, 0.f, 0.f};
                    v[i] = 0.5f;
                    draw_string(font, "DirectWrite rasterizer testing", 50 + 250*i, 120, v[0], v[1], v[2], 1.f);
                }
                draw_string(font, "Fore = Red Green Blue", 550, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
            
            case TF_YCP:
            {
                for (int32_t i = 0; i < 3; i += 1){
                    float v[3] = {0.5f, 0.5f, 0.5f};
                    v[(i + 2)%3] = 0.f;
                    draw_string(font, "DirectWrite rasterizer testing", 50 + 250*i, 120, v[0], v[1], v[2], 1.f);
                }
                draw_string(font, "Fore = Yellow Cyan Purple", 550, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
            
            case TF_AlphaGray:
            {
                for (int32_t j = 1; j <= 6; j += 1){
                    float a = (j - 1)/5.f;
                    draw_string(font, "DirectWrite rasterizer testing",  50, j*80 + 40, 1.f, 1.f, 1.f, a);
                    draw_string(font, "DirectWrite rasterizer testing", 300, j*80 + 40, 0.f, 0.f, 0.f, a);
                }
                draw_string(font, "Fore = Alpha Black and White", 550, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
            
            case TF_AlphaRGB:
            {
                for (int32_t j = 1; j <= 6; j += 1){
                    float a = (j - 1)/5.f;
                    for (int32_t i = 0; i < 3; i += 1){
                        float v[3] = {0.f, 0.f, 0.f};
                        v[i] = 0.5f;
                        draw_string(font, "DirectWrite rasterizer testing", 50 + 250*i, j*80 + 40, v[0], v[1], v[2], a);
                    }
                }
                draw_string(font, "Fore = Alpha Red Green Blue", 550, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
            
            case TF_AlphaYCP:
            {
                for (int32_t j = 1; j <= 6; j += 1){
                    float a = (j - 1)/5.f;
                    for (int32_t i = 0; i < 3; i += 1){
                        float v[3] = {0.5f, 0.5f, 0.5f};
                        v[(i + 2)%3] = 0.f;
                        draw_string(font, "DirectWrite rasterizer testing", 50 + 250*i, j*80 + 40, v[0], v[1], v[2], a);
                    }
                }
                draw_string(font, "Fore = Alpha Yellow Cyan Purple", 550, 60, pop_r, pop_g, pop_b, 1.f);
            }break;
        }
        
        if (!paused){
            draw_string(font, "Press space to pause cycle",  50, 60, pop_r, pop_g, pop_b, 1.f);
        }
        else{
            draw_string(font, "Press space to resume cycle", 50, 60, pop_r, pop_g, pop_b, 1.f);
        }
        
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glBlitFramebuffer(0, 0, window_width, window_height, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        
        SwapBuffers(dc);
        
        Sleep(100);
        if (!paused){
            mode += 1;
        }
        ShowWindow(wnd, TRUE);
    }
    
    return(0);
}

////////////////////////////////

#define WGL_NUMBER_PIXEL_FORMATS_ARB            0x2000
#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_DRAW_TO_BITMAP_ARB                  0x2002
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_NEED_PALETTE_ARB                    0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB             0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB              0x2006
#define WGL_SWAP_METHOD_ARB                     0x2007
#define WGL_NUMBER_OVERLAYS_ARB                 0x2008
#define WGL_NUMBER_UNDERLAYS_ARB                0x2009
#define WGL_TRANSPARENT_ARB                     0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB           0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB         0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB          0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB         0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB         0x203B
#define WGL_SHARE_DEPTH_ARB                     0x200C
#define WGL_SHARE_STENCIL_ARB                   0x200D
#define WGL_SHARE_ACCUM_ARB                     0x200E
#define WGL_SUPPORT_GDI_ARB                     0x200F
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_STEREO_ARB                          0x2012
#define WGL_PIXEL_TYPE_ARB                      0x2013
#define WGL_COLOR_BITS_ARB                      0x2014
#define WGL_RED_BITS_ARB                        0x2015
#define WGL_RED_SHIFT_ARB                       0x2016
#define WGL_GREEN_BITS_ARB                      0x2017
#define WGL_GREEN_SHIFT_ARB                     0x2018
#define WGL_BLUE_BITS_ARB                       0x2019
#define WGL_BLUE_SHIFT_ARB                      0x201A
#define WGL_ALPHA_BITS_ARB                      0x201B
#define WGL_ALPHA_SHIFT_ARB                     0x201C
#define WGL_ACCUM_BITS_ARB                      0x201D
#define WGL_ACCUM_RED_BITS_ARB                  0x201E
#define WGL_ACCUM_GREEN_BITS_ARB                0x201F
#define WGL_ACCUM_BLUE_BITS_ARB                 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB                0x2021
#define WGL_DEPTH_BITS_ARB                      0x2022
#define WGL_STENCIL_BITS_ARB                    0x2023
#define WGL_AUX_BUFFERS_ARB                     0x2024

#define WGL_NO_ACCELERATION_ARB                 0x2025
#define WGL_GENERIC_ACCELERATION_ARB            0x2026
#define WGL_FULL_ACCELERATION_ARB               0x2027

#define WGL_SWAP_EXCHANGE_ARB                   0x2028
#define WGL_SWAP_COPY_ARB                       0x2029
#define WGL_SWAP_UNDEFINED_ARB                  0x202A

#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_TYPE_COLORINDEX_ARB                 0x202C

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

#define ERROR_INVALID_VERSION_ARB               0x2095
#define ERROR_INVALID_PROFILE_ARB               0x2096

static void*
get_procedure(char *name){
    void *result = wglGetProcAddress(name);
    return(result);
}

typedef HGLRC wglCreateContextAttribsARB_Function(HDC hDC,
                                                  HGLRC hShareContext,
                                                  const int *attribList);

typedef BOOL wglChoosePixelFormatARB_Function(HDC hdc,
                                              const int *piAttribIList,
                                              const FLOAT *pfAttribFList,
                                              UINT nMaxFormats,
                                              int *piFormats,
                                              UINT *nNumFormats);

LRESULT
window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    LRESULT result = 0;
    switch (uMsg){
        case WM_CREATE:
        {}break;
        
        case WM_CLOSE:
        case WM_DESTROY:
        {
            ExitProcess(0);
        }break;
        
        default:
        {
            result = DefWindowProc(hwnd, uMsg, wParam, lParam);
        }break;
    }
    return(result);
}

HWND
window_setup(HINSTANCE hInstance){
#define L_STARTER_WINDOW_CLASS_NAME L"starter-window"
#define L_REAL_WINDOW_CLASS_NAME L"window"
    wchar_t title[] = L"Example DirectWrite Based Rasterizer";
    
    // NOTE(allen): Setup a starter window
    WNDCLASSEX starter_window_class = {0};
    starter_window_class.cbSize = sizeof(starter_window_class);
    starter_window_class.lpfnWndProc = DefWindowProc;
    starter_window_class.hInstance = hInstance;
    starter_window_class.lpszClassName = L_STARTER_WINDOW_CLASS_NAME;
    ATOM starter_window_class_atom = RegisterClassEx(&starter_window_class);
    assert(starter_window_class_atom != 0);
    
    HWND starter_window = CreateWindowEx(0, L_STARTER_WINDOW_CLASS_NAME, L"",
                                         0,
                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                         NULL,
                                         NULL,
                                         hInstance,
                                         0);
    
    assert(starter_window != 0);
    
    HDC hdc = GetDC(starter_window);
    assert(hdc != 0);
    
    PIXELFORMATDESCRIPTOR px_format = {0};
    px_format.nSize = sizeof(px_format);
    px_format.nVersion = 1;
    px_format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    px_format.iPixelType = PFD_TYPE_RGBA;
    px_format.cColorBits = 24;
    
    int32_t pixel_format_index = ChoosePixelFormat(hdc, &px_format);
    assert(pixel_format_index != 0);
    
    bool32 success = DescribePixelFormat(hdc, pixel_format_index, sizeof(px_format), &px_format);
    assert(success);
    success = SetPixelFormat(hdc, pixel_format_index, &px_format);
    assert(success);
    
    HGLRC starter_context = wglCreateContext(hdc);
    assert(starter_context != 0);
    
    wglMakeCurrent(hdc, starter_context);
    
    // NOTE(allen): Get extensions from the starter window
    wglCreateContextAttribsARB_Function *wglCreateContextAttribsARB = (wglCreateContextAttribsARB_Function*)get_procedure("wglCreateContextAttribsARB");
    
    wglChoosePixelFormatARB_Function *wglChoosePixelFormatARB = (wglChoosePixelFormatARB_Function*)get_procedure("wglChoosePixelFormatARB");
    
    // NOTE(allen): Setup the real window
    WNDCLASSEX real_window_class = {0};
    real_window_class.cbSize = sizeof(real_window_class);
    real_window_class.style = CS_HREDRAW | CS_VREDRAW;
    real_window_class.lpfnWndProc = window_proc;
    real_window_class.hInstance = hInstance;
    real_window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    real_window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    real_window_class.lpszClassName = L_REAL_WINDOW_CLASS_NAME;
    real_window_class.hIconSm = NULL;
    ATOM real_window_class_atom = RegisterClassEx(&real_window_class);
    assert(real_window_class_atom != 0);
    
    RECT window_rect = {0, 0, window_width, window_height};
    AdjustWindowRect(&window_rect, WS_OVERLAPPED, FALSE);
    
    uint32_t real_style = WS_OVERLAPPEDWINDOW;
    HWND real_window = CreateWindowEx(0, L_REAL_WINDOW_CLASS_NAME, (wchar_t*)title,
                                      real_style,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      window_rect.right - window_rect.left, 
                                      window_rect.bottom - window_rect.top,
                                      NULL,
                                      NULL,
                                      hInstance,
                                      0);
    
    DWORD error_code = GetLastError();
    
    assert(real_window != 0);
    
    HDC real_hdc = GetDC(real_window);
    assert(real_hdc != 0);
    
    int32_t px_format_attributes[] = {
        WGL_DRAW_TO_WINDOW_ARB, TRUE,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_SUPPORT_OPENGL_ARB, TRUE,
        WGL_DOUBLE_BUFFER_ARB, TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        0,
    };
    int32_t real_pixel_format_index = 0;
    uint32_t number_of_formats = 0;
    success = wglChoosePixelFormatARB(hdc, px_format_attributes, 0,
                                      1, &real_pixel_format_index, &number_of_formats);
    assert(success);
    assert(number_of_formats != 0);
    
    PIXELFORMATDESCRIPTOR real_px_format = {0};
    DescribePixelFormat(hdc, real_pixel_format_index, sizeof(real_px_format), &real_px_format);
    success = SetPixelFormat(real_hdc, real_pixel_format_index, &real_px_format);
    assert(success);
    
    int32_t context_attributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 0,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
    };
    HGLRC real_context = wglCreateContextAttribsARB(real_hdc, 0, context_attributes);
    assert(real_context != 0);
    wglMakeCurrent(real_hdc, real_context);
    
    ReleaseDC(real_window, real_hdc);
    
    // NOTE(allen): Close the starter window
    ReleaseDC(starter_window, hdc);
    success = wglDeleteContext(starter_context);
    assert(success);
    DestroyWindow(starter_window);
    
    // NOTE(allen): Load functions
#define GL_FUNC(N,R,P) N = (N##_Type*)get_procedure(#N);
#include "example_gl_funcs.h"
#define GL_FUNC(N,R,P) assert(N != 0);
#include "example_gl_funcs.h"
    
    return(real_window);
}

