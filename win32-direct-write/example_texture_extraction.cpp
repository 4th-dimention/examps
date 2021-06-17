// DirectWrite texture extraction example

#include <windows.h>
#include <dwrite.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

// Bitmap structs
#pragma pack(push, 1)
struct Header{
    char sig[2];
    uint32_t file_size;
    uint32_t reserved;
    uint32_t data_offset;
};

struct Info_Header{
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    uint32_t x_pixels_per_meter;
    uint32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t important_colors;
};

struct Color_Table{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t A;
};
#pragma pack(pop)

int main(){
    COLORREF back_color = RGB(0,0,0);
    COLORREF fore_color = RGB(255,0,0);
    int32_t raster_target_w = 200;
    int32_t raster_target_h = 200;
    float raster_target_x = 100.f;
    float raster_target_y = 100.f;
    wchar_t font_path[] = L"C:\\Windows\\Fonts\\arial.ttf";
    float point_size = 12.f;
    uint32_t codepoint = '?';
    char *test_output_file_name = "test.bmp";
    
    HRESULT error = 0;
    
    // DWrite Factory
    IDWriteFactory *dwrite_factory = 0;
    error = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwrite_factory);
    assert(error == S_OK);
    
    // DWrite Font File Reference
    IDWriteFontFile *font_file = 0;
    error = dwrite_factory->CreateFontFileReference(font_path, 0, &font_file);
    assert(error == S_OK);
    
    // DWrite Font Face
    IDWriteFontFace *font_face = 0;
    error = dwrite_factory->CreateFontFace(DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &font_file, 0, DWRITE_FONT_SIMULATIONS_NONE, &font_face);
    assert(error == S_OK);
    
    // DWrite Rendering Params
    IDWriteRenderingParams *base_rendering_params = 0;
    error = dwrite_factory->CreateRenderingParams(&base_rendering_params);
    assert(error == S_OK);
    
    IDWriteRenderingParams *rendering_params = 0;
    {
        FLOAT gamma = 1.f;
        //FLOAT gamma = base_rendering_params->GetGamma();
        FLOAT enhanced_contrast = base_rendering_params->GetEnhancedContrast();
        FLOAT clear_type_level = base_rendering_params->GetClearTypeLevel();
        error = dwrite_factory->CreateCustomRenderingParams(
            gamma,
            enhanced_contrast,
            clear_type_level,
            DWRITE_PIXEL_GEOMETRY_RGB,
            DWRITE_RENDERING_MODE_NATURAL,
            &rendering_params);
        assert(error == S_OK);
    }
    
    // DWrite GDI Interop
    IDWriteGdiInterop *dwrite_gdi_interop = 0;
    error = dwrite_factory->GetGdiInterop(&dwrite_gdi_interop);
    assert(error == S_OK);
    
    // DWrite Bitmap Render Target
    IDWriteBitmapRenderTarget *render_target = 0;
    error = dwrite_gdi_interop->CreateBitmapRenderTarget(0, raster_target_w, raster_target_h, &render_target);
    assert(error == S_OK);
    
    // Clear the Render Target
    HDC dc = render_target->GetMemoryDC();
    
    {
        HGDIOBJ original = SelectObject(dc, GetStockObject(DC_PEN));
        SetDCPenColor(dc, back_color);
        SelectObject(dc, GetStockObject(DC_BRUSH));
        SetDCBrushColor(dc, back_color);
        Rectangle(dc, 0, 0, raster_target_w, raster_target_h);
        SelectObject(dc, original);
    }
    
    // Find the glyph index for the codepoint we want to render
    uint16_t index = 0;
    error = font_face->GetGlyphIndices(&codepoint, 1, &index);
    assert(error == S_OK);
    
    // Render the glyph
    DWRITE_GLYPH_RUN glyph_run = {0};
    glyph_run.fontFace = font_face;
    glyph_run.fontEmSize = point_size*96.f/72.f;
    glyph_run.glyphCount = 1;
    glyph_run.glyphIndices = &index;
    RECT bounding_box = {0};
    error = render_target->DrawGlyphRun(raster_target_x, raster_target_y, DWRITE_MEASURING_MODE_NATURAL, &glyph_run, rendering_params, fore_color, &bounding_box);
    assert(error == S_OK);
    
    // Get the Bitmap
    HBITMAP bitmap = (HBITMAP)GetCurrentObject(dc, OBJ_BITMAP);
    DIBSECTION dib = {0};
    GetObject(bitmap, sizeof(dib), &dib);
    
    // Save the Bitmap
    {
        uint8_t *in_data = (uint8_t*)dib.dsBm.bmBits;
        int32_t in_pitch = dib.dsBm.bmWidthBytes;
        int32_t width = raster_target_w;
        int32_t height = raster_target_h;
        char *file_name = test_output_file_name;
        
        void *memory = (void*)malloc(1 << 20);
        assert(memory != 0);
        void *ptr = memory;
        int32_t out_pitch = (width*3) + 3;
        out_pitch = out_pitch - (out_pitch%4);
        
        Header *header = (Header*)ptr;
        ptr = header + 1;
        Info_Header *info_header = (Info_Header*)ptr;
        ptr = info_header + 1;
        uint8_t *out_data = (uint8_t*)ptr;
        ptr = out_data + out_pitch*height;
        
        header->sig[0] = 'B';
        header->sig[1] = 'M';
        header->file_size = (uint8_t*)ptr - (uint8_t*)memory;
        header->reserved = 0;
        header->data_offset = out_data - (uint8_t*)memory;
        info_header->size = sizeof(*info_header);
        info_header->width = width;
        info_header->height = height;
        info_header->planes = 1;
        info_header->bits_per_pixel = 24;
        info_header->compression = 0;
        info_header->image_size = 0;
        info_header->x_pixels_per_meter = 0;
        info_header->y_pixels_per_meter = 0;
        info_header->colors_used = 0;
        info_header->important_colors = 0;
        
        uint8_t *in_line = (uint8_t*)in_data;
        uint8_t *out_line = out_data + out_pitch*(height - 1);
        for (int32_t y = 0; y < height; y += 1){
            uint8_t *in_pixel = in_line;
            uint8_t *out_pixel = out_line;
            for (int32_t x = 0; x < width; x += 1){
                out_pixel[0] = in_pixel[0];
                out_pixel[1] = in_pixel[1];
                out_pixel[2] = in_pixel[2];
                in_pixel += 4;
                out_pixel += 3;
            }
            in_line += in_pitch;
            out_line -= out_pitch;
        }
        
        FILE *out = fopen(file_name, "wb");
        assert(out != 0);
        fwrite(memory, 1, header->file_size, out);
        fclose(out);
    }
    
    return(0);
}

