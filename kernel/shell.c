/* ============================================================================
 * kernel/shell.c — MiniOS Interactive Command Shell
 * ============================================================================
 */

#include "../include/shell.h"
#include "../include/editor.h"
#include "../include/filesystem.h"
#include "../include/keyboard.h"
#include "../include/memory.h"
#include "../include/screen.h"
#include "../include/timer.h"
#include "../include/pic.h"

#define INPUT_MAX 256

static char input_buf[INPUT_MAX];
static char text_buf[FS_MAX_FILESIZE];

/* History tracking */
#define HISTORY_MAX 10
static char shell_history[HISTORY_MAX][INPUT_MAX];
static int history_count = 0;
static int total_commands_run = 0;

/* Todo tracking */
#define TODO_MAX 20
typedef struct {
  int id;
  int done;
  char text[64];
} todo_item_t;
static todo_item_t todos[TODO_MAX];
static int todo_count = 0;
static int todo_next_id = 1;
static char current_dir[64] = "/root";

static void resolve_path(const char *arg, char *out_path) {
  if (!arg || *arg == '\0') {
    strcpy(out_path, current_dir);
    return;
  }
  if (arg[0] == '/') {
    strncpy(out_path, arg, 63);
    out_path[63] = '\0';
    return;
  }
  
  strcpy(out_path, current_dir);
  if (strcmp(out_path, "/") != 0) {
    strcat(out_path, "/");
  }
  strncat(out_path, arg, 63 - strlen(out_path));
  out_path[63] = '\0';
}

static const char *skip_spaces(const char *s) {
  while (*s == ' ' || *s == '\t')
    s++;
  return s;
}

static char *mini_strstr(const char *haystack, const char *needle) {
  if (!*needle) return (char *)haystack;
  for (const char *h = haystack; *h; h++) {
    const char *h1 = h;
    const char *n1 = needle;
    while (*h1 && *n1 && *h1 == *n1) {
      h1++;
      n1++;
    }
    if (!*n1) return (char *)h;
  }
  return NULL;
}

static void shell_readline(char *buf, size_t len) {
  int line_len = 0;
  int cursor_pos = 0;
  int history_idx = history_count;
  
  buf[0] = '\0';
  
  while (1) {
    char c = keyboard_getchar();
    
    if (c == '\n' || c == '\r') {
      while (cursor_pos < line_len) {
        terminal_putchar(buf[cursor_pos]);
        cursor_pos++;
      }
      terminal_putchar('\n');
      buf[line_len] = '\0';
      break;
    } else if (c == 8 || c == 127) { /* Backspace */
      if (cursor_pos > 0) {
        cursor_pos--;
        line_len--;
        for (int i = cursor_pos; i < line_len; i++) {
          buf[i] = buf[i+1];
        }
        buf[line_len] = '\0';
        
        terminal_putchar(8);
        int save_r = terminal_row();
        int save_c = terminal_col();
        
        for (int i = cursor_pos; i < line_len; i++) {
          terminal_putchar(buf[i]);
        }
        terminal_putchar(' ');
        
        terminal_setcursor(save_r, save_c);
      }
    } else if (c == KEY_LEFT_ARROW) {
      if (cursor_pos > 0) {
        cursor_pos--;
        int r = terminal_row();
        int c_col = terminal_col();
        if (c_col > 0) c_col--;
        else if (r > 0) { r--; c_col = 79; }
        terminal_setcursor(r, c_col);
      }
    } else if (c == KEY_RIGHT_ARROW) {
      if (cursor_pos < line_len) {
        int r = terminal_row();
        int c_col = terminal_col();
        if (c_col < 79) c_col++;
        else { r++; c_col = 0; }
        terminal_setcursor(r, c_col);
        cursor_pos++;
      }
    } else if (c == KEY_UP_ARROW) {
      if (history_idx > 0) {
        history_idx--;
        goto load_history;
      }
    } else if (c == KEY_DOWN_ARROW) {
      if (history_idx < history_count) {
        history_idx++;
        goto load_history;
      }
    } else if (c >= 32 && c < 127) {
      if (line_len < (int)len - 1) {
        for (int i = line_len; i > cursor_pos; i--) {
          buf[i] = buf[i-1];
        }
        buf[cursor_pos] = c;
        line_len++;
        buf[line_len] = '\0';
        
        int save_r = terminal_row();
        int save_c = terminal_col();
        
        for (int i = cursor_pos; i < line_len; i++) {
          terminal_putchar(buf[i]);
        }
        
        cursor_pos++;
        save_c++;
        if (save_c >= 80) { save_c = 0; save_r++; }
        if (save_r >= 24) { save_r = 23; } /* Simple cap for scroll */
        terminal_setcursor(save_r, save_c);
      }
    }
    continue;

  load_history:
    {
      int r = terminal_row();
      int c_col = terminal_col();
      for (int i = 0; i < cursor_pos; i++) {
        if (c_col > 0) c_col--;
        else if (r > 0) { r--; c_col = 79; }
      }
      terminal_setcursor(r, c_col);
      
      for (int i = 0; i < line_len; i++) terminal_putchar(' ');
      terminal_setcursor(r, c_col);
      
      if (history_idx == history_count) {
        buf[0] = '\0';
        line_len = 0;
        cursor_pos = 0;
      } else {
        strncpy(buf, shell_history[history_idx], len - 1);
        buf[len - 1] = '\0';
        line_len = strlen(buf);
        cursor_pos = line_len;
        terminal_writestring(buf);
      }
    }
  }
}

