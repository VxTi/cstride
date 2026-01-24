#pragma once

// Foreground colors
#define ANSI_BLACK "\x1b[30m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_WHITE "\x1b[37m"

// Bright foreground colors
#define ANSI_BRIGHT_BLACK "\x1b[90m"
#define ANSI_BRIGHT_RED "\x1b[91m"
#define ANSI_BRIGHT_GREEN "\x1b[92m"
#define ANSI_BRIGHT_YELLOW "\x1b[93m"
#define ANSI_BRIGHT_BLUE "\x1b[94m"
#define ANSI_BRIGHT_MAGENTA "\x1b[95m"
#define ANSI_BRIGHT_CYAN "\x1b[96m"
#define ANSI_BRIGHT_WHITE "\x1b[97m"

// Background colors
#define ANSI_BG_BLACK "\x1b[40m"
#define ANSI_BG_RED "\x1b[41m"
#define ANSI_BG_GREEN "\x1b[42m"
#define ANSI_BG_YELLOW "\x1b[43m"
#define ANSI_BG_BLUE "\x1b[44m"
#define ANSI_BG_MAGENTA "\x1b[45m"
#define ANSI_BG_CYAN "\x1b[46m"
#define ANSI_BG_WHITE "\x1b[47m"

// Bright background colors
#define ANSI_BG_BRIGHT_BLACK "\x1b[100m"
#define ANSI_BG_BRIGHT_RED "\x1b[101m"
#define ANSI_BG_BRIGHT_GREEN "\x1b[102m"
#define ANSI_BG_BRIGHT_YELLOW "\x1b[103m"
#define ANSI_BG_BRIGHT_BLUE "\x1b[104m"
#define ANSI_BG_BRIGHT_MAGENTA "\x1b[105m"
#define ANSI_BG_BRIGHT_CYAN "\x1b[106m"
#define ANSI_BG_BRIGHT_WHITE "\x1b[107m"

// Text formatting
#define ANSI_BOLD "\x1b[1m"
#define ANSI_DIM "\x1b[2m"
#define ANSI_ITALIC "\x1b[3m"
#define ANSI_UNDERLINE "\x1b[4m"
#define ANSI_BLINK "\x1b[5m"
#define ANSI_REVERSE "\x1b[7m"
#define ANSI_HIDDEN "\x1b[8m"
#define ANSI_STRIKETHROUGH "\x1b[9m"

// Reset codes
#define ANSI_RESET "\x1b[0m"
#define ANSI_RESET_BOLD "\x1b[22m"
#define ANSI_RESET_DIM "\x1b[22m"
#define ANSI_RESET_ITALIC "\x1b[23m"
#define ANSI_RESET_UNDERLINE "\x1b[24m"
#define ANSI_RESET_BLINK "\x1b[25m"
#define ANSI_RESET_REVERSE "\x1b[27m"
#define ANSI_RESET_HIDDEN "\x1b[28m"
#define ANSI_RESET_STRIKETHROUGH "\x1b[29m"

// Cursor control
#define ANSI_CLEAR_SCREEN "\x1b[2J"
#define ANSI_CLEAR_LINE "\x1b[2K"
#define ANSI_CURSOR_HOME "\x1b[H"
#define ANSI_CURSOR_UP "\x1b[A"
#define ANSI_CURSOR_DOWN "\x1b[B"
#define ANSI_CURSOR_RIGHT "\x1b[C"
#define ANSI_CURSOR_LEFT "\x1b[D"
#define ANSI_SAVE_CURSOR "\x1b[s"
#define ANSI_RESTORE_CURSOR "\x1b[u"
#define ANSI_HIDE_CURSOR "\x1b[?25l"
#define ANSI_SHOW_CURSOR "\x1b[?25h"