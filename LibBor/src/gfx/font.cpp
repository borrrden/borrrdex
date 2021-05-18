#include <borrrdex/graphics/font.h>

#include <map>
#include <string>

namespace borrrdex::graphics {
    const char* font_exception::error_strings[] = {
        "Unknown font error",
        "Failed to open font file",
        "Freetype error on loading font",
        "Error setting font size",
        "Error rendering font"
    };

    font* main_font = nullptr;
    static FT_Library library;
    int font_state = 0;

    static std::map<std::string, font *>* fonts; //Don't rely on initialization order for this

    __attribute__((constructor))
    void intialize_fonts() {
        font_state = -1;
        if(FT_Init_FreeType(&library)) {
            printf("Error initializing freetype\n");
            return;
        }

        main_font = new font();
        if(int err = FT_New_Face(library, "/system/borrrdex/fonts/sourcecodepro.ttf", 0, &main_font->face)) {
            printf("Freetype Error (%d) loading font /system/borrrdex/fonts/sourcecodepro.ttf\n", err);
            return;
        }

        main_font->height = 10;
        if(int err = FT_Set_Pixel_Sizes(main_font->face, 0, main_font->height / 72.f * 96)) {
            printf("Freetype Error (%d) Setting Font Size\n", err);
            return;
        }

        main_font->pixel_height = main_font->height / 72.f * 96;
        main_font->height = main_font->pixel_height;
        main_font->line_height = main_font->face->size->metrics.height / 64;
        main_font->id = (char *)malloc(8);
        main_font->width = 8;
        main_font->tab_width = 4;
        strncpy(main_font->id, "default", 8);

        fonts = new std::map<std::string, font *>();
        font_state = 1;
    }
}