static void print_prompt(void) {
  terminal_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
  terminal_writestring("minios:");
  terminal_setcolor(COLOR_LIGHT_BLUE, COLOR_BLACK);
  terminal_writestring(current_dir);
  terminal_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
  terminal_writestring("> ");
  terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
}

/* --------------------------------------------------------------------------
 * Built-in command handlers
 * -------------------------------------------------------------------------- */

static void cmd_cmds(void) {
  terminal_setcolor(COLOR_YELLOW, COLOR_BLACK);
  terminal_writestring("\nAvailable commands:\n");
  terminal_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
  terminal_writestring("  cmds          -- show this list\n");
  terminal_writestring("  osinfo        -- show OS info and author\n");
  terminal_writestring("  cls           -- clear the screen\n");
  terminal_writestring("  wrt [text]    -- print text back to screen\n");
  terminal_writestring("  memo          -- show heap usage stats\n");
  terminal_writestring("  new [file]    -- create a new file\n");
  terminal_writestring("  edit [file]   -- open file in MiniEdit (ESC=save&quit)\n");
  terminal_writestring("  write  [file] -- write text to a file\n");
  terminal_writestring("  see [file]    -- display file contents\n");
  terminal_writestring("  laf           -- list all files\n");
  terminal_writestring("  date          -- show a hardcoded timestamp\n");
  terminal_writestring("  alive         -- show how long OS has been running\n");
  terminal_writestring("  calc [expr]   -- basic calculator (e.g. calc 5 + 3)\n");
  terminal_writestring("  color [color] -- change terminal text colour\n");
  terminal_writestring("  history       -- show last 10 typed commands\n");
  terminal_writestring("  todo [opts]   -- todo manager (add/list/done/clear)\n");
  terminal_writestring("  fm            -- interactive file manager\n");
  terminal_writestring("  passgen [len] -- generate a random password\n");
  terminal_writestring("  notes [opts]  -- notes app (new/list/view)\n");
  terminal_writestring("  sysinfo       -- system info dashboard\n");
  terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
}

static void cmd_osinfo(void) {
  terminal_writestring("\n");
  terminal_setcolor(COLOR_CYAN, COLOR_BLACK);
  terminal_writestring("  MiniOS v1.0\n");
  terminal_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
  terminal_writestring("  Built by Ahmed Ali\n");
  terminal_writestring("  Stack: C99 + x86 Assembly, GRUB 2, QEMU\n");
  terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
}

static void cmd_wrt(const char *arg) {
  terminal_putchar('\n');
  terminal_writestring(arg);
  terminal_putchar('\n');
}

static void cmd_memo(void) {
  terminal_writestring("\n");
  memory_print_stats();
}

static void cmd_new(const char *arg) {
  if (!arg || *arg == '\0') {
    terminal_writestring("Usage: new [filename]\n");
    return;
  }
  char path[64];
  resolve_path(arg, path);
  if (fs_create(path) == 0) {
    terminal_printf("\nCreated: %s\n", path);
  }
}

static void cmd_edit(const char *filename)
{
    if (!filename || *filename == '\0') {
        terminal_writestring("Usage: edit [filename]\n");
        return;
    }
    char path[64];
    resolve_path(filename, path);
    editor_open(path);
}

static void cmd_write(const char *arg) {
  if (!arg || *arg == '\0') {
    terminal_writestring("Usage: write [filename]\n");
    return;
  }
  char path[64];
  resolve_path(arg, path);
  terminal_writestring("\nEnter text: ");
  keyboard_readline(text_buf, sizeof(text_buf));
  int bytes = fs_write(path, text_buf, strlen(text_buf));
  if (bytes >= 0) {
    terminal_printf("Written to %s (%d bytes)\n", path, bytes);
  }
}

