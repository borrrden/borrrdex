#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <borrrdex/core/framebuffer.h>
#include <borrrdex/core/keyboard.h>
#include <borrrdex/graphics/font.h>
#include <borrrdex/graphics/graphics.h>
#include <borrrdex/graphics/types.h>
#include <borrrdex/syscall.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <vector>
#include <cctype>

#include "fterm.h"
#include "escape.h"
#include "colors.h"

using namespace borrrdex::graphics;
using namespace std;

constexpr uint16_t ESC_BUFFER_MAX = 256;

surface_t fb;
surface_t render;
int pty;
font terminal_font;
winsize w_size;
char esc_buffer[ESC_BUFFER_MAX];
vector2i_t current_pos {0, 0};

struct terminal_state {
    bool bold:1;
    bool italic:1;
    bool faint:1;
    bool underline:1;
    bool blink:1;
    bool reverse:1;
    bool strikethrough:1;

    uint8_t fg_color {33};
    uint8_t bg_color {0};

    terminal_state() {
        bold = false;
        italic = false;
        faint = false;
        underline = false;
        blink = false;
        reverse = false;
        strikethrough = false;
    }
};

struct terminal_char {
    terminal_char()
    {

    }

    terminal_char(terminal_state s, char c)
        :s(s)
        ,c(c)
    {

    }

    terminal_state s;
    char c {' '};
};

terminal_state state;
vector<vector<terminal_char>> screen_buffer;

static void on_key(int key) {
    const char* esc = nullptr;
    switch(key) {
        case KEY_ARROW_UP:
            esc = "\e[A";
            break;
        case KEY_ARROW_DOWN:
            esc = "\e[B";
            break;
        case KEY_ARROW_RIGHT:
            esc = "\e[C";
            break;
        case KEY_ARROW_LEFT:
            esc = "\e[D";
            break;
        case KEY_END:
            esc = "\e[F";
            break;
        case KEY_HOME:
            esc = "\e[H";
            break;
        case KEY_ESCAPE:
            esc = "\e\e";
            break;
        default:
            break;
    }

    if(esc) {
        write(pty, esc, strlen(esc));
    } else if(key < 128) {
        write(pty, &key, 1);
    }
}

bool is_escape_seq = false;
int escape_type = 0;
static void do_ansi_sgr() {
    int r = -1;
    char* scolon = strchr(esc_buffer, ';');
    if(scolon) {
        char tmp[scolon - esc_buffer + 1];
        tmp[scolon - esc_buffer] = 0;
        strncpy(tmp, esc_buffer, scolon - esc_buffer);
        r = atoi(tmp);
    } else {
        r = atoi(esc_buffer);
    }

    if(r < 30) {
        switch(r) {
            case 0:
                state = terminal_state();
                break;
            case 1:
                state.bold = true;
                break;
            case 2:
                state.faint = true;
                break;
            case 3:
                state.italic = true;
                break;
            case 4:
                state.underline = true;
                break;
            case 5:
            case 6:
                state.blink = true;
                break;
            case 7:
                state.reverse = true;
                break;
            case 9:
                state.strikethrough = true;
                break;
            case 21:
                state.underline = true;
                break;
            case 24:
                state.underline = false;
                break;
            case 25:
                state.blink = false;
                break;
            case 27:
                state.reverse = false;
                break;
            default:
                break;
        }
    } else if(r >= ANSI_CSI_SGR_FG_BLACK && r <= ANSI_CSI_SGR_FG_WHITE) {
        state.fg_color = r - ANSI_CSI_SGR_FG_BLACK;
    } else if(r >= ANSI_CSI_SGR_BG_BLACK && r <= ANSI_CSI_SGR_BG_WHITE) {
        state.bg_color = r - ANSI_CSI_SGR_BG_BLACK;
    } else if(r >= ANSI_CSI_SGR_FG_BLACK_BRIGHT && r <= ANSI_CSI_SGR_FG_WHITE_BRIGHT) {
        state.fg_color = r - ANSI_CSI_SGR_FG_BLACK_BRIGHT + 8;
    } else if(r >= ANSI_CSI_SGR_BG_BLACK_BRIGHT && r <= ANSI_CSI_SGR_BG_WHITE_BRIGHT) {
        state.bg_color = r - ANSI_CSI_SGR_BG_BLACK_BRIGHT + 8;
    } else if(r == ANSI_CSI_SGR_FG) {
        if(scolon && *(scolon + 1) == '5') {
            scolon = strchr(scolon + 1, ';');
            if(scolon) {
                state.fg_color = atoi(scolon + 1);
            }
        }
    } else if(r == ANSI_CSI_SGR_BG) {
        if(scolon && *(scolon + 1) == '5') {
            scolon = strchr(scolon + 1, ';');
            if(scolon) {
                state.bg_color = atoi(scolon + 1);
            }
        }
    }
}

