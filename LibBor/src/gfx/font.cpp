#include <borrrdex/graphics/font.h>

#include <cassert>
#include <map>
#include <string>
#include <stdexcept>
#include <vector>

namespace borrrdex::graphics {
    const char* font_exception::error_strings[] = {
        "Unknown font error",
        "Failed to open font file",
        "Freetype error on loading font",
        "Error setting font size",
        "Error rendering font"
    };

    font main_font;
    static FT_Library library;
    int font_state = 0;

    static std::map<std::string, font>* fonts; //Don't rely on initialization order for this

    __attribute__((constructor))
    void initialize_fonts() {
        font_state = -1;
        if(FT_Init_FreeType(&library)) {
            printf("Error initializing freetype\n");
            return;
        }

        if(int err = FT_New_Face(library, "/system/borrrdex/fonts/notosans.ttf", 0, &main_font.face)) {
            printf("Freetype Error (%d) loading font /system/borrrdex/fonts/notosans.ttf\n", err);
            return;
        }

        main_font.height = 24;
        if(int err = FT_Set_Pixel_Sizes(main_font.face, 0, main_font.height / 72.f * 96)) {
            printf("Freetype Error (%d) Setting Font Size\n", err);
            return;
        }

        main_font.pixel_height = main_font.height / 72.f * 96;
        main_font.height = main_font.pixel_height;
        main_font.line_height = main_font.face->size->metrics.height / 64;
        main_font.id = (char *)malloc(8);
        main_font.width = 8;
        main_font.tab_width = 4;
        strncpy(main_font.id, "default", 8);

        fonts = new std::map<std::string, font>();
        font_state = 1;
    }

    const font& load_font(const char* path, const char* id, int sz) {
        font f;
        if(int err = FT_New_Face(library, path, 0, &f.face)) {
            throw font_exception(font_exception::font_error::load_error, err);
        }

        if(int err = FT_Set_Pixel_Sizes(f.face, 0, sz / 72.f * 96)) {
            throw font_exception(font_exception::font_error::size_error, err);
        }

        if(!id) {
            char buf[32];
            sprintf(buf, "%s", path);
            f.id = (char *)malloc(strlen(buf) + 1);
            strncpy(f.id, buf, 32);
        } else {
            f.id = (char *)malloc(strlen(id) + 1);
            strcpy(f.id, id);
        }

        f.pixel_height = sz / 72.f * 96;
        f.height = f.pixel_height;
        f.line_height = f.face->size->metrics.height / 64;
        f.monospace = FT_IS_FIXED_WIDTH(f.face);
        f.width = 8;
        f.tab_width = 4;

        assert(fonts);
        fonts->insert({f.id, f});

        std::string foo;
        std::vector<char> v(foo.begin(), foo.end());

        font_state = 1;
        return fonts->at(f.id);
    }

    const font& get_font(const char* id) {
        try {
            return fonts->at(id);
        } catch(std::out_of_range&) {
            throw font_exception(font_exception::font_error::unknown_font);
        }
    }

    const font& default_font() {
        if(font_state != 1) {
            initialize_fonts();
        }

        return main_font;
    }
}