static void cmd_see(const char *arg) {
  if (!arg || *arg == '\0') {
    terminal_writestring("Usage: see [filename]\n");
    return;
  }
  char path[64];
  resolve_path(arg, path);
  char read_buf[FS_MAX_FILESIZE + 1];
  int n = fs_read(path, read_buf, sizeof(read_buf));
  if (n < 0) {
    terminal_printf("Error: file '%s' not found.\n", path);
  } else {
    terminal_putchar('\n');
    terminal_writestring(read_buf);
    terminal_putchar('\n');
  }
}

static void cmd_laf(void) {
  terminal_writestring("\n");
  fs_list_dir(current_dir);
}

static void cmd_date(void) {
  uint32_t secs = timer_get_uptime_seconds();
  terminal_printf("\nSystem Date: 2026-06-11 12:00:%02d UTC (Simulated)\n",
                  (secs % 60));
}

static void cmd_alive(void) {
  uint32_t secs = timer_get_uptime_seconds();
  uint32_t h = secs / 3600;
  uint32_t m = (secs % 3600) / 60;
  uint32_t s = secs % 60;
  terminal_printf("\nUptime: %02d:%02d:%02d\n", h, m, s);
}

static void cmd_calc(const char *arg) {
  char op = 0;
  int n1 = 0, n2 = 0;
  int i = 0;

  while (arg[i] && arg[i] != '+' && arg[i] != '-' && arg[i] != '*' &&
         arg[i] != '/')
    i++;
  if (!arg[i]) {
    terminal_writestring("\nError: Invalid expression. Use: calc 5 + 3\n");
    return;
  }
  op = arg[i];

  char buf1[32], buf2[32];
  strncpy(buf1, arg, i);
  buf1[i] = '\0';
  strcpy(buf2, arg + i + 1);

  n1 = atoi(skip_spaces(buf1));
  n2 = atoi(skip_spaces(buf2));

  int result = 0;
  if (op == '+')
    result = n1 + n2;
  else if (op == '-')
    result = n1 - n2;
  else if (op == '*')
    result = n1 * n2;
  else if (op == '/') {
    if (n2 == 0) {
      terminal_writestring("\nError: Division by zero.\n");
      return;
    }
    result = n1 / n2;
  }
  terminal_printf("\nResult: %d\n", result);
}

static void cmd_color(const char *arg) {
  if (strcmp(arg, "red") == 0)
    terminal_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
  else if (strcmp(arg, "green") == 0)
    terminal_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
  else if (strcmp(arg, "blue") == 0)
    terminal_setcolor(COLOR_LIGHT_BLUE, COLOR_BLACK);
  else if (strcmp(arg, "white") == 0)
    terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
  else if (strcmp(arg, "yellow") == 0)
    terminal_setcolor(COLOR_YELLOW, COLOR_BLACK);
  else if (strcmp(arg, "cyan") == 0)
    terminal_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
  else {
    terminal_writestring("\nUsage: color [red|green|blue|white|yellow|cyan]\n");
    return;
  }
  terminal_writestring("\nColor changed.\n");
}

static void cmd_show_history(void) {
  terminal_writestring("\nCommand History:\n");
  for (int i = 0; i < history_count; i++) {
    terminal_printf("  [%d] %s\n", i + 1, shell_history[i]);
  }
}

static void cmd_todo(const char *arg) {
  terminal_writestring("\n");
  if (strncmp(arg, "add ", 4) == 0) {
    if (todo_count >= TODO_MAX) {
      terminal_writestring("Todo list is full!\n");
      return;
    }
    const char *text = arg + 4;
    if (*text == '"')
      text++;
    int len = strlen(text);
    if (len > 0 && text[len - 1] == '"')
      len--;

    todos[todo_count].id = todo_next_id++;
    todos[todo_count].done = 0;
    strncpy(todos[todo_count].text, text, len);
    todos[todo_count].text[len] = '\0';
    todo_count++;
    terminal_writestring("Task added.\n");
  } else if (strcmp(arg, "list") == 0) {
    if (todo_count == 0)
      terminal_writestring("No tasks in your to-do list.\n");
    for (int i = 0; i < todo_count; i++) {
      terminal_printf("  [%d] [%c] %s\n", todos[i].id,
                      todos[i].done ? 'x' : ' ', todos[i].text);
    }
  } else if (strncmp(arg, "done ", 5) == 0) {
    int id = atoi(arg + 5);
    for (int i = 0; i < todo_count; i++) {
      if (todos[i].id == id) {
        todos[i].done = 1;
        terminal_writestring("Task marked as done.\n");
        return;
      }
    }
    terminal_writestring("Task ID not found.\n");
  } else if (strcmp(arg, "clear") == 0) {
    todo_count = 0;
    todo_next_id = 1;
    terminal_writestring("Todo list cleared.\n");
  } else {
    terminal_writestring(
        "Usage: todo [add \"text\" | list | done <id> | clear]\n");
  }
}