static void do_cursor(bool on) {
    auto font_height = terminal_font.line_height;
    auto color = on ? state.fg_color : state.bg_color;
    draw_rect(current_pos.x * terminal_font.width, current_pos.y * font_height + (font_height / 4 * 3),
            terminal_font.width, font_height / 4, colors[color], &render);
}

static void do_prompt() {
    printf("borrrdex> ");
    fflush(stdout);
}

static void scroll() {
    bool redraw = false;
    while(current_pos.y >= w_size.ws_row) {
        redraw = true;
        screen_buffer.erase(screen_buffer.begin());
        screen_buffer.emplace_back();

        current_pos.y--;
    }

    if(redraw) {
        draw_rect(0, 0, render.width, render.height, colors[state.bg_color], &render);
    }
}

static void do_ansi_csi(char ch) {
    switch(ch) {
        case ANSI_CSI_SGR:
            do_ansi_sgr();
            break;
        case ANSI_CSI_CUU: {
            int amount = 1;
            if(strlen(esc_buffer)) {
                amount = atoi(esc_buffer);
            }

            current_pos.y = std::max(current_pos.y - amount, 0);
            break;
        }
        case ANSI_CSI_CUD: {
            int amount = 1;
            if(strlen(esc_buffer)) {
                amount = atoi(esc_buffer);
            }

            current_pos.y += amount;
            scroll();
            break;
        }
        case ANSI_CSI_CUF: {
            int amount = 1;
            if(strlen(esc_buffer)) {
                amount = atoi(esc_buffer);
            }

            current_pos.x += amount;
            while(current_pos.x > w_size.ws_col) {
                current_pos.x -= w_size.ws_col;
                current_pos.y++;
                scroll();
            }

            break;
        }
        case ANSI_CSI_CUB: {
            int amount = 1;
            if(strlen(esc_buffer)) {
                amount = atoi(esc_buffer);
            }

            current_pos.x -= amount;
            while(current_pos.x < 0) {
                current_pos.x += w_size.ws_col;
                current_pos.y--;
            }

            if(current_pos.y < 0) {
                current_pos.x = current_pos.y = 0;
            }

            break;
        }
        default:
            break;
    }
}

static void do_ansi_osc(char ch) {

}

