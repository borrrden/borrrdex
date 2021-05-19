#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <exception>
#include <memory>

namespace borrrdex::graphics {
    struct font {
        bool monospace {false};
        FT_Face face;
        int height;
        int pixel_height;
        int line_height;
        int width;
        int tab_width {4};
        char* id;
    };

    class font_exception : public std::exception {
    public:
        enum font_error {
            unknown_font,
            file_error,
            load_error,
            size_error,
            render_error
        };


        font_exception(int error_type, int error_code = 0)
            :_error_type(error_type)
            ,_error_code(error_code)
        {

        }

        const char* what() const noexcept override;

    private:
        static const char* error_strings[];

        int _error_type {font_error::unknown_font};
        int _error_code {0};
    };

    void initialize_fonts();
    const font& load_font(const char* path, const char* id = nullptr, int sz = 10);
    const font& get_font(const char* id);
    const font& default_font();
}