static void cmd_passgen(const char *arg) {
  int len = 0;
  int numbers_only = 0;
  int alpha_only = 0;

  const char *p = arg;
  while (*p) {
    if (strncmp(p, "number", 6) == 0) {
      numbers_only = 1;
      break;
    }
    if (strncmp(p, "alpha", 5) == 0) {
      alpha_only = 1;
      break;
    }
    p++;
  }

  len = atoi(arg);

  if (len <= 0 || len > 64)
    len = 16; 

  srand(timer_get_ticks());

  terminal_writestring("\n  Generated: ");
  for (int i = 0; i < len; i++) {
    char c;
    if (numbers_only) {
      c = '0' + (rand() % 10);
    } else if (alpha_only) {
      int r = rand() % 52;
      c = r < 26 ? 'A' + r : 'a' + (r - 26);
    } else {
      c = 33 + (rand() % 94); 
    }
    terminal_putchar(c);
  }
  terminal_writestring("\n");
}

static void cmd_notes(const char *arg) {
  terminal_writestring("\n");
  if (strcmp(arg, "new") == 0) {
    terminal_writestring("Title: ");
    char title[32];
    keyboard_readline(title, sizeof(title));

    terminal_writestring("Body: ");
    char body[FS_MAX_FILESIZE];
    keyboard_readline(body, sizeof(body));

    char raw_filename[64];
    strcpy(raw_filename, "note_");
    strncpy(raw_filename + 5, title, 20);
    raw_filename[25] = '\0';
    
    char path[64];
    resolve_path(raw_filename, path);

    fs_create(path);
    fs_write(path, body, strlen(body));
    terminal_writestring("Saved.\n");
  } else if (strcmp(arg, "list") == 0) {
    terminal_writestring("Notes:\n");
    fs_list_notes(current_dir);
  } else if (strncmp(arg, "view ", 5) == 0) {
    char raw_filename[64];
    strcpy(raw_filename, "note_");

    const char *title = arg + 5;
    if (*title == '"')
      title++;
    int tlen = strlen(title);
    if (tlen > 0 && title[tlen - 1] == '"')
      tlen--;

    strncpy(raw_filename + 5, title, tlen);
    raw_filename[5 + tlen] = '\0';

    char path[64];
    resolve_path(raw_filename, path);

    char read_buf[FS_MAX_FILESIZE + 1];
    if (fs_read(path, read_buf, sizeof(read_buf)) >= 0) {
      terminal_printf("--- %s ---\n%s\n", title, read_buf);
    } else {
      terminal_writestring("Note not found.\n");
    }
  } else {
    terminal_writestring("Usage: notes [new | list | view <title>]\n");
  }
}

static void cmd_fm(void) {
  while (1) {
    terminal_clear();
    terminal_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    terminal_writestring("  === MiniOS File Manager ===\n");
    terminal_setcolor(COLOR_WHITE, COLOR_BLACK);

    terminal_writestring("  Files:\n");
    fs_list_dir(current_dir);

    terminal_setcolor(COLOR_YELLOW, COLOR_BLACK);
    terminal_writestring("\n  [C]reate  [R]ead  [D]elete  [Q]uit\n  > ");

    char c = keyboard_getchar();
    if (c == 'q' || c == 'Q')
      break;
    if (c == 'c' || c == 'C') {
      terminal_writestring("C\n  Enter filename: ");
      char name[64];
      keyboard_readline(name, sizeof(name));
      if (fs_create(name) == 0) {
        terminal_writestring("  Enter text: ");
        char text[FS_MAX_FILESIZE];
        keyboard_readline(text, sizeof(text));
        fs_write(name, text, strlen(text));
      }
    } else if (c == 'r' || c == 'R') {
      terminal_writestring("R\n  Enter filename: ");
      char name[64];
      keyboard_readline(name, sizeof(name));
      char read_buf[FS_MAX_FILESIZE + 1];
      if (fs_read(name, read_buf, sizeof(read_buf)) >= 0) {
        terminal_printf("\n  --- %s ---\n  %s\n  ------------\n  Press any key "
                        "to return...",
                        name, read_buf);
        keyboard_getchar();
      }
    } else if (c == 'd' || c == 'D') {
      terminal_writestring("D\n  Enter filename: ");
      char name[64];
      keyboard_readline(name, sizeof(name));
      fs_delete(name);
    }
  }
  terminal_clear();
}