static void print_char(char ch) {
    if(is_escape_seq) {
        if(isspace(ch)) {
            return;
        }

        if(!escape_type) {
            escape_type = ch;
            ch = 0;
        }

        switch(escape_type) {
            case ANSI_CSI:
            case ANSI_OSC:
                if(!isalpha(ch)) {
                    if(strnlen(esc_buffer, ESC_BUFFER_MAX - 1) == ESC_BUFFER_MAX - 1) {
                        is_escape_seq = false;
                        return;
                    }

                    char s[2] = { ch, 0 };
                    strncat(esc_buffer, s, 2);
                    return;
                }

                if(escape_type == ANSI_CSI) {
                    do_ansi_csi(ch);
                } else {
                    do_ansi_osc(ch);
                }

                break;
        }
    } else {
        switch(ch) {
            case ANSI_ESC:
                is_escape_seq = true;
                escape_type = 0;
                esc_buffer[0] = 0;
                break;
            case '\n':
                do_cursor(false);
                current_pos.y++;
                do_prompt();
                current_pos.x = 0;
                scroll();
                break;
            case '\b':
                if(current_pos.x > 0) {
                    current_pos.x--;
                    draw_rect(current_pos.x * terminal_font.width, current_pos.y * terminal_font.line_height, terminal_font.width * 2, 
                        terminal_font.line_height, colors[state.bg_color], &render);
                }

                break;
            default: {
                if(!(isgraph(ch) || isspace(ch))) {
                    break;
                }

                if(current_pos.x >= screen_buffer[current_pos.y].size()) {
                    screen_buffer[current_pos.y].emplace_back(state, ch);
                } else {
                    screen_buffer[current_pos.y][current_pos.x] = {state, ch};
                }

                auto fg = screen_buffer[current_pos.y][current_pos.x].s.fg_color;
                draw_rect(current_pos.x * terminal_font.width, current_pos.y * terminal_font.line_height, terminal_font.width, 
                        terminal_font.line_height, colors[state.bg_color], &render);
                draw_char(ch, current_pos.x * terminal_font.width, current_pos.y * terminal_font.line_height, colors[fg], &render, &terminal_font);
                current_pos.x++;
                if(current_pos.x > w_size.ws_col) {
                    current_pos.x = 0;
                    current_pos.y++;
                    do_cursor(false);
                    scroll();
                }
            }

                break;
        }
    }
}

static void paint() {
    int line_pos = 0;
    int font_height = terminal_font.line_height;
    // for(const auto& line : screen_buffer) {
    //     for(int j = 0; j < (long)line.size(); j++) {
    //         auto ch = line[j];
    //         auto fg = colors[ch.s.fg_color];
    //         auto bg = colors[ch.s.bg_color];

    //         draw_rect(j * 8, line_pos * font_height, 8, font_height, bg, &render);
    //         if(isprint(ch.c)) {
    //             draw_char(ch.c, j * 8, line_pos * font_height, fg, &render, &terminal_font);
    //         }
    //     }

    //     if(++line_pos >= w_size.ws_row) {
    //         break;
    //     }
    // }

    timespec t;
	clock_gettime(CLOCK_BOOTTIME, &t);
    long msec = (t.tv_nsec / 1000000.0);
    uint8_t color = state.bg_color;

    do_cursor((msec > 250 && msec < 500) || msec >= 750);
    surface_copy(&fb, &render);
}

int main(int argc, char** argv) {
    create_framebuffer_surface(fb);
    render = fb;
    render.buffer = (uint8_t *)malloc(fb.width * fb.height * 4);

    w_size.ws_xpixel = render.width;
    w_size.ws_ypixel = render.height;

    try {
        terminal_font = load_font("/system/borrrdex/fonts/sourcecodepro.ttf", "termmonospace", 12);
    } catch(font_exception&) {
        terminal_font = default_font();
    }

    w_size.ws_col = render.width / terminal_font.width;
    w_size.ws_row = render.height / terminal_font.line_height;

    input_manager input(on_key);

    for(int i = 0; i < w_size.ws_row; i++) {
        screen_buffer.emplace_back(w_size.ws_col);
    }

    syscalln1(SYSCALL_GRANT_PTY, (uintptr_t)&pty);

    setenv("TERM", "xterm-256color", 1);
    ioctl(pty, TIOCSWINSZ, &w_size);

    char buf[512];
    bool do_paint = true;
    draw_rect(0, 0, render.width, render.height, colors[0], &render);
    do_prompt();

    termios initial_attr;
    tcgetattr(STDOUT_FILENO, &initial_attr);
    termios read_attr = initial_attr;
    read_attr.c_lflag &= ~(ICANON | ECHO);

    while(true) {
        input.poll();
        tcsetattr(STDOUT_FILENO, TCSANOW, &read_attr);
        while(int len = read(pty, buf, 512)) {
            for(int i = 0; i < len; i++) {
                print_char(buf[i]);
            }

            do_paint = true;
        }

        fflush(stdout);
        tcsetattr(STDOUT_FILENO, TCSANOW, &initial_attr);

        if(do_paint) {
            paint();
        }
    }

    return 0;
}