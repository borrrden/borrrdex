#include <borrrdex/graphics/font.h>
#include <borrrdex/graphics/surface.h>
#include <borrrdex/graphics/graphics.h>

#include <cctype>

namespace borrrdex::graphics {
    extern int font_state;

    int draw_char(char c, int x, int y, uint8_t r, uint8_t g, uint8_t b, surface_t* surface, font* font) {
        if(!isprint(c)) {
            return 0;
        }

        if(y >= surface->height || x >= surface->width) {
            return 0;
        }

        if(font_state != 1 || !font->face) {
            initialize_fonts();
        }

        uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;
        uint32_t* buffer = (uint32_t *)surface->buffer;
        if(int err = FT_Load_Char(font->face, c, FT_LOAD_RENDER)) {
            return 0;
        }

        int max_height = font->line_height - (font->height - font->face->glyph->bitmap_top);
        if(y + max_height >= surface->height) {
            max_height = surface->height - y;
        }

        if(y > 0 && -y > max_height) {
            return 0;
        }

        int i = y < 0 ? -y : 0;
        if(y + (font->height- font->face->glyph->bitmap_top) < 0) {
            i = -(y + (font->height- font->face->glyph->bitmap_top));
        }

        for(; i < (int)font->face->glyph->bitmap.rows && i < max_height; i++) {
            if(y + i < 0) {
                continue;
            }

            uint32_t y_off = (i + y + (font->height - font->face->glyph->bitmap_top)) * surface->width;
            for(unsigned j = 0; j < font->face->glyph->bitmap.width && (long)j + x < surface->width; j++) {
                if(x + (long)j < 0) {
                    continue;
                }

                unsigned char font_val = font->face->glyph->bitmap.buffer[i * font->face->glyph->bitmap.width + j];
                if(font_val == 255) {
                    buffer[y_off + j + x] = color;
                } else if(font_val > 0) {
                    double color_val = (double)font_val / 255.0;
                    buffer[y_off + j + x] = alpha_blend(buffer[y_off + j + x], r, g, b, color_val);
                }
            }
        }

        return font->face->glyph->advance.x >> 6;
    }

    int draw_string(const char* str, int x, int y, uint8_t r, uint8_t g, uint8_t b, surface_t* surface, font* font) {
        if(y >= surface->height || x >= surface->width) {
            return 0;
        }

        uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;
        uint32_t* buffer = (uint32_t *)surface->buffer;
        unsigned int max_height = font->line_height;
        if(y < 0 && (unsigned)-y > max_height) {
            return 0;
        }

        if(y + (int)max_height >= surface->height) {
            max_height = surface->height - y;
        }

        if(font_state != 1 || !font->face) {
            initialize_fonts();
        }

        unsigned int last_glyph = 0;
        int x_off = x;
        while(*str) {
            if(*str == '\n') {
                break;
            }

            if(!isprint(*str)) {
                str++;
                continue;
            }

            unsigned glyph = FT_Get_Char_Index(font->face, *str);
            if(FT_HAS_KERNING(font->face) && last_glyph) {
                FT_Vector delta;
                FT_Get_Kerning(font->face, last_glyph, glyph, FT_KERNING_DEFAULT, &delta);
                x_off += delta.x >> 6;
            }

            last_glyph = glyph;
            if(int err = FT_Load_Glyph(font->face, glyph, FT_LOAD_RENDER)) {
                str++;
                continue;
            }

            if(x_off + (font->face->glyph->advance.x >> 6) < 0) {
                x_off += font->face->glyph->advance.x >> 6;
                str++;
                continue;
            }

            int i = y < 0 ? -y : 0;
            if(y + (font->height- font->face->glyph->bitmap_top) < 0) {
                i = -(y + (font->height- font->face->glyph->bitmap_top));
            }

            for(; i < font->face->glyph->bitmap.rows && i + (font->height - font->face->glyph->bitmap_top) < max_height; i++) {
                uint32_t y_off = (i + y + (font->height - font->face->glyph->bitmap_top)) * surface->width;
                int j = 0;
                if(x_off < 0) {
                    j = -x;
                }

                for(; j < font->face->glyph->bitmap.width && (x_off + (long)j) < surface->width; j++) {
                    unsigned off = y_off + (j + x_off);
                    unsigned char font_val = font->face->glyph->bitmap.buffer[i * font->face->glyph->bitmap.width + j];
                    if(font_val == 255) {
                        buffer[off] = color;
                    } else if(font_val > 0) {
                        double color_val = (double)font_val / 255.0;
                        buffer[off] = alpha_blend(buffer[off], r, g, b, color_val);
                    }
                }
            }

            x_off += font->face->glyph->advance.x >> 6;
            str++;
        }

        return x_off - x;
    }
}