static void cmd_del(const char *arg) {
  if (!arg || *arg == '\0') {
    terminal_writestring("Usage: del [filename]\n");
    return;
  }
  char path[64];
  resolve_path(arg, path);
  if (fs_delete(path) == 0) {
    terminal_printf("Deleted: %s\n", path);
  } else {
    terminal_printf("Error: file '%s' not found.\n", path);
  }
}

static void cmd_cpy(const char *arg) {
  char src[32], dst[32];
  if (!arg) {
    terminal_writestring("Usage: cpy [src] [dst]\n");
    return;
  }
  arg = skip_spaces(arg);
  const char *space = strchr(arg, ' ');
  if (!space) {
    terminal_writestring("Usage: cpy [src] [dst]\n");
    return;
  }
  size_t len1 = space - arg;
  if (len1 >= sizeof(src)) len1 = sizeof(src) - 1;
  strncpy(src, arg, len1);
  src[len1] = '\0';
  
  const char *arg2 = skip_spaces(space);
  strncpy(dst, arg2, sizeof(dst) - 1);
  dst[sizeof(dst) - 1] = '\0';
  
  if (src[0] == '\0' || dst[0] == '\0') {
    terminal_writestring("Usage: cpy [src] [dst]\n");
    return;
  }
  char buf[FS_MAX_FILESIZE + 1];
  
  char path_src[64], path_dst[64];
  resolve_path(src, path_src);
  resolve_path(dst, path_dst);

  int n = fs_read(path_src, buf, sizeof(buf));
  if (n < 0) {
    terminal_printf("Error: source file '%s' not found.\n", path_src);
    return;
  }
  if (fs_create(path_dst) < 0) return;
  fs_write(path_dst, buf, n);
  terminal_printf("Copied: %s -> %s\n", path_src, path_dst);
}

static void cmd_mov(const char *arg) {
  char src[32], dst[32];
  if (!arg) {
    terminal_writestring("Usage: mov [src] [dst]\n");
    return;
  }
  arg = skip_spaces(arg);
  const char *space = strchr(arg, ' ');
  if (!space) {
    terminal_writestring("Usage: mov [src] [dst]\n");
    return;
  }
  size_t len1 = space - arg;
  if (len1 >= sizeof(src)) len1 = sizeof(src) - 1;
  strncpy(src, arg, len1);
  src[len1] = '\0';
  
  const char *arg2 = skip_spaces(space);
  strncpy(dst, arg2, sizeof(dst) - 1);
  dst[sizeof(dst) - 1] = '\0';
  
  if (src[0] == '\0' || dst[0] == '\0') {
    terminal_writestring("Usage: mov [src] [dst]\n");
    return;
  }
  char buf[FS_MAX_FILESIZE + 1];
  char path_src[64], path_dst[64];
  resolve_path(src, path_src);
  resolve_path(dst, path_dst);

  int n = fs_read(path_src, buf, sizeof(buf));
  if (n < 0) {
    terminal_printf("Error: source file '%s' not found.\n", path_src);
    return;
  }
  if (fs_create(path_dst) < 0) return;
  fs_write(path_dst, buf, n);
  fs_delete(path_src);
  terminal_printf("Moved/Renamed: %s -> %s\n", path_src, path_dst);
}

static void cmd_look(const char *arg) {
  terminal_writestring("Usage: look [name]\n");
  terminal_writestring("MiniFS does not support directories, searching root...\n");
  char buf[FS_MAX_FILESIZE + 1];
  if (fs_read(arg, buf, sizeof(buf)) >= 0) {
    terminal_printf("Found: /root/%s\n", arg);
  } else {
    terminal_printf("Not found: %s\n", arg);
  }
}

static void cmd_wdir(void) {
  terminal_printf("%s\n", current_dir);
}

