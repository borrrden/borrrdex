#pragma once

#include <stdint.h>

constexpr uint8_t ANSI_ESC = 27;

constexpr uint8_t ESC_SAVE_CURSOR       = '7';
constexpr uint8_t ESC_RESTORE_CURSOR    = '8';

constexpr uint8_t ANSI_DCS  = 'P'; // Device Control String
constexpr uint8_t ANSI_CSI  = '['; // Control 
constexpr uint8_t ANSI_ST   = '\\'; // String Terminator
constexpr uint8_t ANSI_OSC  = ']'; // Operating System Command
constexpr uint8_t ANSI_SOS  = 'X'; // Start of String
constexpr uint8_t ANSI_PM   = '^'; // Privacy Message
constexpr uint8_t ANSI_APC  = '_'; // Application Program Command
constexpr uint8_t ANSI_RIS  = 'c'; // Reset to initial state

constexpr uint8_t ANSI_CSI_CUU = 'A'; // Cursor Up
constexpr uint8_t ANSI_CSI_CUD = 'B'; // Cursor Down
constexpr uint8_t ANSI_CSI_CUF = 'C'; // Cursor Forward
constexpr uint8_t ANSI_CSI_CUB = 'D'; // Cursor Back
constexpr uint8_t ANSI_CSI_CNL = 'E'; // Cursor Next Line
constexpr uint8_t ANSI_CSI_CPL = 'F'; // Cursor Previous Line
constexpr uint8_t ANSI_CSI_CHA = 'G'; // Cursor Horizontal Absolute
constexpr uint8_t ANSI_CSI_CUP = 'H'; // Cursor Position
constexpr uint8_t ANSI_CSI_ED  = 'J'; // Erase in Display
constexpr uint8_t ANSI_CSI_EL  = 'K'; // Erase in Line
constexpr uint8_t ANSI_CSI_IL  = 'L'; // Insert n Blank Lines
constexpr uint8_t ANSI_CSI_DL  = 'M'; // Delete n Lines
constexpr uint8_t ANSI_CSI_SU  = 'S'; // Scroll Up
constexpr uint8_t ANSI_CSI_SD  = 'T'; // Scroll Down

constexpr uint8_t ANSI_CSI_SGR                      = 'm'; // Set Graphics Rendition
constexpr uint8_t ANSI_CSI_SGR_RESET                = 0;
constexpr uint8_t ANSI_CSI_SGR_BOLD                 = 1;
constexpr uint8_t ANSI_CSI_SGR_FAINT                = 2; // Faint/Halfbright
constexpr uint8_t ANSI_CSI_SGR_ITALIC               = 3;
constexpr uint8_t ANSI_CSI_SGR_UNDERLINE            = 4;
constexpr uint8_t ANSI_CSI_SGR_SLOW_BLINK           = 5;
constexpr uint8_t ANSI_CSI_SGR_RAPID_BLINK          = 6;
constexpr uint8_t ANSI_CSI_SGR_REVERSE_VIDEO        = 7; // Swap foreground and background colours
constexpr uint8_t ANSI_CSI_SGR_CONCEAL              = 8;
constexpr uint8_t ANSI_CSI_SGR_STRIKETHROUGH        = 9;
constexpr uint8_t ANSI_CSI_SGR_FONT_PRIMARY         = 10;
constexpr uint8_t ANSI_CSI_SGR_FONT_ALTERNATE       = 11; // Select font n - 10
constexpr uint8_t ANSI_CSI_SGR_DOUBLE_UNDERLINE     = 21;
constexpr uint8_t ANSI_CSI_SGR_NORMAL_INTENSITY     = 22; // Disable blink and bold
constexpr uint8_t ANSI_CSI_SGR_UNDERLINE_OFF        = 24;
constexpr uint8_t ANSI_CSI_SGR_BLINK_OFF            = 25;
constexpr uint8_t ANSI_CSI_SGR_REVERSE_VIDEO_OFF    = 27;

constexpr uint8_t ANSI_CSI_SGR_FG_BLACK     = 30; // Set foreground colour to black
constexpr uint8_t ANSI_CSI_SGR_FG_RED       = 31;
constexpr uint8_t ANSI_CSI_SGR_FG_GREEN     = 32;
constexpr uint8_t ANSI_CSI_SGR_FG_BROWN     = 33;
constexpr uint8_t ANSI_CSI_SGR_FG_BLUE      = 34;
constexpr uint8_t ANSI_CSI_SGR_FG_MAGENTA   = 35;
constexpr uint8_t ANSI_CSI_SGR_FG_CYAN      = 36;
constexpr uint8_t ANSI_CSI_SGR_FG_WHITE     = 37;
constexpr uint8_t ANSI_CSI_SGR_FG           = 38; // Set foreground to 256/24-bit colour (;5;x (256 colour) or ;2;r;g;b (24bit colour))
constexpr uint8_t ANSI_CSI_SGR_FG_DEFAULT   = 39;

constexpr uint8_t ANSI_CSI_SGR_BG_BLACK     = 40; // Set background colour to black
constexpr uint8_t ANSI_CSI_SGR_BG_RED       = 41;
constexpr uint8_t ANSI_CSI_SGR_BG_GREEN     = 42;
constexpr uint8_t ANSI_CSI_SGR_BG_BROWN     = 43;
constexpr uint8_t ANSI_CSI_SGR_BG_BLUE      = 44;
constexpr uint8_t ANSI_CSI_SGR_BG_MAGENTA   = 45;
constexpr uint8_t ANSI_CSI_SGR_BG_CYAN      = 46;
constexpr uint8_t ANSI_CSI_SGR_BG_WHITE     = 47;
constexpr uint8_t ANSI_CSI_SGR_BG           = 48; // Set background to 256/24-bit colour
constexpr uint8_t ANSI_CSI_SGR_BG_DEFAULT   = 49;

constexpr uint8_t ANSI_CSI_SGR_FG_BLACK_BRIGHT      = 90; // Set foreground colour to black
constexpr uint8_t ANSI_CSI_SGR_FG_RED_BRIGHT        = 91;
constexpr uint8_t ANSI_CSI_SGR_FG_GREEN_BRIGHT      = 92;
constexpr uint8_t ANSI_CSI_SGR_FG_BROWN_BRIGHT      = 93;
constexpr uint8_t ANSI_CSI_SGR_FG_BLUE_BRIGHT       = 94;
constexpr uint8_t ANSI_CSI_SGR_FG_MAGENTA_BRIGHT    = 95;
constexpr uint8_t ANSI_CSI_SGR_FG_CYAN_BRIGHT       = 96;
constexpr uint8_t ANSI_CSI_SGR_FG_WHITE_BRIGHT      = 97;

constexpr uint8_t ANSI_CSI_SGR_BG_BLACK_BRIGHT      = 100; // Set background colour to black
constexpr uint8_t ANSI_CSI_SGR_BG_RED_BRIGHT        = 101;
constexpr uint8_t ANSI_CSI_SGR_BG_GREEN_BRIGHT      = 102;
constexpr uint8_t ANSI_CSI_SGR_BG_BROWN_BRIGHT      = 103;
constexpr uint8_t ANSI_CSI_SGR_BG_BLUE_BRIGHT       = 104;
constexpr uint8_t ANSI_CSI_SGR_BG_MAGENTA_BRIGHT    = 105;
constexpr uint8_t ANSI_CSI_SGR_BG_CYAN_BRIGHT       = 106;
constexpr uint8_t ANSI_CSI_SGR_BG_WHITE_BRIGHT      = 107;