static void cmd_goto(const char *arg) {
  char new_dir[64];
  if (!arg || *arg == '\0') {
    strcpy(new_dir, "/root");
  } else if (strcmp(arg, "..") == 0) {
    strcpy(new_dir, current_dir);
    if (strcmp(new_dir, "/") != 0 && strcmp(new_dir, "/root") != 0) {
      char *last_slash = strrchr(new_dir, '/');
      if (last_slash && last_slash != new_dir) {
        *last_slash = '\0';
      } else {
        strcpy(new_dir, "/");
      }
    }
  } else if (arg[0] == '/') {
    strncpy(new_dir, arg, sizeof(new_dir) - 1);
    new_dir[sizeof(new_dir) - 1] = '\0';
  } else {
    strcpy(new_dir, current_dir);
    int len = strlen(new_dir);
    if (new_dir[len - 1] != '/') {
      strncat(new_dir, "/", sizeof(new_dir) - len - 1);
    }
    strncat(new_dir, arg, sizeof(new_dir) - strlen(new_dir) - 1);
  }

  if (!fs_is_dir(new_dir)) {
    terminal_printf("Error: Directory '%s' does not exist.\n", new_dir);
    return;
  }
  strcpy(current_dir, new_dir);
  terminal_printf("Moved to: %s\n", current_dir);
}

static void cmd_mkd(const char *arg) {
  if (!arg || *arg == '\0') {
    terminal_writestring("Usage: mkd [dir]\n");
    return;
  }
  char path[64];
  resolve_path(arg, path);
  if (fs_create_dir(path) == 0) {
    terminal_printf("Directory created: %s\n", path);
  } else {
    terminal_printf("Error: could not create directory '%s'\n", path);
  }
}

static void cmd_deld(const char *arg) {
  if (!arg || *arg == '\0') {
    terminal_writestring("Usage: deld [dir]\n");
    return;
  }
  char path[64];
  resolve_path(arg, path);
  if (fs_delete(path) == 0) {
    terminal_printf("Directory deleted: %s\n", path);
  } else {
    terminal_printf("Error: directory '%s' not found.\n", path);
  }
}

static void cmd_dtree(void) {
  terminal_writestring("/root\n  (Filesystem is flat, directories not fully supported)\n");
}

static void cmd_whoiam(void) {
  terminal_writestring("root\n");
}

static void cmd_rst(void) {
  terminal_writestring("Restarting OS...\n");
  struct {
    uint16_t limit;
    uint32_t base;
  } __attribute__((packed)) idtr = {0, 0};
  __asm__ volatile("lidt %0; int3" :: "m"(idtr));
}

static void cmd_bye(void) {
  terminal_writestring("Shutting down MiniOS... Goodbye.\n");
  __asm__ volatile("cli; hlt;");
}

static void cmd_disk(void) {
  terminal_writestring("Filesystem: MiniFS v1\n");
  terminal_printf("Total slots : %d files\n", FS_MAX_FILES);
  terminal_printf("Used slots  : %d files\n", fs_get_total_files());
  terminal_printf("Free slots  : %d files\n", FS_MAX_FILES - fs_get_total_files());
}

static void cmd_hunt(const char *arg) {
  char file[32], word[32];
  if (!arg) {
    terminal_writestring("Usage: hunt [file] [word]\n");
    return;
  }
  arg = skip_spaces(arg);
  const char *space = strchr(arg, ' ');
  if (!space) {
    terminal_writestring("Usage: hunt [file] [word]\n");
    return;
  }
  size_t len1 = space - arg;
  if (len1 >= sizeof(file)) len1 = sizeof(file) - 1;
  strncpy(file, arg, len1);
  file[len1] = '\0';
  
  const char *arg2 = skip_spaces(space);
  strncpy(word, arg2, sizeof(word) - 1);
  word[sizeof(word) - 1] = '\0';
  
  if (file[0] == '\0' || word[0] == '\0') {
    terminal_writestring("Usage: hunt [file] [word]\n");
    return;
  }
  char buf[FS_MAX_FILESIZE + 1];
  int n = fs_read(file, buf, sizeof(buf));
  if (n < 0) {
    terminal_printf("Error: file '%s' not found.\n", file);
    return;
  }
  int line = 1;
  char *p = buf;
  char *start = buf;
  while (*p) {
    if (*p == '\n') {
      *p = '\0';
      if (mini_strstr(start, word)) {
        terminal_printf("Line %d: \"%s\"\n", line, start);
      }
      *p = '\n';
      start = p + 1;
      line++;
    }
    p++;
  }
  if (mini_strstr(start, word)) {
    terminal_printf("Line %d: \"%s\"\n", line, start);
  }
}

static void cmd_cnt(const char *arg) {
  if (!arg || *arg == '\0') {
    terminal_writestring("Usage: cnt [file]\n");
    return;
  }
  char buf[FS_MAX_FILESIZE + 1];
  int bytes = fs_read(arg, buf, sizeof(buf));
  if (bytes < 0) {
    terminal_printf("Error: file '%s' not found.\n", arg);
    return;
  }
  int lines = 0, words = 0;
  int in_word = 0;
  for (int i = 0; i < bytes; i++) {
    if (buf[i] == '\n') lines++;
    if (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\t') {
      in_word = 0;
    } else if (!in_word) {
      in_word = 1;
      words++;
    }
  }
  if (bytes > 0 && buf[bytes-1] != '\n') lines++;
  terminal_printf("Lines : %d\nWords : %d\nBytes : %d\n", lines, words, bytes);
}

static void cmd_top(const char *arg) {
  char file[32];
  int count = 10;
  if (!arg) {
    terminal_writestring("Usage: top [file] [n]\n");
    return;
  }
  arg = skip_spaces(arg);
  const char *space = strchr(arg, ' ');
  if (space) {
    size_t len1 = space - arg;
    if (len1 >= sizeof(file)) len1 = sizeof(file) - 1;
    strncpy(file, arg, len1);
    file[len1] = '\0';
    count = atoi(skip_spaces(space));
  } else {
    strncpy(file, arg, sizeof(file) - 1);
    file[sizeof(file) - 1] = '\0';
  }
  
  if (file[0] == '\0') {
    terminal_writestring("Usage: top [file] [n]\n");
    return;
  }
  char buf[FS_MAX_FILESIZE + 1];
  if (fs_read(file, buf, sizeof(buf)) < 0) {
    terminal_printf("Error: file '%s' not found.\n", file);
    return;
  }
  int line = 1;
  char *p = buf;
  char *start = buf;
  while (*p && line <= count) {
    if (*p == '\n') {
      *p = '\0';
      terminal_printf("Line %d: %s\n", line, start);
      *p = '\n';
      start = p + 1;
      line++;
    }
    p++;
  }
  if (*start && line <= count) {
    terminal_printf("Line %d: %s\n", line, start);
  }
}

static void cmd_btm(const char *arg) {
  (void)arg;
  terminal_writestring("btm not fully implemented yet.\n");
}

static void cmd_flip(void) {
  srand(timer_get_ticks());
  if (rand() % 2 == 0) terminal_writestring("Heads!\n");
  else terminal_writestring("Tails!\n");
}

static void cmd_dice(void) {
  srand(timer_get_ticks());
  terminal_printf("You rolled: %d\n", (rand() % 6) + 1);
}

static void cmd_splash(void) {
  terminal_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
  terminal_writestring("\n  === Welcome to MiniOS ===\n\n");
  terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
}

static void cmd_ver(void) {
  terminal_writestring("MiniOS v1.0\n");
}

static void cmd_ref(const char *arg) {
  if (!arg || *arg == '\0') {
    terminal_writestring("Usage: ref [cmd]\n");
    return;
  }
  terminal_printf("%s -- see 'cmds' for a full list of commands.\n", arg);
}

static void cmd_sysinfo(void) {
  uint32_t secs = timer_get_uptime_seconds();
  uint32_t h = secs / 3600;
  uint32_t m = (secs % 3600) / 60;
  uint32_t s = secs % 60;

  extern void memory_print_stats(void);

  terminal_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
  terminal_writestring("\n  ================================\n");
  terminal_writestring("  MiniOS v1.0 - by Ahmed Ali\n");
  terminal_writestring("  ================================\n");
  terminal_setcolor(COLOR_WHITE, COLOR_BLACK);

  terminal_printf("  Files stored: %d\n", fs_get_total_files());
  terminal_printf("  Uptime      : %02d:%02d:%02d\n", h, m, s);
  terminal_printf("  Commands run: %d\n", total_commands_run);

  terminal_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
  terminal_writestring("  ================================\n");
  terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
}

/* --------------------------------------------------------------------------
 * Shell main loop
 * -------------------------------------------------------------------------- */
void shell_run(void) {
  /* Enable interrupts so the keyboard IRQ fires */
  __asm__ volatile("sti");

  while (1) {
    print_prompt();
    shell_readline(input_buf, INPUT_MAX);

    const char *line = skip_spaces(input_buf);
    if (*line == '\0')
      continue;

    /* Add to history */
    if (history_count < HISTORY_MAX) {
      strncpy(shell_history[history_count], line, INPUT_MAX - 1);
      shell_history[history_count][INPUT_MAX - 1] = '\0';
      history_count++;
    } else {
      for (int i = 1; i < HISTORY_MAX; i++) {
        strcpy(shell_history[i - 1], shell_history[i]);
      }
      strncpy(shell_history[HISTORY_MAX - 1], line, INPUT_MAX - 1);
      shell_history[HISTORY_MAX - 1][INPUT_MAX - 1] = '\0';
    }
    total_commands_run++;

    char cmd[64];
    const char *arg = "";
    const char *space = strchr(line, ' ');
    if (space) {
      size_t cmd_len = (size_t)(space - line);
      if (cmd_len >= sizeof(cmd))
        cmd_len = sizeof(cmd) - 1;
      strncpy(cmd, line, cmd_len);
      cmd[cmd_len] = '\0';
      arg = skip_spaces(space + 1);
    } else {
      strncpy(cmd, line, sizeof(cmd) - 1);
      cmd[sizeof(cmd) - 1] = '\0';
    }

    /* Dispatch */
    if (strcmp(cmd, "cmds") == 0) cmd_cmds();
    else if (strcmp(cmd, "osinfo") == 0) cmd_osinfo();
    else if (strcmp(cmd, "cls") == 0) terminal_clear();
    else if (strcmp(cmd, "wrt") == 0) cmd_wrt(arg);
    else if (strcmp(cmd, "memo") == 0) cmd_memo();
    else if (strcmp(cmd, "new") == 0) cmd_new(arg);
    else if (strcmp(cmd, "edit") == 0) cmd_edit(arg);
    else if (strcmp(cmd, "write") == 0) cmd_write(arg);
    else if (strcmp(cmd, "see") == 0) cmd_see(arg);
    else if (strcmp(cmd, "laf") == 0) cmd_laf();
    else if (strcmp(cmd, "date") == 0) cmd_date();
    else if (strcmp(cmd, "alive") == 0) cmd_alive();
    else if (strcmp(cmd, "calc") == 0) cmd_calc(arg);
    else if (strcmp(cmd, "color") == 0) cmd_color(arg);
    else if (strcmp(cmd, "history") == 0) cmd_show_history();
    else if (strcmp(cmd, "todo") == 0) cmd_todo(arg);
    else if (strcmp(cmd, "passgen") == 0) cmd_passgen(arg);
    else if (strcmp(cmd, "notes") == 0) cmd_notes(arg);
    else if (strcmp(cmd, "fm") == 0) cmd_fm();
    else if (strcmp(cmd, "sysinfo") == 0) cmd_sysinfo();
    else if (strcmp(cmd, "del") == 0) cmd_del(arg);
    else if (strcmp(cmd, "cpy") == 0) cmd_cpy(arg);
    else if (strcmp(cmd, "mov") == 0) cmd_mov(arg);
    else if (strcmp(cmd, "look") == 0) cmd_look(arg);
    else if (strcmp(cmd, "wdir") == 0) cmd_wdir();
    else if (strcmp(cmd, "goto") == 0) cmd_goto(arg);
    else if (strcmp(cmd, "mkd") == 0) cmd_mkd(arg);
    else if (strcmp(cmd, "deld") == 0) cmd_deld(arg);
    else if (strcmp(cmd, "dtree") == 0) cmd_dtree();
    else if (strcmp(cmd, "whoiam") == 0) cmd_whoiam();
    else if (strcmp(cmd, "rst") == 0) cmd_rst();
    else if (strcmp(cmd, "bye") == 0) cmd_bye();
    else if (strcmp(cmd, "disk") == 0) cmd_disk();
    else if (strcmp(cmd, "hunt") == 0) cmd_hunt(arg);
    else if (strcmp(cmd, "cnt") == 0) cmd_cnt(arg);
    else if (strcmp(cmd, "top") == 0) cmd_top(arg);
    else if (strcmp(cmd, "btm") == 0) cmd_btm(arg);
    else if (strcmp(cmd, "flip") == 0) cmd_flip();
    else if (strcmp(cmd, "dice") == 0) cmd_dice();
    else if (strcmp(cmd, "splash") == 0) cmd_splash();
    else if (strcmp(cmd, "ver") == 0) cmd_ver();
    else if (strcmp(cmd, "ref") == 0) cmd_ref(arg);
    else {
      terminal_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
      terminal_printf("\nUnknown command: '%s'\n", cmd);
      terminal_writestring("Type 'cmds' for a list of commands.\n");
      terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
    }
  }
}
