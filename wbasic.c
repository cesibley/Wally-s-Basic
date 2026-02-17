/* Optional embedded icon support.
 * If you link an object produced from `ld -r -b binary icon.png`, these symbols will exist.
 * Otherwise they will be NULL (weak) and we simply skip setting the window icon.
 */
#if !defined(WBASIC_NO_UI)
extern const unsigned char _binary_icon_png_start[] __attribute__((weak));
extern const unsigned char _binary_icon_png_end[]   __attribute__((weak));
#endif

/*
 * WBASIC — GW-BASIC–style interpreter with GTK UI
 *
 * ============================================================
 * VERSION / BASELINE IDENTIFIERS (authoritative; update here)
 * ============================================================
 */

#include <stddef.h>   /* for NULL */
#include <stdbool.h>  /* for bool */

/*
 * Embedded export default:
 *   If you compile an exported stub that defines WBASIC_EMBEDDED_BUILD,
 *   we default to headless (no GTK init / no UI) unless you explicitly
 *   compile with -DWBASIC_FORCE_UI.
 */
#if defined(WBASIC_EMBEDDED_BUILD) && !defined(WBASIC_FORCE_UI)
#ifndef WBASIC_NO_UI
#define WBASIC_NO_UI 1
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#define WB_UNUSED __attribute__((unused))
#else
#define WB_UNUSED
#endif
#define WBASIC_VERSION_MAJOR 1
#define WBASIC_VERSION_MINOR 20
#define WBASIC_VERSION_PATCH_STR ""
#define WBASIC_VERSION_STR "1.20"
#define WBASIC_BASELINE_DATE "2026-02-13"
#define WBASIC_BASELINE_REV "2026-02-13 v1.20"
#define WBASIC_SOURCE_FILE __FILE__

// Wally's Basic.c - MS-BASIC-ish interpreter with GTK3 desktop UI + menus (single-file)
// Build:
//   sudo apt install build-essential pkg-config libgtk-3-dev
//   gcc -O2 -Wall -Wextra -o Wally's Basic Wally's Basic.c $(pkg-config --cflags --libs gtk+-3.0)
//
// Extensions added (per request):
//  - Multi-character variable names (case-insensitive): SCALE, SUM, A$, etc.
//  - Floating point numbers (double) for numeric variables and expressions
//  - Multiple statements per line using ':' (outside of quotes)
//  - GOSUB / RETURN
//  - DIM for up-to-5D numeric arrays, indexed as A(I) or A(I,J,...) (max 5 dims)
//    (arrays are numeric-only in this version; strings remain scalar vars)
//
// Supported statements:
//  REM, END, PRINT, INPUT, LET/assignment, IF ... THEN <line|statement>, GOTO,
//  FOR/NEXT, GOSUB/RETURN, DIM
//
// Immediate commands:
//  RUN, LIST, NEW, LOAD "file", SAVE "file", CLEAR
//  Also supports CLS as an alias for CLEAR.
//
// Notes:
//  - Numeric variables default to 0.0, string variables default to "".
//  - Arrays are 0..N (N inclusive), like many BASICs; change easily if you prefer 1..N.
//  - PRINT supports string literals, string vars, numeric exprs; separators ',' and ';'.
//  - Output wraps and auto-scrolls.
/*
 * WINDOW SIZE PERSISTENCE NOTE
 * ----------------------------
 * Window size is tracked continuously during runtime via GTK events
 * (configure-event / size-allocate) and persisted on exit using the
 * last known good values stored in App->win_w / App->win_h.
 *
 * IMPORTANT:
 * Do NOT re-measure window size during delete-event or destroy.
 * Many window managers report stale or default sizes during teardown,
 * which can overwrite correct values and break persistence.
 *
 * This design was validated Jan 2026 after extensive debugging.
 */

#include <string.h>
#include <strings.h>

/*
 * For headless/export builds we still rely on GLib for common types/utilities
 * (gboolean/guint/GHashTable/GPtrArray/g_free/etc.).
 */
#include <glib.h>
#include <math.h>
/* =============================================================
 * PHASE 2: Core delay helper + optional UI tickle plumbing
 * -------------------------------------------------------------
 * This introduces:
 *  - throttle delay computation (GTK-free)
 *  - nanosleep-based delay executor
 *  - optional tickle callback (GTK sets it, headless leaves NULL)
 *
 * IMPORTANT:
 *  - No behavior change yet.
 *  - Nothing calls these helpers in Phase 2.
 * ============================================================= */

#include <time.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <conio.h>
#endif

static const char *wbasic_ascii_strcasestr(const char *haystack, const char *needle)
{
    if (!haystack || !needle || *needle == '\0') return haystack;

    const char *h = haystack;
    const size_t needle_len = strlen(needle);

    for (; *h; h++) {
        if (g_ascii_strncasecmp(h, needle, needle_len) == 0) {
            return h;
        }
    }

    return NULL;
}

typedef void (*wbasic_tickle_fn)(void *user);

typedef struct {
    wbasic_tickle_fn fn;   /* NULL in headless */
    void *user;
} WbasicTickle;

/* Core delay executor: GTK-free */
static void wbasic_delay_ms(int ms, const WbasicTickle *tickle)
{
    if (ms <= 0) return;

    const int slice_ms = 20;
    int remaining = ms;

    while (remaining > 0) {
        int step = (remaining > slice_ms) ? slice_ms : remaining;

#ifdef _WIN32
        Sleep((DWORD)step);
#else
        struct timespec ts;
        ts.tv_sec  = step / 1000;
        ts.tv_nsec = (long)(step % 1000) * 1000000L;

        while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
            /* retry */
        }
#endif

        if (tickle && tickle->fn) {
            tickle->fn(tickle->user);
        }

        remaining -= step;
    }
}

/* Placeholder throttle curve (wired in Phase 3) */
/* deleted unused static function: wbasic_compute_throttle_ms */

/* Phase 3: PRINT-statement delay (ms) from output_speed (0..1), using curve (exp 4). */
/* deleted unused static function: wbasic_compute_print_delay_ms_from_output_speed */


static double wbasic_compute_print_delay_ms_f_from_output_speed(double output_speed)
{
    double s = output_speed;
    if (s < 0.0) s = 0.0;
    if (s > 1.0) s = 1.0;
    /* s=1.0 => 0ms, s=0.0 => 500ms */
    double t = 1.0 - s;
    double curve = pow(t, 4.0);
    double ms = curve * 500.0;
    if (ms < 0.0) ms = 0.0;
    if (ms > 500.0) ms = 500.0;
    return ms;
}


/* =============================================================
 * PHASE 1: Disable legacy interpreter/output throttling
 * -------------------------------------------------------------
 * We intentionally neutralize legacy g_usleep()-based pacing so a
 * single PRINT-statement throttle can be added safely later.
 *
 * IMPORTANT: This override must come AFTER GLib headers are
 * included, so it does not break the g_usleep() prototype.
 *
 * Notes:
 *   - GTK render throttling via timers is unaffected.
 *   - Speed slider / preferences remain intact but disconnected
 *     from legacy pacing paths.
 *   - This intentionally makes execution effectively unthrottled.
 * ============================================================= */
#define WBASIC_PHASE1_DISABLE_LEGACY_THROTTLE 1
#if WBASIC_PHASE1_DISABLE_LEGACY_THROTTLE
#undef g_usleep
#define g_usleep(usec) do { (void)(usec); } while (0)
#endif
#ifndef WBASIC_NO_UI
#include <gtk/gtk.h>

#ifdef _WIN32
#include <gdk/gdkwin32.h>

static gboolean windows_prefers_dark_mode(void) {
    DWORD value = 1;
    DWORD size = sizeof(value);
    DWORD type = 0;
    LONG rc = RegGetValueA(
        HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        "AppsUseLightTheme",
        RRF_RT_REG_DWORD,
        &type,
        &value,
        &size
    );
    if (rc == ERROR_SUCCESS && type == REG_DWORD) {
        return value == 0;
    }
    return FALSE;
}

static void apply_windows_dark_mode_preference(void) {
    GtkSettings *settings = gtk_settings_get_default();
    if (!settings) return;
    g_object_set(settings,
                 "gtk-application-prefer-dark-theme",
                 windows_prefers_dark_mode(),
                 NULL);
}

static void apply_windows_dark_titlebar(GtkWidget *widget) {
    if (!windows_prefers_dark_mode()) return;
    if (!gtk_widget_get_realized(widget)) return;

    GdkWindow *gdk_win = gtk_widget_get_window(widget);
    if (!gdk_win) return;

    HWND hwnd = gdk_win32_window_get_handle(gdk_win);
    if (!hwnd) return;

    HMODULE dwm = LoadLibraryA("dwmapi.dll");
    if (!dwm) return;

	    typedef HRESULT (WINAPI *DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);
	    FARPROC p = GetProcAddress(dwm, "DwmSetWindowAttribute");
	    if (!p) {
	        FreeLibrary(dwm);
	        return;
	    }
	    /*
	     * Avoid -Wcast-function-type on MinGW/GCC: FARPROC is an untyped function
	     * pointer, so casting directly to a specific signature triggers warnings.
	     */
	    union {
	        FARPROC p;
	        DwmSetWindowAttributeFn f;
	    } u;
	    u.p = p;
	    DwmSetWindowAttributeFn set_attr = u.f;
	    if (!set_attr) {
	        FreeLibrary(dwm);
	        return;
	    }

    BOOL enabled = TRUE;
    const DWORD dwmwa_use_immersive_dark_mode = 20;
    HRESULT hr = set_attr(hwnd, dwmwa_use_immersive_dark_mode, &enabled, sizeof(enabled));
    if (FAILED(hr)) {
        const DWORD dwmwa_use_immersive_dark_mode_old = 19;
        set_attr(hwnd, dwmwa_use_immersive_dark_mode_old, &enabled, sizeof(enabled));
    }

    FreeLibrary(dwm);
}

static void on_win_realize_apply_dark_titlebar(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    apply_windows_dark_titlebar(widget);
}

static void attach_windows_dark_titlebar(GtkWidget *widget) {
    if (!widget) return;
    g_signal_connect(widget, "realize", G_CALLBACK(on_win_realize_apply_dark_titlebar), NULL);
}
#else
static void apply_windows_dark_mode_preference(void) { }
static void attach_windows_dark_titlebar(GtkWidget *widget) { (void)widget; }
#endif


/* Embedded WBASIC icon (PNG) linked into the executable (from icon.png). */
extern const unsigned char _binary_icon_png_start[];
extern const unsigned char _binary_icon_png_end[];
extern const unsigned int  _binary_icon_png_size;


/* Embedded WBASIC icon (PNG) for About / window icon (linked from icon.png) */
#ifndef WBASIC_NO_UI
/* Load embedded PNG icon into a GdkPixbuf without deprecated APIs. */
static GdkPixbuf *ui_load_wbasic_icon_pixbuf(void) {
    if ((const void*)_binary_icon_png_start == NULL || (const void*)_binary_icon_png_end == NULL) return NULL;
    const unsigned char *start = _binary_icon_png_start;
    const unsigned char *end   = _binary_icon_png_end;
    if (!start || !end || end <= start) return NULL;

    GBytes *bytes = g_bytes_new_static(_binary_icon_png_start, (gsize)(_binary_icon_png_end - _binary_icon_png_start));
    GInputStream *stream = g_memory_input_stream_new_from_bytes(bytes);
    GError *err = NULL;
    GdkPixbuf *pix = gdk_pixbuf_new_from_stream(stream, NULL, &err);
    if (err) {
        g_error_free(err);
        err = NULL;
    }
    g_object_unref(stream);
    g_bytes_unref(bytes);
    return pix;
}
#endif /* !WBASIC_NO_UI */

#endif


#ifdef WBASIC_NO_UI
/* Headless builds must not require GTK headers.
   We forward-declare UI types used only as pointers in structs,
   and provide a tiny GdkRGBA stand-in for persisted color prefs. */
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTextBuffer GtkTextBuffer;
typedef struct _GtkTextView GtkTextView;
typedef struct _GtkCssProvider GtkCssProvider;
typedef struct _GtkAccelGroup GtkAccelGroup;
typedef struct _GtkMenuItem GtkMenuItem;
typedef struct _GtkFileChooser GtkFileChooser;
typedef struct _GtkWindow GtkWindow;
typedef struct _GtkEntry GtkEntry;
typedef struct _GtkClipboard GtkClipboard;
typedef struct _GtkEditable GtkEditable;
typedef struct _GtkTextTagTable GtkTextTagTable;
typedef struct _GtkTextMark GtkTextMark;
typedef struct _GtkStyleContext GtkStyleContext;


/* -------------------------------------------------------------------------
 * Headless (WBASIC_NO_UI) GTK shim
 *
 * Goal: allow exported standalone builds to compile/link WITHOUT GTK headers
 * or GTK libraries. We provide minimal typedefs/defines/stubs to satisfy the
 * compiler and linker for UI-only helper code that is never executed in
 * headless exported binaries.
 * ------------------------------------------------------------------------- */

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Common GTK/GDK cast-style macros used throughout the UI layer */
#ifndef GTK_WINDOW
#define GTK_WINDOW(x) ((GtkWindow*)(x))
#endif
#ifndef GTK_DIALOG
#define GTK_DIALOG(x) ((void*)(x))
#endif
#ifndef GTK_ENTRY
#define GTK_ENTRY(x) ((GtkEntry*)(x))
#endif
#ifndef GTK_EDITABLE
#define GTK_EDITABLE(x) ((GtkEditable*)(x))
#endif
#ifndef GTK_TEXT_VIEW
#define GTK_TEXT_VIEW(x) ((GtkTextView*)(x))
#endif
#ifndef GTK_FILE_CHOOSER
#define GTK_FILE_CHOOSER(x) ((GtkFileChooser*)(x))
#endif
#ifndef GTK_MESSAGE_DIALOG
#define GTK_MESSAGE_DIALOG(x) ((GtkWidget*)(x))
#endif

#ifndef GTK_IS_EDITABLE
#define GTK_IS_EDITABLE(x) (0)
#endif
#ifndef GTK_IS_TEXT_VIEW
#define GTK_IS_TEXT_VIEW(x) (0)
#endif

/* Minimal constants used by file dialogs / message dialogs */
#ifndef GTK_FILE_CHOOSER_ACTION_OPEN
#define GTK_FILE_CHOOSER_ACTION_OPEN 0
#endif
#ifndef GTK_FILE_CHOOSER_ACTION_SAVE
#define GTK_FILE_CHOOSER_ACTION_SAVE 1
#endif
#ifndef GTK_RESPONSE_CANCEL
#define GTK_RESPONSE_CANCEL -1
#endif
#ifndef GTK_RESPONSE_ACCEPT
#define GTK_RESPONSE_ACCEPT 1
#endif
#ifndef GTK_RESPONSE_REJECT
#define GTK_RESPONSE_REJECT 2
#endif
#ifndef GTK_RESPONSE_DELETE_EVENT
#define GTK_RESPONSE_DELETE_EVENT -4
#endif
#ifndef GTK_DIALOG_MODAL
#define GTK_DIALOG_MODAL 0
#endif
#ifndef GTK_MESSAGE_QUESTION
#define GTK_MESSAGE_QUESTION 0
#endif
#ifndef GTK_BUTTONS_NONE
#define GTK_BUTTONS_NONE 0
#endif
#ifndef GDK_SELECTION_CLIPBOARD
#define GDK_SELECTION_CLIPBOARD 1
#endif

/* GtkTextIter is a stack-only cursor type in GTK; for headless builds a dummy is fine */
#ifndef __WBASIC_GTK_TEXT_ITER_DEFINED
#define __WBASIC_GTK_TEXT_ITER_DEFINED
typedef struct _GtkTextIter { int _dummy; } GtkTextIter;
#endif

/* Forward declare GDK types used in a couple of helper paths */
typedef struct _GdkDisplay GdkDisplay;
typedef struct _GdkEvent   GdkEvent;
typedef struct _GdkEventKey GdkEventKey;

/* No-op stubs for GTK entry points and helpers */
static inline void gtk_init(int *argc, char ***argv) { (void)argc; (void)argv; }
static inline void gtk_main(void) {}
static inline void gtk_main_quit(void) {}
static inline int  gtk_init_check(int *argc, char ***argv) { (void)argc; (void)argv; return TRUE; }
static inline void gtk_widget_show_all(GtkWidget *w) { (void)w; }
static inline void gtk_widget_destroy(GtkWidget *w) { (void)w; }
static inline void gtk_widget_set_sensitive(GtkWidget *w, int sensitive) { (void)w; (void)sensitive; }

static inline void gtk_entry_set_text(GtkEntry *e, const char *s) { (void)e; (void)s; }
static inline const char *gtk_entry_get_text(GtkEntry *e) { (void)e; return ""; }
static inline void gtk_entry_set_visibility(GtkEntry *e, int visible) { (void)e; (void)visible; }
static inline void gtk_entry_set_invisible_char(GtkEntry *e, unsigned int ch) { (void)e; (void)ch; }
static inline void gtk_editable_set_position(GtkEditable *e, int pos) { (void)e; (void)pos; }
static inline void gtk_editable_select_region(GtkEditable *e, int start, int end) { (void)e; (void)start; (void)end; }
static inline void gtk_editable_cut_clipboard(GtkEditable *e) { (void)e; }
static inline void gtk_editable_copy_clipboard(GtkEditable *e) { (void)e; }
static inline void gtk_editable_paste_clipboard(GtkEditable *e) { (void)e; }

static inline GtkWidget *gtk_window_get_focus(GtkWindow *w) { (void)w; return NULL; }

static inline GtkWidget *gtk_file_chooser_dialog_new(const char *title, GtkWindow *parent, int action, ...) {
    (void)title; (void)parent; (void)action; return NULL;
}
static inline int gtk_dialog_run(void *dlg) { (void)dlg; return GTK_RESPONSE_CANCEL; }
static inline char *gtk_file_chooser_get_filename(GtkFileChooser *c) { (void)c; return NULL; }
static inline void gtk_file_chooser_set_filename(GtkFileChooser *c, const char *f) { (void)c; (void)f; }
static inline void gtk_file_chooser_set_current_name(GtkFileChooser *c, const char *n) { (void)c; (void)n; }
static inline void gtk_file_chooser_set_do_overwrite_confirmation(GtkFileChooser *c, int b) { (void)c; (void)b; }

static inline GtkWidget *gtk_message_dialog_new(GtkWindow *parent, int flags, int type, int buttons, const char *msg, ...) {
    (void)parent; (void)flags; (void)type; (void)buttons; (void)msg; return NULL;
}
static inline void gtk_message_dialog_format_secondary_text(GtkWidget *dlg, const char *msg, ...) { (void)dlg; (void)msg; }
static inline void gtk_dialog_add_button(void *dlg, const char *label, int response) { (void)dlg; (void)label; (void)response; }

static inline GtkTextBuffer *gtk_text_view_get_buffer(GtkTextView *tv) { (void)tv; return NULL; }
static inline int gtk_text_view_get_editable(GtkTextView *tv) { (void)tv; return FALSE; }
static inline GtkClipboard *gtk_clipboard_get(int sel) { (void)sel; return NULL; }

static inline void gtk_text_buffer_get_start_iter(GtkTextBuffer *b, GtkTextIter *i) { (void)b; if(i) i->_dummy=0; }
static inline void gtk_text_buffer_get_end_iter(GtkTextBuffer *b, GtkTextIter *i) { (void)b; if(i) i->_dummy=0; }
static inline void gtk_text_buffer_select_range(GtkTextBuffer *b, GtkTextIter *a, GtkTextIter *c) { (void)b; (void)a; (void)c; }
static inline void gtk_text_buffer_copy_clipboard(GtkTextBuffer *b, GtkClipboard *cb) { (void)b; (void)cb; }
static inline void gtk_text_buffer_cut_clipboard(GtkTextBuffer *b, GtkClipboard *cb, int del) { (void)b; (void)cb; (void)del; }
static inline void gtk_text_buffer_paste_clipboard(GtkTextBuffer *b, GtkClipboard *cb, void *iter, int def) { (void)b; (void)cb; (void)iter; (void)def; }
static inline char *gtk_text_buffer_get_text(GtkTextBuffer *b, GtkTextIter *a, GtkTextIter *c, int inc) {
    (void)b; (void)a; (void)c; (void)inc; return NULL;
}

static inline GdkDisplay *gdk_display_get_default(void) { return NULL; }
static inline void gdk_display_beep(GdkDisplay *d) { (void)d; }

/* Headless stubs for a few UI helpers that are referenced by parser/editor code paths */
/*
 * IMPORTANT:
 *   Step A (normal UI build) compiles without WBASIC_NO_UI defined.
 *   So any App forward declaration must be visible *outside* the WBASIC_NO_UI block.
 *
 * We use a guard macro so we can safely repeat the forward decl in multiple regions
 * without risking a typedef redefinition.
 */
#ifndef WBASIC_APP_FWD_DECL
#define WBASIC_APP_FWD_DECL
typedef struct App App;
static void set_current_path(App *app, const char *path_or_null);
static void mark_dirty(App *app, bool dirty);
#endif

/* NOTE: UI-facing helpers are provided later in the file with proper WBASIC_NO_UI guards.
   Do not define any stubs here, or we can collide with the later guarded definitions. */

typedef struct _GdkEvent GdkEvent;
typedef struct _GdkDisplay GdkDisplay;
typedef struct _PangoFontDescription PangoFontDescription;
typedef int PangoStyle;
typedef int PangoWeight;
#ifndef PANGO_SCALE
#define PANGO_SCALE 1024
#endif

typedef struct _GdkRGBA { double red, green, blue, alpha; } GdkRGBA;
#endif

/* Ensure App is forward-declared for the normal UI build as well (WBASIC_NO_UI not defined). */
#ifndef WBASIC_APP_FWD_DECL
#define WBASIC_APP_FWD_DECL
typedef struct App App;
#endif

/* Forward declarations for helpers used before their definitions (avoid implicit int). */
static void *xmalloc(size_t n);
static void *xrealloc(void *ptr, size_t n);
static char *xstrdup(const char *s);
static char *inputdollar_read_keyboard(App *app, int n, int current_line);
static char *pack_i16(int v);
static char *pack_f32(float f);
static char *pack_f64(double d);
static int   unpack_i16(const char *s);
static float unpack_f32(const char *s);
static double unpack_f64(const char *s);





static bool is_word_char_(char c);

#ifndef WBASIC_NO_UI
#define WBASIC_RECENT_MAX 10

/* Recent Files forward declarations */
static void recent_menu_rebuild(App *app);
static void set_current_path(App *app, const char *path_or_null);
static void mark_dirty(App *app, bool dirty);
static void update_window_title(App *app);

static void on_recent_menu_activate(GtkMenuItem *mi, gpointer user_data);
static bool file_load_into_editor(App *app, const char *path);
#endif /* !WBASIC_NO_UI */

#include <string.h>
#include <strings.h>

/* Portable strdup replacement (avoids feature-macro/prototype issues). */
static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char*)malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}
#ifndef WBASIC_NO_UI
#include <gdk/gdkkeysyms.h>
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <ctype.h>
#include <time.h>
#if !defined(_WIN32)
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include <stddef.h>   /* for NULL */
#include <stdint.h>



static void print_usage(const char *prog)
{
#ifdef WBASIC_NO_UI
    fprintf(stderr, "Usage: %s [-s N]\n", prog);
    fprintf(stderr, "  -s N, --speed N, --speed=N   Set PRINT throttle speed (0=slowest, 100=fastest).\n");
    fprintf(stderr, "  -h, --help                   Show this help and exit.\n");
#else
    fprintf(stderr, "Usage: %s [-r] [program.bas]\n", prog);
    fprintf(stderr, "If a BASIC program file is provided, WBASIC will load it on startup.\n");
    fprintf(stderr, "  -r          Load and RUN the program automatically.\n");
    fprintf(stderr, "  -c, --cli   Run in terminal/CLI mode (no GTK UI).\n");
    fprintf(stderr, "      --headless  Alias for --cli.\n");
    fprintf(stderr, "      --gtk       Force GTK UI (do not auto-fallback to CLI).\n");
    fprintf(stderr, "  -h, --help  Show this help and exit.\n");
#endif
}

typedef struct {
    int line_no;
    char *text;      // raw text after line number (may contain ':')
} Line;

typedef struct {
    char *text;     // stored token text (trimmed; string literals have quotes removed)
    bool quoted;    // true if this came from a quoted string literal in DATA
    int line_no;  // BASIC line number where this DATA item originated
} DataItem;

typedef struct { int li; int si; } StmtPos;

typedef struct {
    StmtPos if_pos;     /* position of the IF <expr> THEN (block form) statement */
    bool has_else;
    StmtPos else_pos;   /* position of ELSE statement (if present) */
    StmtPos end_pos;    /* position of END IF / ENDIF statement */
    int elseif_count;
    StmtPos elseif_pos[32]; /* Phase 1: record ELSEIF positions (no semantics yet) */
} IfBlockMapEntry;



/* ===================== Block IF runtime exec frame (Phase 2) ===================== */
/* NOTE: Phase 2 Step 1 only introduces the state container. No behavior uses it yet. */
typedef struct IfExecFrame {
    int if_entry_index;
    int else_entry_index;
    int end_entry_index;

    /* Existing / Phase 1 */
    bool condition_true;

    /* Phase 2 ELSEIF runtime support */
    bool branch_taken;          /* any IF / ELSEIF branch executed */
    int  active_branch;         /* 0=IF, 1..N=ELSEIF, -1=ELSE */
} IfExecFrame;


/* ===================== DEF FN (user-defined functions; numeric) ===================== */
typedef struct {
    char *name;        /* uppercase, e.g., FNSQR */
    char **params;     /* uppercase parameter names */
    int param_count;
    char *expr_src;    /* RHS expression text */
    int ret_kind;    /* 0=NUM, 1=STR */
} FnDef;


typedef struct {
    Line *lines;
    size_t count;
    size_t cap;

// DATA / READ pool (built at RUN time from DATA statements)
DataItem *data;
size_t data_count;
size_t data_cap;
size_t data_ptr;

/* Block IF mapping (built at RUN time) */
IfBlockMapEntry *ifmap;
size_t ifmap_count;
size_t ifmap_cap;

    /* DEF FN definitions (numeric) */
    FnDef *fndefs;
    size_t fndef_count;
    size_t fndef_cap;

    // Fast lookup: BASIC line number -> index
    int *line_index_map;
    int line_index_min;
    int line_index_max;
} Program;


/* ===================== Block IF (build-time map) forward declarations ===================== */
static bool program_build_ifmap(App *app);
static bool stmt_is_block_else(const char *stmt);
static bool stmt_is_block_elseif(const char *stmt);
static bool stmt_is_block_end_if(const char *stmt);
static IfBlockMapEntry *program_ifmap_find_by_if(Program *p, int li, int si);
static IfBlockMapEntry *program_ifmap_find_by_else(Program *p, int li, int si);
static WB_UNUSED IfBlockMapEntry *program_ifmap_find_by_end(Program *p, int li, int si);
static IfBlockMapEntry *program_ifmap_find_by_elseif(Program *p, int li, int si, int *out_elseif_index);
static WB_UNUSED void prog_next_stmt(App *app, int li, int si, int *out_li, int *out_si);
static void prog_goto_stmt(App *app, int li, int si, int *out_li, int *out_si);


typedef struct {
    char *name;   /* uppercase var name (e.g., "A$") */
    int offset;   /* byte offset into record_buf */
    int len;      /* slice length in bytes */
} FieldMap;

typedef enum {
    FILE_MODE_RANDOM,

    V_NUM = 0,
    V_STR = 1,
} VarKind;

typedef enum {
    RUN_IDLE = 0,
    RUN_RUNNING = 1,
    RUN_WAITING = 2,
    RUN_PAUSED = 3,
    RUN_STOPPED = 4,
} RunState;



/* ===================== File I/O (sequential text) ===================== */

#define BASIC_MAX_FILES 10  /* handles #0..#9; we use #1..#9 */

typedef enum {
    BF_CLOSED = 0,
    BF_INPUT  = 1,
    BF_OUTPUT = 2,
    BF_APPEND = 3,
    BF_RANDOM = 4,
} BasicFileMode;

typedef struct {
    FILE *fp;
    int record_len; /* RANDOM file record length */
    unsigned char *record_buf; /* RANDOM record buffer (LEN bytes) */
    FieldMap *fields;          /* FIELD mappings (RANDOM only) */
    int field_count;
    int field_cap;
    BasicFileMode mode;
    bool eof_latched; /* once EOF is observed for INPUT, it stays true until CLOSE/OPEN */
} BasicFile;
static BasicFile *file_get(App *app, int n);

typedef struct {
    VarKind kind;
    bool is_array;
    bool num_is_int;    // GW-BASIC DEFINT / integer default typing (16-bit signed semantics on assignment)       // array (numeric or string)
    unsigned char num_type; /* 0=SNG,1=INT,2=DBL (for numeric vars/arrays). For strings ignored. */
    double num;          // scalar numeric
    char *str;           // scalar string

    // Numeric arrays (up to 5 dimensions).
    // Indices are arr_dim_lo[i]..arr_dim_max[i] (inclusive) for each dimension.
    // OPTION BASE affects the default lower bound (0 or 1) used at DIM time.
    double *arr;                 // flattened storage
    char **sarr;                // flattened storage for string arrays (NULL or strdup'd entries)
    int arr_dims;                // 1..5
    int arr_dim_lo[5];           // inclusive lower bound for each dim
    int arr_dim_max[5];          // inclusive upper bound for each dim
    size_t arr_total;            // total elements
    size_t arr_stride[5];        // row-major strides (last dim varies fastest)
} Var;

typedef struct {
    // FOR frame supports multi-statement execution
    char *var_name;          // numeric loop var name (stored uppercase)
    double end_value;
    double step_value;
    int start_line_idx;      // line index to jump back to
    int start_stmt_idx;      // statement index within line to jump back to
} ForFrame;


typedef struct {
    // WHILE frame supports multi-statement execution
    int while_line_idx;     // line index of the WHILE statement
    int while_stmt_idx;     // statement index within line of the WHILE statement
} WhileFrame;



typedef enum {
    DO_PRE_NONE = 0,     
// DO ... LOOP (no condition on DO)
    DO_PRE_WHILE = 1,    // DO WHILE <expr> ... LOOP
    DO_PRE_UNTIL = 2     // DO UNTIL <expr> ... LOOP
} DoPreCondKind;

typedef struct {
    // DO frame supports multi-statement execution across lines and within ':' chains
    int do_line_idx;      // line index of the DO statement
    int do_stmt_idx;      // statement index within line of the DO statement
    DoPreCondKind pre_kind; // whether DO had a pre-condition (WHILE/UNTIL)
} DoFrame;

typedef enum {
    GOSUB_RET_NORMAL = 0,   // return to (line_idx, stmt_idx)
    GOSUB_RET_CHAIN  = 1    // return into an in-progress ':' statement chain
    ,
    GOSUB_RET_KEYTRAP = 2 // return to (line_idx, stmt_idx) and clear ON KEY in-progress flag
    ,
    GOSUB_RET_TIMERTRAP = 3 // return to (line_idx, stmt_idx) and clear ON TIMER in-progress flag
} GosubRetKind;

typedef struct {
    GosubRetKind kind;

    // Always valid:
    int ret_line_idx;   // program line index (NOT BASIC line number)
    int ret_stmt_idx;   // top-level statement index within that line

    // Only for kind == GOSUB_RET_CHAIN:
    int ret_chain_next_si;  // next statement index within the chain
    char *ret_chain_text;   // strdup() of the full chain text
} GosubFrame;

typedef struct UndoStack UndoStack;

/* Output scrollback for the GTK interpreter output pane.
   UI-only: must be excluded from headless/export builds. */
#ifndef WBASIC_NO_UI
#define WBASIC_SCROLLBACK_MAX_LINES 1000

typedef struct {
    char *text;
    unsigned char *fg;
    unsigned char *bg;
    int cols;
} ScrollLine;
#endif /* !WBASIC_NO_UI */

typedef enum {
    WB_VIDEO_TEXT = 0,
    WB_VIDEO_GFX1 = 1,
    WB_VIDEO_GFX2 = 2,
    WB_VIDEO_GFX3 = 3,
    WB_VIDEO_GFX7 = 7,
    WB_VIDEO_GFX8 = 8,
    WB_VIDEO_GFX9 = 9,
    WB_VIDEO_GFX10 = 10,
    WB_VIDEO_GFX11 = 11,
    WB_VIDEO_GFX12 = 12,
    WB_VIDEO_GFX13 = 13
} WbVideoMode;

typedef struct {
    int mode;
    int w;
    int h;
    int nominal_colors;
    unsigned int policy_flags;
} ScreenModeSpec;

enum {
    SCREEN_POLICY_REQUIRES_UI = 1u << 0,
    SCREEN_POLICY_ALLOC_GFX   = 1u << 1
};

static const ScreenModeSpec k_screen_mode_specs[] = {
    { .mode = 0,  .w = 0,   .h = 0,   .nominal_colors = 16, .policy_flags = 0 },
    { .mode = 1,  .w = 320, .h = 200, .nominal_colors = 4,  .policy_flags = SCREEN_POLICY_REQUIRES_UI | SCREEN_POLICY_ALLOC_GFX },
    { .mode = 2,  .w = 640, .h = 200, .nominal_colors = 2,  .policy_flags = SCREEN_POLICY_REQUIRES_UI | SCREEN_POLICY_ALLOC_GFX },
    { .mode = 3,  .w = 640, .h = 400, .nominal_colors = 16, .policy_flags = SCREEN_POLICY_REQUIRES_UI | SCREEN_POLICY_ALLOC_GFX },
    { .mode = 7,  .w = 320, .h = 200, .nominal_colors = 16, .policy_flags = SCREEN_POLICY_REQUIRES_UI | SCREEN_POLICY_ALLOC_GFX },
    { .mode = 8,  .w = 640, .h = 200, .nominal_colors = 16, .policy_flags = SCREEN_POLICY_REQUIRES_UI | SCREEN_POLICY_ALLOC_GFX },
    { .mode = 9,  .w = 640, .h = 350, .nominal_colors = 16, .policy_flags = SCREEN_POLICY_REQUIRES_UI | SCREEN_POLICY_ALLOC_GFX },
    { .mode = 10, .w = 640, .h = 350, .nominal_colors = 4,  .policy_flags = SCREEN_POLICY_REQUIRES_UI | SCREEN_POLICY_ALLOC_GFX },
    { .mode = 11, .w = 640, .h = 480, .nominal_colors = 2,  .policy_flags = SCREEN_POLICY_REQUIRES_UI | SCREEN_POLICY_ALLOC_GFX },
    { .mode = 12, .w = 640, .h = 480, .nominal_colors = 16, .policy_flags = SCREEN_POLICY_REQUIRES_UI | SCREEN_POLICY_ALLOC_GFX },
    { .mode = 13, .w = 320, .h = 200, .nominal_colors = 256,.policy_flags = SCREEN_POLICY_REQUIRES_UI | SCREEN_POLICY_ALLOC_GFX }
};

static bool video_mode_is_graphics(WbVideoMode mode) {
    return mode != WB_VIDEO_TEXT;
}

static const ScreenModeSpec *screen_mode_spec_find(int mode) {
    for (size_t i = 0; i < (sizeof(k_screen_mode_specs) / sizeof(k_screen_mode_specs[0])); i++) {
        if (k_screen_mode_specs[i].mode == mode) {
            return &k_screen_mode_specs[i];
        }
    }
    return NULL;
}

typedef struct App {
    bool resume_from_gosub;
    GtkWidget *win;
    GtkTextBuffer *editor_buf;
    GtkTextBuffer *output_buf;
    GtkWidget *cmd_entry;
    bool ui_destroyed; /* set true once the GTK window is destroyed; prevents widget access */
    bool suppress_cmd_changed; /* internal: block cmd_entry changed handler during internal updates */

    /* Runtime INPUT (terminal-style): capture input via cmd_entry while program runs */
    bool input_waiting;
    bool input_ready;
    char *input_line;   /* owned; set when user hits Enter */
    int input_echo_row;
    int input_echo_col;
    int input_echo_len;
    int input_echo_draw_len;   /* includes cursor glyph when drawn */
    bool input_cursor_on;
    gint64 input_cursor_next_toggle_us; /* monotonic time (usec) for next blink toggle */

    // Status indicator (lower-right)
    GtkWidget *status_led;
    GtkWidget *status_label;
    RunState run_state;
    bool inkey_ready;
    char inkey_char;

    /* Headless (CLI) terminal I/O state. Present in unified builds too. */
    int headless_tty_fd;
#ifdef _WIN32
    void *headless_tty_old;
#else
    struct termios headless_tty_old;
#endif
    bool headless_tty_inited;
    bool headless_tty_using_stdin;
    bool headless_cursor_dirty; /* legacy safety net: LOCATE now moves cursor immediately */


    /* UI helper: last executing BASIC line number (used for PAUSE/STOP jump) */
    int ui_last_exec_line;

// GW-BASIC KEY macro support (F1..F10)
bool key_trap_enabled;              // KEY ON/OFF
char *key_macros[10];               // KEY 1..10 strings (NULL = unassigned)
    // ON KEY(n) GOSUB traps (F1..F10)
    int  on_key_gosub_line[10];        // target BASIC line number (0=unset)
    bool on_key_enabled[10];           // KEY(n) ON/OFF
    int  on_key_pending;               // pending key index 0..9, or -1
    bool on_key_in_progress;           // reentrancy guard for ON KEY trap

    char *runtime_key_macro;            // pending macro to execute in RUN context (owned)

    // ON TIMER(interval) GOSUB trap (GW-BASIC style)
    double on_timer_interval;           // seconds (0 = unset)
    int    on_timer_gosub_line;         // BASIC line number target (0 = unset/disabled)
    bool   timer_enabled;              // TIMER ON/OFF
    bool   timer_stopped;              // TIMER STOP (pauses)
    double timer_next_fire;            // next scheduled fire time (TIMER seconds)
    bool   timer_in_progress;          // reentrancy guard for ON TIMER trap

char *pending_key_macro;            // queued macro to execute (owned)
bool pending_key_macro_scheduled;   // idle callback scheduled
    char *pending_load_path;   // deferred LOAD path when requested while running
    bool  pending_load_clear_output;

    // AUTO command entry mode
    bool auto_mode;
    int auto_line;
    int auto_step;

    // TextViews (for theming)
    GtkWidget *editor_view;
    UndoStack *editor_undo;
    GtkWidget *output_view;
    GtkWidget *output_sw;
    GtkWidget *output_stack;
    GtkWidget *gfx_area;

#ifndef WBASIC_NO_UI
    /* GTK output optimization: keep a transcript scrollback (max lines) in the GtkTextBuffer,
       and render the current screen below a stable mark. */
    GtkTextMark *out_screen_start_mark;
    int out_scrollback_lines;
    int out_scrollback_max_lines;

    /* Output scrollback: captures lines that scroll off the fixed screen grid. */
    ScrollLine *scrollback_lines;
    int scrollback_cap;
    int scrollback_len;
    int scrollback_head;
    int scrollback_cols;
#endif /* !WBASIC_NO_UI */

    // UI colors via CSS
    bool have_fg;
    bool have_bg;
    GdkRGBA fg_color;
    GdkRGBA bg_color;
    GtkCssProvider *css_provider;
    GtkAccelGroup *accel;

    // RNG / timing
    double last_rnd;
    bool have_last_rnd;
    struct timeval start_tv;

    // Persisted UI settings
    GtkWidget *paned;
    bool have_win_size;
    int win_w;
    int win_h;
    bool have_win_pos;
    int win_x;
    int win_y;
    bool have_paned_pos;
    int paned_pos;
    char *font_name; // e.g., "Monospace 12"

    // Output speed preference: 0.0 = Slow (slow), 1.0 = Fast (no delay)
    double output_speed;
        double default_output_speed; /* speed to restore on RUN (cli/embedded/default) */
double print_throttle_carry_ms;
    bool export_include_speed; // if true, embed output speed into exported headless builds
    bool show_splash;          // if true, show startup splash dialog (GUI only)
    GtkWidget *splash_dlg;     // active splash dialog (or NULL)
    char *deferred_startup_file; // if set, load this file after splash dismiss
    bool deferred_autorun;       // if true, run after splash dismiss (after optional load)

    WbasicTickle tickle;
    // Output cursor tracking (1-based row/col) for LOCATE.
    // Lightweight model intended for common BASIC patterns like:
    //   CLS : LOCATE r,c : PRINT "..."
    // Cursor is updated as text is appended.
    int out_row;
    int out_col;
    bool out_just_wrapped;

// Screen buffer model for LOCATE/overwrite-style output.
// Default "text screen" size. Adjust if desired.
int screen_rows;
int screen_cols;
char *screen; // size = screen_rows * screen_cols, filled with spaces

/* Graphics v1 state (SCREEN 1): mode + indexed pixel buffer */
WbVideoMode video_mode;
int gfx_width;
int gfx_height;
unsigned char *gfx_pixels; /* size = gfx_width * gfx_height, color index 0..15 */
int gfx_draw_x;
int gfx_draw_y;
int gfx_draw_scale; /* DRAW scale multiplier (GW-BASIC compatibility) */
int gfx_draw_angle; /* DRAW angle quadrant 0..3 */

/* Text mode COLOR state (0-15) */
int cur_fg;   /* foreground */
int cur_bg;   /* background */

/* Per-cell color buffers for screen model (same size as screen[]) */
unsigned char *screen_fg;
unsigned char *screen_bg;

/* Screen render throttling */
bool screen_dirty;
guint screen_render_idle_id;

    // Cached interpreter pacing (microseconds)
    int exec_delay_us_per_stmt;
    int exec_sleep_chunk_us;

    // Accumulator for interpreter execution pacing (microseconds)
    int64_t exec_pace_accum_us;

    bool stop_flag;
    bool pause_flag;           // true when paused
    RunState pre_pause_state;  // state to restore on resume

    // Last runtime error (for error-message parity). Set by expression/array evaluators and
    // consumed by statement executors (PRINT/LET/etc.) to report specific GW-BASIC-like errors.
    bool err_pending;
    char err_msg[80];


    // BASIC OPTION BASE (0 or 1). Affects DIM default lower bounds.
    int option_base;
    bool option_base_locked;  // true once any array has been DIM'd (explicitly or implicitly)

    // Default typing map for variables without an explicit suffix (% ! # $).
    // Index 0='A' .. 25='Z'. Values: 0=SNG,1=INT,2=DBL,3=STR (GW-BASIC DEFINT/DEFSNG/DEFDBL/DEFSTR).
    unsigned char def_type[26]; /* 0=SNG,1=INT,2=DBL,3=STR (GW-BASIC DEFxxx default typing) */

    // Set when user requests quit (menu Quit or window close). Used to break
    // out of long-running interpreter loops so the process can exit cleanly.
    bool quitting;

    Program prog;

    // variable table (uppercase keys)
    GHashTable *vars; // key: gchar* (owned), value: Var*

    /* DEF FN call stack (recursion guard) */
    const char *fn_call_stack[32];
    int fn_call_sp;

    ForFrame for_stack[128];
    int for_sp;


    WhileFrame while_stack[128];
    int while_sp;

    
DoFrame do_stack[128];
int do_sp;

GosubFrame gosub_stack[128];
    int gosub_sp;

    // --- ':' statement-chain context (needed so GOSUB/RETURN can resume inside IF tails)
    bool chain_active;
    int chain_base_line_idx;     // top-level line index that owns the chain
    int chain_base_stmt_idx;     // top-level statement index that owns the chain
    const char *chain_text;      // points to current chain text (valid only while chain_active)

    // Pending resume into a chain after RETURN
    bool resume_chain_pending;
    int resume_chain_line_idx;
    int resume_chain_stmt_idx;
    int resume_chain_next_si;
    char *resume_chain_text;     // owns strdup() buffer

    IfExecFrame if_stack[128];
    int if_sp;

    // Current program filename tracking (for title bar)
    char *current_path;   // full path, or NULL for Untitled
    bool dirty;           // modified since last load/save/new
    bool suppress_dirty;  // internal: ignore buffer change signals

    GtkCssProvider *font_css;
    // BASIC file handles (#1..#9)
    BasicFile files[BASIC_MAX_FILES];

    /* ON ERROR GOTO support (GW-BASIC style) */
    int on_error_goto_line;          /* BASIC line number (0 = disabled) */
    bool error_trap_pending;         /* runtime_error requested a trap jump */
    int error_trap_line_idx;         /* program index to jump to */
    int error_trap_stmt_idx;         /* stmt index (usually 0) */
    bool in_error_handler;           /* prevent recursive trapping until control leaves handler */
    int last_err_line;               /* ERL-like: line number where last error occurred */
    int last_err_code;               /* ERR-like: numeric code (coarse) */

    /* Phase 0 (RESUME groundwork): capture exact execution cursor at time of error.
       This preserves WBASIC's (line_idx, stmt_idx) model so future RESUME can be correct
       even inside ':' statement chains. */
    bool exec_cursor_valid;
    int exec_line_idx;
    int exec_stmt_idx;
    int exec_line_no;

    bool err_origin_valid;
    int err_origin_line_idx;
    int err_origin_stmt_idx;

    bool err_origin_in_chain;
    int err_origin_chain_base_line_idx;
    int err_origin_chain_base_stmt_idx;
    int err_origin_chain_stmt_idx;      /* chain-local stmt index */
    char *err_origin_chain_text;        /* strdup() of chain text when in-chain */

    /* KEY macro queue (F1-F10) */
    char *key_macro_pending;             /* pending macro string to execute */
    gboolean key_macro_idle_scheduled;   /* idle handler scheduled? */

    guint key_macro_idle_id;            /* g_idle_add source id (0 if none) */

    /* File menu widgets (for enabling/disabling) */
    GtkWidget *mi_save;
    GtkWidget *mi_save_as;

    /* Recent files */
    GPtrArray *recent_files; /* array of char* (full paths) */
    GtkWidget *mi_recent_menu; /* submenu container */
    GtkWidget *mi_clear_recent;
    /* When WBASIC_NO_UI, the program text comes from here (not GTK editor). */
    const char *embedded_text;

} App;

/* ---- Unified build helpers ---- */
static inline bool wbasic_ui_active(const App *app) {
    return app && !app->ui_destroyed && app->win && app->output_buf && app->output_view;
}
static inline bool wbasic_has_ui_buffers(const App *app) {
    return app && !app->ui_destroyed && app->editor_buf && app->output_buf;
}






/* --- ANSI color + LOCATE support for exported/CLI builds ---
   Behavior:
   - Each printed line begins by applying the current BASIC COLOR (cached).
   - Each printed line ends with ANSI reset (\x1b[0m) before CR/LF.
   - LOCATE row,col moves the terminal cursor (TTY only) using ESC[row;colH.
   - COLOR with no arguments emits reset (\x1b[0m) immediately and restores defaults (cur_fg/cur_bg = 16).
*/

#ifndef WBASIC_HAS_HEADLESS_STDOUT_IS_TTY
#define WBASIC_HAS_HEADLESS_STDOUT_IS_TTY 1
static bool headless_stdout_is_tty(void) {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout));
#endif
}
#endif

static void headless_stdout_prepare_ansi(void) {
#ifdef _WIN32
    static bool tried = false;
    if (tried) return;
    tried = true;

    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hout == INVALID_HANDLE_VALUE || hout == NULL) return;

    DWORD mode = 0;
    if (!GetConsoleMode(hout, &mode)) return;

    (void)SetConsoleMode(hout, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

/* Forward decls for headless ANSI helpers (defined later in this file). */
static inline void headless_ansi_move(int row, int col);
static inline void headless_ansi_clear(void);
static inline void headless_ansi_color_cache_reset(void);
static inline void headless_ansi_apply_color(int fg, int bg);

/*
   Headless ANSI color caches.
   Declared here so the CLI writer can compare against them, and initialized
   to -1 so the first output forces a color emit.
*/
static int headless_last_fg = -1;
static int headless_last_bg = -1;

static void wbasic_cli_write_text_ansi(App *app, const char *s)
{
    static bool bol = true; /* beginning-of-line */
    if (!s) return;

    if (headless_stdout_is_tty()) {
        headless_stdout_prepare_ansi();
    }

    /* Honor LOCATE (cursor positioning) when writing to a real terminal. */
    if (app && app->headless_cursor_dirty && headless_stdout_is_tty()) {
        headless_ansi_move(app->out_row, app->out_col);
        app->headless_cursor_dirty = false;
        bol = true; /* ensure color prefix is applied at the new cursor location */
    }

    int fg = 7, bg = 0;
    if (app) { fg = app->cur_fg; bg = app->cur_bg; }

    const unsigned char *p = (const unsigned char*)s;
    while (*p) {
        /* Handle CRLF as a unit, ensuring reset occurs before line break. */
        if (*p == '\r' && p[1] == '\n') {
            fputs("\x1b[0m\r\n", stdout);
            headless_ansi_color_cache_reset();
            p += 2;
            bol = true;
            continue;
        }

        /*
           IMPORTANT:
           We reset attributes before every LF/CR to prevent background "banding".
           That means the *next* printable output must re-assert the current BASIC
           COLOR. Also, BASIC can change COLOR mid-line (e.g. PRINT 2;:COLOR 4:PRINT 4)
           without a newline, so we must apply color changes even when bol==false.

           Therefore: apply color whenever (a) we're at BOL, OR (b) the desired fg/bg
           differs from the last applied fg/bg.
        */
        if (headless_stdout_is_tty()) {
            if (bol || fg != headless_last_fg || bg != headless_last_bg) {
                headless_ansi_apply_color(fg, bg);
            }
        }
        if (bol) bol = false;

        if (*p == '\n') {
            fputs("\x1b[0m\n", stdout);
            headless_ansi_color_cache_reset();
            p++;
            bol = true;
            continue;
        }
        if (*p == '\r') {
            fputs("\x1b[0m\r", stdout);
            headless_ansi_color_cache_reset();
            p++;
            bol = true;
            continue;
        }

        fputc(*p, stdout);
        p++;
    }
    fflush(stdout);
}
/* --- end ANSI color + LOCATE support --- */

#ifndef WBASIC_NO_UI
static gboolean on_win_key_press(GtkWidget *w, GdkEventKey *e, gpointer user_data);

/* Title bar / document tracking (forward decls) */
static void update_window_title(App *app);
static void set_current_path(App *app, const char *path_or_null);
static void mark_dirty(App *app, bool dirty);
#ifndef WBASIC_NO_UI
static void on_editor_buf_changed(GtkTextBuffer *buf, gpointer user_data);

static bool ui_save_current_or_prompt(App *app);
static bool ui_save_as_prompt(App *app);
static bool ui_confirm_save_if_dirty(App *app);
static void request_quit(App *app);

// Status LED helpers
#ifndef WBASIC_NO_UI
static gboolean on_status_led_draw(GtkWidget *w, cairo_t *cr, gpointer user_data);
static void set_run_state(App *app, RunState st);
#ifndef WBASIC_NO_UI
static gboolean on_cmd_focus_in(GtkWidget *w, GdkEvent *e, gpointer user_data);

/* =======================================================================================
 * Recent Files (persistent list at ~/.config/wbasic/recent.txt)
 * =======================================================================================
 */

#ifndef WBASIC_NO_UI
static char *recent_store_path(void) {
    const char *cfg = g_get_user_config_dir(); /* e.g., ~/.config */
    char *dir = g_build_filename(cfg, "wbasic", NULL);
    (void)g_mkdir_with_parents(dir, 0755);
    char *path = g_build_filename(dir, "recent.txt", NULL);
    g_free(dir);
    return path; /* g_free() */
}
#endif /* !WBASIC_NO_UI */

#endif /* !WBASIC_NO_UI */
#endif /* !WBASIC_NO_UI */
#endif /* !WBASIC_NO_UI */
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void recent_save(App *app) {
    if (!app || !app->recent_files) return;
    char *path = recent_store_path();
    FILE *f = fopen(path, "w");
    if (!f) { g_free(path); return; }
    for (guint i = 0; i < app->recent_files->len; i++) {
        const char *p = (const char*)g_ptr_array_index(app->recent_files, i);
        if (p && *p) fprintf(f, "%s\n", p);
    }
    fclose(f);
    g_free(path);
}
#endif /* !WBASIC_NO_UI */


#ifndef WBASIC_NO_UI
static void recent_load(App *app) {
    if (!app) return;
    if (!app->recent_files) app->recent_files = g_ptr_array_new_with_free_func(g_free);

    char *path = recent_store_path();
    FILE *f = fopen(path, "r");
    if (!f) { g_free(path); return; }

    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        size_t n = strlen(line);
        while (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = 0;
        if (!line[0]) continue;
        if (app->recent_files->len >= WBASIC_RECENT_MAX) break;
        g_ptr_array_add(app->recent_files, g_strdup(line));
    }
    fclose(f);
    g_free(path);
}
#endif /* !WBASIC_NO_UI */


#ifndef WBASIC_NO_UI

static void recent_add(App *app, const char *path) {
    if (!app || !path || !*path) return;
    if (!app->recent_files) app->recent_files = g_ptr_array_new_with_free_func(g_free);

    char *full = g_canonicalize_filename(path, NULL);

    for (guint i = 0; i < app->recent_files->len; i++) {
        const char *p = (const char*)g_ptr_array_index(app->recent_files, i);
        if (p && !g_strcmp0(p, full)) {
            g_ptr_array_remove_index(app->recent_files, i);
            break;
        }
    }

    g_ptr_array_insert(app->recent_files, 0, full); /* ownership taken */

    while (app->recent_files->len > WBASIC_RECENT_MAX) {
        g_ptr_array_remove_index(app->recent_files, app->recent_files->len - 1);
    }

    recent_save(app);
    recent_menu_rebuild(app);
}

static void recent_clear(App *app) {
    if (!app) return;

    if (!app->recent_files) app->recent_files = g_ptr_array_new_with_free_func(g_free);
    g_ptr_array_set_size(app->recent_files, 0);
    recent_save(app);
    recent_menu_rebuild(app);
}

#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void out_clear(App *app, bool terminal_clear);

static void on_recent_item_activate(GtkMenuItem *mi, gpointer user_data) {
    App *app = (App*)user_data;
    const char *path = (const char*)g_object_get_data(G_OBJECT(mi), "recent_path");
    if (!app || !path) return;

    if (!ui_confirm_save_if_dirty(app)) return;

    // Match File/Open behavior: if a program is running, stop it and defer the load until idle.
    if (app->run_state == RUN_RUNNING || app->run_state == RUN_WAITING) {
        app->stop_flag = true;
        set_run_state(app, RUN_STOPPED);
        if (app->pending_load_path) g_free(app->pending_load_path);
        app->pending_load_path = g_strdup(path);
        app->pending_load_clear_output = true;
        return;
    }

    out_clear(app, false);
    file_load_into_editor(app, path);
    app->stop_flag = false;
    set_run_state(app, RUN_IDLE);
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void on_recent_clear_activate(GtkMenuItem *mi, gpointer user_data) {
    (void)mi;
    App *app = (App*)user_data;
    recent_clear(app);
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void recent_menu_rebuild(App *app) {
    if (!app || !app->mi_recent_menu) return;
    GtkWidget *menu = app->mi_recent_menu;

    GList *children = gtk_container_get_children(GTK_CONTAINER(menu));
    for (GList *l = children; l; l = l->next) gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    if (app->recent_files && app->recent_files->len > 0) {
        for (guint i = 0; i < app->recent_files->len; i++) {
            const char *p = (const char*)g_ptr_array_index(app->recent_files, i);
            if (!p) continue;

            const char *base = strrchr(p, G_DIR_SEPARATOR);
            base = base ? base + 1 : p;

            GtkWidget *mi2 = gtk_menu_item_new_with_label(base);
            g_object_set_data_full(G_OBJECT(mi2), "recent_path", g_strdup(p), g_free);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi2);
            g_signal_connect(mi2, "activate", G_CALLBACK(on_recent_item_activate), app);
        }
        GtkWidget *sep = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);
    }

    GtkWidget *mi_clear = gtk_menu_item_new_with_label("Clear Recent Files");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_clear);
    g_signal_connect(mi_clear, "activate", G_CALLBACK(on_recent_clear_activate), app);

    gtk_widget_show_all(menu);
}
#endif /* !WBASIC_NO_UI */


#ifndef WBASIC_NO_UI
static void on_recent_menu_activate(GtkMenuItem *mi, gpointer user_data) {
    (void)mi;
    App *app = (App*)user_data;
    recent_menu_rebuild(app);
}
#endif /* !WBASIC_NO_UI */

/* =======================================================================================
 * UI helper: jump editor to a BASIC line number (used on PAUSE/STOP only)
 * =======================================================================================
 */
#ifndef WBASIC_NO_UI
static void editor_jump_to_basic_line(App *app, int line_no) {
    if (!app || !app->editor_view || line_no <= 0) return;

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->editor_view));
    GtkTextIter it;
    gtk_text_buffer_get_start_iter(buf, &it);

    while (!gtk_text_iter_is_end(&it)) {
        GtkTextIter line_start = it;
        GtkTextIter line_end = it;
        gtk_text_iter_forward_to_line_end(&line_end);

        char *line = gtk_text_buffer_get_text(buf, &line_start, &line_end, FALSE);
        if (line) {
            char *s = line;
            while (*s == ' ' || *s == '\t') s++;
            char *endp = NULL;
            long ln = strtol(s, &endp, 10);
            if (endp != s && ln == line_no) {
                gtk_text_buffer_place_cursor(buf, &line_start);
                gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(app->editor_view),
                                             &line_start, 0.15, TRUE, 0.0, 0.2);
                g_free(line);
                return;
            }
            g_free(line);
        }

        if (!gtk_text_iter_forward_line(&it)) break;
    }
}
#endif /* !WBASIC_NO_UI */





#ifndef WBASIC_NO_UI
static gboolean on_cmd_focus_out(GtkWidget *w, GdkEvent *e, gpointer user_data);


/* ===================== Output helpers ===================== */

// ===================== Status LED =====================

static const char *run_state_text(RunState st) {
    switch (st) {
        case RUN_RUNNING: return "Running";
        case RUN_WAITING: return "Waiting";
        case RUN_PAUSED: return "Paused";
        case RUN_STOPPED: return "Stopped";
        case RUN_IDLE:
        default: return "Idle";
    }
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void set_run_state(App *app, RunState st) {
    if (!app) return;
    RunState prev = app->run_state;
    app->run_state = st;
    if (app->ui_destroyed) return;
    if (app->status_label) {
        gtk_label_set_text(GTK_LABEL(app->status_label), run_state_text(st));
    }
    if (app->status_led) {
        gtk_widget_queue_draw(app->status_led);
    }

    /* Jump editor to current line ONLY when entering PAUSED or STOPPED. */
    if ((st == RUN_PAUSED || st == RUN_STOPPED) && st != prev) {
        int target = 0;
        if (app->last_err_line > 0) target = app->last_err_line;
        else if (app->ui_last_exec_line > 0) target = app->ui_last_exec_line;
        if (target > 0) editor_jump_to_basic_line(app, target);
    }
}
#else
static void set_run_state(App *app, RunState st) {
    if (!app) return;
    app->run_state = st;
}
#endif /* !WBASIC_NO_UI */

// Draw a simple round "LED" in the current state color.
#ifndef WBASIC_NO_UI
static gboolean on_status_led_draw(GtkWidget *w, cairo_t *cr, gpointer user_data) {
    App *app = (App*)user_data;
    if (!app) return FALSE;

    GtkAllocation a;
    gtk_widget_get_allocation(w, &a);
    const double cx = a.width / 2.0;
    const double cy = a.height / 2.0;
    const double r  = (a.width < a.height ? a.width : a.height) * 0.40;

    // Choose color by state.
    double R=0.65, G=0.65, B=0.65; // idle gray
    switch (app->run_state) {
        case RUN_RUNNING: R=0.15; G=0.80; B=0.20; break; // green
        case RUN_WAITING: R=0.95; G=0.65; B=0.10; break; // amber (waiting)
        case RUN_PAUSED:  R=0.95; G=0.80; B=0.10; break; // yellow (paused)
        case RUN_STOPPED: R=0.90; G=0.20; B=0.20; break; // red
        case RUN_IDLE:
        default: break;
    }

    // Outer ring
    cairo_set_source_rgba(cr, 0, 0, 0, 0.35);
    cairo_arc(cr, cx, cy, r + 1.2, 0, 2 * 3.141592653589793);
    cairo_fill(cr);

    // LED fill
    cairo_set_source_rgb(cr, R, G, B);
    cairo_arc(cr, cx, cy, r, 0, 2 * 3.141592653589793);
    cairo_fill(cr);

    // Small highlight
    cairo_set_source_rgba(cr, 1, 1, 1, 0.35);
    cairo_arc(cr, cx - r*0.35, cy - r*0.35, r*0.35, 0, 2 * 3.141592653589793);
    cairo_fill(cr);

    return FALSE;
}
#endif /* !WBASIC_NO_UI */

// When idle, focusing the Command entry means we're "Waiting" for a command.
#ifndef WBASIC_NO_UI
static gboolean on_cmd_focus_in(GtkWidget *w, GdkEvent *e, gpointer user_data) {
    (void)w; (void)e; (void)user_data;
    /* Immediate command entry focus MUST NOT affect interpreter run state.
       RUN_WAITING is reserved for runtime INPUT/LINE INPUT while executing. */
    return FALSE;
}
#endif /* !WBASIC_NO_UI */


#ifndef WBASIC_NO_UI
static gboolean on_cmd_focus_out(GtkWidget *w, GdkEvent *e, gpointer user_data) {
    (void)w; (void)e; (void)user_data;
    /* No run-state changes on focus transitions for immediate command entry. */
    return FALSE;
}
#endif /* !WBASIC_NO_UI */




// Pump GTK events during long-running interpreter loops and delays.
// This keeps the UI responsive and allows window-close / Quit to take effect.
// Pump GTK events during long-running interpreter loops and delays.
// This keeps the UI responsive and allows window-close / Quit to take effect.
/* Cmd-entry stealth mode: during runtime INPUT we still capture keystrokes in the GtkEntry,
   but visually hide caret + focus ring so it looks like typing is going directly to output. */
#ifndef WBASIC_NO_UI
static void cmd_entry_install_stealth_css(App *app) {
    if (!app || app->ui_destroyed) return;
    if (!app->cmd_entry || !GTK_IS_WIDGET(app->cmd_entry) || !GTK_IS_ENTRY(app->cmd_entry)) return;
    static bool installed = false;
    if (installed) return;

    GtkCssProvider *prov = gtk_css_provider_new();
    const char *css =
        ".wbasic-cmd-stealth { caret-color: transparent; }\n"
        ".wbasic-cmd-stealth:focus { outline-style: none; outline-width: 0; box-shadow: none; }\n"
        ".wbasic-cmd-stealth:focus { border-color: transparent; }\n";
    gtk_css_provider_load_from_data(prov, css, -1, NULL);

    GdkScreen *screen = gdk_screen_get_default();
    if (screen) {
        gtk_style_context_add_provider_for_screen(
            screen, GTK_STYLE_PROVIDER(prov),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    g_object_unref(prov);
    installed = true;
}

static void cmd_entry_set_stealth(App *app, bool on) {
    if (!app || app->ui_destroyed) return;
    if (!app->cmd_entry || !GTK_IS_WIDGET(app->cmd_entry) || !GTK_IS_ENTRY(app->cmd_entry)) return;
    cmd_entry_install_stealth_css(app);
    GtkStyleContext *ctx = gtk_widget_get_style_context(app->cmd_entry);
    if (!ctx) return;

    if (on) {
        gtk_style_context_add_class(ctx, "wbasic-cmd-stealth");
        gtk_entry_set_has_frame(GTK_ENTRY(app->cmd_entry), FALSE);
    } else {
        gtk_style_context_remove_class(ctx, "wbasic-cmd-stealth");
        gtk_entry_set_has_frame(GTK_ENTRY(app->cmd_entry), TRUE);
    }
    gtk_widget_queue_draw(app->cmd_entry);
}

#else
static inline void cmd_entry_install_stealth_css(App *app) { (void)app; }
static inline void cmd_entry_set_stealth(App *app, bool on) { (void)app; (void)on; }
#endif /* !WBASIC_NO_UI */

#ifdef WBASIC_NO_UI
static inline void ui_pump_raw(App *app) { (void)app; }
static inline void ui_pump(App *app) { (void)app; }
static void ui_delay_ms(App *app, int ms) { (void)app; if (ms>0) g_usleep((gulong)ms * 1000UL); }
#else
static inline void ui_pump_raw(App *app) {
    if (!app || app->ui_destroyed || !app->win) return;
    while (gtk_events_pending()) gtk_main_iteration_do(FALSE);
    /* Ensure widget redraws (GtkEntry text/cursor) are flushed while we are in a busy-wait loop */
    if (app && !app->ui_destroyed && app->win && GTK_IS_WIDGET(app->win)) {
        GdkWindow *gw = gtk_widget_get_window(app->win);
        if (gw) {
            gtk_widget_queue_draw(app->win);
            GdkDisplay *dpy = gdk_window_get_display(gw);
            if (dpy) gdk_display_flush(dpy);
        }
        }
    if (app && app->quitting) app->stop_flag = true;
}

/* deleted unused static function: wbasic_ui_tickle */

// If paused, wait here while still pumping GTK events.
static void ui_pause_wait(App *app) {
    if (!app) return;
    if (!app->pause_flag) return;
    // Only meaningful while executing.
    if (app->run_state == RUN_IDLE || app->run_state == RUN_STOPPED) { app->pause_flag = false; return; }
    if (app->run_state != RUN_PAUSED) set_run_state(app, RUN_PAUSED);
    while (app->pause_flag && !app->stop_flag && !app->quitting) {
        ui_pump_raw(app);
        // Keep CPU usage low while paused.
        g_usleep(10 * 1000);
    }
    // On resume, restore state (but per UI requirement, show Running).
    if (!app->stop_flag && !app->quitting) {
        app->pause_flag = false;

    }
}

static inline void ui_pump(App *app) {
    ui_pump_raw(app);
    if (app && app->pause_flag) ui_pause_wait(app);
}

// Sleep while keeping the GTK UI responsive.
static void ui_delay_ms(App *app, int ms) {
    if (ms <= 0) return;
    if (!app || app->ui_destroyed || !app->win) { g_usleep((gulong)ms * 1000UL); return; }
    // Sleep in small chunks so we can pump GTK events.
    int remaining = ms;
    while (remaining > 0) {
        int chunk = remaining > 10 ? 10 : remaining;
        g_usleep((gulong)chunk * 500UL);
        ui_pump(app);
        if (app && app->quitting) return;
        remaining -= chunk;
    }
}
#endif

/* ---- TIMER event support (GW-BASIC ON TIMER / TIMER ON|OFF|STOP) ---- */
static double timer_now_sec(void) {
    struct timeval tv; gettimeofday(&tv, NULL);
    struct tm lt;
    time_t t = tv.tv_sec;
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    return (double)(lt.tm_hour*3600 + lt.tm_min*60 + lt.tm_sec) + (double)tv.tv_usec/1e6;
}

/* Midnight wrap-safe: return true if now has reached or passed target, assuming target was set based on timer_now_sec(). */
static bool timer_reached(double now, double target) {
    // TIMER wraps at 86400 seconds.
    const double DAY = 86400.0;
    double d = now - target;
    if (d >= 0.0 && d < DAY/2) return true;
    // Handle wrap: if target is near end of day and now is small.
    if (d < -DAY/2) return true;
    return false;
}

static void timer_schedule_next(App *app, double now) {
    if (!app || app->on_timer_interval <= 0.0) return;
    double next = now + app->on_timer_interval;
    if (next >= 86400.0) next -= 86400.0;
    app->timer_next_fire = next;
}


#ifndef WBASIC_NO_UI
static void scrollback_ensure(App *app);
static void ensure_color_tag(App *app, int fg, int bg, char tagname_out[32]);
#endif

static void screen_ensure(App *app) {
    if (!app) return;
    if (app->screen_rows <= 0) app->screen_rows = 25;
    if (app->screen_cols <= 0) app->screen_cols = 80;
    if (app->cur_fg < 0) app->cur_fg = 7;
    if (app->cur_bg < 0) app->cur_bg = 0;
    size_t need = (size_t)app->screen_rows * (size_t)app->screen_cols;
    if (!app->screen) {
        app->screen = (char*)malloc(need);
        if (!app->screen) return;
        memset(app->screen, ' ', need);
    }
    if (!app->screen_fg) {
        app->screen_fg = (unsigned char*)malloc(need);
        if (app->screen_fg) memset(app->screen_fg, (unsigned char)app->cur_fg, need);
    }
    if (!app->screen_bg) {
        app->screen_bg = (unsigned char*)malloc(need);
        if (app->screen_bg) memset(app->screen_bg, (unsigned char)app->cur_bg, need);
    }
#ifndef WBASIC_NO_UI
    scrollback_ensure(app);
#endif
    // Ensure output cursor initialized.
    if (app->out_row <= 0) app->out_row = 1;
    if (app->out_col <= 0) app->out_col = 1;
}

static void screen_clear(App *app) {
    screen_ensure(app);
    if (!app || !app->screen) return;
    size_t n = (size_t)app->screen_rows * (size_t)app->screen_cols;
    memset(app->screen, ' ', n);
    if (app->screen_fg) memset(app->screen_fg, (unsigned char)app->cur_fg, n);
    int clear_bg = video_mode_is_graphics(app->video_mode) ? 16 : app->cur_bg;
    if (app->screen_bg) memset(app->screen_bg, (unsigned char)clear_bg, n);
    app->out_row = 1;
    app->out_col = 1;
}

static void gfx_free(App *app) {
    if (!app) return;
    if (app->gfx_pixels) {
        free(app->gfx_pixels);
        app->gfx_pixels = NULL;
    }
    app->gfx_width = 0;
    app->gfx_height = 0;
}

static bool gfx_alloc(App *app, int w, int h) {
    if (!app || w <= 0 || h <= 0) return false;
    size_t n = (size_t)w * (size_t)h;
    unsigned char *buf = (unsigned char*)malloc(n);
    if (!buf) return false;
    memset(buf, 0, n);
    gfx_free(app);
    app->gfx_pixels = buf;
    app->gfx_width = w;
    app->gfx_height = h;
    return true;
}

static void gfx_clear(App *app, unsigned char color_idx) {
    if (!app || !app->gfx_pixels || app->gfx_width <= 0 || app->gfx_height <= 0) return;
    size_t n = (size_t)app->gfx_width * (size_t)app->gfx_height;
    memset(app->gfx_pixels, (int)(color_idx & 0x0F), n);
}

static void gfx_draw_reset_defaults(App *app) {
    if (!app) return;
    app->gfx_draw_x = (app->gfx_width > 0) ? (app->gfx_width / 2) : 0;
    app->gfx_draw_y = (app->gfx_height > 0) ? (app->gfx_height / 2) : 0;
    app->gfx_draw_scale = 1;
    app->gfx_draw_angle = 0;
}

static bool gfx_pset(App *app, int x, int y, int color_idx) {
    if (!app || !app->gfx_pixels) return false;
    if (x < 0 || y < 0 || x >= app->gfx_width || y >= app->gfx_height) return true;
    size_t idx = (size_t)y * (size_t)app->gfx_width + (size_t)x;
    app->gfx_pixels[idx] = (unsigned char)(color_idx & 0x0F);
    return true;
}

static int gfx_point(App *app, int x, int y) {
    if (!app || !app->gfx_pixels) return -1;
    if (x < 0 || y < 0 || x >= app->gfx_width || y >= app->gfx_height) return -1;
    size_t idx = (size_t)y * (size_t)app->gfx_width + (size_t)x;
    return (int)app->gfx_pixels[idx];
}

static void gfx_line(App *app, int x0, int y0, int x1, int y1, int color_idx) {
    if (!app || !app->gfx_pixels) return;
    int dx = abs(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (1) {
        (void)gfx_pset(app, x0, y0, color_idx);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void gfx_circle_plot8(App *app, int cx, int cy, int x, int y, int color_idx) {
    (void)gfx_pset(app, cx + x, cy + y, color_idx);
    (void)gfx_pset(app, cx - x, cy + y, color_idx);
    (void)gfx_pset(app, cx + x, cy - y, color_idx);
    (void)gfx_pset(app, cx - x, cy - y, color_idx);
    (void)gfx_pset(app, cx + y, cy + x, color_idx);
    (void)gfx_pset(app, cx - y, cy + x, color_idx);
    (void)gfx_pset(app, cx + y, cy - x, color_idx);
    (void)gfx_pset(app, cx - y, cy - x, color_idx);
}

static void gfx_circle(App *app, int cx, int cy, int radius, int color_idx) {
    if (!app || !app->gfx_pixels || radius < 0) return;
    int x = radius;
    int y = 0;
    int d = 1 - radius;

    while (x >= y) {
        gfx_circle_plot8(app, cx, cy, x, y, color_idx);
        y++;
        if (d < 0) {
            d += 2 * y + 1;
        } else {
            x--;
            d += 2 * (y - x) + 1;
        }
    }
}


static bool gfx_paint(App *app, int sx, int sy, int color_idx, bool have_border, int border_idx) {
    if (!app || !app->gfx_pixels) return false;
    if (sx < 0 || sy < 0 || sx >= app->gfx_width || sy >= app->gfx_height) return true;

    int w = app->gfx_width;
    int h = app->gfx_height;
    int newc = color_idx & 0x0F;
    int border = border_idx & 0x0F;

    size_t n = (size_t)w * (size_t)h;
    int *queue = (int*)malloc(n * sizeof(int));
    unsigned char *seen = (unsigned char*)calloc(n, sizeof(unsigned char));
    if (!queue || !seen) {
        free(queue);
        free(seen);
        return false;
    }

    int oldc = gfx_point(app, sx, sy);
    if (oldc < 0) {
        free(queue);
        free(seen);
        return true;
    }

    if (have_border) {
        if (oldc == border || oldc == newc) {
            free(queue);
            free(seen);
            return true;
        }
    } else if (oldc == newc) {
        free(queue);
        free(seen);
        return true;
    }

    size_t head = 0, tail = 0;
    int start = sy * w + sx;
    queue[tail++] = start;
    seen[start] = 1;

    while (head < tail) {
        int idx = queue[head++];
        int x = idx % w;
        int y = idx / w;
        int c = gfx_point(app, x, y);

        bool fillable = have_border ? (c != border && c != newc) : (c == oldc);
        if (!fillable) continue;

        (void)gfx_pset(app, x, y, newc);

        if (x > 0) {
            int ni = idx - 1;
            if (!seen[ni]) { seen[ni] = 1; queue[tail++] = ni; }
        }
        if (x + 1 < w) {
            int ni = idx + 1;
            if (!seen[ni]) { seen[ni] = 1; queue[tail++] = ni; }
        }
        if (y > 0) {
            int ni = idx - w;
            if (!seen[ni]) { seen[ni] = 1; queue[tail++] = ni; }
        }
        if (y + 1 < h) {
            int ni = idx + w;
            if (!seen[ni]) { seen[ni] = 1; queue[tail++] = ni; }
        }
    }

    free(seen);
    free(queue);
    return true;
}

#ifndef WBASIC_NO_UI
static void scrollback_clear(App *app) {
    if (!app || !app->scrollback_lines) {
        if (app) { app->scrollback_cap = app->scrollback_len = app->scrollback_head = 0; app->scrollback_cols = 0; }
        return;
    }
    for (int i = 0; i < app->scrollback_cap; i++) {
        ScrollLine *sl = &app->scrollback_lines[i];
        free(sl->text);
        free(sl->fg);
        free(sl->bg);
        sl->text = NULL;
        sl->fg = NULL;
        sl->bg = NULL;
        sl->cols = 0;
    }
    app->scrollback_len = 0;
    app->scrollback_head = 0;
}

static void scrollback_ensure(App *app) {
    if (!app) return;
    int rows = app->screen_rows > 0 ? app->screen_rows : 25;
    int cols = app->screen_cols > 0 ? app->screen_cols : 80;
    int cap = WBASIC_SCROLLBACK_MAX_LINES - rows;
    if (cap < 0) cap = 0;

    if (app->scrollback_lines && (app->scrollback_cap != cap || app->scrollback_cols != cols)) {
        scrollback_clear(app);
        free(app->scrollback_lines);
        app->scrollback_lines = NULL;
        app->scrollback_cap = 0;
        app->scrollback_cols = 0;
    }

    if (!app->scrollback_lines && cap > 0) {
        app->scrollback_lines = (ScrollLine*)calloc((size_t)cap, sizeof(ScrollLine));
        if (!app->scrollback_lines) {
            app->scrollback_cap = 0;
            app->scrollback_cols = 0;
            return;
        }
        app->scrollback_cap = cap;
        app->scrollback_len = 0;
        app->scrollback_head = 0;
        app->scrollback_cols = cols;
    }
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static WB_UNUSED void scrollback_push_row(App *app, const char *row, const unsigned char *fg, const unsigned char *bg, int cols) {
    if (!app || !row || cols <= 0) return;
#ifndef WBASIC_NO_UI
    scrollback_ensure(app);
#endif
    if (!app->scrollback_lines || app->scrollback_cap <= 0) return;

    int idx = 0;
    if (app->scrollback_len < app->scrollback_cap) {
        idx = (app->scrollback_head + app->scrollback_len) % app->scrollback_cap;
        app->scrollback_len++;
    } else {
        idx = app->scrollback_head;
        app->scrollback_head = (app->scrollback_head + 1) % app->scrollback_cap;
    }

    ScrollLine *sl = &app->scrollback_lines[idx];
    free(sl->text);
    free(sl->fg);
    free(sl->bg);
    sl->text = (char*)malloc((size_t)cols);
    sl->fg   = fg ? (unsigned char*)malloc((size_t)cols) : NULL;
    sl->bg   = bg ? (unsigned char*)malloc((size_t)cols) : NULL;
    if (!sl->text) { sl->cols = 0; return; }
    memcpy(sl->text, row, (size_t)cols);
    if (sl->fg && fg) memcpy(sl->fg, fg, (size_t)cols);
    if (sl->bg && bg) memcpy(sl->bg, bg, (size_t)cols);
    sl->cols = cols;


#ifndef WBASIC_NO_UI
    /* GTK optimized transcript append:
       When a terminal-style scroll occurs, append the popped row (trimmed) above the screen mark.
       This avoids rebuilding the entire buffer every flush and provides smooth scrollback. */
    if (app && app->output_buf && app->output_view) {
        GtkTextMark *m = app->out_screen_start_mark;
        if (!m) {
            GtkTextIter it0;
            gtk_text_buffer_get_start_iter(app->output_buf, &it0);
            m = gtk_text_buffer_get_mark(app->output_buf, "out_screen_start");
            if (!m) m = gtk_text_buffer_create_mark(app->output_buf, "out_screen_start", &it0, TRUE);
            else gtk_text_buffer_move_mark(app->output_buf, m, &it0);
            app->out_screen_start_mark = m;
        }

        /* Build a trimmed C-string (strip trailing spaces) */
        int last = cols - 1;
        while (last >= 0 && row && row[last] == ' ') last--;
        int upto = last + 1;
        if (upto < 0) upto = 0;

        GtkTextIter ins;
        gtk_text_buffer_get_iter_at_mark(app->output_buf, &ins, m);
        if (upto > 0 && row) {
            if (fg && bg) {
                int i = 0;
                while (i < upto) {
                    int f = (int)fg[i];
                    int b = (int)bg[i];
                    int j = i + 1;
                    while (j < upto && (int)fg[j] == f && (int)bg[j] == b) j++;
                    char tagname[32];
                    ensure_color_tag(app, f, b, tagname);
                    gtk_text_buffer_insert_with_tags_by_name(app->output_buf, &ins, row + i, j - i, tagname, NULL);
                    i = j;
                }
            } else {
                gtk_text_buffer_insert(app->output_buf, &ins, row, upto);
            }
        }
        gtk_text_buffer_insert(app->output_buf, &ins, "\n", 1);
        /* Keep the mark at the boundary: advance it past the appended transcript line. */
        gtk_text_buffer_move_mark(app->output_buf, m, &ins);


        app->out_scrollback_lines++;
        if (app->out_scrollback_max_lines <= 0) app->out_scrollback_max_lines = 1000;
        while (app->out_scrollback_lines > app->out_scrollback_max_lines) {
            /* Delete first logical line from the top of the transcript region. */
            GtkTextIter a, b;
            gtk_text_buffer_get_start_iter(app->output_buf, &a);
            /* If the buffer has fewer than 1 line, stop. */
            if (!gtk_text_iter_ends_line(&a)) {
                /* move to end of first line */
                b = a;
                gtk_text_iter_forward_to_line_end(&b);
            } else {
                b = a;
            }
            /* include newline if present */
            if (gtk_text_iter_get_char(&b) != '\n') {
                GtkTextIter tmp = b;
                if (gtk_text_iter_forward_char(&tmp) && gtk_text_iter_get_char(&tmp) == '\n') b = tmp;
            }
            GtkTextIter b2 = b;
            if (gtk_text_iter_get_char(&b2) == '\n') gtk_text_iter_forward_char(&b2);
            gtk_text_buffer_delete(app->output_buf, &a, &b2);
            app->out_scrollback_lines--;
        }

        /* If running, keep view pinned to bottom. */
        if (app->run_state == RUN_RUNNING) {
            GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(app->output_view));
            if (vadj) {
                double upper = gtk_adjustment_get_upper(vadj);
                double page  = gtk_adjustment_get_page_size(vadj);
                double maxv  = upper - page;
                if (maxv < 0) maxv = 0;
                gtk_adjustment_set_value(vadj, maxv);
            }
        }
    }
#endif
}
#else
static void WB_UNUSED scrollback_push_row(App *app, const char *row, const unsigned char *fg, const unsigned char *bg, int cols) {
    (void)app; (void)row; (void)fg; (void)bg; (void)cols;
}
#endif /* WBASIC_NO_UI */



// Scroll the screen up by one row (like a terminal) when output runs past the bottom.
static void screen_scroll_up(App *app) {
    if (!app || !app->screen) return;
    int R = app->screen_rows, C = app->screen_cols;
    if (R <= 1 || C <= 0) return;
#ifndef WBASIC_NO_UI
    /* Capture the top row into scrollback before it scrolls off. */
    scrollback_push_row(app, app->screen, app->screen_fg, app->screen_bg, C);
#endif
    memmove(app->screen, app->screen + C, (size_t)(R - 1) * (size_t)C);
    if (app->screen_fg) memmove(app->screen_fg, app->screen_fg + C, (size_t)(R - 1) * (size_t)C);
    if (app->screen_bg) memmove(app->screen_bg, app->screen_bg + C, (size_t)(R - 1) * (size_t)C);
    memset(app->screen + (size_t)(R - 1) * (size_t)C, ' ', (size_t)C);
    if (app->screen_fg) memset(app->screen_fg + (size_t)(R - 1) * (size_t)C, (unsigned char)app->cur_fg, (size_t)C);
    if (app->screen_bg) memset(app->screen_bg + (size_t)(R - 1) * (size_t)C, (unsigned char)app->cur_bg, (size_t)C);
    if (app->out_row > 1) app->out_row--;
}

static void screen_put_at(App *app, int row1, int col1, char ch) {
    if (!app || !app->screen) return;
    if (row1 < 1) row1 = 1;
    if (col1 < 1) col1 = 1;
    if (row1 > app->screen_rows) row1 = app->screen_rows;
    if (col1 > app->screen_cols) col1 = app->screen_cols;
    size_t idx = (size_t)(row1 - 1) * (size_t)app->screen_cols + (size_t)(col1 - 1);
    app->screen[idx] = ch;
    if (app->screen_fg) app->screen_fg[idx] = (unsigned char)app->cur_fg;
    if (app->screen_bg) app->screen_bg[idx] = (unsigned char)app->cur_bg;
}

static void screen_advance(App *app) {
    if (!app) return;
    app->out_col++;
    if (app->out_col > app->screen_cols) {
        app->out_col = 1;
        app->out_row++;
        if (app->out_row > app->screen_rows) {
            screen_scroll_up(app);
            app->out_row = app->screen_rows;
        }
        app->out_just_wrapped = true;
    }
}

static void screen_newline(App *app) {
    if (!app) return;
    app->out_row++;
    app->out_col = 1;
    if (app->out_row > app->screen_rows) {
        screen_scroll_up(app);
        app->out_row = app->screen_rows;
    }
    app->out_just_wrapped = false;
}

// Write a string into the screen buffer at the current cursor.
static void screen_write(App *app, const char *s) {
    screen_ensure(app);
    if (!app || !app->screen || !s) return;

    for (const unsigned char *q = (const unsigned char*)s; *q; q++) {
        unsigned char c = *q;
        if (c == '\n') {
            screen_newline(app);
            app->out_just_wrapped = false;
        } else if (c == '\r') {
            app->out_col = 1;
            app->out_just_wrapped = false;
        } else if (c == '\t') {
            // 8-column tab stops (1-based).
            int next = ((app->out_col - 1) / 8 + 1) * 8 + 1;
            if (next < 1) next = 1;
            if (next > app->screen_cols) {
                app->out_col = 1;
                app->out_row++;
                if (app->out_row > app->screen_rows) {
                    screen_scroll_up(app);
                    app->out_row = app->screen_rows;
                }
            } else {
                app->out_col = next;
            }
            app->out_just_wrapped = false;
        } else {
            // Printable (or at least displayable) byte: place and advance.
            screen_put_at(app, app->out_row, app->out_col, (char)c);
            screen_advance(app);
        }
    }
}

// Render the screen buffer into the GTK output TextView.

static WB_UNUSED const char *vga16_hex[16] = {
    "#000000", "#0000AA", "#00AA00", "#00AAAA",
    "#AA0000", "#AA00AA", "#AA5500", "#AAAAAA",
    "#555555", "#5555FF", "#55FF55", "#55FFFF",
    "#FF5555", "#FF55FF", "#FFFF55", "#FFFFFF"
};

static const unsigned char vga16_rgb[16][3] = {
    {0x00,0x00,0x00}, {0x00,0x00,0xAA}, {0x00,0xAA,0x00}, {0x00,0xAA,0xAA},
    {0xAA,0x00,0x00}, {0xAA,0x00,0xAA}, {0xAA,0x55,0x00}, {0xAA,0xAA,0xAA},
    {0x55,0x55,0x55}, {0x55,0x55,0xFF}, {0x55,0xFF,0x55}, {0x55,0xFF,0xFF},
    {0xFF,0x55,0x55}, {0xFF,0x55,0xFF}, {0xFF,0xFF,0x55}, {0xFF,0xFF,0xFF}
};

#ifndef WBASIC_NO_UI

static WB_UNUSED void ensure_color_tag(App *app, int fg, int bg, char tagname_out[32]) {
    if (!app || !app->output_buf) { tagname_out[0] = 0; return; }

    // 0-15 = GW-BASIC palette index; 16 = use Preferences exact colors
    if (fg < 0) fg = 16;
    if (bg < 0) bg = 16;

    if (fg > 16) fg = 16;
    if (bg > 16) bg = 16;

    snprintf(tagname_out, 32, "clr_%d_%d", fg, bg);

    GtkTextTagTable *tt = gtk_text_buffer_get_tag_table(app->output_buf);
    GtkTextTag *existing = gtk_text_tag_table_lookup(tt, tagname_out);

    const char *fg_hex = NULL;
    const char *bg_hex = NULL;

    char fg_buf[64] = {0};
    char bg_buf[64] = {0};

    if (fg == 16) {
        if (app->have_fg) {
            int r = (int)(app->fg_color.red   * 255.0 + 0.5);
            int g = (int)(app->fg_color.green * 255.0 + 0.5);
            int b = (int)(app->fg_color.blue  * 255.0 + 0.5);
            snprintf(fg_buf, sizeof(fg_buf), "#%02x%02x%02x", r & 255, g & 255, b & 255);
            fg_hex = fg_buf;
        } else {
            fg_hex = vga16_hex[7];
        }
    } else {
        fg_hex = vga16_hex[fg & 15];
    }

    if (bg == 16) {
        if (app->have_bg) {
            int r = (int)(app->bg_color.red   * 255.0 + 0.5);
            int g = (int)(app->bg_color.green * 255.0 + 0.5);
            int b = (int)(app->bg_color.blue  * 255.0 + 0.5);
            snprintf(bg_buf, sizeof(bg_buf), "#%02x%02x%02x", r & 255, g & 255, b & 255);
            bg_hex = bg_buf;
        } else {
            bg_hex = vga16_hex[0];
        }
    } else {
        bg_hex = vga16_hex[bg & 15];
    }

    if (existing) {
        if (fg == 16 || bg == 16) {
            g_object_set(existing,
                         "foreground", fg_hex,
                         "background", bg_hex,
                         NULL);
        }
        return;
    }

    gtk_text_buffer_create_tag(app->output_buf, tagname_out,
                               "foreground", fg_hex,
                               "background", bg_hex,
                               NULL);
}
#endif /* !WBASIC_NO_UI */

#ifdef WBASIC_NO_UI
static WB_UNUSED void ensure_color_tag(App *app, int fg, int bg, char tagname_out[32]) {
    (void)app; (void)fg; (void)bg;
    if (tagname_out) tagname_out[0] = 0;
}
#endif

#ifndef WBASIC_NO_UI

static void ui_update_output_mode(App *app) {
    if (!app || !app->output_stack) return;
    if (video_mode_is_graphics(app->video_mode)) {
        gtk_stack_set_visible_child_name(GTK_STACK(app->output_stack), "gfx");
    } else {
        gtk_stack_set_visible_child_name(GTK_STACK(app->output_stack), "text");
    }
}

static void ui_color_idx_to_rgb(const App *app, int idx, double *r, double *g, double *b, bool for_bg) {
    if (!r || !g || !b) return;
    if (idx == 16) {
        if (for_bg && app && app->have_bg) {
            *r = app->bg_color.red;
            *g = app->bg_color.green;
            *b = app->bg_color.blue;
            return;
        }
        if (!for_bg && app && app->have_fg) {
            *r = app->fg_color.red;
            *g = app->fg_color.green;
            *b = app->fg_color.blue;
            return;
        }
        idx = for_bg ? 0 : 7;
    }
    idx &= 0x0F;
    *r = (double)vga16_rgb[idx][0] / 255.0;
    *g = (double)vga16_rgb[idx][1] / 255.0;
    *b = (double)vga16_rgb[idx][2] / 255.0;
}

static void ui_draw_gfx_text_overlay(App *app, cairo_t *cr,
                                     double ox, double oy,
                                     double draw_w, double draw_h) {
    if (!app || !app->screen || app->screen_rows <= 0 || app->screen_cols <= 0) return;

    int R = app->screen_rows;
    int C = app->screen_cols;
    double cell_w = draw_w / (double)C;
    double cell_h = draw_h / (double)R;
    if (cell_w <= 0.0 || cell_h <= 0.0) return;

    cairo_save(cr);
    cairo_translate(cr, ox, oy);
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    /* Keep glyphs within cell bounds: SCREEN 1/2 often have much taller rows than columns,
       so purely height-based sizing causes horizontal overlap (e.g., "COLOR" -> "CCLOR"). */
    double fs_h = cell_h * 0.82;
    double fs_w = cell_w * 1.45;
    double font_sz = fs_h;
    if (fs_w < font_sz) font_sz = fs_w;
    if (font_sz < 6.0) font_sz = 6.0;
    cairo_set_font_size(cr, font_sz);

    cairo_font_extents_t fe;
    cairo_font_extents(cr, &fe);
    double y_base_off = (cell_h - fe.height) * 0.5 + fe.ascent;

    /* Pass 1: draw text-cell backgrounds first so later cells never erase earlier glyphs. */
    for (int r = 0; r < R; r++) {
        for (int c = 0; c < C; c++) {
            size_t idx = (size_t)r * (size_t)C + (size_t)c;
            int bg = app->screen_bg ? app->screen_bg[idx] : app->cur_bg;
            if (bg < 0) bg = 0;
            if (bg == 16) continue;

            double x = (double)c * cell_w;
            double y = (double)r * cell_h;
            double br, bgc, bb;
            ui_color_idx_to_rgb(app, bg, &br, &bgc, &bb, true);
            cairo_set_source_rgb(cr, br, bgc, bb);
            cairo_rectangle(cr, floor(x), floor(y), ceil(cell_w), ceil(cell_h));
            cairo_fill(cr);
        }
    }

    /* Pass 2: draw glyphs on top. */
    for (int r = 0; r < R; r++) {
        for (int c = 0; c < C; c++) {
            size_t idx = (size_t)r * (size_t)C + (size_t)c;
            char ch = app->screen[idx];
            int fg = app->screen_fg ? app->screen_fg[idx] : app->cur_fg;
            if (fg < 0) fg = 7;
            if (ch == ' ') continue;

            double x = (double)c * cell_w;
            double y = (double)r * cell_h;
            double fr, fgc, fb;
            ui_color_idx_to_rgb(app, fg, &fr, &fgc, &fb, false);
            cairo_set_source_rgb(cr, fr, fgc, fb);

            char txt[2] = { ch, 0 };
            cairo_text_extents_t te;
            cairo_text_extents(cr, txt, &te);
            double tx = x + (cell_w - te.x_advance) * 0.5;
            if (tx < x) tx = x;
            cairo_move_to(cr, tx, y + y_base_off);
            cairo_show_text(cr, txt);
        }
    }

    cairo_restore(cr);
}

static gboolean on_gfx_area_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)widget;
    App *app = (App*)user_data;
    if (!app) return FALSE;

    int ww = gtk_widget_get_allocated_width(app->gfx_area);
    int wh = gtk_widget_get_allocated_height(app->gfx_area);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    if (ww <= 0 || wh <= 0 || !app->gfx_pixels || app->gfx_width <= 0 || app->gfx_height <= 0) {
        return FALSE;
    }

    int w = app->gfx_width;
    int h = app->gfx_height;
    size_t n = (size_t)w * (size_t)h;
    guint32 *pix = (guint32*)g_malloc((gsize)(n * sizeof(guint32)));
    if (!pix) return FALSE;

    for (size_t i = 0; i < n; i++) {
        unsigned char c = app->gfx_pixels[i] & 0x0F;
        unsigned char r = vga16_rgb[c][0];
        unsigned char g = vga16_rgb[c][1];
        unsigned char b = vga16_rgb[c][2];
        pix[i] = (0xFFu << 24) | ((guint32)r << 16) | ((guint32)g << 8) | (guint32)b;
    }

    cairo_surface_t *surf = cairo_image_surface_create_for_data((unsigned char*)pix,
                                                                CAIRO_FORMAT_ARGB32,
                                                                w, h, w * 4);

    /* Graphics modes are presented in a fixed 4:3 viewport for consistent display. */
    double target_aspect = 4.0 / 3.0;

    double draw_w = (double)ww;
    double draw_h = draw_w / target_aspect;
    if (draw_h > (double)wh) {
        draw_h = (double)wh;
        draw_w = draw_h * target_aspect;
    }

    if (draw_w <= 0.0 || draw_h <= 0.0) {
        cairo_surface_destroy(surf);
        g_free(pix);
        return FALSE;
    }

    double ox = ((double)ww - draw_w) * 0.5;
    double oy = ((double)wh - draw_h) * 0.5;
    double sx = draw_w / (double)w;
    double sy = draw_h / (double)h;

    cairo_save(cr);
    cairo_translate(cr, ox, oy);
    cairo_scale(cr, sx, sy);
    cairo_set_source_surface(cr, surf, 0.0, 0.0);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
    cairo_paint(cr);
    cairo_restore(cr);

    ui_draw_gfx_text_overlay(app, cr, ox, oy, draw_w, draw_h);

    cairo_surface_destroy(surf);
    g_free(pix);
    return FALSE;
}

static void screen_render_now(App *app) {
    if (!wbasic_ui_active(app)) return;
    if (!app->output_buf || !app->output_view) return;

    ui_update_output_mode(app);
    if (video_mode_is_graphics(app->video_mode)) {
        if (app->gfx_area) gtk_widget_queue_draw(app->gfx_area);
        return;
    }

    /* While a program is RUNNING, keep the view pinned to the bottom. */
    GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(app->output_view));

    /* Ensure the screen-start mark exists. */
    GtkTextMark *screen_mark = app->out_screen_start_mark;
    if (!screen_mark) {
        GtkTextIter it0;
        gtk_text_buffer_get_start_iter(app->output_buf, &it0);
        screen_mark = gtk_text_buffer_get_mark(app->output_buf, "out_screen_start");
        if (!screen_mark) screen_mark = gtk_text_buffer_create_mark(app->output_buf, "out_screen_start", &it0, TRUE);
        else gtk_text_buffer_move_mark(app->output_buf, screen_mark, &it0);
        app->out_screen_start_mark = screen_mark;
    }

    /* Delete and rebuild ONLY the current screen region (below the mark). */
    GtkTextIter start_it, end_it;
    gtk_text_buffer_get_iter_at_mark(app->output_buf, &start_it, screen_mark);
    gtk_text_buffer_get_end_iter(app->output_buf, &end_it);
    gtk_text_buffer_delete(app->output_buf, &start_it, &end_it);

    GtkTextIter it;
    gtk_text_buffer_get_iter_at_mark(app->output_buf, &it, screen_mark);

    screen_ensure(app);
    if (!app->screen) return;
    int R = app->screen_rows, C = app->screen_cols;

    for (int r = 0; r < R; r++) {
        const char *rowp = app->screen + (size_t)r * (size_t)C;
        const unsigned char *rowfg = app->screen_fg ? (app->screen_fg + (size_t)r * (size_t)C) : NULL;
        const unsigned char *rowbg = app->screen_bg ? (app->screen_bg + (size_t)r * (size_t)C) : NULL;

        int last = C;
        while (last > 0 && rowp[last - 1] == ' ') last--;

        int c = 0;
        while (c < last) {
            int fg = rowfg ? rowfg[c] : 7;
            int bg = rowbg ? rowbg[c] : 0;
            int run = 1;
            while (c + run < last) {
                int fg2 = rowfg ? rowfg[c + run] : 7;
                int bg2 = rowbg ? rowbg[c + run] : 0;
                if (fg2 != fg || bg2 != bg) break;
                run++;
            }

            char tagname[32];
            ensure_color_tag(app, fg, bg, tagname);

            gtk_text_buffer_insert_with_tags_by_name(app->output_buf, &it,
                                                     rowp + c, run,
                                                     tagname[0] ? tagname : NULL,
                                                     NULL);
            c += run;
        }

        if (r != R - 1) {
            gtk_text_buffer_insert(app->output_buf, &it, "\n", 1);
        }
    }

    /* Pin to bottom while running. */
    if (app->run_state == RUN_RUNNING && vadj) {
        double upper = gtk_adjustment_get_upper(vadj);
        double page  = gtk_adjustment_get_page_size(vadj);
        double maxv  = upper - page;
        if (maxv < 0) maxv = 0;
        gtk_adjustment_set_value(vadj, maxv);
    }
}


/* Throttled screen rendering: coalesce many writes into one GTK update */
/* Render throttle: at most ~60fps via timeout */
static gboolean on_screen_render_timer(gpointer user_data) {
    App *app = (App *)user_data;
    if (!app) return G_SOURCE_REMOVE;
    app->screen_render_idle_id = 0; /* reuse field as 'pending render source id' */
    if (app->screen_dirty) {
        app->screen_dirty = false;
        screen_render_now(app);
    }
    return G_SOURCE_REMOVE;
}


static void screen_render_flush(App *app) {
    if (!app) return;
    /* If a timer-based render is pending, cancel it and render immediately. */
    if (app->screen_render_idle_id != 0) {
        g_source_remove(app->screen_render_idle_id);
        app->screen_render_idle_id = 0;
    }
    app->screen_dirty = false;
    screen_render_now(app);
    /* Ensure the GTK TextView actually redraws and applies scroll-to-bottom before we continue. */
    ui_pump_raw(app);
    ui_pump_raw(app);
}

static void screen_render(App *app) {
    if (!app) return;
    app->screen_dirty = true;
    if (app->screen_render_idle_id == 0) {
        app->screen_render_idle_id = g_timeout_add(8, on_screen_render_timer, app);
    }
}

#else /* WBASIC_NO_UI */

/* Headless build: no GTK, no render scheduling. */
static void screen_render_now(App *app) { (void)app; }
static void screen_render_flush(App *app) { (void)app; }
static void screen_render(App *app) { (void)app; }

#endif /* WBASIC_NO_UI */




// Headless terminal helpers (TTY only): use ANSI escape sequences for CLS/LOCATE/COLOR fidelity.
static inline void headless_ansi_move(int row, int col) {
    if (row < 1) row = 1;
    if (col < 1) col = 1;
    fprintf(stdout, "\x1b[%d;%dH", row, col);
}
static inline void headless_ansi_clear(void) {
    /* Clear visible screen and home cursor (no scrollback clear). */
    fputs("\x1b[2J\x1b[H", stdout);
}

static inline void headless_ansi_color_cache_reset(void) {
    headless_last_fg = -1;
    headless_last_bg = -1;
}

static inline void headless_ansi_apply_color(int fg, int bg) {
    /* GW-BASIC/CGA indices -> ANSI SGR codes */
    static const int fg_map[16] = {
        30, /* 0 black */
        34, /* 1 blue */
        32, /* 2 green */
        36, /* 3 cyan */
        31, /* 4 red */
        35, /* 5 magenta */
        33, /* 6 brown/yellow */
        37, /* 7 light gray */
        90, /* 8 dark gray */
        94, /* 9 bright blue */
        92, /* 10 bright green */
        96, /* 11 bright cyan */
        91, /* 12 bright red */
        95, /* 13 bright magenta */
        93, /* 14 bright yellow */
        97  /* 15 bright white */
    };
    static const int bg_map[16] = {
        40, 44, 42, 46, 41, 45, 43, 47,
        100, 104, 102, 106, 101, 105, 103, 107
    };

    /* 16 means "default" in WBASIC */
    if (fg == 16 && bg == 16) {
        if (headless_last_fg != 16 || headless_last_bg != 16) {
            fputs("\x1b[0m", stdout);
            headless_last_fg = 16;
            headless_last_bg = 16;
        }
        return;
    }

    if (fg == headless_last_fg && bg == headless_last_bg) return;

    /* IMPORTANT: Do NOT emit a global reset here.
       - A reset (ESC[0m) immediately before cursor moves makes it appear that LOCATE resets color.
       - We keep ESC[0m at end-of-line (before LF/CR) to prevent background "banding".
       - Here, prefer targeted attribute changes (fg/bg only).
    */
    if (fg == 16) fputs("\x1b[39m", stdout);
    else          fprintf(stdout, "\x1b[%dm", fg_map[fg & 15]);
    if (bg == 16) fputs("\x1b[49m", stdout);
    else          fprintf(stdout, "\x1b[%dm", bg_map[bg & 15]);

    headless_last_fg = fg;
    headless_last_bg = bg;
}

static void out_append(App *app, const char *s) {
    if (!app || !s) return;

    /* CLI mode (unified --cli/--headless): write directly to stdout using ANSI fidelity.
       Keep the internal screen/cursor model coherent for LOCATE/COLOR behavior. */
    if (!wbasic_ui_active(app)) {
        screen_write(app, s);
        wbasic_cli_write_text_ansi(app, s);
        if (strchr(s, '\n')) fflush(stdout);
        return;
    }

    // GTK mode: write into screen buffer, then render.
    screen_write(app, s);
    screen_render(app);

    if (strchr(s, '\n')) {
        screen_render_flush(app);
    }

    const char *p = strchr(s, '\n');
    if (p) {
        double spd = app->output_speed;
        if (spd < 0.0) spd = 0.0;
        if (spd > 1.0) spd = 1.0;
        int delay_ms = (int)lround((1.0 - spd) * 200.0);

        int nl = 0;
        for (const char *q = s; *q; q++) if (*q == '\n') nl++;

        ui_pump(app);
        if (app->quitting) return;
        if (delay_ms > 0) {
            for (int i = 0; i < nl; i++) {
                ui_delay_ms(app, delay_ms);
                if (app->quitting) return;
            }
        }
    }
}

#ifndef WBASIC_NO_UI
static gboolean on_cmd_key_press(GtkWidget *w, GdkEventKey *e, gpointer user_data) {
    (void)w;
    (void)e;
    App *app = (App*)user_data;
    if (!app) return FALSE;

    /* Ctrl+C special-casing removed.
       ESC-to-stop is handled in the main window key handler (UI-only).
       Returning FALSE lets GTK's default behavior apply. */
    return FALSE;
}
#endif /* !WBASIC_NO_UI */



static void out_printf(App *app, const char *fmt, ...) {
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    out_append(app, buf);
}


static void out_clear(App *app, bool terminal_clear) {
    if (!app) return;

    // Reset to Preferences exact colors at each CLS/OUT_CLEAR.
    app->cur_fg = 16;
    app->cur_bg = 16;

    /* CLI mode: do not touch GTK buffers/widgets. */
    if (!wbasic_ui_active(app)) {
        if (headless_stdout_is_tty()) {
            headless_stdout_prepare_ansi();
            fputs("\x1b[0m", stdout);
            if (terminal_clear) headless_ansi_clear();
            headless_ansi_color_cache_reset();
            app->headless_cursor_dirty = false;
            fflush(stdout);
        }
        screen_clear(app);
        return;
    }

    (void)terminal_clear;

    screen_clear(app);

#ifndef WBASIC_NO_UI
    scrollback_clear(app);
    if (app->output_buf) gtk_text_buffer_set_text(app->output_buf, "", -1);
    /* Reset optimized scrollback transcript + screen mark */
    app->out_scrollback_lines = 0;
    app->out_scrollback_max_lines = 1000;
    if (app->output_buf) {
        GtkTextIter it0;
        gtk_text_buffer_get_start_iter(app->output_buf, &it0);
        GtkTextMark *m = gtk_text_buffer_get_mark(app->output_buf, "out_screen_start");
        if (!m) m = gtk_text_buffer_create_mark(app->output_buf, "out_screen_start", &it0, TRUE);
        else gtk_text_buffer_move_mark(app->output_buf, m, &it0);
        app->out_screen_start_mark = m;
    }
    // Render a blank screen so LOCATE targets start from a clean slate.
    screen_render(app);
#endif
}


#ifndef WBASIC_NO_UI

static void apply_font_css(App *app)
{
    if (!app->font_name || !*app->font_name) return;

    /* app->font_name is a Pango font description string (e.g. "Monospace 12").
       GTK's CSS wants CSS properties (font-family/font-size/etc), not Pango "font:" shorthand. */
    PangoFontDescription *fd = pango_font_description_from_string(app->font_name);
    if (!fd) return;

    const char *family = pango_font_description_get_family(fd);
    int pango_size = pango_font_description_get_size(fd); /* in Pango units */
    gboolean abs_sz = pango_font_description_get_size_is_absolute(fd);

    double sz = (pango_size > 0) ? ((double)pango_size / (double)PANGO_SCALE) : 0.0;

    PangoStyle style = pango_font_description_get_style(fd);
    PangoWeight weight = pango_font_description_get_weight(fd);

    const char *css_style = "normal";
    if (style == PANGO_STYLE_ITALIC) css_style = "italic";
    else if (style == PANGO_STYLE_OBLIQUE) css_style = "oblique";

    /* Build CSS. Use pt for normal sizes, px for absolute sizes. */
    char css[768];
    if (family && *family && sz > 0.0) {
        snprintf(css, sizeof(css),
            "textview, textview text { "
            "font-family: \"%s\"; "
            "font-style: %s; "
            "font-weight: %d; "
            "font-size: %.2f%s; "
            "}",
            family,
            css_style,
            (int)weight,
            sz,
            abs_sz ? "px" : "pt"
        );
    } else if (family && *family) {
        snprintf(css, sizeof(css),
            "textview, textview text { font-family: \"%s\"; font-style: %s; font-weight: %d; }",
            family, css_style, (int)weight
        );
    } else {
        /* Fallback: do nothing if we can't parse */
        pango_font_description_free(fd);
        return;
    }

    pango_font_description_free(fd);

    gtk_css_provider_load_from_data(app->font_css, css, -1, NULL);

    GtkStyleContext *ctx;

    if (app->editor_view) {
        ctx = gtk_widget_get_style_context(app->editor_view);
        gtk_style_context_add_provider(
            ctx,
            GTK_STYLE_PROVIDER(app->font_css),
            GTK_STYLE_PROVIDER_PRIORITY_USER
        );
    }

    if (app->output_view) {
        ctx = gtk_widget_get_style_context(app->output_view);
        gtk_style_context_add_provider(
            ctx,
            GTK_STYLE_PROVIDER(app->font_css),
            GTK_STYLE_PROVIDER_PRIORITY_USER
        );
    }
}

// Map arbitrary RGB (from Preferences) to nearest GW-BASIC 16-color index (text mode).
// Palette is EGA/CGA-like; this is only used to choose a default COLOR at RUN start / CLS.
// Users can still override with COLOR statements inside the program.
#endif /* !WBASIC_NO_UI */

#ifdef WBASIC_NO_UI
static WB_UNUSED void apply_font_css(App *app) { (void)app; }
#endif

#ifndef WBASIC_NO_UI

static WB_UNUSED void apply_theme(App *app) {
    if (!app) return;

    if (!app->css_provider) {
        app->css_provider = gtk_css_provider_new();
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(app->css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }

    gchar *fg = app->have_fg ? gdk_rgba_to_string(&app->fg_color) : NULL;
    gchar *bg = app->have_bg ? gdk_rgba_to_string(&app->bg_color) : NULL;

    char *fam_dup = NULL;
    int font_px = 0;
    if (app->font_name && *app->font_name) {
        PangoFontDescription *fd = pango_font_description_from_string(app->font_name);
        const char *fam0 = pango_font_description_get_family(fd);
        int sz = pango_font_description_get_size(fd); // Pango units (points * PANGO_SCALE)
        double pts = 0.0;

        // Some font chooser strings can omit size; in that case GTK returns 0.
        // Use a sensible default so the first change always applies.
        if (sz > 0) pts = (double)sz / (double)PANGO_SCALE;
        else pts = 12.0;

        // Convert points to px (GTK CSS behaves most consistently with px)
        font_px = (int)floor(pts * (96.0 / 72.0) + 0.5);
        if (font_px < 6) font_px = 6;

        if (fam0 && *fam0) fam_dup = g_strdup(fam0);
        pango_font_description_free(fd);
    }

    GString *css = g_string_new(NULL);

    // Editor
    g_string_append(css, "textview#wbasic_editor, textview#wbasic_editor text {");
    if (fg) g_string_append_printf(css, " color: %s;", fg);
    if (bg) g_string_append_printf(css, " background-color: %s;", bg);
    if (fg) g_string_append_printf(css, " caret-color: %s;", fg);
    else g_string_append(css, " caret-color: currentColor;");
    if (fam_dup && *fam_dup) {
        g_string_append_printf(css, " font-family: \"%s\";", fam_dup);
        if (font_px > 0) g_string_append_printf(css, " font-size: %dpx;", font_px);
    }
    g_string_append(css, " }\n");

    // Output
    g_string_append(css, "textview#wbasic_output, textview#wbasic_output text {");
    if (fg) g_string_append_printf(css, " color: %s;", fg);
    if (bg) g_string_append_printf(css, " background-color: %s;", bg);
    if (fam_dup && *fam_dup) {
        g_string_append_printf(css, " font-family: \"%s\";", fam_dup);
        if (font_px > 0) g_string_append_printf(css, " font-size: %dpx;", font_px);
    }
    g_string_append(css, " }\n");

    gtk_css_provider_load_from_data(app->css_provider, css->str, -1, NULL);

    // In GTK3, theme styles or widget properties can still prevent CSS font updates
    // from taking effect immediately. Force the chosen font onto the text views.
    if (app->font_name && *app->font_name) {
        PangoFontDescription *fd2 = pango_font_description_from_string(app->font_name);
        pango_font_description_free(fd2);
    }

    g_string_free(css, TRUE);
    if (fg) g_free(fg);
    if (bg) g_free(bg);
    if (fam_dup) g_free(fam_dup);
    apply_font_css(app);
}



static char *prefs_get_path(void) {
    const char *cfgdir = g_get_user_config_dir(); // usually ~/.config
    char *dir = g_build_filename(cfgdir, "wbasic", NULL);
    g_mkdir_with_parents(dir, 0700);
    char *path = g_build_filename(dir, "prefs.ini", NULL);
    g_free(dir);
    return path; // must g_free
}

#endif /* !WBASIC_NO_UI */

#ifdef WBASIC_NO_UI
static WB_UNUSED void apply_theme(App *app) { (void)app; }
#endif


static WB_UNUSED void prefs_save(App *app) {
#ifndef WBASIC_NO_UI
    if (!app) return;
    GKeyFile *kf = g_key_file_new();

    g_key_file_set_boolean(kf, "colors", "have_fg", app->have_fg);
    g_key_file_set_boolean(kf, "colors", "have_bg", app->have_bg);

    // UI settings
    g_key_file_set_boolean(kf, "ui", "have_win_size", app->have_win_size);
    if (app->have_win_size) {
        g_key_file_set_integer(kf, "ui", "win_w", app->win_w);
        g_key_file_set_integer(kf, "ui", "win_h", app->win_h);
    }
    g_key_file_set_boolean(kf, "ui", "have_win_pos", app->have_win_pos);
    if (app->have_win_pos) {
        g_key_file_set_integer(kf, "ui", "win_x", app->win_x);
        g_key_file_set_integer(kf, "ui", "win_y", app->win_y);
    }
    g_key_file_set_boolean(kf, "ui", "have_paned_pos", app->have_paned_pos);
    if (app->have_paned_pos) {
        g_key_file_set_integer(kf, "ui", "paned_pos", app->paned_pos);
    }
    if (app->font_name && *app->font_name) {
        g_key_file_set_string(kf, "ui", "font", app->font_name);
    }

    // Output speed (0.0..1.0)
    {
        double spd = app->output_speed;
        if (spd < 0.0) spd = 0.0;
        if (spd > 1.0) spd = 1.0;
        g_key_file_set_double(kf, "ui", "output_speed", spd);
    }

    // Export: include speed control in exported programs
    g_key_file_set_boolean(kf, "ui", "export_include_speed", app->export_include_speed ? TRUE : FALSE);

    // UI: splash screen
    g_key_file_set_boolean(kf, "ui", "show_splash", app->show_splash ? TRUE : FALSE);


    if (app->have_fg) {
        gchar *fg = gdk_rgba_to_string(&app->fg_color);
        g_key_file_set_string(kf, "colors", "fg", fg);
        g_free(fg);
    }
    if (app->have_bg) {
        gchar *bg = gdk_rgba_to_string(&app->bg_color);
        g_key_file_set_string(kf, "colors", "bg", bg);
        g_free(bg);
    }

    gsize len = 0;
    GError *err = NULL;
    gchar *data = g_key_file_to_data(kf, &len, &err);
    if (!err && data) {
        char *p = prefs_get_path();
        // Best-effort save
        g_file_set_contents(p, data, (gssize)len, NULL);
        g_free(p);
    }
    if (err) g_error_free(err);
    if (data) g_free(data);
    g_key_file_free(kf);

#else
    (void)app;
#endif
}

static WB_UNUSED void prefs_load(App *app) {
#ifndef WBASIC_NO_UI
    if (!app) return;

    char *p = prefs_get_path();
    gchar *contents = NULL;
    gsize len = 0;
    if (!g_file_get_contents(p, &contents, &len, NULL)) {
        g_free(p);
        return;
    }
    g_free(p);

    GKeyFile *kf = g_key_file_new();
    GError *err = NULL;
    if (!g_key_file_load_from_data(kf, contents, len, G_KEY_FILE_NONE, &err)) {
        if (err) g_error_free(err);
        g_key_file_free(kf);
        g_free(contents);
        return;
    }
    g_free(contents);

    app->have_fg = g_key_file_get_boolean(kf, "colors", "have_fg", NULL);
    app->have_bg = g_key_file_get_boolean(kf, "colors", "have_bg", NULL);

    // UI settings
    app->have_win_size = g_key_file_get_boolean(kf, "ui", "have_win_size", NULL);
    if (app->have_win_size) {
        app->win_w = g_key_file_get_integer(kf, "ui", "win_w", NULL);
        app->win_h = g_key_file_get_integer(kf, "ui", "win_h", NULL);
    }
    app->have_win_pos = g_key_file_get_boolean(kf, "ui", "have_win_pos", NULL);
    if (app->have_win_pos) {
        app->win_x = g_key_file_get_integer(kf, "ui", "win_x", NULL);
        app->win_y = g_key_file_get_integer(kf, "ui", "win_y", NULL);
    }
    app->have_paned_pos = g_key_file_get_boolean(kf, "ui", "have_paned_pos", NULL);
    if (app->have_paned_pos) {
        app->paned_pos = g_key_file_get_integer(kf, "ui", "paned_pos", NULL);
    }
    gchar *font = g_key_file_get_string(kf, "ui", "font", NULL);
    if (font) {
        free(app->font_name);
        app->font_name = xstrdup(font);
        g_free(font);
    }

    // Output speed (default to Fast if not present)
    {
        GError *e2 = NULL;
        double spd = g_key_file_get_double(kf, "ui", "output_speed", &e2);
        if (e2) {
            g_error_free(e2);
            // key missing or invalid -> keep current default
        } else {
            if (spd < 0.0) spd = 0.0;
            if (spd > 1.0) spd = 1.0;
            app->output_speed = spd;
        // Cache interpreter pacing (tuned curve)
        double t = 1.0 - app->output_speed;
        double curve = pow(t, 2.32);
        app->exec_delay_us_per_stmt = (int)(curve * 500.0);
        app->exec_sleep_chunk_us = 1000;
    
        }

    // Export: include speed control in exported programs (default false)
    {
        GError *e3 = NULL;
        gboolean b = g_key_file_get_boolean(kf, "ui", "export_include_speed", &e3);
        if (e3) {
            g_error_free(e3);
            app->export_include_speed = false;
        } else {
            app->export_include_speed = b ? true : false;
        }
    }

    }


    // UI: splash screen (default true)
    {
        GError *e4 = NULL;
        gboolean b2 = g_key_file_get_boolean(kf, "ui", "show_splash", &e4);
        if (e4) {
            g_error_free(e4);
            app->show_splash = true;
        } else {
            app->show_splash = b2 ? true : false;
        }
    }
    if (app->have_fg) {
        gchar *fg = g_key_file_get_string(kf, "colors", "fg", NULL);
        if (fg) {
            if (!gdk_rgba_parse(&app->fg_color, fg)) app->have_fg = false;
            g_free(fg);
        } else app->have_fg = false;
    }
    if (app->have_bg) {
        gchar *bg = g_key_file_get_string(kf, "colors", "bg", NULL);
        if (bg) {
            if (!gdk_rgba_parse(&app->bg_color, bg)) app->have_bg = false;
            g_free(bg);
        } else app->have_bg = false;
    }

    g_key_file_free(kf);

#else
    (void)app;
#endif
}

/* ===================== Small helpers ===================== */

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *e = s + strlen(s) - 1;
    while (e > s && isspace((unsigned char)*e)) *e-- = 0;
    return s;
}

static bool is_word_boundary(char c) {
    // Boundary after a keyword: end-of-string or any non-identifier char.
    // Identifier chars in WBASIC are [A-Za-z0-9_$].
    if (c == 0) return true;
    unsigned char uc = (unsigned char)c;
    return !(isalnum(uc) || c == '_' || c == '$');
}

static bool starts_ci(const char *s, const char *p) {
    while (*p && *s) {
        if (toupper((unsigned char)*s) != toupper((unsigned char)*p)) return false;
        s++; p++;
    }
    return *p == 0;
}


/* Find single-line ELSE keyword in an IF ... THEN <stmt> [ELSE <stmt>] tail.
   Returns pointer to the 'E' in ELSE within the same buffer, or NULL.
   Skips quoted string literals ("...") and respects doubled quotes (""). */
static char *find_else_kw(char *s)
{
    int in_str = 0;
    for (char *p = s; *p; p++) {
        if (*p == '"') {
            if (in_str) {
                if (p[1] == '"') { p++; continue; } // doubled quote inside string
                in_str = 0;
            } else {
                in_str = 1;
            }
            continue;
        }
        if (in_str) continue;

        // check ELSE with word boundaries
        if ((toupper((unsigned char)p[0]) == 'E') &&
            (toupper((unsigned char)p[1]) == 'L') &&
            (toupper((unsigned char)p[2]) == 'S') &&
            (toupper((unsigned char)p[3]) == 'E') &&
            is_word_boundary(p[4]) &&
            (p == s || is_word_boundary(p[-1]))) {
            return p;
        }
    }
    return NULL;
}


/* ===================== Program storage ===================== */

static void program_init(Program *p) {
    p->lines = NULL;
    p->count = 0;
    p->cap = 0;
    p->data = NULL;
    p->data_count = 0;
    p->data_cap = 0;
    p->data_ptr = 0;
    p->ifmap = NULL;
    p->ifmap_count = 0;
    p->ifmap_cap = 0;
    p->fndefs = NULL;
    p->fndef_count = 0;
    p->fndef_cap = 0;
    p->line_index_map = NULL;
    p->line_index_min = 0;
    p->line_index_max = 0;
}

static void program_free(Program *p) {
    if (p && p->fndefs) {
        for (size_t i = 0; i < p->fndef_count; i++) {
            FnDef *fd = &p->fndefs[i];
            free(fd->name);
            if (fd->params) {
                for (int j = 0; j < fd->param_count; j++) free(fd->params[j]);
                free(fd->params);
            }
            free(fd->expr_src);
        }
        free(p->fndefs);
        p->fndefs = NULL;
    }
    if (p) { p->fndef_count = 0; p->fndef_cap = 0; }

    if (p && p->ifmap) { free(p->ifmap); p->ifmap = NULL; }
    if (p) { p->ifmap_count = 0; p->ifmap_cap = 0; }
    if (p && p->line_index_map) { free(p->line_index_map); p->line_index_map = NULL; }
    for (size_t i = 0; i < p->count; i++) free(p->lines[i].text);
    free(p->lines);
    p->lines = NULL;
    p->count = 0;
    p->cap = 0;
}
/* ===================== DEF FN helpers ===================== */
static FnDef *program_fndef_find(Program *p, const char *name_upper) {
    if (!p || !name_upper) return NULL;
    for (size_t i = 0; i < p->fndef_count; i++) {
        if (!strcasecmp(p->fndefs[i].name, name_upper)) return &p->fndefs[i];
    }
    return NULL;
}

static bool program_fndef_set(Program *p, const char *name_upper, char **params, int param_count, const char *expr_src, int ret_kind) {
    if (!p || !name_upper || !expr_src) return false;

    // replace if exists
    FnDef *existing = program_fndef_find(p, name_upper);
    FnDef *fd = existing;
    if (!fd) {
        if (p->fndef_count + 1 > p->fndef_cap) {
            size_t nc = p->fndef_cap ? p->fndef_cap * 2 : 8;
            FnDef *nn = (FnDef*)realloc(p->fndefs, nc * sizeof(FnDef));
            if (!nn) return false;
            memset(nn + p->fndef_cap, 0, (nc - p->fndef_cap) * sizeof(FnDef));
            p->fndefs = nn;
            p->fndef_cap = nc;
        }
        fd = &p->fndefs[p->fndef_count++];
        memset(fd, 0, sizeof(*fd));
    } else {
        free(fd->name);
        if (fd->params) {
            for (int j = 0; j < fd->param_count; j++) free(fd->params[j]);
            free(fd->params);
        }
        free(fd->expr_src);
        memset(fd, 0, sizeof(*fd));
    }

    fd->name = xstrdup(name_upper);
    fd->param_count = param_count;
    fd->expr_src = xstrdup(expr_src);
    fd->ret_kind = ret_kind;
    if (param_count > 0) {
        fd->params = (char**)calloc((size_t)param_count, sizeof(char*));
        if (!fd->params) return false;
        for (int i = 0; i < param_count; i++) fd->params[i] = params[i];
    }
    return true;
}





static int program_find_index(const Program *p, int line_no) {
    int lo = 0, hi = (int)p->count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int v = p->lines[mid].line_no;
        if (v == line_no) return mid;
        if (v < line_no) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

static int program_find_insert_pos(const Program *p, int line_no) {
    int lo = 0, hi = (int)p->count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (p->lines[mid].line_no < line_no) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

static void program_ensure_cap(Program *p, size_t want) {
    if (p->cap >= want) return;
    size_t ncap = p->cap ? p->cap * 2 : 64;
    while (ncap < want) ncap *= 2;
    p->lines = (Line*)realloc(p->lines, ncap * sizeof(Line));
    p->cap = ncap;
}

static void program_delete_line(Program *p, int line_no) {
    int idx = program_find_index(p, line_no);
    if (idx < 0) return;
    free(p->lines[idx].text);
    memmove(&p->lines[idx], &p->lines[idx+1], (p->count - (size_t)idx - 1) * sizeof(Line));
    p->count--;
}

static void program_set_line(Program *p, int line_no, const char *text) {
    if (!text) text = "";
    char *tmp = xstrdup(text);
    char *t = trim(tmp);

    /* GW-BASIC compatibility:
       Preserve separator-only lines like ":" as real program lines so GOTO/GOSUB targets exist.
       (Empty text still deletes the line, matching immediate-mode edit behavior.) */
    if (*t == 0) { free(tmp); program_delete_line(p, line_no); return; }

    int idx = program_find_index(p, line_no);
    if (idx >= 0) {
        free(p->lines[idx].text);
        p->lines[idx].text = xstrdup(t);
        free(tmp);
        return;
    }

    int pos = program_find_insert_pos(p, line_no);
    program_ensure_cap(p, p->count + 1);
    memmove(&p->lines[pos+1], &p->lines[pos], (p->count - (size_t)pos) * sizeof(Line));
    p->lines[pos].line_no = line_no;
    p->lines[pos].text = xstrdup(t);
    p->count++;
    free(tmp);
}

/* ===================== Editor <-> Program sync ===================== */

#ifndef WBASIC_NO_UI
static char *editor_get_text(GtkTextBuffer *buf) {
    if (!buf) return g_strdup("");
    GtkTextIter a, b;
    gtk_text_buffer_get_start_iter(buf, &a);
    gtk_text_buffer_get_end_iter(buf, &b);
    return gtk_text_buffer_get_text(buf, &a, &b, FALSE);
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void editor_set_text(GtkTextBuffer *buf, const char *text) {
    if (!buf) return;
    gtk_text_buffer_set_text(buf, text ? text : "", -1);
}
#else
static void editor_set_text(void *buf, const char *text) {
    (void)buf;
    (void)text;
}
#endif /* !WBASIC_NO_UI */

static void program_to_editor(App *app) {
    if (!app) return;
    if (!wbasic_has_ui_buffers(app)) return; /* no GTK editor in CLI mode */
    GString *gs = g_string_new(NULL);
    for (size_t i = 0; i < app->prog.count; i++) {
        g_string_append_printf(gs, "%d %s\n", app->prog.lines[i].line_no, app->prog.lines[i].text);
    }
    editor_set_text(app->editor_buf, gs->str);
    g_string_free(gs, TRUE);
}



static bool editor_to_program(App *app) {
    if (!app) return true;
    program_free(&app->prog);
    program_init(&app->prog);

    /* In unified builds, CLI mode doesn't have GTK buffers. We treat the loaded
       file text as "embedded_text" and parse from that. In GTK mode we parse
       from the editor buffer. */
    char *all = NULL;
    const char *src = NULL;

    if (wbasic_has_ui_buffers(app)) {
#ifndef WBASIC_NO_UI
        all = editor_get_text(app->editor_buf);
        src = all ? all : "";
#else
        /* Should not happen in WBASIC_NO_UI builds, but keep safe. */
        all = g_strdup("");
        src = all;
#endif
    } else {
        src = (app->embedded_text) ? app->embedded_text : "";
        all = g_strdup(src);
        src = all ? all : "";
    }

    char *saveptr = NULL;
    for (char *line = strtok_r(all, "\n", &saveptr); line; line = strtok_r(NULL, "\n", &saveptr)) {
        char *t = trim(line);
        if (*t == 0) continue;

        char *endp = NULL;
        long ln = strtol(t, &endp, 10);
        if (endp != t) {
            int line_no = (int)ln;
            char *stmt = trim(endp);
            program_set_line(&app->prog, line_no, stmt);
        } else {
            char msg[256];
            snprintf(msg, sizeof(msg), "ERROR: Program lines must start with a line number: %.200s\n", t);
            if (wbasic_ui_active(app)) out_append(app, msg);
            else fputs(msg, stderr);
            g_free(all);
            return false;
        }
    }

    g_free(all);
    return true;
}



/* ===================== Variable table ===================== */

static void var_free(Var *v) {
    if (!v) return;
    free(v->str);

    // Free array storage (numeric or string)
    if (v->arr) free(v->arr);
    if (v->sarr) {
        for (size_t i = 0; i < v->arr_total; i++) free(v->sarr[i]);
        free(v->sarr);
    }

    free(v);
}


/* ---- GW-BASIC DEFxxx default typing ---- */
typedef enum {
    DT_SNG = 0, /* single precision (WBASIC stores as double internally) */
    DT_INT = 1, /* 16-bit integer */
    DT_DBL = 2, /* double precision */
    DT_STR = 3  /* string */
} DefType;

static DefType app_def_type_for_name(App *app, const char *name_upper) {
    if (!name_upper || !*name_upper) return DT_SNG;
    size_t n = strlen(name_upper);
    char last = name_upper[n - 1];
    /* Explicit suffix always wins */
    if (last == '$') return DT_STR;
    if (last == '%') return DT_INT;
    if (last == '#') return DT_DBL;
    if (last == '!') return DT_SNG;

    char c0 = name_upper[0];
    if (c0 >= 'A' && c0 <= 'Z') return (DefType)app->def_type[c0 - 'A'];
    return DT_SNG;
}

static bool ident_is_string_var(App *app, const char *name_upper) {
    return app_def_type_for_name(app, name_upper) == DT_STR;
}

/* deleted unused static function: ident_is_int_var */

static bool name_is_string(const char *name_upper);

static Var *vars_lookup(App *app, const char *name_upper) {
    return (Var*)g_hash_table_lookup(app->vars, name_upper);
}

static Var *vars_get_or_create(App *app, const char *name_upper) {
    Var *v = (Var*)g_hash_table_lookup(app->vars, name_upper);
    if (v) return v;

    // create new entry
    char *key = xstrdup(name_upper);
    v = (Var*)calloc(1, sizeof(Var));
    /* Determine default kind from suffix or DEFxxx map. */
    if (ident_is_string_var(app, name_upper)) {
        v->kind = V_STR;
        v->is_array = false;
        v->num = 0.0;
        v->num_is_int = false;
        v->num_type = (unsigned char)DT_STR;
        v->str = xstrdup("");
    } else {
        v->kind = V_NUM;
        v->is_array = false;
        v->num = 0.0;
        DefType dt = app_def_type_for_name(app, name_upper);
        /* For numeric vars, dt is one of DT_SNG/DT_INT/DT_DBL. */
        if (dt == DT_INT) {
            v->num_type = (unsigned char)DT_INT;
            v->num_is_int = true;
        } else if (dt == DT_DBL) {
            v->num_type = (unsigned char)DT_DBL;
            v->num_is_int = false;
        } else { /* DT_SNG default */
            v->num_type = (unsigned char)DT_SNG;
            v->num_is_int = false;
        }
        v->str = xstrdup("");
    }
    v->arr = NULL;
    v->sarr = NULL;
    v->arr_dims = 0;
    memset(v->arr_dim_lo, 0, sizeof(v->arr_dim_lo));
    memset(v->arr_dim_max, 0, sizeof(v->arr_dim_max));
    v->arr_total = 0;
    memset(v->arr_stride, 0, sizeof(v->arr_stride));

    g_hash_table_insert(app->vars, key, v);
    return v;
}

static bool name_is_string(const char *name_upper) {
    size_t n = strlen(name_upper);
    return n > 0 && name_upper[n-1] == '$';
}



static inline double coerce_int16(double v) {
    long long iv = llround(v);
    if (iv < -32768) iv = -32768;
    if (iv >  32767) iv =  32767;
    return (double)iv;
}

static inline double coerce_numeric_store(App *app, Var *var, const char *name_upper, double v) {
    (void)app; (void)name_upper;
    if (var) {
        /* Integer variables store as 16-bit signed (GW-BASIC semantics). */
        if (var->num_is_int || var->num_type == (unsigned char)DT_INT) return coerce_int16(v);
        /* Single-precision variables round to float on store. */
        if (var->num_type == (unsigned char)DT_SNG) return (double)(float)v;
        /* Double keeps full precision. */
    }
    return v;
}


static void vars_reset(App *app) {
    g_hash_table_remove_all(app->vars); // destroys keys and values via destroy funcs
}

/* ===================== Parser helpers ===================== */

typedef struct { const char *s; } Parser;

static void skip_ws(Parser *p) { while (isspace((unsigned char)*p->s)) p->s++; }

/* Peek next non-whitespace character without consuming. */
static char peek(Parser *p) { skip_ws(p); return *p->s; }

static bool consume(Parser *p, char c) {
    skip_ws(p);
    if (*p->s == c) { p->s++; return true; }
    return false;
}


static bool consume_word_ci(Parser *p, const char *kw) {
    skip_ws(p);
    size_t n = strlen(kw);
    const char *s = p->s;
    if (strncasecmp(s, kw, n) != 0) return false;
    char next = s[n];
    if (isalnum((unsigned char)next) || next == '_' || next == '$') return false; // word boundary
    p->s += (int)n;
    return true;
}

static bool parse_number(Parser *p, double *out) {
    skip_ws(p);
    char *endp = NULL;
    double v = strtod(p->s, &endp);
    if (endp == p->s) return false;
    p->s = endp;
    *out = v;
    return true;
}

static bool parse_identifier(Parser *p, char **out_upper) {
    skip_ws(p);
    const char *s = p->s;
    if (!(isalpha((unsigned char)*s) || *s == '_')) return false;
    s++;
    while (isalnum((unsigned char)*s) || *s == '_') s++;
    bool is_str = false;
    // GW-BASIC style type suffixes can be attached with no whitespace: $, %, !, #
    if (*s == '$' || *s == '%' || *s == '!' || *s == '#') {
        is_str = (*s == '$');
        s++;
    }

    size_t len = (size_t)(s - p->s);
    char *raw = (char*)malloc(len + 1);
    memcpy(raw, p->s, len);
    raw[len] = 0;

    // uppercase
    for (size_t i = 0; i < len; i++) raw[i] = (char)toupper((unsigned char)raw[i]);
    if (is_str) raw[len-1] = '$'; // already upper
    p->s = s;
    *out_upper = raw;
    return true;
}

static bool parse_string_literal(Parser *p, char **out) {
    skip_ws(p);
    if (*p->s != '"') return false;
    p->s++;
    GString *gs = g_string_new(NULL);
    while (*p->s) {
        // GW-BASIC style embedded quote: "" inside a quoted string means a literal "
        if (*p->s == '"' && p->s[1] == '"') {
            g_string_append_c(gs, '"');
            p->s += 2;
            continue;
        }
        if (*p->s == '"') break;
        if (*p->s == '\\' && p->s[1]) {
            char n = p->s[1];
            if (n == 'n') { g_string_append_c(gs, '\n'); p->s += 2; continue; }
            if (n == '"') { g_string_append_c(gs, '"'); p->s += 2; continue; }
        }
        g_string_append_c(gs, *p->s++);
    }
    if (*p->s != '"') { g_string_free(gs, TRUE); return false; }
    p->s++;
    *out = g_string_free(gs, FALSE);
    return true;
}

/* ===================== Array helpers (numeric, up to 5D) ===================== */

static void var_free_array_storage(Var *v) {
    if (!v) return;
    if (v->arr) { free(v->arr); v->arr = NULL; }
    if (v->sarr) {
        for (size_t i = 0; i < v->arr_total; i++) free(v->sarr[i]);
        free(v->sarr);
        v->sarr = NULL;
    }
}

static void var_erase_array(Var *v) {
    if (!v) return;
    var_free_array_storage(v);
    v->is_array = false;
    v->arr_dims = 0;
    v->arr_total = 0;
    memset(v->arr_dim_lo, 0, sizeof(v->arr_dim_lo));
    memset(v->arr_dim_max, 0, sizeof(v->arr_dim_max));
    memset(v->arr_stride, 0, sizeof(v->arr_stride));
}

static bool var_define_num_array(Var *v, int ndims, int base, const int dim_max[5]) {
    if (!v || ndims < 1 || ndims > 5) return false;
    if (!(base == 0 || base == 1)) base = 0;

    int lo[5] = {0,0,0,0,0};
    int hi[5] = {0,0,0,0,0};
    size_t extent[5] = {0,0,0,0,0};

    for (int i = 0; i < ndims; i++) {
        lo[i] = base;
        hi[i] = dim_max[i];
        if (hi[i] < lo[i]) return false; // invalid range
        extent[i] = (size_t)(hi[i] - lo[i] + 1);
    }

    // Compute strides (row-major; last dim varies fastest)
    size_t stride[5] = {0,0,0,0,0};
    size_t total = 1;
    for (int i = ndims - 1; i >= 0; i--) {
        stride[i] = total;
        total *= extent[i];
    }

    // Drop any previous array storage (numeric or string)
    var_free_array_storage(v);

    v->arr = (double*)calloc(total, sizeof(double));
    if (!v->arr) return false;

    v->kind = V_NUM;
    v->is_array = true;
    v->arr_dims = ndims;
    v->arr_total = total;
    memset(v->arr_dim_lo, 0, sizeof(v->arr_dim_lo));
    memset(v->arr_dim_max, 0, sizeof(v->arr_dim_max));
    memset(v->arr_stride, 0, sizeof(v->arr_stride));
    for (int i = 0; i < ndims; i++) {
        v->arr_dim_lo[i] = lo[i];
        v->arr_dim_max[i] = hi[i];
        v->arr_stride[i] = stride[i];
    }
    return true;
}

static bool var_define_str_array(Var *v, int ndims, int base, const int dim_max[5]) {
    if (!v || ndims < 1 || ndims > 5) return false;
    if (!(base == 0 || base == 1)) base = 0;

    int lo[5] = {0,0,0,0,0};
    int hi[5] = {0,0,0,0,0};
    size_t extent[5] = {0,0,0,0,0};

    for (int i = 0; i < ndims; i++) {
        lo[i] = base;
        hi[i] = dim_max[i];
        if (hi[i] < lo[i]) return false; // invalid range
        extent[i] = (size_t)(hi[i] - lo[i] + 1);
    }

    // Compute strides (row-major; last dim varies fastest)
    size_t stride[5] = {0,0,0,0,0};
    size_t total = 1;
    for (int i = ndims - 1; i >= 0; i--) {
        stride[i] = total;
        total *= extent[i];
    }

    // Drop any previous array storage (numeric or string)
    var_free_array_storage(v);

    v->sarr = (char**)calloc(total, sizeof(char*)); // NULL entries mean ""
    if (!v->sarr) return false;

    v->kind = V_STR;
    v->is_array = true;
    v->arr_dims = ndims;
    v->arr_total = total;
    memset(v->arr_dim_lo, 0, sizeof(v->arr_dim_lo));
    memset(v->arr_dim_max, 0, sizeof(v->arr_dim_max));
    memset(v->arr_stride, 0, sizeof(v->arr_stride));
    for (int i = 0; i < ndims; i++) {
        v->arr_dim_lo[i] = lo[i];
        v->arr_dim_max[i] = hi[i];
        v->arr_stride[i] = stride[i];
    }
    return true;
}

// REDIM helper: re-dimension an existing array. If preserve==false, this is equivalent to DIM (realloc, zero/NULL init).
// If preserve==true, we preserve existing values. For safety/compatibility we implement GW/QBasic-style rule:
// PRESERVE may only change the LAST dimension size; number of dims and other dimension ranges must remain unchanged.
static bool var_redim_array(Var *v, bool is_string, int ndims, int base, const int dim_max[5], bool preserve) {
    if (!v || ndims < 1 || ndims > 5) return false;
    if (!(base == 0 || base == 1)) base = 0;

    // If not preserving, just define fresh.
    if (!preserve) {
        return is_string ? var_define_str_array(v, ndims, base, dim_max)
                         : var_define_num_array(v, ndims, base, dim_max);
    }

    // PRESERVE path: if variable isn't already an array, treat like fresh DIM.
    if (!v->is_array) {
        return is_string ? var_define_str_array(v, ndims, base, dim_max)
                         : var_define_num_array(v, ndims, base, dim_max);
    }

    // Type must match
    if (is_string) {
        if (!(v->kind == V_STR && v->sarr)) return false;
    } else {
        if (!(v->kind == V_NUM && v->arr)) return false;
    }

    // Dims must match
    if (v->arr_dims != ndims) return false;

    // Old extents
    size_t old_extent[5] = {0,0,0,0,0};
    for (int i = 0; i < ndims; i++) {
        int lo = v->arr_dim_lo[i];
        int hi = v->arr_dim_max[i];
        if (hi < lo) return false;
        old_extent[i] = (size_t)(hi - lo + 1);
    }

    // New lo/hi/extents (must keep same lo for all dims under OPTION BASE semantics)
    int new_lo[5] = {0,0,0,0,0};
    int new_hi[5] = {0,0,0,0,0};
    size_t new_extent[5] = {0,0,0,0,0};
    for (int i = 0; i < ndims; i++) {
        new_lo[i] = base;
        new_hi[i] = dim_max[i];
        if (new_hi[i] < new_lo[i]) return false;
        new_extent[i] = (size_t)(new_hi[i] - new_lo[i] + 1);
    }

    // Enforce PRESERVE rule: all dims except last must match exactly (lo and hi).
    for (int i = 0; i < ndims - 1; i++) {
        if (v->arr_dim_lo[i] != new_lo[i]) return false;
        if (v->arr_dim_max[i] != new_hi[i]) return false;
    }
    // Last dim must keep same lo, hi can change.
    if (v->arr_dim_lo[ndims-1] != new_lo[ndims-1]) return false;

    // Compute new strides/total
    size_t new_stride[5] = {0,0,0,0,0};
    size_t new_total = 1;
    for (int i = ndims - 1; i >= 0; i--) {
        new_stride[i] = new_total;
        new_total *= new_extent[i];
    }

    // Copy common region: because only last extent can differ, we can copy per "row block"
    size_t old_last = old_extent[ndims-1];
    size_t new_last = new_extent[ndims-1];
    size_t common_last = old_last < new_last ? old_last : new_last;

    size_t blocks = 1;
    for (int i = 0; i < ndims - 1; i++) blocks *= old_extent[i];

    if (!is_string) {
        double *old_arr = v->arr;
        double *new_arr = (double*)calloc(new_total, sizeof(double));
        if (!new_arr) return false;

        for (size_t b = 0; b < blocks; b++) {
            size_t old_off = b * old_last;
            size_t new_off = b * new_last;
            memcpy(&new_arr[new_off], &old_arr[old_off], common_last * sizeof(double));
        }

        free(old_arr);
        v->arr = new_arr;
        v->sarr = NULL;
    } else {
        char **old_sarr = v->sarr;
        char **new_sarr = (char**)calloc(new_total, sizeof(char*));
        if (!new_sarr) return false;

        // Move pointers for common region; NULL out moved entries so we can safely free the old array.
        for (size_t b = 0; b < blocks; b++) {
            size_t old_off = b * old_last;
            size_t new_off = b * new_last;
            for (size_t i = 0; i < common_last; i++) {
                new_sarr[new_off + i] = old_sarr[old_off + i];
                old_sarr[old_off + i] = NULL;
            }
        }

        // Free any remaining old strings (those not moved)
        for (size_t i = 0; i < v->arr_total; i++) {
            free(old_sarr[i]);
        }
        free(old_sarr);

        v->sarr = new_sarr;
        v->arr = NULL;
    }

    // Update metadata
    v->kind = is_string ? V_STR : V_NUM;
    v->is_array = true;
    v->arr_dims = ndims;
    v->arr_total = new_total;
    memset(v->arr_dim_lo, 0, sizeof(v->arr_dim_lo));
    memset(v->arr_dim_max, 0, sizeof(v->arr_dim_max));
    memset(v->arr_stride, 0, sizeof(v->arr_stride));
    for (int i = 0; i < ndims; i++) {
        v->arr_dim_lo[i] = new_lo[i];
        v->arr_dim_max[i] = new_hi[i];
        v->arr_stride[i] = new_stride[i];
    }
    return true;
}


static void runtime_error(App *app, int line_no, const char *msg);
static bool parse_expr(App *app, Parser *p, double *out);

static double func_eof(App *app, int n);
static bool parse_array_indices(App *app, Parser *p, int *out_ndims, int out_idx[5]) {
    // Called after the leading '(' has already been consumed.
    // Parses: <expr> [, <expr> ...] ')'  (up to 5 exprs). Leaves p positioned after ')'.
    int nd = 0;
    while (1) {
        if (nd >= 5) return false;
        double d = 0.0;
        if (!parse_expr(app, p, &d)) return false;
        out_idx[nd++] = (int)llround(d);
        skip_ws(p);
        if (consume(p, ',')) continue;
        if (!consume(p, ')')) return false;
        break;
    }
    *out_ndims = nd;
    return true;
}

static bool array_calc_offset(const Var *v, int ndims, const int idx[5], size_t *out_off) {
    if (!v || !v->is_array || v->arr_dims < 1) return false;
    if (ndims != v->arr_dims) return false;
    size_t off = 0;
    for (int i = 0; i < ndims; i++) {
                if (idx[i] < v->arr_dim_lo[i] || idx[i] > v->arr_dim_max[i]) return false;
        off += (size_t)(idx[i] - v->arr_dim_lo[i]) * v->arr_stride[i];
    }
    if (off >= v->arr_total) return false;
    *out_off = off;
    return true;
}

/* ===================== Expression parsing (numeric) ===================== */

/* ---- Error-message parity helpers (forward decl) ---- */
static inline void err_set(App *app, const char *msg);

static bool parse_string_value(App *app, Parser *p, char **out);

static bool parse_primary(App *app, Parser *p, double *out) {
    skip_ws(p);


    if (consume(p, '(')) {
        if (!parse_expr(app, p, out)) return false;
        if (!consume(p, ')')) return false;
        return true;
    }

    // number literal
    const char *save = p->s;
    double num = 0.0;
    if (parse_number(p, &num)) { *out = num; return true; }
    p->s = save;

    // variable or array element
    char *name = NULL;
    if (parse_identifier(p, &name)) {
        if (ident_is_string_var(app, name)) { free(name); return false; } // numeric expr can't use string vars
        // Built-in numeric functions like SIN(x), COS(x), etc.
        // If an identifier is followed by '(' and matches a known function name, treat it as a call
        skip_ws(p);
        // DEF FN user-defined numeric functions
        if (strncmp(name, "FN", 2) == 0) {
            FnDef *fd = program_fndef_find(&app->prog, name);
            if (fd) {
                if (fd->ret_kind == 1) { err_set(app, "Type mismatch"); free(name); return false; }

                // recursion guard
                for (int k = 0; k < app->fn_call_sp; k++) {
                    if (app->fn_call_stack[k] == fd->name) { err_set(app, "Illegal function call"); free(name); return false; }
                }
                if (app->fn_call_sp < 32) app->fn_call_stack[app->fn_call_sp++] = fd->name;

                // parse args (parens required for >=1 args; optional for 0-arg)
                double *argv = NULL;
                int argc = 0;
                skip_ws(p);
                if (*p->s == '(') {
                    consume(p, '(');
                    skip_ws(p);
                    if (!consume(p, ')')) {
                        while (1) {
                            double dv = 0.0;
                            if (!parse_expr(app, p, &dv)) { app->fn_call_sp--; free(name); return false; }
                            argv = (double*)realloc(argv, (size_t)(argc + 1) * sizeof(double));
                            argv[argc++] = dv;
                            skip_ws(p);
                            if (consume(p, ',')) continue;
                            if (!consume(p, ')')) { err_set(app, "Syntax error"); free(argv); app->fn_call_sp--; free(name); return false; }
                            break;
                        }
                    }
                } else {
                    // no parens => 0-arg only
                    argc = 0;
                }

                if (argc != fd->param_count) { err_set(app, "Illegal function call"); free(argv); app->fn_call_sp--; free(name); return false; }

                typedef struct { bool existed; VarKind kind; unsigned char num_type; bool num_is_int; bool is_array; double num; char *str; } Saved;
                Saved *saved = NULL;
                if (fd->param_count > 0) saved = (Saved*)calloc((size_t)fd->param_count, sizeof(Saved));

                // bind args
                for (int i = 0; i < fd->param_count; i++) {
                    const char *pn = fd->params[i];
                    Var *v = (Var*)g_hash_table_lookup(app->vars, pn);
                    if (v) {
                        if (v->is_array) { err_set(app, "Illegal function call"); goto fn_fail; }
                        saved[i].existed = true;
                        saved[i].kind = v->kind;
                        saved[i].num_type = v->num_type;
                        saved[i].num_is_int = v->num_is_int;
                        saved[i].is_array = v->is_array;
                        saved[i].num = v->num;
                        saved[i].str = xstrdup(v->str ? v->str : "");
                    } else {
                        saved[i].existed = false;
                    }
                    Var *bv = vars_get_or_create(app, pn);
                    bv->kind = V_NUM;
                    bv->is_array = false;
                    bv->num = coerce_numeric_store(app, bv, pn, argv ? argv[i] : 0.0);
                }

                Parser fp = { fd->expr_src };
                double rv = 0.0;
                bool ok = parse_expr(app, &fp, &rv);
                if (ok) { skip_ws(&fp); if (*fp.s != 0) { err_set(app, "Syntax error"); ok = false; } }

                // restore args
                for (int i = 0; i < fd->param_count; i++) {
                    const char *pn = fd->params[i];
                    if (!saved[i].existed) {
                        g_hash_table_remove(app->vars, pn);
                    } else {
                        Var *v = vars_get_or_create(app, pn);
                        v->kind = saved[i].kind;
                        v->num_type = saved[i].num_type;
                        v->num_is_int = saved[i].num_is_int;
                        v->is_array = saved[i].is_array;
                        v->num = saved[i].num;
                        free(v->str);
                        v->str = saved[i].str;
                        saved[i].str = NULL;
                    }
                }

                free(argv);
                if (saved) {
                    for (int i = 0; i < fd->param_count; i++) free(saved[i].str);
                    free(saved);
                }
                app->fn_call_sp--;
                free(name);
                if (!ok) return false;
                *out = rv;
                return true;

            fn_fail:
                free(argv);
                if (saved) {
                    for (int i = 0; i < fd->param_count; i++) free(saved[i].str);
                    free(saved);
                }
                app->fn_call_sp--;
                free(name);
                return false;
            }

            /* GW-BASIC: FNxxx is reserved for user-defined functions.
               If the function is not defined, raise "Undefined user function". */
            err_set(app, "Undefined user function");
            free(name);
            return false;
        }

        // GW-BASIC allows some 0-arg functions/constants without parentheses (notably TIMER, PI, and RND).
        // In classic BASIC these are keywords, not legal variable names, so treat them as built-ins when not followed by "(".
        if (*p->s != '(') {
            if (!strcasecmp(name, "PI")) {
                *out = 3.14159265358979323846;
                free(name);
                return true;
            }
            if (!strcasecmp(name, "TIMER")) {
                struct timeval tv; gettimeofday(&tv, NULL);
                struct tm lt;
                time_t t = tv.tv_sec;
#ifdef _WIN32
                localtime_s(&lt, &t);
#else
                localtime_r(&t, &lt);
#endif
                double sec = (double)(lt.tm_hour*3600 + lt.tm_min*60 + lt.tm_sec) + (double)tv.tv_usec/1e6;
                *out = sec;
                free(name);
                return true;
            }
            if (!strcasecmp(name, "RND")) {
                double r = (double)rand() / (double)RAND_MAX;
                app->last_rnd = r; app->have_last_rnd = true;
                *out = r;
                free(name);
                return true;
    if (!strcasecmp(name, "ERR")) {
    *out = (double)app->last_err_code;
    free(name);
    return true;
}
if (!strcasecmp(name, "ERL")) {
    *out = (double)app->last_err_line;
    free(name);
    return true;
}
        }
if (!strcasecmp(name, "ERR")) {
    *out = (double)app->last_err_code;
    free(name);
    return true;
}
if (!strcasecmp(name, "ERL")) {
    *out = (double)app->last_err_line;
    free(name);
    return true;
}
        }

        if (*p->s == '(') {
            // Don't consume '(' unless we recognize the function; arrays also use name(...)
            if (!strcasecmp(name, "SIN") || !strcasecmp(name, "COS") || !strcasecmp(name, "TAN") ||
                !strcasecmp(name, "ATN") || !strcasecmp(name, "SQR") || !strcasecmp(name, "LOG") ||
                !strcasecmp(name, "EXP") || !strcasecmp(name, "ABS") || !strcasecmp(name, "INT") ||
                !strcasecmp(name, "SGN") || !strcasecmp(name, "FIX") || !strcasecmp(name, "CINT") ||
                !strcasecmp(name, "RND") || !strcasecmp(name, "TIMER") || !strcasecmp(name, "PI") || !strcasecmp(name, "EOF") || !strcasecmp(name, "LOF") || !strcasecmp(name, "SEEK") || !strcasecmp(name, "CVI") || !strcasecmp(name, "CVS") || !strcasecmp(name, "CVD") ||
                !strcasecmp(name, "LBOUND") || !strcasecmp(name, "UBOUND") || !strcasecmp(name, "POINT")) {

                consume(p, '(');
                /* EOF(n) returns -1 at EOF, else 0. */
                if (!strcasecmp(name, "EOF")) {
                    double hv2 = 0.0;
                    if (!parse_expr(app, p, &hv2)) { free(name); return false; }
                    skip_ws(p);
                    if (!consume(p, ')')) { free(name); return false; }
                    int h2 = (int)llround(hv2);
                    *out = func_eof(app, h2);
                    free(name);
                    return true;
                }
                
                /* LOF(n) returns file length in bytes. */
                if (!strcasecmp(name, "LOF")) {
                    double hv2 = 0.0;
                    if (!parse_expr(app, p, &hv2)) { free(name); return false; }
                    skip_ws(p);
                    if (!consume(p, ')')) { free(name); return false; }
                    int h2 = (int)llround(hv2);
                    if (h2 <= 0 || h2 >= BASIC_MAX_FILES || !app->files[h2].fp) {
                        free(name);
                        runtime_error(app, 0, "Invalid file handle");
                        return false;
                    }
                    long cur = ftell(app->files[h2].fp);
                    fseek(app->files[h2].fp, 0, SEEK_END);
                    long end = ftell(app->files[h2].fp);
                    fseek(app->files[h2].fp, cur, SEEK_SET);
                    *out = (double)end;
                    free(name);
                    return true;
                }

                /* SEEK(n) returns current position: record number for RANDOM, else 1-based byte position. */
                if (!strcasecmp(name, "SEEK")) {
                    double hv2 = 0.0;
                    if (!parse_expr(app, p, &hv2)) { free(name); return false; }
                    skip_ws(p);
                    if (!consume(p, ')')) { free(name); return false; }
                    int h2 = (int)llround(hv2);
                    if (h2 <= 0 || h2 >= BASIC_MAX_FILES || !app->files[h2].fp) {
                        free(name);
                        runtime_error(app, 0, "Invalid file handle");
                        return false;
                    }
                    long cur = ftell(app->files[h2].fp);
                    BasicFile *bf = &app->files[h2];
                    if (bf->mode == BF_RANDOM && bf->record_len > 0) *out = (double)(cur / bf->record_len + 1);
                    else *out = (double)(cur + 1);
                    free(name);
                    return true;
                }

                /* CVI(s$): unpack 16-bit signed integer */
if (!strcasecmp(name, "CVI")) {
    char *sv=NULL;
    if (!parse_string_value(app, p, &sv)) { free(name); return false; }
    skip_ws(p);
    if (!consume(p, ')')) { free(name); free(sv); return false; }
    *out = (double)unpack_i16(sv);
    free(sv);
    free(name);
    return true;
}

/* CVS(s$): unpack single-precision float */
if (!strcasecmp(name, "CVS")) {
    char *sv=NULL;
    if (!parse_string_value(app, p, &sv)) { free(name); return false; }
    skip_ws(p);
    if (!consume(p, ')')) { free(name); free(sv); return false; }
    *out = (double)unpack_f32(sv);
    free(sv);
    free(name);
    return true;
}

/* CVD(s$): unpack double */
if (!strcasecmp(name, "CVD")) {
    char *sv=NULL;
    if (!parse_string_value(app, p, &sv)) { free(name); return false; }
    skip_ws(p);
    if (!consume(p, ')')) { free(name); free(sv); return false; }
    *out = unpack_f64(sv);
    free(sv);
    free(name);
    return true;
}

/* PI() is a zero-argument constant; PI without () is a normal variable. */
                if (!strcasecmp(name, "PI")) {
                    skip_ws(p);
                    if (!consume(p, ')')) { free(name); return false; }
                    *out = 3.14159265358979323846;
                    free(name);
                    return true;
                }
/* ERR() / ERL() are zero-argument functions (GW-BASIC) */
if (!strcasecmp(name, "ERR")) {
    skip_ws(p);
    if (!consume(p, ')')) { free(name); return false; }
    *out = (double)app->last_err_code;
    free(name);
    return true;
}
if (!strcasecmp(name, "ERL")) {
    skip_ws(p);
    if (!consume(p, ')')) { free(name); return false; }
    *out = (double)app->last_err_line;
    free(name);
    return true;
}


                /* TIMER() is a zero-argument function; TIMER without () is a normal variable. */
                if (!strcasecmp(name, "TIMER")) {
                    skip_ws(p);
                    if (!consume(p, ')')) { free(name); return false; }
                    struct timeval tv; gettimeofday(&tv, NULL);
                    struct tm lt;
                    time_t t = tv.tv_sec;
#ifdef _WIN32
                    localtime_s(&lt, &t);
#else
                    localtime_r(&t, &lt);
#endif
                    double sec = (double)(lt.tm_hour*3600 + lt.tm_min*60 + lt.tm_sec) + (double)tv.tv_usec/1e6;
                    *out = sec;
                    free(name);
                    return true;
                }

                /* RND() is allowed (no-arg) and returns next random; RND without () is a normal variable. */
                if (!strcasecmp(name, "RND")) {
                    skip_ws(p);
                    if (consume(p, ')')) {
                        double r = (double)rand() / (double)RAND_MAX;
                        app->last_rnd = r; app->have_last_rnd = true;
                        *out = r;
                        free(name);
                        return true;
                    }
                    /* Otherwise parse RND(expr) */
                }

                /* LBOUND(array[,dim]) / UBOUND(array[,dim]) */
                if (!strcasecmp(name, "LBOUND") || !strcasecmp(name, "UBOUND")) {
                    bool want_upper = !strcasecmp(name, "UBOUND");
                    char *aname = NULL;
                    if (!parse_identifier(p, &aname)) { err_set(app, "Type mismatch"); free(name); return false; }
                    Var *av = (Var*)g_hash_table_lookup(app->vars, aname);
                    if (!av || !av->is_array) { err_set(app, "Type mismatch"); free(aname); free(name); return false; }

                    int dim = 1;
                    skip_ws(p);
                    if (consume(p, ',')) {
                        double dv = 0.0;
                        if (!parse_expr(app, p, &dv)) { free(aname); free(name); return false; }
                        dim = (int)llround(dv);
                    }
                    skip_ws(p);
                    if (!consume(p, ')')) { free(aname); free(name); return false; }
                    if (dim < 1 || dim > av->arr_dims) { err_set(app, "Subscript out of range"); free(aname); free(name); return false; }

                    int di = dim - 1;
                    *out = (double)(want_upper ? av->arr_dim_max[di] : av->arr_dim_lo[di]);
                    free(aname);
                    free(name);
                    return true;
                }

                if (!strcasecmp(name, "POINT")) {
                    double xv = 0.0, yv = 0.0;
                    if (!parse_expr(app, p, &xv)) { free(name); return false; }
                    if (!consume(p, ',')) { free(name); return false; }
                    if (!parse_expr(app, p, &yv)) { free(name); return false; }
                    skip_ws(p);
                    if (!consume(p, ')')) { free(name); return false; }
                    if (!wbasic_ui_active(app)) { err_set(app, "Graphics not available in CLI/headless mode"); free(name); return false; }
                    if (!video_mode_is_graphics(app->video_mode)) { err_set(app, "POINT requires graphics mode"); free(name); return false; }
                    int xi = (int)llround(xv);
                    int yi = (int)llround(yv);
                    int c = gfx_point(app, xi, yi);
                    *out = (c < 0) ? -1.0 : (double)c;
                    free(name);
                    return true;
                }

                double arg = 0.0;
                if (!parse_expr(app, p, &arg)) { free(name); return false; }
                if (!consume(p, ')')) { free(name); return false; }

                if (!strcasecmp(name, "SIN")) { *out = sin(arg); free(name); return true; }
                if (!strcasecmp(name, "COS")) { *out = cos(arg); free(name); return true; }
                if (!strcasecmp(name, "TAN")) { *out = tan(arg); free(name); return true; }
                if (!strcasecmp(name, "ATN")) { *out = atan(arg); free(name); return true; }
                if (!strcasecmp(name, "SQR")) { *out = sqrt(arg); free(name); return true; }
                if (!strcasecmp(name, "LOG")) { *out = log(arg); free(name); return true; }
                if (!strcasecmp(name, "EXP")) { *out = exp(arg); free(name); return true; }
                if (!strcasecmp(name, "ABS")) { *out = fabs(arg); free(name); return true; }
                if (!strcasecmp(name, "INT")) { *out = floor(arg); free(name); return true; }
                if (!strcasecmp(name, "SGN")) { *out = (arg > 0) - (arg < 0); free(name); return true; }
                if (!strcasecmp(name, "FIX")) { *out = (arg >= 0) ? floor(arg) : ceil(arg); free(name); return true; }
                if (!strcasecmp(name, "CINT")) { *out = floor(arg + 0.5); free(name); return true; }
                if (!strcasecmp(name, "TIMER")) {
                    struct timeval tv; gettimeofday(&tv, NULL);
                    struct tm lt;
                    time_t t = tv.tv_sec;
#ifdef _WIN32
                    localtime_s(&lt, &t);
#else
                    localtime_r(&t, &lt);
#endif
                    double sec = (double)(lt.tm_hour*3600 + lt.tm_min*60 + lt.tm_sec) + (double)tv.tv_usec/1e6;
                    *out = sec; free(name); return true;
                }
                if (!strcasecmp(name, "RND")) {
                    // GW-BASIC-ish: RND(0) repeats last; RND(<0) seeds; otherwise returns next
                    if (arg == 0.0 && app->have_last_rnd) { *out = app->last_rnd; free(name); return true; }
                    if (arg < 0.0) { srand((unsigned)fabs(arg)); app->have_last_rnd = false; }
                    double r = (double)rand() / (double)RAND_MAX;
                    app->last_rnd = r; app->have_last_rnd = true;
                    *out = r; free(name); return true;
                }
            }
        }

        
        // Built-in numeric functions with arguments
        if (!strcasecmp(name, "LEN")) {
            skip_ws(p);
            if (!consume(p, '(')) { free(name); return false; }
            char *sarg = NULL;
            if (!parse_string_value(app, p, &sarg)) { free(name); return false; }
            if (!consume(p, ')')) { free(sarg); free(name); return false; }
            *out = (double)strlen(sarg ? sarg : "");
            free(sarg);
            free(name);
            return true;
        }

        if (!strcasecmp(name, "INSTR")) {
            // INSTR([start,] s$, sub$) -> 1-based position, 0 if not found
            skip_ws(p);
            if (!consume(p, '(')) { free(name); return false; }

            int startpos = 1;
            const char *save2 = p->s;

            // Try optional numeric start argument: <expr> ,
            double dstart = 0.0;
            if (parse_expr(app, p, &dstart)) {
                skip_ws(p);
                if (consume(p, ',')) {
                    startpos = (int)llround(dstart);
                } else {
                    // Not actually a start argument; rewind and treat as INSTR(s$,sub$)
                    p->s = save2;
                }
            } else {
                p->s = save2;
            }

            char *sarg = NULL;
            if (!parse_string_value(app, p, &sarg)) { free(name); return false; }
            if (!consume(p, ',')) { free(sarg); free(name); return false; }
            char *sub = NULL;
            if (!parse_string_value(app, p, &sub)) { free(sarg); free(name); return false; }
            if (!consume(p, ')')) { free(sub); free(sarg); free(name); return false; }

            if (startpos <= 0) { err_set(app, "Illegal function call"); free(sub); free(sarg); free(name); return false; }

            const char *S = sarg ? sarg : "";
            const char *P = sub ? sub : "";
            int Ls = (int)strlen(S);
            int Lp = (int)strlen(P);

            if (Lp == 0) {
                // Common BASIC behavior: empty pattern returns start position
                *out = (double)startpos;
                free(sub); free(sarg); free(name);
                return true;
            }

            if (startpos > Ls) {
                *out = 0.0;
                free(sub); free(sarg); free(name);
                return true;
            }

            const char *found = strstr(S + (startpos - 1), P);
            if (!found) *out = 0.0;
            else *out = (double)((found - S) + 1);

            free(sub); free(sarg); free(name);
            return true;
        }
        if (!strcasecmp(name, "VAL")) {
            skip_ws(p);
            if (!consume(p, '(')) { free(name); return false; }
            char *sarg = NULL;
            if (!parse_string_value(app, p, &sarg)) { free(name); return false; }
            if (!consume(p, ')')) { free(sarg); free(name); return false; }
            char *endp = NULL;
            double v = 0.0;
            if (sarg) {
                v = strtod(sarg, &endp);
                if (endp == sarg) v = 0.0;
            }
            *out = v;
            free(sarg);
            free(name);
            return true;
        }
        if (!strcasecmp(name, "ASC")) {
            skip_ws(p);
            if (!consume(p, '(')) { free(name); return false; }
            char *sarg = NULL;
            if (!parse_string_value(app, p, &sarg)) { free(name); return false; }
            if (!consume(p, ')')) { free(sarg); free(name); return false; }
            if (!sarg || sarg[0] == '\0') { err_set(app, "Illegal function call"); free(sarg); free(name); return false; }
            *out = (unsigned char)sarg[0];
            free(sarg);
            free(name);
            return true;
        }

Var *v = vars_get_or_create(app, name);
        skip_ws(p);
        if (consume(p, '(')) {
            // array element (up to 5D)
            int nd = 0;
            int idx[5] = {0,0,0,0,0};
            if (!parse_array_indices(app, p, &nd, idx)) { free(name); return false; }
// Auto-dimension undefined arrays like GW-BASIC (default upper bound 10 per dimension)
if (!v->is_array) {
    app->option_base_locked = true;
    int dim_max[5] = {10,10,10,10,10};
    if (!var_define_num_array(v, nd, app->option_base, dim_max)) { err_set(app, "Out of memory"); free(name); return false; }
}
            size_t off = 0;
            if (!array_calc_offset(v, nd, idx, &off)) { err_set(app, "Subscript out of range"); free(name); return false; }
            if (v->kind != V_NUM || !v->arr) { err_set(app, "Type mismatch"); free(name); return false; }
            *out = v->arr[off];
            free(name);
            return true;
        } else {
            // scalar
            if (v->kind != V_NUM) { err_set(app, "Type mismatch"); free(name); return false; }
            *out = v->num;
            free(name);
            return true;
        }
    }
    return false;
}

/* Power operator: right-associative '^' (GW-BASIC exponentiation). */
static bool parse_power(App *app, Parser *p, double *out) {
    if (!parse_primary(app, p, out)) return false;
    skip_ws(p);
    if (consume(p, '^')) {
        double rhs = 0.0;
        if (!parse_power(app, p, &rhs)) return false; // right-associative
        *out = pow(*out, rhs);
    }
    return true;
}

static bool parse_factor(App *app, Parser *p, double *out) {
    skip_ws(p);
    // unary +/-
    if (*p->s == '+' || *p->s == '-') {
        char op = *p->s++;
        double v = 0.0;
        if (!parse_factor(app, p, &v)) return false;
        *out = (op == '-') ? -v : v;
        return true;
    }

    return parse_power(app, p, out);
}

static bool parse_term(App *app, Parser *p, double *out) {
    if (!parse_factor(app, p, out)) return false;
    for (;;) {
        skip_ws(p);

        // GW-BASIC: MOD has the same precedence as * and /.
        // It operates on integer-truncated operands (toward zero).
        if (consume_word_ci(p, "MOD")) {
            double rhs = 0.0;
            if (!parse_factor(app, p, &rhs)) return false;
            long long a = (long long)trunc(*out);
            long long b = (long long)trunc(rhs);
            if (b == 0) { err_set(app, "Division by zero"); return false; }
            *out = (double)(a % b);
            continue;
        }

        char op = *p->s;
        // GW-BASIC: integer division operator "\\" has the same precedence as * and /.
        // It operates on integer-truncated operands (toward zero).
        if (op != '*' && op != '/' && op != '\\') break;
        p->s++;
        double rhs = 0.0;
        if (!parse_factor(app, p, &rhs)) return false;
        if (op == '*') {
            *out = (*out) * rhs;
        } else if (op == '/') {
            if (rhs == 0.0) { err_set(app, "Division by zero"); return false; }
            *out = (*out) / rhs;
        } else {
            // Integer division (\): truncate operands toward zero before dividing, and truncate result toward zero.
            long long a = (long long)trunc(*out);
            long long b = (long long)trunc(rhs);
            if (b == 0) { err_set(app, "Division by zero"); return false; }
            *out = (double)(a / b);
        }
    }
    return true;
}


static bool parse_add(App *app, Parser *p, double *out) {
    if (!parse_term(app, p, out)) return false;
    for (;;) {
        skip_ws(p);
        char op = *p->s;
        if (op != '+' && op != '-') break;
        p->s++;
        double rhs = 0.0;
        if (!parse_term(app, p, &rhs)) return false;
        if (op == '+') *out = (*out) + rhs;
        else *out = (*out) - rhs;
    }
    return true;
}

static bool wbasic_parse_expr(Parser *p, char opbuf[3]);
static double basic_truth(bool b);

/* Error state helpers are defined later; forward declare for expression parsing. */
static inline void err_clear(App *app);
static inline void err_set(App *app, const char *msg);


static bool parse_rel(App *app, Parser *p, double *out) {
    /*
     * Relational operators inside numeric expressions (GW-BASIC semantics: TRUE=-1, FALSE=0).
     *
     * GW-BASIC also allows string comparisons to yield numeric truth, e.g.:
     *   X = ("A"="A")
     *   IF ("A"<"B") THEN ...
     *
     * To match that, we first try parsing a string relational expression:
     *   <string-expr> <relop> <string-expr>
     * If that doesn't match, we fall back to the numeric path.
     */
    const char *save0 = p->s;

    /* --- Try string relational: <string> <relop> <string> --- */
    err_clear(app);
    char *sa = NULL;
    if (parse_string_value(app, p, &sa)) {
        char op[3] = {0};
        const char *save_op = p->s;
        if (wbasic_parse_expr(p, op)) {
            char *sb = NULL;
            if (!parse_string_value(app, p, &sb)) { free(sa); return false; }

            int cmp = strcmp(sa ? sa : "", sb ? sb : "");
            free(sa);
            free(sb);

            if (strcmp(op, "=") == 0) *out = basic_truth(cmp == 0);
            else if (strcmp(op, "<>") == 0) *out = basic_truth(cmp != 0);
            else if (strcmp(op, "<") == 0) *out = basic_truth(cmp < 0);
            else if (strcmp(op, ">") == 0) *out = basic_truth(cmp > 0);
            else if (strcmp(op, "<=") == 0) *out = basic_truth(cmp <= 0);
            else if (strcmp(op, ">=") == 0) *out = basic_truth(cmp >= 0);
            else { err_set(app, "Type mismatch"); return false; }

            return true;
        }

        /* Parsed a string expression but not a relational op: not valid in numeric context. */
        p->s = save_op;
        free(sa);
        err_set(app, "Type mismatch");
        return false;
    }

    /* Not a string-relational; restore and fall back to numeric parsing. */
    p->s = save0;
    err_clear(app);

    // Numeric relational: <add> [<relop> <add>]
    double a = 0.0;
    if (!parse_add(app, p, &a)) return false;

    char op[3] = {0};
    const char *save = p->s;
    if (!wbasic_parse_expr(p, op)) {
        p->s = save;
        *out = a;
        return true;
    }

    double b = 0.0;
    if (!parse_add(app, p, &b)) return false;

    if (strcmp(op, "=") == 0) *out = basic_truth(a == b);
    else if (strcmp(op, "<>") == 0) *out = basic_truth(a != b);
    else if (strcmp(op, "<") == 0) *out = basic_truth(a < b);
    else if (strcmp(op, ">") == 0) *out = basic_truth(a > b);
    else if (strcmp(op, "<=") == 0) *out = basic_truth(a <= b);
    else if (strcmp(op, ">=") == 0) *out = basic_truth(a >= b);
    else return false;

    return true;
}


static bool parse_not(App *app, Parser *p, double *out) {
    // GW-BASIC precedence: arithmetic/relational happen before NOT.
    // NOT is bitwise complement on the integer-rounded operand.
    if (consume_word_ci(p, "NOT")) {
        double v = 0.0;
        if (!parse_not(app, p, &v)) return false; // allow NOT NOT ...
        long long iv = (long long)llround(v);
        *out = (double)(~iv);
        return true;
    }
    return parse_rel(app, p, out);
}

static bool parse_bitand(App *app, Parser *p, double *out) {
    if (!parse_not(app, p, out)) return false;
    for (;;) {
        if (!consume_word_ci(p, "AND")) break;
        double rhs = 0.0;
        if (!parse_rel(app, p, &rhs)) return false;
        long long a = (long long)llround(*out);
        long long b = (long long)llround(rhs);
        *out = (double)(a & b);
    }
    return true;
}

static bool parse_bitor(App *app, Parser *p, double *out) {
    if (!parse_bitand(app, p, out)) return false;
    for (;;) {
        if (!consume_word_ci(p, "OR")) break;
        double rhs = 0.0;
        if (!parse_bitand(app, p, &rhs)) return false;
        long long a = (long long)llround(*out);
        long long b = (long long)llround(rhs);
        *out = (double)(a | b);
    }
    return true;
}

static bool parse_xor(App *app, Parser *p, double *out) {
    if (!parse_bitor(app, p, out)) return false;
    for (;;) {
        if (!consume_word_ci(p, "XOR")) break;
        double rhs = 0.0;
        if (!parse_bitor(app, p, &rhs)) return false;
        long long a = (long long)llround(*out);
        long long b = (long long)llround(rhs);
        *out = (double)(a ^ b);
    }
    return true;
}

static bool parse_expr(App *app, Parser *p, double *out) {
    // Numeric expression precedence (GW-BASIC-style, subset):
    // power(^) > unary(+/-) > */ MOD > +/- > relops -> truth(-1/0) > NOT > AND > OR > XOR
    return parse_xor(app, p, out);
}

/* ===================== Condition parsing ===================== */

static bool wbasic_parse_expr(Parser *p, char opbuf[3]) {
    skip_ws(p);
    const char *s = p->s;
    if (s[0] == '=')  { strcpy(opbuf, "=");  p->s += 1; return true; }
    if (s[0] == '<' && s[1] == '>') { strcpy(opbuf, "<>"); p->s += 2; return true; }
    if (s[0] == '<' && s[1] == '=') { strcpy(opbuf, "<="); p->s += 2; return true; }
    if (s[0] == '>' && s[1] == '=') { strcpy(opbuf, ">="); p->s += 2; return true; }
    if (s[0] == '<') { strcpy(opbuf, "<"); p->s += 1; return true; }
    if (s[0] == '>') { strcpy(opbuf, ">"); p->s += 1; return true; }
    return false;
}


// IF condition debug (temporary)
#ifndef WBASIC_DEBUG_IFCOND
#define WBASIC_DEBUG_IFCOND 0
#endif
static double g_last_cond_v = 0.0;

static double basic_truth(bool b) { return b ? -1.0 : 0.0; }

static bool parse_cond_or(App *app, Parser *p, double *out);

static bool parse_cond_atom(App *app, Parser *p, double *out) {
    skip_ws(p);
    /*
     * Parentheses at the start of a condition are ambiguous in GW-BASIC:
     *   - IF (A=1 OR B=2) AND C=3 THEN   -> grouping for boolean logic
     *   - IF (A*2)+B=20 THEN             -> arithmetic subexpression
     *
     * Our condition grammar supports both by only treating a leading '(' as
     * boolean-grouping when the matching ')' is followed by a boolean boundary
     * (end/THEN/AND/OR/)/:, etc). If the ')' is immediately followed by an
     * arithmetic or relational operator (+ - * / ^ = < >), we fall back to numeric
     * expression parsing so constructs like (A*2)+B work correctly.
     */
    {
        const char *save0 = p->s;
        Parser t = { p->s };
        skip_ws(&t);
        if (*t.s == '(') {
            if (consume(p, '(')) {
                if (parse_cond_or(app, p, out) && consume(p, ')')) {
                    Parser la = { p->s };
                    skip_ws(&la);
                    char c = *la.s;
                    if (c=='+' || c=='-' || c=='*' || c=='/' || c=='^' || c=='=' || c=='<' || c=='>') {
                        /* Looks like arithmetic/relational continuation; not a boolean-grouped atom. */
                        p->s = save0;
                    } else {
                        return true;
                    }
                } else {
                    /* Grouping attempt failed; rewind and try other parses. */
                    p->s = save0;
                }
            }
        }
    }

    // String relational: <str> [relop <str>]
    {
        const char *save0 = p->s;
        char *sa = NULL;
        if (parse_string_value(app, p, &sa)) {
            char op[3] = {0};
            const char *save1 = p->s;
            if (!wbasic_parse_expr(p, op)) {
                // No relop: BASIC truthiness for strings (non-empty = true)
                p->s = save1;
                *out = basic_truth(sa && sa[0] != '\0');
                free(sa);
                return true;
            }
            char *sb = NULL;
            if (!parse_string_value(app, p, &sb)) { free(sa); return false; }

            int cmp = strcmp(sa, sb);
            if (strcmp(op, "=") == 0) *out = basic_truth(cmp == 0);
            else if (strcmp(op, "<>") == 0) *out = basic_truth(cmp != 0);
            else if (strcmp(op, "<") == 0) *out = basic_truth(cmp < 0);
            else if (strcmp(op, ">") == 0) *out = basic_truth(cmp > 0);
            else if (strcmp(op, "<=") == 0) *out = basic_truth(cmp <= 0);
            else if (strcmp(op, ">=") == 0) *out = basic_truth(cmp >= 0);
            else { free(sa); free(sb); return false; }

            free(sa);
            free(sb);
            return true;
        }
        p->s = save0;
    }

    // Numeric relational: <arith> [relop <arith>]
    double a = 0.0;
    if (!parse_add(app, p, &a)) return false;

    char op[3] = {0};
    const char *save = p->s;
    if (!wbasic_parse_expr(p, op)) {
        p->s = save;
        *out = a;
        return true;
    }

    double b = 0.0;
    if (!parse_add(app, p, &b)) return false;

    if (strcmp(op, "=") == 0) *out = basic_truth(a == b);
    else if (strcmp(op, "<>") == 0) *out = basic_truth(a != b);
    else if (strcmp(op, "<") == 0) *out = basic_truth(a < b);
    else if (strcmp(op, ">") == 0) *out = basic_truth(a > b);
    else if (strcmp(op, "<=") == 0) *out = basic_truth(a <= b);
    else if (strcmp(op, ">=") == 0) *out = basic_truth(a >= b);
    else return false;
    return true;
}

static bool parse_cond_not(App *app, Parser *p, double *out) {
    if (consume_word_ci(p, "NOT")) {
        double v = 0.0;
        if (!parse_cond_not(app, p, &v)) return false;
        // Logical NOT for conditions: NOT 0 -> -1 (TRUE), NOT nonzero -> 0 (FALSE)
        *out = (v == 0.0) ? -1.0 : 0.0;
        return true;
    }
    return parse_cond_atom(app, p, out);
}

static bool parse_cond_and(App *app, Parser *p, double *out) {
    if (!parse_cond_not(app, p, out)) return false;
    for (;;) {
        if (!consume_word_ci(p, "AND")) break;
        double rhs = 0.0;
        if (!parse_cond_not(app, p, &rhs)) return false;
        bool a = (*out != 0.0);
        bool b = (rhs != 0.0);
        *out = (a && b) ? -1.0 : 0.0;
    }
    return true;
}

static bool parse_cond_or(App *app, Parser *p, double *out) {
    if (!parse_cond_and(app, p, out)) return false;
    for (;;) {
        if (!consume_word_ci(p, "OR")) break;
        double rhs = 0.0;
        if (!parse_cond_and(app, p, &rhs)) return false;
        bool a = (*out != 0.0);
        bool b = (rhs != 0.0);
        *out = (a || b) ? -1.0 : 0.0;
    }
    return true;
}

static bool eval_condition(App *app, Parser *p, bool *out) {
    // Prefer full BASIC condition grammar (string relops, AND/OR/NOT precedence).
    // As a compatibility fallback (for legacy programs), accept a numeric expression
    // and treat nonzero as true (GW-BASIC truth: -1/0).
    double v = 0.0;
    const char *save = p->s;
    if (!parse_cond_or(app, p, &v)) {
        // fallback: numeric expression
        p->s = save;
        if (!parse_expr(app, p, &v)) return false;
    }
    g_last_cond_v = v;
    *out = (v != 0.0);
    return true;
}


#if defined(_WIN32)
static void headless_try_read_inkey(App *app);

static void headless_tty_init(App *app) {
    if (!app || app->headless_tty_inited) return;
    app->headless_tty_fd = -1;
    app->headless_tty_using_stdin = false;
    app->headless_tty_inited = true;
}

static void headless_tty_shutdown(App *app) {
    if (!app || !app->headless_tty_inited) return;
    app->headless_tty_fd = -1;
    app->headless_tty_inited = false;
    app->headless_tty_using_stdin = false;
}

static void headless_try_read_inkey(App *app) {
    if (!app || app->inkey_ready) return;
    headless_tty_init(app);

    /*
     * Windows headless/CLI builds use the CRT console keyboard buffer.
     * INKEY$ must be non-blocking, so we only consume a byte when one is ready.
     */
    if (!_kbhit()) return;

    int c = _getch();

    /*
     * Extended keys arrive as a prefix (0 or 224) then a scan code. For INKEY$
     * compatibility we collapse those to "no key" and consume the scan code.
     */
    if (c == 0 || c == 224) {
        if (_kbhit()) (void)_getch();
        return;
    }

    /* Match headless POSIX behavior: return ESC and printable ASCII bytes. */
    if (c == 27 || (c >= 0x20 && c <= 0x7E)) {
        app->inkey_char = (char)c;
        app->inkey_ready = true;
    }
}
#else /* !_WIN32 */
static void do_stop(App *app);

#ifndef WBASIC_HAS_HEADLESS_STDOUT_IS_TTY
#define WBASIC_HAS_HEADLESS_STDOUT_IS_TTY 1
static bool headless_stdout_is_tty(void) {
    return isatty(fileno(stdout));
}
#endif

static void headless_tty_init(App *app) {
    if (!app || app->headless_tty_inited) return;

    int fd = open("/dev/tty", O_RDONLY | O_NONBLOCK);
    bool using_stdin = false;
    if (fd < 0) {
        fd = STDIN_FILENO;
        using_stdin = true;
    }

    // Save current terminal settings; we will temporarily switch to raw mode
    // only while polling INKEY$, then restore immediately so INPUT/LINE INPUT
    // continue to behave normally (canonical, line-buffered).
    struct termios t;
    if (tcgetattr(fd, &t) == 0) {
        app->headless_tty_old = t;
    } else {
        memset(&app->headless_tty_old, 0, sizeof(app->headless_tty_old));
    }

    app->headless_tty_fd = fd;
    app->headless_tty_using_stdin = using_stdin;
    app->headless_tty_inited = true;
}

static void headless_tty_shutdown(App *app) {
    if (!app || !app->headless_tty_inited) return;
    int fd = app->headless_tty_fd;
    if (fd >= 0) {
                if (headless_stdout_is_tty()) {
            /* Be polite: ensure cursor is visible and attributes reset on exit. */
            fputs("\x1b[?25h\x1b[0m", stdout);
            fflush(stdout);
        }
        (void)tcsetattr(fd, TCSANOW, &app->headless_tty_old);
        if (!app->headless_tty_using_stdin && fd != STDIN_FILENO) close(fd);
    }
    app->headless_tty_fd = -1;
    app->headless_tty_inited = false;
    app->headless_tty_using_stdin = false;
}

static void headless_try_read_inkey(App *app) {
    if (!app || app->inkey_ready) return;
    headless_tty_init(app);

    int fd = app->headless_tty_fd;
    if (fd < 0) return;

    // Temporarily enter raw/noncanonical mode so we can read single keystrokes
    // without waiting for a newline, then restore immediately so subsequent
    // INPUT / LINE INPUT remain line-buffered and do not 'run away'.
    struct termios oldt = app->headless_tty_old;
    struct termios raw = oldt;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    (void)tcsetattr(fd, TCSANOW, &raw);

    unsigned char c = 0;
    ssize_t r = read(fd, &c, 1);

    (void)tcsetattr(fd, TCSANOW, &oldt);

    if (r == 1) {
        // Headless/export: do not treat ESC as a hard stop. If read, return it via INKEY$ (CHR$(27)).
        if (c == 27) {
            app->inkey_char = (char)c;
            app->inkey_ready = true;
            return;
        }
        if (c >= 0x20 && c <= 0x7E) {
            app->inkey_char = (char)c;
            app->inkey_ready = true;
        }
    }
}

#endif /* WBASIC_NO_UI && !_WIN32 */

/* ===================== String value parsing ===================== */

static bool parse_string_atom(App *app, Parser *p, char **out) {
    const char *save = p->s;
    char *lit = NULL;
    if (parse_string_literal(p, &lit)) { *out = lit; return true; }
    p->s = save;
    
// Built-in: INPUT$(n[,#h]) - reads N chars from keyboard or from an open file.
{
    const char *save2 = p->s;
    if (consume_word_ci(p, "INPUT$")) {
        skip_ws(p);
        if (!consume(p, '(')) { p->s = save2; return false; }

        double nv = 0.0;
        if (!parse_expr(app, p, &nv)) { p->s = save2; return false; }
        int n = (int)llround(nv);
        if (n < 0) n = 0;

        skip_ws(p);
        if (consume(p, ',')) {
            skip_ws(p);
            if (!consume(p, '#')) { runtime_error(app, 0, "Bad file number"); return false; }
            double hv = 0.0;
            if (!parse_expr(app, p, &hv)) { runtime_error(app, 0, "Bad file number"); return false; }
            int h = (int)llround(hv);

            BasicFile *bf = file_get(app, h);
            if (!bf || !bf->fp) { runtime_error(app, 0, "Bad file number"); return false; }

            skip_ws(p);
            if (!consume(p, ')')) { runtime_error(app, 0, "INPUT$ missing ')'"); return false; }

            char *buf = (char*)malloc((size_t)n + 1);
            if (!buf) { runtime_error(app, 0, "Out of memory"); return false; }
            size_t got = fread(buf, 1, (size_t)n, bf->fp);
            buf[got] = 0;
            *out = buf;
            return true;
        }

        skip_ws(p);
        if (!consume(p, ')')) { runtime_error(app, 0, "INPUT$ missing ')'"); return false; }

        char *s = inputdollar_read_keyboard(app, n, 0);
        if (!s) return false;
        *out = s;
        return true;
    }
    p->s = save2;
}

// Built-in: INKEY$ (non-blocking). Returns "" if no key available, else a 1-char string.
    {
        const char *save2 = p->s;
        if (consume_word_ci(p, "INKEY$")) {
            if (!wbasic_ui_active(app)) headless_try_read_inkey(app);
            if (app->inkey_ready) {
                char buf[2] = { app->inkey_char, 0 };
                *out = xstrdup(buf);
                app->inkey_ready = false;

                // Key received during polling: show RUNNING again
                if (app->run_state == RUN_WAITING) set_run_state(app, RUN_RUNNING);
                return true;
            } else {
                *out = xstrdup("");

                // INKEY$ is non-blocking. Keep status as RUNNING while polling.
                // RUN_WAITING is reserved for truly blocking waits (e.g., INPUT dialogs).
                return true;
            }
        }
        p->s = save2;
    }





// Built-in: UCASE$(s$) / LCASE$(s$) — ASCII case conversion (GW-BASIC compatible)
    {
        const char *save2 = p->s;

        bool to_upper = false;
        bool to_lower = false;

        if (consume_word_ci(p, "UCASE$")) to_upper = true;
        else if (consume_word_ci(p, "LCASE$")) to_lower = true;

        if (to_upper || to_lower) {
            skip_ws(p);
            if (!consume(p, '(')) { p->s = save2; return false; }

            char *arg = NULL;
            if (!parse_string_value(app, p, &arg)) { p->s = save2; return false; }

            skip_ws(p);
            if (!consume(p, ')')) { free(arg); runtime_error(app, 0, "Illegal function call"); return false; }

            size_t n = arg ? strlen(arg) : 0;
            char *buf = (char*)malloc(n + 1);
            if (!buf) { free(arg); runtime_error(app, 0, "Out of memory"); return false; }

            for (size_t k = 0; k < n; k++) {
                unsigned char c = (unsigned char)arg[k];
                if (to_upper) buf[k] = (char)((c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c);
                else buf[k] = (char)((c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c);
            }
            buf[n] = 0;

            free(arg);
            *out = buf;
            return true;
        }

        p->s = save2;
    }

    char *name = NULL;
    if (parse_identifier(p, &name)) {
        if (!ident_is_string_var(app, name) && strcasecmp(name, "TAB") && strcasecmp(name, "SPC")) { free(name); return false; }
        


        // User-defined DEF FN (string-returning)
        if (starts_ci(name, "FN") && name_is_string(name)) {
            FnDef *fd = program_fndef_find(&app->prog, name);
            if (fd) {
                if (fd->ret_kind != 1) { err_set(app, "Type mismatch"); free(name); return false; }

                // recursion guard
                for (int k = 0; k < app->fn_call_sp; k++) {
                    if (app->fn_call_stack[k] == fd->name) { err_set(app, "Illegal function call"); free(name); return false; }
                }
                if (app->fn_call_sp < 32) app->fn_call_stack[app->fn_call_sp++] = fd->name;

                typedef struct Saved {
                    bool existed;
                    VarKind kind;
                    unsigned char num_type;
                    bool num_is_int;
                    bool is_array;
                    double num;
                    char *str;
                } Saved;

                Saved *saved = NULL;
                double *argv_num = NULL;
                char **argv_str = NULL;
                int argc = 0;

                // parse args (typed by DEF FN parameter names)
                if (*p->s == '(') {
                    consume(p, '(');
                    skip_ws(p);
                    if (*p->s == ')') {
                        consume(p, ')');
                        if (fd->param_count != 0) { err_set(app, "Illegal function call"); goto fnstr_fail; }
                    } else {
                        while (1) {
                            if (argc >= fd->param_count) { err_set(app, "Illegal function call"); goto fnstr_fail; }
                            const char *pn = fd->params[argc];

                            argv_num = (double*)xrealloc(argv_num, (size_t)(argc + 1) * sizeof(double));
                            argv_str = (char**)xrealloc(argv_str, (size_t)(argc + 1) * sizeof(char*));
                            argv_str[argc] = NULL;
                            argv_num[argc] = 0.0;

                            if (name_is_string(pn)) {
                                if (!parse_string_value(app, p, &argv_str[argc])) {
                                    /* GW-BASIC: no coercion between numeric and string args. */
                                    err_set(app, "Type mismatch");
                                    goto fnstr_fail;
                                }
                            } else {
                                if (!parse_expr(app, p, &argv_num[argc])) goto fnstr_fail;
                            }

                            argc++;
                            skip_ws(p);
                            if (*p->s == ',') { consume(p, ','); skip_ws(p); continue; }
                            if (*p->s == ')') { consume(p, ')'); break; }
                            err_set(app, "Syntax error");
                            goto fnstr_fail;
                        }
                        if (argc != fd->param_count) { err_set(app, "Illegal function call"); goto fnstr_fail; }
                    }
                } else {
                    // Allow FNX (no parentheses) only if no params
                    if (fd->param_count != 0) { err_set(app, "Illegal function call"); goto fnstr_fail; }
                }

                if (fd->param_count > 0) saved = (Saved*)calloc((size_t)fd->param_count, sizeof(Saved));
                if (fd->param_count > 0 && !saved) goto fnstr_fail;

                // save existing vars + bind args
                for (int i = 0; i < fd->param_count; i++) {
                    const char *pn = fd->params[i];
                    Var *cur = (Var*)g_hash_table_lookup(app->vars, pn);
                    if (cur) {
                        saved[i].existed = true;
                        saved[i].kind = cur->kind;
                        saved[i].num_type = cur->num_type;
                        saved[i].num_is_int = cur->num_is_int;
                        saved[i].is_array = cur->is_array;
                        saved[i].num = cur->num;
                        saved[i].str = cur->str ? xstrdup(cur->str) : xstrdup("");
                    } else {
                        saved[i].existed = false;
                        saved[i].str = NULL;
                    }

                    Var *v = vars_get_or_create(app, pn);
                    v->is_array = false;
                    v->num_is_int = false;
                    if (name_is_string(pn)) {
                        v->kind = V_STR;
                        free(v->str);
                        v->str = xstrdup(argv_str && argv_str[i] ? argv_str[i] : "");
                        v->num = 0.0;
                    } else {
                        v->kind = V_NUM;
                        /* Local DEF FN params follow normal default typing rules. */
                        DefType dtp = app_def_type_for_name(app, pn);
                        if (dtp == DT_INT) { v->num_type = (unsigned char)DT_INT; v->num_is_int = true; }
                        else if (dtp == DT_DBL) { v->num_type = (unsigned char)DT_DBL; v->num_is_int = false; }
                        else { v->num_type = (unsigned char)DT_SNG; v->num_is_int = false; }
                        double av = argv_num ? argv_num[i] : 0.0;
                        v->num = coerce_numeric_store(app, v, pn, av);
                        free(v->str);
                        v->str = xstrdup("");
                    }
                }

                Parser fp = { fd->expr_src };
                char *rv = NULL;
                bool ok = parse_string_value(app, &fp, &rv);
                if (ok) {
                    skip_ws(&fp);
                    if (*fp.s != 0) { err_set(app, "Syntax error"); ok = false; }
                }

                // restore vars
                for (int i = 0; i < fd->param_count; i++) {
                    const char *pn = fd->params[i];
                    if (!saved[i].existed) {
                        g_hash_table_remove(app->vars, pn);
                    } else {
                        Var *v = vars_get_or_create(app, pn);
                        v->kind = saved[i].kind;
                        v->num_type = saved[i].num_type;
                        v->num_is_int = saved[i].num_is_int;
                        v->is_array = saved[i].is_array;
                        v->num = saved[i].num;
                        free(v->str);
                        v->str = saved[i].str;
                        saved[i].str = NULL;
                    }
                }

                if (argv_str) {
                    for (int i = 0; i < argc; i++) free(argv_str[i]);
                    free(argv_str);
                }
                free(argv_num);
                if (saved) {
                    for (int i = 0; i < fd->param_count; i++) free(saved[i].str);
                    free(saved);
                }
                app->fn_call_sp--;
                free(name);
                if (!ok) { free(rv); return false; }
                *out = rv;
                return true;

            fnstr_fail:
                if (argv_str) {
                    for (int i = 0; i < argc; i++) free(argv_str[i]);
                    free(argv_str);
                }
                free(argv_num);
                if (saved) {
                    for (int i = 0; i < fd->param_count; i++) free(saved[i].str);
                    free(saved);
                }
                if (app->fn_call_sp > 0) app->fn_call_sp--;
                free(name);
                return false;
            }

            /* GW-BASIC: FNxxx$ is reserved for user-defined string functions.
               If the function is not defined, raise "Undefined user function". */
            err_set(app, "Undefined user function");
            free(name);
            return false;
        }
        // Built-in string functions
        if (!strcasecmp(name, "CHR$")) {
            skip_ws(p);
            if (!consume(p, '(')) { free(name); return false; }
            double dv = 0.0;
            if (!parse_expr(app, p, &dv)) { free(name); return false; }
            if (!consume(p, ')')) { free(name); return false; }
            int iv = (int)llround(dv);
            if (iv < 0 || iv > 255) { err_set(app, "Illegal function call"); free(name); return false; }
            char buf[2]; buf[0] = (char)iv; buf[1] = '\0';
            *out = xstrdup(buf);
            free(name);
            return true;
        }
                if (!strcasecmp(name, "MKI$")) {
                    skip_ws(p);
                    if (!consume(p, '(')) { free(name); return false; }
                    double v = 0.0;
                    if (!parse_expr(app, p, &v)) { free(name); return false; }
                    skip_ws(p);
                    if (!consume(p, ')')) { free(name); return false; }
                    char *s = pack_i16((int)llround(v));
                    if (!s) { free(name); runtime_error(app, 0, "Out of memory"); return false; }
                    *out = s;
                    free(name);
                    return true;
                }

                if (!strcasecmp(name, "MKS$")) {
                    skip_ws(p);
                    if (!consume(p, '(')) { free(name); return false; }
                    double v = 0.0;
                    if (!parse_expr(app, p, &v)) { free(name); return false; }
                    skip_ws(p);
                    if (!consume(p, ')')) { free(name); return false; }
                    char *s = pack_f32((float)v);
                    if (!s) { free(name); runtime_error(app, 0, "Out of memory"); return false; }
                    *out = s;
                    free(name);
                    return true;
                }

                if (!strcasecmp(name, "MKD$")) {
                    skip_ws(p);
                    if (!consume(p, '(')) { free(name); return false; }
                    double v = 0.0;
                    if (!parse_expr(app, p, &v)) { free(name); return false; }
                    skip_ws(p);
                    if (!consume(p, ')')) { free(name); return false; }
                    char *s = pack_f64(v);
                    if (!s) { free(name); runtime_error(app, 0, "Out of memory"); return false; }
                    *out = s;
                    free(name);
                    return true;
                }

if (!strcasecmp(name, "STR$")) {
            skip_ws(p);
            if (!consume(p, '(')) { free(name); return false; }
            double dv = 0.0;
            if (!parse_expr(app, p, &dv)) { free(name); return false; }
            if (!consume(p, ')')) { free(name); return false; }
            char numbuf[64];
            // GW-BASIC STR$ prefixes a space for non-negative values
            if (dv >= 0) snprintf(numbuf, sizeof(numbuf), " %.15g", dv);
            else snprintf(numbuf, sizeof(numbuf), "%.15g", dv);
            *out = xstrdup(numbuf);
            free(name);
            return true;
        }

        if (!strcasecmp(name, "SPACE$")) {
            skip_ws(p);
            if (!consume(p, '(')) { free(name); return false; }
            double dn = 0.0;
            if (!parse_expr(app, p, &dn)) { free(name); return false; }
            if (!consume(p, ')')) { free(name); return false; }
            int n = (int)llround(dn);
            if (n < 0) { err_set(app, "Illegal function call"); free(name); return false; }
            char *r = (char*)malloc((size_t)n + 1);
            if (!r) { err_set(app, "Out of memory"); free(name); return false; }
            memset(r, ' ', (size_t)n);
            r[n] = '\0';
            *out = r;
            free(name);
            return true;
        }

        if (!strcasecmp(name, "STRING$")) {
            // STRING$(n, ch) where ch is either a number 0..255 or a 1-char string
            skip_ws(p);
            if (!consume(p, '(')) { free(name); return false; }
            double dn = 0.0;
            if (!parse_expr(app, p, &dn)) { free(name); return false; }
            int n = (int)llround(dn);
            if (!consume(p, ',')) { free(name); return false; }

            skip_ws(p);
            int ch = -1;

            // Try string argument first
            const char *save3 = p->s;
            char *sarg = NULL;
            if (parse_string_value(app, p, &sarg)) {
                if (!sarg || sarg[0] == '\0') { err_set(app, "Illegal function call"); free(sarg); free(name); return false; }
                ch = (unsigned char)sarg[0];
                free(sarg);
            } else {
                p->s = save3;
                double dv = 0.0;
                if (!parse_expr(app, p, &dv)) { free(name); return false; }
                int iv = (int)llround(dv);
                if (iv < 0 || iv > 255) { err_set(app, "Illegal function call"); free(name); return false; }
                ch = iv;
            }

            if (!consume(p, ')')) { free(name); return false; }
            if (n < 0) { err_set(app, "Illegal function call"); free(name); return false; }

            char *r = (char*)malloc((size_t)n + 1);
            if (!r) { err_set(app, "Out of memory"); free(name); return false; }
            memset(r, (char)ch, (size_t)n);
            r[n] = '\0';
            *out = r;
            free(name);
            return true;
        }

        if (!strcasecmp(name, "TAB") || !strcasecmp(name, "SPC")) {
            // In classic BASIC, TAB(n) advances to column n in PRINT, and SPC(n) prints n spaces.
            // Outside PRINT list parsing, WBASIC materializes them as strings of spaces.
            skip_ws(p);
            if (!consume(p, '(')) { free(name); return false; }
            double dn = 0.0;
            if (!parse_expr(app, p, &dn)) { free(name); return false; }
            if (!consume(p, ')')) { free(name); return false; }
            int n = (int)llround(dn);
            if (!strcasecmp(name, "TAB")) {
                // Match GW-BASIC lower bound: TAB(n<1) behaves as TAB(1).
                if (n < 1) n = 1;
            } else {
                if (n < 0) { err_set(app, "Illegal function call"); free(name); return false; }
            }
            // Safety cap to avoid pathological allocations
            if (n > 500000) { err_set(app, "Out of memory"); free(name); return false; }
            char *r = (char*)malloc((size_t)n + 1);
            if (!r) { err_set(app, "Out of memory"); free(name); return false; }
            memset(r, ' ', (size_t)n);
            r[n] = '\0';
            *out = r;
            free(name);
            return true;
        }

        if (!strcasecmp(name, "HEX$") || !strcasecmp(name, "OCT$")) {
            // HEX$(n) / OCT$(n)
            skip_ws(p);
            if (!consume(p, '(')) { free(name); return false; }
            double dv = 0.0;
            if (!parse_expr(app, p, &dv)) { free(name); return false; }
            if (!consume(p, ')')) { free(name); return false; }

            long long iv = (long long)llround(dv);

            char buf[64];
            buf[0] = '\0';

            if (iv < 0) {
                // GW-BASIC-ish: negative values display as 16-bit two's complement
                unsigned short u = (unsigned short)((int)iv);
                if (!strcasecmp(name, "HEX$")) snprintf(buf, sizeof(buf), "%04X", (unsigned)u);
                else snprintf(buf, sizeof(buf), "%06o", (unsigned)u);
            } else {
                if (!strcasecmp(name, "HEX$")) snprintf(buf, sizeof(buf), "%llX", (unsigned long long)iv);
                else snprintf(buf, sizeof(buf), "%llo", (unsigned long long)iv);
            }

            *out = xstrdup(buf);
            if (!*out) { err_set(app, "Out of memory"); free(name); return false; }
            free(name);
            return true;
        }

        if (!strcasecmp(name, "DATE$") || !strcasecmp(name, "TIME$")) {
            // DATE$ returns "MM-DD-YYYY", TIME$ returns "HH:MM:SS"
            skip_ws(p);
            // allow optional empty parentheses: DATE$() / TIME$()
            if (consume(p, '(')) {
                skip_ws(p);
                if (!consume(p, ')')) { free(name); return false; }
            }

            time_t t = time(NULL);
            struct tm lt;
#ifdef _WIN32
            localtime_s(&lt, &t);
#else
            localtime_r(&t, &lt);
#endif

            char buf[64];
            if (!strcasecmp(name, "DATE$")) {
                snprintf(buf, sizeof(buf), "%02d-%02d-%04d", lt.tm_mon + 1, lt.tm_mday, lt.tm_year + 1900);
            } else {
                snprintf(buf, sizeof(buf), "%02d:%02d:%02d", lt.tm_hour, lt.tm_min, lt.tm_sec);
            }
            *out = xstrdup(buf);
            if (!*out) { err_set(app, "Out of memory"); free(name); return false; }
            free(name);
            return true;
        }

if (!strcasecmp(name, "LTRIM$") || !strcasecmp(name, "RTRIM$") || !strcasecmp(name, "TRIM$")) {
            bool do_l = !strcasecmp(name, "LTRIM$") || !strcasecmp(name, "TRIM$");
            bool do_r = !strcasecmp(name, "RTRIM$") || !strcasecmp(name, "TRIM$");
            skip_ws(p);
            if (!consume(p, '(')) { free(name); return false; }
            char *sarg = NULL;
            if (!parse_string_value(app, p, &sarg)) { free(name); return false; }
            if (!consume(p, ')')) { free(sarg); free(name); return false; }

            const char *S = sarg ? sarg : "";
            int L = (int)strlen(S);
            int a = 0, b = L; // [a,b)
            if (do_l) {
                while (a < b && isspace((unsigned char)S[a])) a++;
            }
            if (do_r) {
                while (b > a && isspace((unsigned char)S[b-1])) b--;
            }
            int take = b - a;
            char *r = (char*)malloc((size_t)take + 1);
            if (!r) { err_set(app, "Out of memory"); free(sarg); free(name); return false; }
            memcpy(r, S + a, (size_t)take);
            r[take] = '\0';
            *out = r;

            free(sarg);
            free(name);
            return true;
        }
        if (!strcasecmp(name, "LEFT$") || !strcasecmp(name, "RIGHT$") || !strcasecmp(name, "MID$")) {
            bool is_left = !strcasecmp(name, "LEFT$");
            bool is_right = !strcasecmp(name, "RIGHT$");
            bool is_mid = !strcasecmp(name, "MID$");
            skip_ws(p);
            if (!consume(p, '(')) { free(name); return false; }

            char *sarg = NULL;
            if (!parse_string_value(app, p, &sarg)) { free(name); return false; }

            if (!consume(p, ',')) { free(sarg); free(name); return false; }

            double d1 = 0.0;
            if (!parse_expr(app, p, &d1)) { free(sarg); free(name); return false; }
            int n1 = (int)llround(d1);

            int n2 = -1;
            skip_ws(p);
            if (consume(p, ',')) {
                double d2 = 0.0;
                if (!parse_expr(app, p, &d2)) { free(sarg); free(name); return false; }
                n2 = (int)llround(d2);
            }

            if (!consume(p, ')')) { free(sarg); free(name); return false; }

            const char *ss = sarg ? sarg : "";
            int L = (int)strlen(ss);

            if (is_left || is_right) {
                if (n1 < 0) { err_set(app, "Illegal function call"); free(sarg); free(name); return false; }
                if (n1 > L) n1 = L;
                if (is_left) {
                    char *r = (char*)malloc((size_t)n1 + 1);
                    if (!r) { err_set(app, "Out of memory"); free(sarg); free(name); return false; }
                    memcpy(r, ss, (size_t)n1);
                    r[n1] = '\0';
                    *out = r;
                } else { // right
                    const char *start = ss + (L - n1);
                    char *r = (char*)malloc((size_t)n1 + 1);
                    if (!r) { err_set(app, "Out of memory"); free(sarg); free(name); return false; }
                    memcpy(r, start, (size_t)n1);
                    r[n1] = '\0';
                    *out = r;
                }
                free(sarg);
                free(name);
                return true;
            }

            // MID$(s$, start[, len]) : start is 1-based
            if (is_mid) {
                if (n1 <= 0) { err_set(app, "Illegal function call"); free(sarg); free(name); return false; }
                int start0 = n1 - 1;
                if (start0 >= L) {
                    *out = xstrdup("");
                    free(sarg); free(name);
                    return true;
                }
                int take = (n2 < 0) ? (L - start0) : n2;
                if (take < 0) { err_set(app, "Illegal function call"); free(sarg); free(name); return false; }
                if (start0 + take > L) take = L - start0;

                char *r = (char*)malloc((size_t)take + 1);
                if (!r) { err_set(app, "Out of memory"); free(sarg); free(name); return false; }
                memcpy(r, ss + start0, (size_t)take);
                r[take] = '\0';
                *out = r;
                free(sarg);
                free(name);
                return true;
            }
        }

Var *v = vars_get_or_create(app, name);

        skip_ws(p);
        if (consume(p, '(')) {
            // String array element: NAME$(...)
            int nd = 0;
            int idx[5] = {0,0,0,0,0};
            if (!parse_array_indices(app, p, &nd, idx)) { free(name); return false; }
// Auto-dimension undefined arrays like GW-BASIC (default upper bound 10 per dimension)
if (!v->is_array) {
    app->option_base_locked = true;
    int dim_max[5] = {10,10,10,10,10};
    if (!var_define_str_array(v, nd, app->option_base, dim_max)) { err_set(app, "Out of memory"); free(name); return false; }
}
            if (v->kind != V_STR || !v->is_array || !v->sarr) { free(name); return false; }
            size_t off = 0;
            if (!array_calc_offset(v, nd, idx, &off)) { err_set(app, "Subscript out of range"); free(name); return false; }
            *out = xstrdup(v->sarr[off] ? v->sarr[off] : "");
            free(name);
            return true;
        } else {
            // scalar string
            if (v->kind != V_STR || v->is_array) { err_set(app, "Type mismatch"); free(name); return false; }
            *out = xstrdup(v->str ? v->str : "");
            free(name);
            return true;
        }
    }
    return false;
}

static bool parse_string_value(App *app, Parser *p, char **out) {
    // Full string expression: <atom> ('+' <atom>)*
    char *acc = NULL;
    if (!parse_string_atom(app, p, &acc)) return false;
    for (;;) {
        skip_ws(p);
        if (*p->s != '+') break;
        p->s++;
        char *rhs = NULL;
        if (!parse_string_atom(app, p, &rhs)) { free(acc); return false; }
        size_t la = acc ? strlen(acc) : 0;
        size_t lr = rhs ? strlen(rhs) : 0;
        char *merged = (char*)malloc(la + lr + 1);
        if (!merged) { err_set(app, "Out of memory"); free(acc); free(rhs); return false; }
        if (la) memcpy(merged, acc, la);
        if (lr) memcpy(merged + la, rhs, lr);
        merged[la + lr] = '\0';
        free(acc);
        free(rhs);
        acc = merged;
    }
    *out = acc;
    return true;
}

/* ===================== Statement execution ===================== */


static int gw_error_code_from_msg(const char *msg) {
    if (!msg) return 1;

    /*
     * GW-BASIC ERR mapping (message-based).
     *
     * WBASIC raises runtime_error(app, line, "<message>") in many places.
     * To provide GW-BASIC compatibility, we map known message substrings
     * to GW-BASIC error numbers.
     *
     * IMPORTANT:
     * - Order matters: first match wins.
     * - Prefer stable substrings, not whole sentences.
     */
    typedef struct { const char *needle; int code; } GwErrMap;
    static const GwErrMap map[] = {
        /* Core arithmetic / function errors */
        { "Division by zero", 11 },
        { "Illegal function call", 5 },

        /* Type / array errors */
        { "Type mismatch", 13 },
        { "Subscript out of range", 9 },
        { "Bad subscript", 9 },

        /* DATA / READ */
        { "Out of DATA", 4 },

        /* Control flow / structure */
        { "RETURN without GOSUB", 3 },
        { "NEXT without FOR", 1 },
        { "FOR without NEXT", 26 },
        { "WHILE without WEND", 29 },
        { "WEND without WHILE", 30 },

        /* DEF FN */
        { "Undefined user function", 35 },

        /* Undefined line number / missing targets */
        { "Undefined line number", 8 },
        { "target not found", 8 }, /* GOTO/GOSUB/ON/THEN/ELSE target not found */

        /* INPUT / LINE INPUT past end-of-file */
        { "INPUT past EOF", 62 },
        { "past EOF", 62 },

        /* Files */
        { "File handle out of range", 52 }, /* Bad file number */
        { "Bad file handle", 52 },          /* Bad file number */
        { "File not open", 54 },            /* Bad file mode */
        { "Cannot open file", 53 },         /* File not found / cannot open */
        { "File already open", 55 },        /* File already open */
        { "Bad file mode", 54 },           /* Bad file mode */
        { "Bad file number", 52 },         /* Bad file number */
        { "Bad PRINT", 54 },               /* Usually PRINT# mode/handle errors */

        /* Memory */
        { "Out of memory", 7 },
    };

    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
        if (map[i].needle && strstr(msg, map[i].needle)) return map[i].code;
    }

    /*
     * Parser / syntax-style diagnostics:
     * WBASIC frequently raises messages like "Expected '='", "IF missing THEN",
     * "missing", "Expected", etc. GW-BASIC surfaces most of these as ERR 2.
     *
     * Keep these broad and last so they don't override more specific mappings.
     */
    if (strstr(msg, "Syntax error")) return 2;
    if (strstr(msg, "Expected")) return 2;
    if (strstr(msg, "missing")) return 2;
    if (strstr(msg, "Bad expression")) return 2;
    if (strstr(msg, "Bad variable")) return 2;
    if (strstr(msg, "Bad token")) return 2;

    return 1;
}

static void runtime_error(App *app, int line_no, const char *msg) {
    /* Record for ERR/ERL (GW-BASIC style). */
    app->last_err_line = line_no;
    app->last_err_code = gw_error_code_from_msg(msg);

    /* Phase 0 (RESUME groundwork): save exact error origin cursor.
   Prefer the live execution cursor, if available.
   Also capture ':' chain context (e.g., IF THEN/ELSE tail chains) so RESUME NEXT can continue within the chain. */
if (app->exec_cursor_valid) {
    app->err_origin_valid = true;
    app->err_origin_line_idx = app->exec_line_idx;
    app->err_origin_stmt_idx = app->exec_stmt_idx;

    app->err_origin_in_chain = false;
    app->err_origin_chain_base_line_idx = -1;
    app->err_origin_chain_base_stmt_idx = -1;
    app->err_origin_chain_stmt_idx = -1;
    if (app->err_origin_chain_text) { free(app->err_origin_chain_text); app->err_origin_chain_text = NULL; }

    if (app->chain_active && app->chain_text) {
        app->err_origin_in_chain = true;
        app->err_origin_chain_base_line_idx = app->chain_base_line_idx;
        app->err_origin_chain_base_stmt_idx = app->chain_base_stmt_idx;
        app->err_origin_chain_stmt_idx = app->exec_stmt_idx; /* during chain exec, exec_stmt_idx is chain-local */
        app->err_origin_chain_text = strdup(app->chain_text);
    }
} else {
    app->err_origin_valid = false;
    app->err_origin_line_idx = -1;
    app->err_origin_stmt_idx = -1;
    app->err_origin_in_chain = false;
    app->err_origin_chain_base_line_idx = -1;
    app->err_origin_chain_base_stmt_idx = -1;
    app->err_origin_chain_stmt_idx = -1;
    if (app->err_origin_chain_text) { free(app->err_origin_chain_text); app->err_origin_chain_text = NULL; }

    app->err_origin_in_chain = false;
    app->err_origin_chain_base_line_idx = -1;
    app->err_origin_chain_base_stmt_idx = -1;
    app->err_origin_chain_stmt_idx = -1;
    if (app->err_origin_chain_text) { free(app->err_origin_chain_text); app->err_origin_chain_text = NULL; }
}

/* GW-BASIC style ON ERROR GOTO trap:
       - If a trap target is set and we're running, jump to the handler instead of stopping.
       - Suppress the default "ERROR at ..." message when trapped.
       - Basic ERR/ERL equivalents are stored in app->last_err_code / app->last_err_line (coarse). */
    if (app->run_state == RUN_RUNNING && app->on_error_goto_line > 0 && !app->in_error_handler) {
        int idx = program_find_index(&app->prog, app->on_error_goto_line);
        if (idx >= 0) {
            app->error_trap_pending = true;
            app->error_trap_line_idx = idx;
            app->error_trap_stmt_idx = 0;
            app->in_error_handler = true;
#if WBASIC_RESUME_DEBUG
            {
                char dbuf[128];
                if (app->err_origin_valid) {
                    /* line_no is ERL-style line number for the fault; stmt is the captured statement index */
                    snprintf(dbuf, sizeof(dbuf), "\n[RESUME-DBG] origin line=%d stmt=%d\n", line_no, app->err_origin_stmt_idx);
                } else {
                    snprintf(dbuf, sizeof(dbuf), "\n[RESUME-DBG] origin line=%d stmt=(unknown)\n", line_no);
                }
                out_append(app, dbuf);
            }
#endif
            return;
        }
        /* If the handler line doesn't exist, fall through to normal error behavior. */
    }


    set_run_state(app, RUN_STOPPED);
    if (line_no >= 0) out_printf(app, "ERROR at %d: %s\n", line_no, msg);
    else out_printf(app, "ERROR: %s\n", msg);
}


// Error-message parity: expression evaluators can set a specific runtime error message
// which statement executors can then emit instead of a generic "Bad <stmt>".
static inline void err_clear(App *app) {
    if (!app) return;
    app->err_pending = false;
    app->err_msg[0] = 0;
}
static inline void err_set(App *app, const char *msg) {
    if (!app) return;
    app->err_pending = true;
    if (!msg) msg = "Error";
    snprintf(app->err_msg, sizeof(app->err_msg), "%s", msg);
}
static inline const char* err_peek(App *app) {
    if (!app || !app->err_pending) return NULL;
    return app->err_msg;
}
static inline void runtime_error_or_pending(App *app, int line_no, const char *fallback) {
    /* If a trap jump has already been scheduled by a prior runtime_error() call,
       do NOT raise another error here (would duplicate and/or overwrite ERR/ERL). */
    if (app && app->error_trap_pending) { err_clear(app); return; }

    /* A statement may already have raised a specific runtime error directly.
       Avoid emitting a second fallback error like "Bad PRINT". */
    if (app && app->run_state == RUN_STOPPED) { err_clear(app); return; }

    /* If a parity message is pending (set via err_set), raise THAT message now. */
    const char *m = err_peek(app);
    if (m) {
        runtime_error(app, line_no, m);
        err_clear(app);
        return;
    }

    /* Otherwise raise the provided fallback message. */
    runtime_error(app, line_no, fallback ? fallback : "Error");
    err_clear(app);
}

typedef struct {
    // split of a single line into statements
    char **stmts;
    int count;
} StmtList;

static void stmtlist_free(StmtList *sl) {
    if (!sl || !sl->stmts) return;
    for (int i = 0; i < sl->count; i++) free(sl->stmts[i]);
    free(sl->stmts);
    sl->stmts = NULL;
    sl->count = 0;
}

// Strip inline comments from a statement chunk (outside quotes).
// GW-BASIC allows: A=1 REM comment
// We treat REM as a comment-start only when it appears as a standalone keyword
// outside quotes (word boundaries) and is not the first token of the statement.
static void strip_inline_rem_comment(char *s) {
    if (!s) return;
    bool inq = false;
    for (char *p = s; *p; p++) {
        char c = *p;
        if (c == '"' && (p == s || p[-1] != '\\')) inq = !inq;
        if (inq) continue;
        if ((p[0] == 'R' || p[0] == 'r') && (p[1] == 'E' || p[1] == 'e') && (p[2] == 'M' || p[2] == 'm')) {
            // Require word boundary after REM.
            if (!is_word_boundary(p[3])) continue;
            // Require boundary before REM.
            if (p == s) continue; // leading REM is a full-line comment; keep it intact
            char prev = p[-1];
            if (!(prev == ' ' || prev == '\t' || is_word_boundary(prev))) continue;
            *p = 0;
            trim(s);
            return;
        }
    }
}

static StmtList split_statements(const char *line_text) {
    // split by ':' not within double quotes
    StmtList sl = {0};
    if (!line_text) return sl;

    int cap = 8;
    sl.stmts = (char**)calloc((size_t)cap, sizeof(char*));
    sl.count = 0;

    bool inq = false;
    bool in_if = false;
    bool after_then = false;
    bool then_is_lineno = false;
    const char *start = line_text;
    for (const char *s = line_text; ; s++) {
        char c = *s;
        if (c == '"' && (s == line_text || s[-1] != '\\')) inq = !inq;

        /* Inline REM comment handling (GW-BASIC):
           A=1 REM anything here is ignored, and ':' inside the comment must
           NOT be treated as statement separators.
           We detect REM as a standalone keyword outside quotes, and terminate
           the rest of the line at the 'R' (like apostrophe comments). */
        if (!inq && (c == 'R' || c == 'r') && starts_ci(s, "REM") && is_word_boundary(s[3])) {
            bool ok_prev = (s == line_text);
            if (!ok_prev) {
                char prev = s[-1];
                ok_prev = (prev == ' ' || prev == '\t' || is_word_boundary(prev));
            }
            if (ok_prev) {
                c = 0; /* end-of-line at start of REM */
            }
        }

        /* Apostrophe (') starts an inline comment like REM, but not inside strings.
           It terminates the rest of the *line*, unless this statement itself is REM. */
        if (c == '\'' && !inq) {
            const char *a = start;
            while (a < s && (*a == ' ' || *a == '\t')) a++;
            if ((s - a) >= 3) {
                char r0 = a[0], r1 = a[1], r2 = a[2];
                if (!((r0=='R'||r0=='r') && (r1=='E'||r1=='e') && (r2=='M'||r2=='m') &&
                      (a + 3 >= s || is_word_boundary(*(a+3))))) {
                    c = 0; /* treat as end-of-line split; drops the apostrophe and comment text */
                }
            } else {
                c = 0;
            }
        }


/* IF ... THEN statement-tail handling:
   Do not split ':' inside the THEN/ELSE statement tail, so that:
     IF X THEN A:B
   keeps A:B together and is executed conditionally.
   We intentionally do NOT do this for THEN <line-number> (rare colon-tail cases). */
if (!inq) {
    /* detect IF at start of the current statement (ignoring leading whitespace) */
    if (!in_if) {
        const char *a = start;
        while (a < s && (*a == ' ' || *a == '\t')) a++;
        if (a == s && starts_ci(s, "IF") && is_word_boundary(s[2])) {
            in_if = true;
        }
    }
    if (in_if && !after_then) {
        /* detect THEN keyword */
        if (starts_ci(s, "THEN") && is_word_boundary(s[4]) &&
            (s == start || is_word_boundary(s[-1]) || s[-1] == ' ' || s[-1] == '\t')) {
            after_then = true;
            const char *t = s + 4;
            while (*t == ' ' || *t == '\t') t++;
            then_is_lineno = isdigit((unsigned char)*t) ? true : false;
        }
    }
}

bool do_split = ((c == ':' && !inq) || c == 0);
if (do_split && c == ':' && !inq) {
    if (after_then && !then_is_lineno) {
        do_split = false;
    }
}
        if (do_split && c == ':' && !inq) {
            // Do not split on ':' if this statement is a REM comment.
            const char *a = start;
            while (a < s && (*a == ' ' || *a == '	')) a++;
            if ((s - a) >= 3) {
                char r0 = a[0], r1 = a[1], r2 = a[2];
                if ((r0=='R'||r0=='r') && (r1=='E'||r1=='e') && (r2=='M'||r2=='m')) {
                    const char *after = a + 3;
                    if (after >= s || is_word_boundary(*after)) {
                        do_split = false;
                    }
                }
            }
        }
        if (do_split) {
            size_t len = (size_t)(s - start);
            char *chunk = (char*)malloc(len + 1);
            memcpy(chunk, start, len);
            chunk[len] = 0;
            char *t = trim(chunk);
            // GW-BASIC permits inline REM comments after statements: A=1 REM ...
            // Strip them here so trailing comment text doesn't cause syntax errors.
            strip_inline_rem_comment(t);
            // keep even empty? no, drop empties
            if (*t) {
                char *keep = xstrdup(t);
                if (sl.count >= cap) {
                    cap *= 2;
                    sl.stmts = (char**)realloc(sl.stmts, (size_t)cap * sizeof(char*));
                }
                sl.stmts[sl.count++] = keep;
            }
            free(chunk);
            if (c == 0) break;
            start = s + 1;
            in_if = false;
            after_then = false;
            then_is_lineno = false;
        }
    }
    return sl;
}



static void program_data_clear(Program *p) {
    if (!p) return;
    if (p->data) {
        for (size_t i = 0; i < p->data_count; i++) free(p->data[i].text);
        free(p->data);
    }
    p->data = NULL;
    p->data_count = 0;
    p->data_cap = 0;
    p->data_ptr = 0;
}

static void program_data_push(Program *p, const char *text, bool quoted, int line_no) {
    if (!p) return;
    if (p->data_count + 1 > p->data_cap) {
        size_t ncap = (p->data_cap == 0) ? 64 : (p->data_cap * 2);
        DataItem *nd = (DataItem*)realloc(p->data, ncap * sizeof(DataItem));
        if (!nd) return; // out of memory: best-effort
        p->data = nd;
        p->data_cap = ncap;
    }
    p->data[p->data_count].text = xstrdup(text ? text : "");
    p->data[p->data_count].quoted = quoted;
    p->data[p->data_count].line_no = line_no;
    p->data_count++;
}

static void parse_data_items_into_program(Program *prog, const char *s, int line_no) {
    // Parse comma-separated DATA items from s and append to prog->data.
    // Items may be quoted strings ("" escapes) or unquoted tokens.
    // Empty items (,,) are allowed.
    if (!prog || !s) return;

    const char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;

    while (1) {
        while (*p && isspace((unsigned char)*p)) p++;

        bool quoted = false;
        char buf[4096];
        size_t bi = 0;

        if (*p == '"') {
            quoted = true;
            p++; // opening quote
            while (*p) {
                if (*p == '"') {
                    if (p[1] == '"') { // escaped quote
                        if (bi + 1 < sizeof(buf)) buf[bi++] = '"';
                        p += 2;
                        continue;
                    }
                    p++; // closing quote
                    break;
                }
                if (bi + 1 < sizeof(buf)) buf[bi++] = *p;
                p++;
            }
            buf[bi] = 0;

            while (*p && isspace((unsigned char)*p)) p++;

            // Ignore any trailing spaces up to comma/end; if non-space appears, include it (rare)
            while (*p && *p != ',') {
                if (!isspace((unsigned char)*p)) {
                    if (bi + 1 < sizeof(buf)) buf[bi++] = *p;
                }
                p++;
            }
            buf[bi] = 0;
        } else {
            // unquoted token up to comma/end (may be empty)
            while (*p && *p != ',') {
                if (bi + 1 < sizeof(buf)) buf[bi++] = *p;
                p++;
            }
            buf[bi] = 0;

            // trim whitespace for unquoted tokens
            char *t = buf;
            while (*t && isspace((unsigned char)*t)) t++;
            char *e = t + strlen(t);
            while (e > t && isspace((unsigned char)e[-1])) e--;
            *e = 0;

            if (t != buf) memmove(buf, t, strlen(t) + 1);
        }

        program_data_push(prog, buf, quoted, line_no);

        if (*p == ',') { p++; continue; }
        break;
    }
}

static void program_build_data_pool(Program *prog) {
    // Build the DATA pool by scanning all program lines for DATA statements.
    // In GW-BASIC, DATA statements are not executed; READ pulls sequentially from this pool.
    if (!prog) return;
    program_data_clear(prog);

    for (size_t li = 0; li < prog->count; li++) {
        const char *text = prog->lines[li].text ? prog->lines[li].text : "";
        StmtList sl = split_statements(text);
        for (int si = 0; si < sl.count; si++) {
            const char *st = sl.stmts[si];
            while (*st && isspace((unsigned char)*st)) st++;
            if (starts_ci(st, "DATA") && is_word_boundary(st[4])) {
                parse_data_items_into_program(prog, st + 4, prog->lines[li].line_no);
            }
        }
        stmtlist_free(&sl);
    }
    prog->data_ptr = 0;
}

static bool exec_run_stmt(App *app, const char *s, int current_line, int *line_idx, int *stmt_idx);

static bool exec_single_statement(App *app, const char *stmt, int current_line, int cur_li, int cur_si, int *line_idx, int *stmt_idx);





/* ---- Phase 5: Binary packing helpers ----
   NOTE: WBASIC strings are currently NUL-terminated C strings (not binary-length strings).
   To make MKI$/MKS$/MKD$ and CVI/CVS/CVD usable without a full binary-string refactor, we store floats/doubles in
   big-endian order to avoid leading NULs for common values, and CVS/CVD tolerate truncated
   strings (missing tail bytes are treated as 0).
*/
static char *pack_i16(int v) {
    char *s = (char*)malloc(3);
    if (!s) return NULL;
    s[0] = (char)(v & 0xFF);
    s[1] = (char)((v >> 8) & 0xFF);
    s[2] = 0;
    return s;
}
static char *pack_f32(float f) {
    char *s = (char*)malloc(5);
    if (!s) return NULL;
    union { float f; unsigned char b[4]; } u;
    u.f = f;
    // big-endian
    s[0] = (char)u.b[3];
    s[1] = (char)u.b[2];
    s[2] = (char)u.b[1];
    s[3] = (char)u.b[0];
    s[4] = 0;
    return s;
}
static char *pack_f64(double d) {
    char *s = (char*)malloc(9);
    if (!s) return NULL;
    union { double d; unsigned char b[8]; } u;
    u.d = d;
    // big-endian
    for (int i = 0; i < 8; i++) s[i] = (char)u.b[7 - i];
    s[8] = 0;
    return s;
}
static int unpack_i16(const char *s) {
    if (!s) return 0;
    unsigned char b0 = (unsigned char)s[0];
    unsigned char b1 = (unsigned char)s[1];
    short v = (short)(b0 | (b1 << 8));
    return (int)v;
}
static float unpack_f32(const char *s) {
    union { float f; unsigned char b[4]; } u;
    for (int i = 0; i < 4; i++) u.b[i] = 0;
    if (s) {
        size_t n = strlen(s);
        if (n > 4) n = 4;
        // input big-endian -> place into u.b[3..0]
        for (size_t i = 0; i < n; i++) u.b[3 - (int)i] = (unsigned char)s[i];
    }
    return u.f;
}
static double unpack_f64(const char *s) {
    union { double d; unsigned char b[8]; } u;
    for (int i = 0; i < 8; i++) u.b[i] = 0;
    if (s) {
        size_t n = strlen(s);
        if (n > 8) n = 8;
        // input big-endian -> place into u.b[7..0]
        for (size_t i = 0; i < n; i++) u.b[7 - (int)i] = (unsigned char)s[i];
    }
    return u.d;
}

/* ---- RANDOM FIELD helpers (Phase 4) ---- */
static void field_clear(BasicFile *bf) {
    if (!bf) return;
    if (bf->fields) {
        for (int i = 0; i < bf->field_count; i++) {
            free(bf->fields[i].name);
        }
        free(bf->fields);
    }
    bf->fields = NULL;
    bf->field_count = 0;
    bf->field_cap = 0;
}

static FieldMap *field_find(BasicFile *bf, const char *upper_name) {
    if (!bf || !bf->fields) return NULL;
    for (int i = 0; i < bf->field_count; i++) {
        if (!strcmp(bf->fields[i].name, upper_name)) return &bf->fields[i];
    }
    return NULL;
}

static bool field_lookup(App *app, const char *upper_name, BasicFile **out_bf, FieldMap **out_fm) {
    for (int i = 1; i < BASIC_MAX_FILES; i++) {
        BasicFile *bf = &app->files[i];
        if (!bf->fp) continue;
        FieldMap *fm = field_find(bf, upper_name);
        if (fm) {
            *out_bf = bf;
            *out_fm = fm;
            return true;
        }
    }
    return false;
}

static void field_sync_vars(App *app, BasicFile *bf) {
    if (!bf || !bf->fields || !bf->record_buf) return;
    for (int i = 0; i < bf->field_count; i++) {
        FieldMap *fm = &bf->fields[i];
        Var *v = vars_get_or_create(app, fm->name);
        if (!v) continue;
        // Copy slice into scalar string (fixed-length, but stored as C string)
        char *s = (char*)malloc((size_t)fm->len + 1);
        if (!s) continue;
        memcpy(s, bf->record_buf + fm->offset, (size_t)fm->len);
        s[fm->len] = 0;
        v->kind = V_STR;
        free(v->str);
        v->str = s;
    }
}

static void field_write_slice(BasicFile *bf, FieldMap *fm, const char *src, bool right_justify) {
    // Pad with spaces and truncate. GW-BASIC style for LSET/RSET.
    memset(bf->record_buf + fm->offset, ' ', (size_t)fm->len);
    size_t sl = src ? strlen(src) : 0;
    if (sl > (size_t)fm->len) sl = (size_t)fm->len;
    if (right_justify) {
        size_t start = (size_t)fm->len - sl;
        memcpy(bf->record_buf + fm->offset + (int)start, src, sl);
    } else {
        memcpy(bf->record_buf + fm->offset, src, sl);
    }
}


static void gfx_draw_move(App *app, int *x, int *y, int nx, int ny, int color, bool draw_line) {
    if (draw_line) gfx_line(app, *x, *y, nx, ny, color);
    *x = nx;
    *y = ny;
}

static void gfx_draw_rotate_quadrant(int angle, int vx, int vy, int *rx, int *ry) {
    int a = angle & 3;
    if (a == 0) { *rx = vx;  *ry = vy; return; }
    if (a == 1) { *rx = vy;  *ry = -vx; return; }
    if (a == 2) { *rx = -vx; *ry = -vy; return; }
    *rx = -vy; *ry = vx;
}

static bool exec_draw_gfx(App *app, Parser *p, int current_line) {
    if (!app || !p) return false;
    if (!wbasic_ui_active(app)) {
        runtime_error(app, current_line, "Graphics not available in CLI/headless mode");
        return false;
    }
    if (!video_mode_is_graphics(app->video_mode)) {
        runtime_error(app, current_line, "DRAW requires graphics mode");
        return false;
    }

    skip_ws(p);
    char *script = NULL;
    if (!parse_string_value(app, p, &script)) {
        runtime_error(app, current_line, "DRAW expects string");
        return false;
    }
    skip_ws(p);
    if (*p->s != '\0') {
        free(script);
        runtime_error(app, current_line, "Syntax error");
        return false;
    }

    int x = app->gfx_draw_x;
    int y = app->gfx_draw_y;
    int scale = (app->gfx_draw_scale > 0) ? app->gfx_draw_scale : 1;
    int angle = app->gfx_draw_angle & 3;
    int color = (app->cur_fg >= 0 && app->cur_fg <= 15) ? app->cur_fg : 15;

    const char *q = script;
    while (*q) {
        while (*q && (isspace((unsigned char)*q) || *q == ';' || *q == ',')) q++;
        if (!*q) break;

        bool blank = false;
        bool no_update = false;
        while (*q == 'B' || *q == 'b' || *q == 'N' || *q == 'n') {
            if (*q == 'B' || *q == 'b') blank = true;
            if (*q == 'N' || *q == 'n') no_update = true;
            q++;
        }

        char cmd = (char)toupper((unsigned char)*q);
        if (!cmd) break;
        q++;

        int n = 0;
        bool have_n = false;
        if (cmd != 'M') {
            while (*q && isdigit((unsigned char)*q)) {
                have_n = true;
                n = n * 10 + (*q - '0');
                q++;
            }
            if (!have_n && *q == '=') {
                Parser np = { .s = q + 1 };
                double nv = 0.0;
                if (!parse_expr(app, &np, &nv)) { free(script); runtime_error(app, current_line, "Syntax error"); return false; }
                q = np.s;
                n = (int)nv;
                have_n = true;
            }
        }

        int oldx = x, oldy = y;
        int dx = 0, dy = 0;

        switch (cmd) {
            case 'U': dx = 0; dy = -1; break;
            case 'D': dx = 0; dy = 1; break;
            case 'L': dx = -1; dy = 0; break;
            case 'R': dx = 1; dy = 0; break;
            case 'E': dx = 1; dy = -1; break;
            case 'F': dx = 1; dy = 1; break;
            case 'G': dx = -1; dy = 1; break;
            case 'H': dx = -1; dy = -1; break;
            case 'A': {
                if (!have_n || n < 0 || n > 3) { free(script); runtime_error(app, current_line, "DRAW bad angle"); return false; }
                angle = n & 3;
                continue;
            }
            case 'C': {
                if (!have_n || n < 0 || n > 15) { free(script); runtime_error(app, current_line, "Bad color"); return false; }
                color = n;
                continue;
            }
            case 'S': {
                if (!have_n || n < 1 || n > 255) { free(script); runtime_error(app, current_line, "Illegal function call"); return false; }
                scale = n;
                continue;
            }
            case 'M': {
                while (*q && isspace((unsigned char)*q)) q++;
                bool rel = false;
                if (*q == '+' || *q == '-') {
                    rel = true;
                }
                char *end = NULL;
                long xv = strtol(q, &end, 10);
                if (end == q) { free(script); runtime_error(app, current_line, "Syntax error"); return false; }
                q = end;
                while (*q && isspace((unsigned char)*q)) q++;
                if (*q != ',') { free(script); runtime_error(app, current_line, "Syntax error"); return false; }
                q++;
                while (*q && isspace((unsigned char)*q)) q++;
                long yv = strtol(q, &end, 10);
                if (end == q) { free(script); runtime_error(app, current_line, "Syntax error"); return false; }
                q = end;

                int nx, ny;
                if (rel) {
                    int rx = 0, ry = 0;
                    gfx_draw_rotate_quadrant(angle, (int)xv * scale, (int)yv * scale, &rx, &ry);
                    nx = x + rx;
                    ny = y + ry;
                } else {
                    nx = (int)xv;
                    ny = (int)yv;
                }
                gfx_draw_move(app, &x, &y, nx, ny, color, !blank);
                if (no_update) { x = oldx; y = oldy; }
                continue;
            }
            default:
                free(script);
                runtime_error(app, current_line, "Syntax error");
                return false;
        }

        int count = have_n ? n : 1;
        int rx = 0, ry = 0;
        gfx_draw_rotate_quadrant(angle, dx * scale * count, dy * scale * count, &rx, &ry);
        gfx_draw_move(app, &x, &y, x + rx, y + ry, color, !blank);
        if (no_update) { x = oldx; y = oldy; }
    }

    free(script);
    app->gfx_draw_x = x;
    app->gfx_draw_y = y;
    app->gfx_draw_scale = scale;
    app->gfx_draw_angle = angle;
    app->cur_fg = color;
    screen_render(app);
    return true;
}

typedef struct {
    Var *v;
    size_t start_off;
} GfxArrayRef;

static bool parse_gfx_array_ref(App *app, Parser *p, int current_line, GfxArrayRef *out) {
    if (!app || !p || !out) return false;
    memset(out, 0, sizeof(*out));

    char *name = NULL;
    if (!parse_identifier(p, &name)) {
        runtime_error(app, current_line, "GET/PUT expects array");
        return false;
    }
    if (ident_is_string_var(app, name)) {
        free(name);
        runtime_error(app, current_line, "Type mismatch");
        return false;
    }

    int nd = 0;
    int idx[5] = {0,0,0,0,0};
    bool have_idx = false;
    skip_ws(p);
    if (consume(p, '(')) {
        have_idx = true;
        if (!parse_array_indices(app, p, &nd, idx)) {
            free(name);
            runtime_error(app, current_line, "Bad array index");
            return false;
        }
    }

    Var *v = vars_lookup(app, name);
    free(name);
    if (!v || !v->is_array || !v->arr) {
        runtime_error(app, current_line, "GET/PUT expects numeric array");
        return false;
    }

    size_t start_off = 0;
    if (have_idx) {
        if (!array_calc_offset(v, nd, idx, &start_off)) {
            runtime_error(app, current_line, "Subscript out of range");
            return false;
        }
    }

    out->v = v;
    out->start_off = start_off;
    return true;
}

static bool exec_get_gfx(App *app, Parser *p, int current_line) {
    if (!app || !p) return false;
    if (!wbasic_ui_active(app)) {
        runtime_error(app, current_line, "Graphics not available in CLI/headless mode");
        return false;
    }
    if (!video_mode_is_graphics(app->video_mode)) {
        runtime_error(app, current_line, "GET requires graphics mode");
        return false;
    }

    skip_ws(p);
    if (!consume(p, '(')) { runtime_error(app, current_line, "GET expects (x1,y1)-(x2,y2),array"); return false; }

    double x1v = 0.0, y1v = 0.0, x2v = 0.0, y2v = 0.0;
    if (!parse_expr(app, p, &x1v)) { runtime_error(app, current_line, "GET expects x1"); return false; }
    if (!consume(p, ',')) { runtime_error(app, current_line, "GET expects ','"); return false; }
    if (!parse_expr(app, p, &y1v)) { runtime_error(app, current_line, "GET expects y1"); return false; }
    if (!consume(p, ')')) { runtime_error(app, current_line, "GET missing ')'" ); return false; }
    if (!consume(p, '-')) { runtime_error(app, current_line, "GET expects '-'" ); return false; }
    if (!consume(p, '(')) { runtime_error(app, current_line, "GET expects (x2,y2)"); return false; }
    if (!parse_expr(app, p, &x2v)) { runtime_error(app, current_line, "GET expects x2"); return false; }
    if (!consume(p, ',')) { runtime_error(app, current_line, "GET expects ','"); return false; }
    if (!parse_expr(app, p, &y2v)) { runtime_error(app, current_line, "GET expects y2"); return false; }
    if (!consume(p, ')')) { runtime_error(app, current_line, "GET missing ')'" ); return false; }
    if (!consume(p, ',')) { runtime_error(app, current_line, "GET expects array"); return false; }

    GfxArrayRef ar = {0};
    if (!parse_gfx_array_ref(app, p, current_line, &ar)) return false;

    skip_ws(p);
    if (*p->s != '\0') { runtime_error(app, current_line, "Syntax error"); return false; }

    int x1 = (int)llround(x1v);
    int y1 = (int)llround(y1v);
    int x2 = (int)llround(x2v);
    int y2 = (int)llround(y2v);

    int xmin = (x1 < x2) ? x1 : x2;
    int xmax = (x1 > x2) ? x1 : x2;
    int ymin = (y1 < y2) ? y1 : y2;
    int ymax = (y1 > y2) ? y1 : y2;

    if (xmin < 0) xmin = 0;
    if (ymin < 0) ymin = 0;
    if (xmax >= app->gfx_width) xmax = app->gfx_width - 1;
    if (ymax >= app->gfx_height) ymax = app->gfx_height - 1;
    if (xmin > xmax || ymin > ymax) { runtime_error(app, current_line, "Illegal function call"); return false; }

    int w = xmax - xmin + 1;
    int h = ymax - ymin + 1;
    size_t needed = (size_t)2 + (size_t)w * (size_t)h;
    if (ar.start_off + needed > ar.v->arr_total) {
        runtime_error(app, current_line, "GET/PUT array too small");
        return false;
    }

    ar.v->arr[ar.start_off] = (double)w;
    ar.v->arr[ar.start_off + 1] = (double)h;

    size_t out = ar.start_off + 2;
    for (int yy = ymin; yy <= ymax; yy++) {
        for (int xx = xmin; xx <= xmax; xx++) {
            int c = gfx_point(app, xx, yy);
            if (c < 0) c = 0;
            ar.v->arr[out++] = (double)(c & 0x0F);
        }
    }
    return true;
}

static bool exec_put_gfx(App *app, Parser *p, int current_line) {
    if (!app || !p) return false;
    if (!wbasic_ui_active(app)) {
        runtime_error(app, current_line, "Graphics not available in CLI/headless mode");
        return false;
    }
    if (!video_mode_is_graphics(app->video_mode)) {
        runtime_error(app, current_line, "PUT requires graphics mode");
        return false;
    }

    skip_ws(p);
    if (!consume(p, '(')) { runtime_error(app, current_line, "PUT expects (x,y),array"); return false; }

    double xv = 0.0, yv = 0.0;
    if (!parse_expr(app, p, &xv)) { runtime_error(app, current_line, "PUT expects x"); return false; }
    if (!consume(p, ',')) { runtime_error(app, current_line, "PUT expects ','"); return false; }
    if (!parse_expr(app, p, &yv)) { runtime_error(app, current_line, "PUT expects y"); return false; }
    if (!consume(p, ')')) { runtime_error(app, current_line, "PUT missing ')'" ); return false; }
    if (!consume(p, ',')) { runtime_error(app, current_line, "PUT expects array"); return false; }

    GfxArrayRef ar = {0};
    if (!parse_gfx_array_ref(app, p, current_line, &ar)) return false;

    int op_mode = 0; /* PSET/copy */
    skip_ws(p);
    if (consume(p, ',')) {
        skip_ws(p);
        if (starts_ci(p->s, "PSET") && is_word_boundary(p->s[4])) {
            op_mode = 0;
            p->s += 4;
        } else if (starts_ci(p->s, "OR") && is_word_boundary(p->s[2])) {
            op_mode = 1;
            p->s += 2;
        } else if (starts_ci(p->s, "AND") && is_word_boundary(p->s[3])) {
            op_mode = 2;
            p->s += 3;
        } else if (starts_ci(p->s, "XOR") && is_word_boundary(p->s[3])) {
            op_mode = 3;
            p->s += 3;
        } else {
            runtime_error(app, current_line, "PUT unsupported action");
            return false;
        }
    }

    skip_ws(p);
    if (*p->s != '\0') { runtime_error(app, current_line, "Syntax error"); return false; }

    if (ar.start_off + 2 > ar.v->arr_total) { runtime_error(app, current_line, "GET/PUT array too small"); return false; }
    int w = (int)llround(ar.v->arr[ar.start_off]);
    int h = (int)llround(ar.v->arr[ar.start_off + 1]);
    if (w <= 0 || h <= 0) { runtime_error(app, current_line, "Illegal function call"); return false; }

    size_t needed = (size_t)2 + (size_t)w * (size_t)h;
    if (ar.start_off + needed > ar.v->arr_total) { runtime_error(app, current_line, "GET/PUT array too small"); return false; }

    int x0 = (int)llround(xv);
    int y0 = (int)llround(yv);
    size_t in = ar.start_off + 2;

    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            int dstx = x0 + xx;
            int dsty = y0 + yy;
            int src = ((int)llround(ar.v->arr[in++])) & 0x0F;

            if (dstx < 0 || dsty < 0 || dstx >= app->gfx_width || dsty >= app->gfx_height) continue;

            int oldc = gfx_point(app, dstx, dsty);
            if (oldc < 0) oldc = 0;

            int outc = src;
            if (op_mode == 1) outc = (oldc | src) & 0x0F;
            else if (op_mode == 2) outc = (oldc & src) & 0x0F;
            else if (op_mode == 3) outc = (oldc ^ src) & 0x0F;

            (void)gfx_pset(app, dstx, dsty, outc);
        }
    }

    app->gfx_draw_x = x0;
    app->gfx_draw_y = y0;
    screen_render(app);
    return true;
}


static bool exec_circle_gfx(App *app, Parser *p, int current_line) {
    if (!app || !p) return false;
    if (!wbasic_ui_active(app)) {
        runtime_error(app, current_line, "Graphics not available in CLI/headless mode");
        return false;
    }
    if (!video_mode_is_graphics(app->video_mode)) {
        runtime_error(app, current_line, "CIRCLE requires graphics mode");
        return false;
    }

    skip_ws(p);
    if (!consume(p, '(')) { runtime_error(app, current_line, "CIRCLE expects (x,y),r"); return false; }

    double xv = 0.0, yv = 0.0, rv = 0.0;
    if (!parse_expr(app, p, &xv)) { runtime_error(app, current_line, "CIRCLE expects x"); return false; }
    if (!consume(p, ',')) { runtime_error(app, current_line, "CIRCLE expects ','"); return false; }
    if (!parse_expr(app, p, &yv)) { runtime_error(app, current_line, "CIRCLE expects y"); return false; }
    if (!consume(p, ')')) { runtime_error(app, current_line, "CIRCLE missing ')'" ); return false; }
    if (!consume(p, ',')) { runtime_error(app, current_line, "CIRCLE expects radius"); return false; }
    if (!parse_expr(app, p, &rv)) { runtime_error(app, current_line, "CIRCLE expects radius"); return false; }

    int color = (app->cur_fg >= 0 && app->cur_fg <= 15) ? app->cur_fg : 15;
    skip_ws(p);
    if (consume(p, ',')) {
        double cv = 0.0;
        if (!parse_expr(app, p, &cv)) { runtime_error(app, current_line, "CIRCLE expects color"); return false; }
        color = (int)llround(cv);
    }

    skip_ws(p);
    if (*p->s != '\0') { runtime_error(app, current_line, "Syntax error"); return false; }
    if (color < 0 || color > 15) { runtime_error(app, current_line, "Bad color"); return false; }

    int r = (int)llround(rv);
    if (r < 0) { runtime_error(app, current_line, "Illegal function call"); return false; }

    int x = (int)llround(xv);
    int y = (int)llround(yv);
    gfx_circle(app, x, y, r, color);
    screen_render(app);
    return true;
}


static bool exec_paint_gfx(App *app, Parser *p, int current_line) {
    if (!app || !p) return false;
    if (!wbasic_ui_active(app)) {
        runtime_error(app, current_line, "Graphics not available in CLI/headless mode");
        return false;
    }
    if (!video_mode_is_graphics(app->video_mode)) {
        runtime_error(app, current_line, "PAINT requires graphics mode");
        return false;
    }

    skip_ws(p);
    if (!consume(p, '(')) { runtime_error(app, current_line, "PAINT expects (x,y),color"); return false; }

    double xv = 0.0, yv = 0.0, cv = 0.0;
    bool have_border = false;
    int border = 0;
    if (!parse_expr(app, p, &xv)) { runtime_error(app, current_line, "PAINT expects x"); return false; }
    if (!consume(p, ',')) { runtime_error(app, current_line, "PAINT expects ','"); return false; }
    if (!parse_expr(app, p, &yv)) { runtime_error(app, current_line, "PAINT expects y"); return false; }
    if (!consume(p, ')')) { runtime_error(app, current_line, "PAINT missing ')'" ); return false; }
    if (!consume(p, ',')) { runtime_error(app, current_line, "PAINT expects color"); return false; }
    if (!parse_expr(app, p, &cv)) { runtime_error(app, current_line, "PAINT expects color"); return false; }

    /* Optional border color argument for boundary fill semantics. */
    skip_ws(p);
    if (consume(p, ',')) {
        double bv = 0.0;
        if (!parse_expr(app, p, &bv)) { runtime_error(app, current_line, "PAINT expects border"); return false; }
        border = (int)llround(bv);
        have_border = true;
    }

    skip_ws(p);
    if (*p->s != '\0') { runtime_error(app, current_line, "Syntax error"); return false; }

    int color = (int)llround(cv);
    if (color < 0 || color > 15) { runtime_error(app, current_line, "Bad color"); return false; }

    int x = (int)llround(xv);
    int y = (int)llround(yv);
    if (!gfx_paint(app, x, y, color, have_border, border)) {
        runtime_error(app, current_line, "Out of memory");
        return false;
    }

    app->gfx_draw_x = x;
    app->gfx_draw_y = y;
    screen_render(app);
    return true;
}

/* ---- File I/O helpers ---- */
static void files_close_all(App *app);
static BasicFile *file_get(App *app, int n);
static bool exec_defint(App *app, Parser *p, int current_line);
static bool exec_open(App *app, Parser *p, int current_line);
static bool exec_field(App *app, Parser *p, int current_line);
static bool exec_lset_rset(App *app, Parser *p, int current_line, bool right_justify);
static bool exec_get(App *app, Parser *p, int current_line);
static bool exec_put(App *app, Parser *p, int current_line);
static bool exec_seek(App *app, Parser *p, int current_line);
static bool exec_close(App *app, Parser *p, int current_line);
static bool exec_print_file(App *app, Parser *p, int current_line);
static bool exec_write(App *app, Parser *p, int current_line);
static bool exec_write_file(App *app, Parser *p, int current_line);
static bool exec_input_file(App *app, Parser *p, int current_line, bool line_mode);
static bool exec_statement_chain_from(App *app, const char *text, int current_line,
                                     int base_line_idx, int base_stmt_idx,
                                     int start_si,
                                     int *line_idx, int *stmt_idx) {
    int li_before = *line_idx;
    int si_before = *stmt_idx;

    StmtList sl = split_statements(text);
    if (sl.count <= 0) { stmtlist_free(&sl); return true; }
    if (start_si < 0) start_si = 0;
    if (start_si >= sl.count) { stmtlist_free(&sl); return true; }

    // Establish chain context so GOSUB can record a "return into chain" address.
    bool prev_chain_active = app->chain_active;
    int prev_chain_base_li = app->chain_base_line_idx;
    int prev_chain_base_si = app->chain_base_stmt_idx;
    const char *prev_chain_text = app->chain_text;

    app->chain_active = true;
    app->chain_base_line_idx = base_line_idx;
    app->chain_base_stmt_idx = base_stmt_idx;
    app->chain_text = text;

    for (int i = start_si; i < sl.count; i++) {
        // IMPORTANT: cur_si is the chain-local statement index (i),
        // but the "base_stmt_idx" is stored in app->chain_base_stmt_idx.
        /* Publish execution cursor so runtime_error can capture the exact failing chain statement. */
app->exec_cursor_valid = true;
app->exec_line_idx = base_line_idx;
app->exec_stmt_idx = i; /* chain-local statement index */
app->exec_line_no = current_line;

if (!exec_single_statement(app, sl.stmts[i], current_line, base_line_idx, i, line_idx, stmt_idx)) {
            app->chain_active = prev_chain_active;
            app->chain_base_line_idx = prev_chain_base_li;
            app->chain_base_stmt_idx = prev_chain_base_si;
            app->chain_text = prev_chain_text;
            stmtlist_free(&sl);
            return false;
        }

        // If statement changed flow (GOTO/GOSUB/etc), stop chaining here.
        if (*line_idx != li_before || *stmt_idx != si_before) {
            app->chain_active = prev_chain_active;
            app->chain_base_line_idx = prev_chain_base_li;
            app->chain_base_stmt_idx = prev_chain_base_si;
            app->chain_text = prev_chain_text;
            stmtlist_free(&sl);
            return true;
        }
    }

    // Restore and keep caller's indices unchanged (the caller is the owning statement).
    app->chain_active = prev_chain_active;
    app->chain_base_line_idx = prev_chain_base_li;
    app->chain_base_stmt_idx = prev_chain_base_si;
    app->chain_text = prev_chain_text;

    *line_idx = li_before;
    *stmt_idx = si_before;

    stmtlist_free(&sl);
    return true;
}

static bool exec_statement_chain(App *app, const char *text, int current_line, int *line_idx, int *stmt_idx) {
    return exec_statement_chain_from(app, text, current_line, *line_idx, *stmt_idx, 0, line_idx, stmt_idx);
}


/* ---- PRINT ---- */


#ifndef WBASIC_NO_UI
static bool exec_beep(App *app) {
    (void)app;
    GdkDisplay *d = gdk_display_get_default();
    if (d) gdk_display_beep(d);
    return true;
}
#else
static bool exec_beep(App *app) {
    (void)app;
    fputc('\a', stdout);
    fflush(stdout);
    return true;
}
#endif /* WBASIC_NO_UI */


static void print_spc(App *app, int n) {
    if (!app) return;
    if (n <= 0) return;
    if (n > 2000) n = 2000; // safety
    // emit spaces in chunks to avoid huge stack buffers
    while (n > 0) {
        int chunk = n;
        if (chunk > 256) chunk = 256;
        char buf[257];
        for (int i = 0; i < chunk; i++) buf[i] = ' ';
        buf[chunk] = 0;
        out_append(app, buf);
        n -= chunk;
    }
}

static void print_tab_to(App *app, int col) {
    if (!app) return;
    screen_ensure(app);
    if (!app->screen) return;

    /* TAB(n): absolute column positioning within the active text width.
     *
     * - n is normalized into a 1-based column via modulo screen width.
     * - if current column has already passed target, move to next line first.
     * - GUI sets the column directly; headless/CLI emits spaces to preserve stream output.
     */
    int width = (app->screen_cols > 0) ? app->screen_cols : 80;
    if (col < 1) col = 1;
    col = ((col - 1) % width) + 1;

    if (col < app->out_col) {
        out_append(app, "\n");
    }

    if (!wbasic_ui_active(app)) {
        /* Headless/CLI output is stream-oriented: TAB must be relative to current
           cursor position, so emit literal spaces instead of absolute ANSI moves. */
        int spaces = col - app->out_col;
        if (spaces > 0) print_spc(app, spaces);
    } else {
        app->out_col = col;
    }
    app->out_just_wrapped = false;
}

static void print_comma_zone(App *app) {
    if (!app) return;
    screen_ensure(app);
    if (!app->screen) return;

    // GW-BASIC print zones are 14 columns wide (1-based columns).
    const int zone = 14;
    int col = app->out_col;
    int next = ((col - 1) / zone + 1) * zone + 1;
    if (next > app->screen_cols) {
        out_append(app, "\n");
        return;
    }
    int spaces = next - app->out_col;
    if (spaces > 0) print_spc(app, spaces);
}

typedef enum {
    PRINT_USING_TOKEN_LIT = 0,
    PRINT_USING_TOKEN_NUM = 1,
    PRINT_USING_TOKEN_STR = 2
} PrintUsingTokenKind;

typedef struct {
    PrintUsingTokenKind kind;
    char *text;
    char str_mode;
    int str_width;
} PrintUsingToken;

static void print_using_tokens_free(PrintUsingToken *tokens, int count) {
    if (!tokens) return;
    for (int i = 0; i < count; i++) free(tokens[i].text);
    free(tokens);
}

static bool print_using_tokens_push(App *app, PrintUsingToken **tokens, int *count, int *cap,
                                    PrintUsingTokenKind kind, const char *src, int len) {
    if (!tokens || !count || !cap || !src || len < 0) return false;
    if (*count >= *cap) {
        int ncap = (*cap == 0) ? 8 : (*cap * 2);
        PrintUsingToken *n = (PrintUsingToken*)realloc(*tokens, sizeof(PrintUsingToken) * (size_t)ncap);
        if (!n) { err_set(app, "Out of memory"); return false; }
        *tokens = n;
        *cap = ncap;
    }
    char *dup = (char*)malloc((size_t)len + 1);
    if (!dup) { err_set(app, "Out of memory"); return false; }
    if (len > 0) memcpy(dup, src, (size_t)len);
    dup[len] = 0;
    (*tokens)[*count].kind = kind;
    (*tokens)[*count].text = dup;
    (*tokens)[*count].str_mode = 0;
    (*tokens)[*count].str_width = 0;
    (*count)++;
    return true;
}

static bool print_using_compile(App *app, const char *fmt, PrintUsingToken **out_tokens, int *out_count) {
    if (!out_tokens || !out_count) return false;
    *out_tokens = NULL;
    *out_count = 0;
    if (!fmt) return true;

    PrintUsingToken *tokens = NULL;
    int count = 0, cap = 0;
    int i = 0;
    while (fmt[i]) {
        if (fmt[i] == '!') {
            if (!print_using_tokens_push(app, &tokens, &count, &cap, PRINT_USING_TOKEN_STR, fmt + i, 1)) {
                print_using_tokens_free(tokens, count);
                return false;
            }
            tokens[count - 1].str_mode = '!';
            tokens[count - 1].str_width = 1;
            i++;
            continue;
        }

        if (fmt[i] == '&') {
            if (!print_using_tokens_push(app, &tokens, &count, &cap, PRINT_USING_TOKEN_STR, fmt + i, 1)) {
                print_using_tokens_free(tokens, count);
                return false;
            }
            tokens[count - 1].str_mode = '&';
            tokens[count - 1].str_width = 0;
            i++;
            continue;
        }

        if (fmt[i] == '\\') {
            int j = i + 1;
            while (fmt[j] && fmt[j] != '\\') j++;
            if (fmt[j] != '\\') {
                print_using_tokens_free(tokens, count);
                err_set(app, "Bad format string");
                return false;
            }
            if (!print_using_tokens_push(app, &tokens, &count, &cap, PRINT_USING_TOKEN_STR, fmt + i, j - i + 1)) {
                print_using_tokens_free(tokens, count);
                return false;
            }
            tokens[count - 1].str_mode = '\\';
            tokens[count - 1].str_width = (j - i - 1);
            i = j + 1;
            continue;
        }

        if (fmt[i] == '#' || fmt[i] == '.') {
            int j = i;
            int hashes = 0;
            while (fmt[j] == '#' || fmt[j] == '.') {
                if (fmt[j] == '#') hashes++;
                j++;
            }
            if (hashes <= 0) {
                print_using_tokens_free(tokens, count);
                err_set(app, "Bad format string");
                return false;
            }
            if (!print_using_tokens_push(app, &tokens, &count, &cap, PRINT_USING_TOKEN_NUM, fmt + i, j - i)) {
                print_using_tokens_free(tokens, count);
                return false;
            }
            i = j;
            continue;
        }

        int j = i;
        while (fmt[j] && fmt[j] != '#' && fmt[j] != '.' && fmt[j] != '!' && fmt[j] != '&' && fmt[j] != '\\') j++;
        if (!print_using_tokens_push(app, &tokens, &count, &cap, PRINT_USING_TOKEN_LIT, fmt + i, j - i)) {
            print_using_tokens_free(tokens, count);
            return false;
        }
        i = j;
    }

    *out_tokens = tokens;
    *out_count = count;
    return true;
}

static bool print_using_format_string(const PrintUsingToken *token, const char *sv, char **out) {
    if (!token || !out) return false;
    *out = NULL;
    const char *s = sv ? sv : "";
    if (token->str_mode == '&') {
        *out = xstrdup(s);
        return *out != NULL;
    }
    if (token->str_mode == '!') {
        char *buf = (char*)malloc(2);
        if (!buf) return false;
        buf[0] = s[0] ? s[0] : ' ';
        buf[1] = 0;
        *out = buf;
        return true;
    }
    if (token->str_mode == '\\') {
        int width = (token->str_width >= 0) ? token->str_width : 0;
        char *buf = (char*)malloc((size_t)width + 1);
        if (!buf) return false;
        for (int i = 0; i < width; i++) buf[i] = ' ';
        int slen = (int)strlen(s);
        int ncpy = (slen < width) ? slen : width;
        if (ncpy > 0) memcpy(buf, s, (size_t)ncpy);
        buf[width] = 0;
        *out = buf;
        return true;
    }
    return false;
}

static bool print_using_format_numeric(const char *mask, double v, char **out) {
    if (!mask || !out) return false;
    *out = NULL;

    int width = (int)strlen(mask);
    int dot_idx = -1;
    int int_slots = 0;
    int frac_slots = 0;
    for (int i = 0; i < width; i++) {
        if (mask[i] == '.') {
            if (dot_idx >= 0) return false;
            dot_idx = i;
        } else if (mask[i] == '#') {
            if (dot_idx < 0) int_slots++;
            else frac_slots++;
        } else {
            return false;
        }
    }

    double av = fabs(v);
    char numbuf[256];
    if (frac_slots > 0) snprintf(numbuf, sizeof(numbuf), "%.*f", frac_slots, av);
    else {
        double rv = round(av);
        snprintf(numbuf, sizeof(numbuf), "%.0f", rv);
    }

    const char *dot = strchr(numbuf, '.');
    int int_digits = dot ? (int)(dot - numbuf) : (int)strlen(numbuf);
    const char *frac_ptr = dot ? dot + 1 : "";

    /* For masks with no integer slots (e.g. ".##"), allow values in [0,1)
       to render without a leading zero. */
    bool suppress_leading_zero = (int_slots == 0 && int_digits == 1 && numbuf[0] == '0');
    int visible_int_digits = suppress_leading_zero ? 0 : int_digits;
    if (visible_int_digits > int_slots) {
        char *ov = (char*)malloc((size_t)width + 1);
        if (!ov) return false;
        for (int i = 0; i < width; i++) ov[i] = '%';
        ov[width] = 0;
        *out = ov;
        return true;
    }

    char *buf = (char*)malloc((size_t)width + 1);
    if (!buf) return false;
    for (int i = 0; i < width; i++) buf[i] = ' ';
    buf[width] = 0;

    int int_idx = suppress_leading_zero ? -1 : (int_digits - 1);
    int frac_len = (int)strlen(frac_ptr);
    int frac_idx = frac_len - 1;
    for (int i = width - 1; i >= 0; i--) {
        if (mask[i] == '#') {
            if (dot_idx >= 0 && i > dot_idx) {
                if (frac_idx >= 0) buf[i] = frac_ptr[frac_idx--];
                else buf[i] = '0';
            } else {
                if (int_idx >= 0) buf[i] = numbuf[int_idx--];
                else buf[i] = ' ';
            }
        } else if (mask[i] == '.') {
            buf[i] = '.';
        }
    }

    if (v < 0.0) {
        bool placed = false;
        for (int i = 0; i < width; i++) {
            if (buf[i] == ' ') { buf[i] = '-'; placed = true; break; }
            if (buf[i] >= '0' && buf[i] <= '9') break;
        }
        if (!placed) {
            free(buf);
            char *ov = (char*)malloc((size_t)width + 1);
            if (!ov) return false;
            for (int i = 0; i < width; i++) ov[i] = '%';
            ov[width] = 0;
            *out = ov;
            return true;
        }
    }

    *out = buf;
    return true;
}

static bool print_using_emit_literals_until_field(App *app, const PrintUsingToken *tokens,
                                                  int token_count, int *cursor,
                                                  PrintUsingTokenKind *found_kind) {
    if (!app || !tokens || token_count <= 0 || !cursor) return false;
    if (found_kind) *found_kind = PRINT_USING_TOKEN_LIT;

    int start = (*cursor % token_count + token_count) % token_count;
    for (int n = 0; n < token_count; n++) {
        int idx = (start + n) % token_count;
        if (tokens[idx].kind != PRINT_USING_TOKEN_LIT) {
            *cursor = idx;
            if (found_kind) *found_kind = tokens[idx].kind;
            return true;
        }
        out_append(app, tokens[idx].text ? tokens[idx].text : "");
        *cursor = (idx + 1) % token_count;
    }
    return false;
}


static void print_using_emit_literals_to_pass_end(App *app, const PrintUsingToken *tokens,
                                                  int token_count, int *cursor) {
    if (!app || !tokens || token_count <= 0 || !cursor) return;
    if (*cursor < 0 || *cursor >= token_count) return;

    for (int idx = *cursor; idx < token_count; idx++) {
        if (tokens[idx].kind != PRINT_USING_TOKEN_LIT) {
            *cursor = idx;
            return;
        }
        out_append(app, tokens[idx].text ? tokens[idx].text : "");
        *cursor = idx + 1;
    }
}

static bool exec_print_using(App *app, Parser *p, int current_line) {
    if (!app || !p) return false;

    skip_ws(p);
    char *fmt = NULL;
    if (!parse_string_value(app, p, &fmt)) {
        runtime_error_or_pending(app, current_line, "Bad format string");
        return false;
    }

    skip_ws(p);
    if (*p->s != ';' && *p->s != ',') {
        free(fmt);
        runtime_error(app, current_line, "Syntax error");
        return false;
    }
    p->s++;

    PrintUsingToken *tokens = NULL;
    int token_count = 0;
    if (!print_using_compile(app, fmt, &tokens, &token_count)) {
        free(fmt);
        runtime_error_or_pending(app, current_line, "Bad format string");
        return false;
    }
    free(fmt);

    int field_count = 0;
    for (int i = 0; i < token_count; i++) if (tokens[i].kind != PRINT_USING_TOKEN_LIT) field_count++;
    if (field_count <= 0) {
        print_using_tokens_free(tokens, token_count);
        runtime_error(app, current_line, "Bad format string");
        return false;
    }

    char last_sep = 0;
    int next_field = 0;
    for (;;) {
        skip_ws(p);
        if (*p->s == 0) break;

        const char *save = p->s;
        bool is_string_expr = false;
        char *sv = NULL;
        double nv = 0.0;
        if (parse_string_value(app, p, &sv)) {
            is_string_expr = true;
        } else {
            p->s = save;
            if (!parse_expr(app, p, &nv)) {
                print_using_tokens_free(tokens, token_count);
                return false;
            }
        }

        PrintUsingTokenKind got_kind = PRINT_USING_TOKEN_LIT;
        if (!print_using_emit_literals_until_field(app, tokens, token_count, &next_field, &got_kind)) {
            print_using_tokens_free(tokens, token_count);
            runtime_error(app, current_line, "Bad format string");
            free(sv);
            return false;
        }

        int idx = next_field;
        if ((is_string_expr && got_kind != PRINT_USING_TOKEN_STR) || (!is_string_expr && got_kind != PRINT_USING_TOKEN_NUM)) {
            print_using_tokens_free(tokens, token_count);
            runtime_error(app, current_line, "Type mismatch");
            free(sv);
            return false;
        }

        char *formatted = NULL;
        bool ok = false;
        if (is_string_expr) ok = print_using_format_string(&tokens[idx], sv, &formatted);
        else ok = print_using_format_numeric(tokens[idx].text, nv, &formatted);
        if (!ok) {
            print_using_tokens_free(tokens, token_count);
            runtime_error(app, current_line, "Bad format string");
            free(sv);
            return false;
        }
        out_append(app, formatted ? formatted : "");
        free(formatted);
        free(sv);
        next_field = idx + 1;
        if (next_field >= token_count) next_field = 0;

        skip_ws(p);
        if (*p->s == ',') {
            p->s++;
            print_comma_zone(app);
            last_sep = ',';
            continue;
        }
        if (*p->s == ';') { p->s++; last_sep = ';'; continue; }
        last_sep = 0;
        break;
    }

    if (next_field > 0) print_using_emit_literals_to_pass_end(app, tokens, token_count, &next_field);
    print_using_tokens_free(tokens, token_count);
    if (last_sep == 0) out_append(app, "\n");
    return true;
}

static bool exec_print(App *app, Parser *p, int current_line) {
    // GW-BASIC-ish PRINT semantics (text mode):
    // - PRINT with no args prints a newline.
    // - Newline is printed unless the *final* separator is ';' or ','.
    // - ';' concatenates (no spacing).
    // - ',' advances to the next print zone (14-column zones in GW-BASIC).
    // - TAB(n) positions to column n (1-based) within the current line; it does not move backward.
    // - SPC(n) prints n spaces.
    // - Empty items are allowed (e.g., PRINT ,,, or PRINT ;;;).

    if (!app || !p) return false;

    skip_ws(p);
    if (starts_ci(p->s, "USING") && is_word_boundary(p->s[5])) {
        consume_word_ci(p, "USING");
        return exec_print_using(app, p, current_line);
    }

    char last_sep = 0;
    app->out_just_wrapped = false;

    for (;;) {
        bool allow_implicit_next_item = false;
        skip_ws(p);
        if (*p->s == 0) break;

        // Empty items: PRINT ,,, or PRINT ;;;
        if (*p->s == ',') {
            p->s++;
            print_comma_zone(app);
            last_sep = ',';
            continue;
        }
        if (*p->s == ';') {
            p->s++;
            last_sep = ';';
            continue;
        }

        // TAB(n) and SPC(n) are PRINT list functions
        if (starts_ci(p->s, "TAB") && is_word_boundary(p->s[3])) {
            consume_word_ci(p, "TAB");
            skip_ws(p);
            if (!consume(p, '(')) { runtime_error(app, current_line, "Syntax error"); return false; }
            double v = 0.0;
            if (!parse_expr(app, p, &v)) return false;
            skip_ws(p);
            if (!consume(p, ')')) { runtime_error(app, current_line, "Syntax error"); return false; }

            /* GW-BASIC compatibility:
             *  - TAB rounds non-integers to the nearest integer.
             *  - TAB(n<1) behaves as TAB(1).
             *  - TAB does not move backwards.
             *  - TAB uses absolute positioning with width-based modulo normalization.
             */
            long long colll = llround(v);
            int col = (colll < 1) ? 1 : (int)colll;
            print_tab_to(app, col);
            last_sep = 0;
            allow_implicit_next_item = true;
        } else if (starts_ci(p->s, "SPC") && is_word_boundary(p->s[3])) {
            consume_word_ci(p, "SPC");
            skip_ws(p);
            if (!consume(p, '(')) { runtime_error(app, current_line, "Syntax error"); return false; }
            double v = 0.0;
            if (!parse_expr(app, p, &v)) return false;
            skip_ws(p);
            if (!consume(p, ')')) { runtime_error(app, current_line, "Syntax error"); return false; }

            /* GW-BASIC compatibility: SPC rounds to nearest integer and clamps n<0 to 0. */
            long long nll = llround(v);
            int n = (nll < 0) ? 0 : (int)nll;
            print_spc(app, n);
            last_sep = 0;
            allow_implicit_next_item = true;
        } else {
            // Normal PRINT item: string or numeric
            const char *save = p->s;
            char *sv = NULL;
            if (parse_string_value(app, p, &sv)) {
                out_append(app, sv);
                free(sv);
            } else {
                p->s = save;
                double nv = 0.0;
                if (!parse_expr(app, p, &nv)) return false;
                double r = round(nv);
                if (fabs(nv - r) < 1e-12) out_printf(app, "%.0f", r);
                else out_printf(app, "%.12g", nv);
            }
            last_sep = 0;
        }

        skip_ws(p);
        if (*p->s == ',') {
            p->s++;
            print_comma_zone(app);
            last_sep = ',';
            continue;
        }
        if (*p->s == ';') {
            p->s++;
            last_sep = ';';
            continue;
        }

        if (*p->s != 0 &&
            (allow_implicit_next_item ||
             (starts_ci(p->s, "TAB") && is_word_boundary(p->s[3])) ||
             (starts_ci(p->s, "SPC") && is_word_boundary(p->s[3])))) {
            // GW-BASIC accepts adjacent print-list TAB/SPC items with implied ';'.
            continue;
        }

        // No trailing separator => newline
        break;
    }

    if (last_sep == 0) {
        if (app->out_just_wrapped) app->out_just_wrapped = false;
        else out_append(app, "\n");
    }

    /* Phase 3: one delay per PRINT statement (screen only).
     * - independent of printed length
     * - not applied to PRINT # (handled in exec_print_file)
     * - not applied during INPUT prompting/wait
     */
    if (app && !app->input_waiting) {
        /* Fractional-ms accumulation to avoid cliffs around the fast end:
         * accumulate desired delay and sleep only when we reach >= 1ms. */
        double want_ms = wbasic_compute_print_delay_ms_f_from_output_speed(app->output_speed);
        app->print_throttle_carry_ms += want_ms;
        int sleep_ms = (int)floor(app->print_throttle_carry_ms);
        if (sleep_ms > 0) {
            app->print_throttle_carry_ms -= (double)sleep_ms;
            wbasic_delay_ms(sleep_ms, &app->tickle);
        }
    }

    return true;
}

static bool exec_speed(App *app, Parser *p, int current_line);


/* ---- SPEED (custom) ---- */

static bool exec_speed(App *app, Parser *p, int current_line)
{
    // SPEED n : n=1 (slowest) .. 100 (fastest)
    skip_ws(p);
    if (*p->s == 0) {
        runtime_error(app, current_line, "SPEED expects 1-100");
        return false;
    }

    double v = 0.0;
    if (!parse_expr(app, p, &v)) {
        runtime_error(app, current_line, "SPEED expects number");
        return false;
    }

    int n = (int)llround(v);
    if (n < 1) n = 1;
    if (n > 100) n = 100;

    if (app) {
        app->output_speed = (double)n / 100.0;
        app->print_throttle_carry_ms = 0.0;
    }
    return true;
}

/* ---- LOCATE ---- */


static bool exec_locate(App *app, Parser *p, int current_line) {
    // GW-BASIC: LOCATE row, col [, cursor]
    // With the screen-buffer output model, LOCATE can move freely and supports overwrite.
    skip_ws(p);
    if (*p->s == 0) { runtime_error(app, current_line, "LOCATE expects row,col"); return false; }

    double r = 0.0, c = 0.0;
    if (!parse_expr(app, p, &r)) { runtime_error(app, current_line, "LOCATE expects row"); return false; }
    skip_ws(p);
    if (*p->s != ',') { runtime_error(app, current_line, "LOCATE expects ','"); return false; }
    p->s++;
    // Accept optional whitespace after comma (e.g., LOCATE 2, 20)
    skip_ws(p);
    if (!parse_expr(app, p, &c)) { runtime_error(app, current_line, "LOCATE expects col"); return false; }

    // Optional cursor param: 0 hides cursor, nonzero shows cursor (TTY/headless only).
    bool have_cursor = false;
    int cursor_vis = 1;
    skip_ws(p);
    if (*p->s == ',') {
        p->s++;
        double vcur = 0.0;
        (void)parse_expr(app, p, &vcur);
        have_cursor = true;
        cursor_vis = ((int)llround(vcur) != 0) ? 1 : 0;
    }

    int row = (int)llround(r);
    int col = (int)llround(c);
    if (row < 1 || col < 1) { runtime_error(app, current_line, "Illegal function call"); return false; }

    if (!app) return false;
    screen_ensure(app);
    if (!app->screen) return false;

    // Clamp within our virtual screen.
    if (row > app->screen_rows) row = app->screen_rows;
    if (col > app->screen_cols) col = app->screen_cols;

    app->out_row = row;
    app->out_col = col;

    if (!wbasic_ui_active(app)) {
        /* Cursor positioning is meaningful only for terminal/CLI output. */
        app->headless_cursor_dirty = true;

        if (headless_stdout_is_tty()) {
            headless_stdout_prepare_ansi();

            /* Apply cursor visibility if requested. */
            if (have_cursor) {
                if (cursor_vis) fputs("\x1b[?25h", stdout);
                else            fputs("\x1b[?25l", stdout);
            }

            /* Move cursor immediately. */
            headless_ansi_move(app->out_row, app->out_col);
            app->headless_cursor_dirty = false;

            /* After LOCATE, do not disturb cached color; LOCATE must not reset attributes. */
            /* (LOCATE fix) Do NOT reset ANSI attributes/color cache here. */
            fflush(stdout);
        }
    } else {
        (void)have_cursor;
        (void)cursor_vis;
    }
    return true;
}

static bool exec_color(App *app, Parser *p, int current_line) {
    // GW-BASIC: COLOR [fg][,[bg][,[border]]]
    // Text-mode only for now. Border (3rd arg) is accepted but ignored.
    if (!app || !p) return false;

    // No arguments -> reset defaults
    const char *s = p->s;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') {
        app->cur_fg = 16;
        app->cur_bg = 16;
        if (!wbasic_ui_active(app) && headless_stdout_is_tty()) {
            headless_stdout_prepare_ansi();
            /* COLOR with no args => ANSI reset */
            fputs("\x1b[0m", stdout);
        }
        // No need to re-render: COLOR affects subsequent output only.
        return true;
    }

    int new_fg = app->cur_fg;
    int new_bg = app->cur_bg;

    bool have_fg = false;
    bool have_bg = false;

    // fg may be omitted if first non-space is comma
    p->s = s;
    if (*p->s != ',') {
        double v = 0.0;
        if (!parse_expr(app, p, &v)) { runtime_error(app, current_line, "Syntax error"); return false; }
        new_fg = (int)v;
        have_fg = true;
    }

    // optional ,bg
    while (*p->s && isspace((unsigned char)*p->s)) p->s++;
    if (*p->s == ',') {
        p->s++;
        while (*p->s && isspace((unsigned char)*p->s)) p->s++;
        if (*p->s != ',' && *p->s != '\0') {
            double v = 0.0;
            if (!parse_expr(app, p, &v)) { runtime_error(app, current_line, "Syntax error"); return false; }
            new_bg = (int)v;
            have_bg = true;
        }
        // optional ,border (ignored)
        while (*p->s && isspace((unsigned char)*p->s)) p->s++;
        if (*p->s == ',') {
            p->s++;
            while (*p->s && isspace((unsigned char)*p->s)) p->s++;
            if (*p->s != '\0') {
                double dummy = 0.0;
                if (!parse_expr(app, p, &dummy)) { runtime_error(app, current_line, "Syntax error"); return false; }
            }
        }
    }

    if (have_fg) {
        /*
           GW-BASIC accepts foreground values 0..31 for COLOR attributes
           (high bit range carries intensity/blink semantics).
           WBASIC stores the full attribute value for compatibility.
        */
        if (new_fg < 0 || new_fg > 31) { runtime_error(app, current_line, "Bad color"); return false; }
        app->cur_fg = new_fg;
    }
    if (have_bg) {
        if (new_bg < 0 || new_bg > 15) { runtime_error(app, current_line, "Bad color"); return false; }
        app->cur_bg = new_bg;
    }

    // Changing COLOR affects subsequent output; existing buffer remains as-is.
    return true;
}

static bool exec_screen(App *app, Parser *p, int current_line) {
    if (!app || !p) return false;
    skip_ws(p);

    double mode_v = 0.0;
    if (!parse_expr(app, p, &mode_v)) {
        runtime_error(app, current_line, "SCREEN expects mode");
        return false;
    }
    int mode = (int)llround(mode_v);

    /*
       GW-BASIC accepts optional SCREEN arguments:
         SCREEN mode[,colorburst][,apage][,vpage]
       For compatibility we parse (and currently ignore) up to three optional
       numeric expressions after mode.
    */
    skip_ws(p);
    int opt_count = 0;
    while (*p->s == ',') {
        p->s++;
        opt_count++;
        if (opt_count > 3) {
            runtime_error(app, current_line, "Syntax error");
            return false;
        }

        skip_ws(p);
        if (*p->s == ',' || *p->s == '\0') {
            /* Empty optional argument is allowed (e.g., SCREEN 0,,0) */
            continue;
        }

        double ignored = 0.0;
        if (!parse_expr(app, p, &ignored)) {
            runtime_error(app, current_line, "Syntax error");
            return false;
        }
        skip_ws(p);
    }

    if (*p->s != '\0') {
        runtime_error(app, current_line, "Syntax error");
        return false;
    }

    const ScreenModeSpec *spec = screen_mode_spec_find(mode);
    if (!spec) {
        runtime_error(app, current_line, "Unsupported SCREEN mode");
        return false;
    }

    if ((spec->policy_flags & SCREEN_POLICY_REQUIRES_UI) && !wbasic_ui_active(app)) {
        runtime_error(app, current_line, "Graphics not available in CLI/headless mode");
        return false;
    }

    if ((spec->policy_flags & SCREEN_POLICY_ALLOC_GFX) && !gfx_alloc(app, spec->w, spec->h)) {
        runtime_error(app, current_line, "Out of memory");
        return false;
    }

    app->video_mode = (WbVideoMode)spec->mode;
    if (spec->policy_flags & SCREEN_POLICY_ALLOC_GFX) {
        gfx_clear(app, (unsigned char)((app->cur_bg >= 0) ? app->cur_bg : 0));
        gfx_draw_reset_defaults(app);
    }
    screen_clear(app);
    screen_render(app);
    return true;
}

static bool exec_pset(App *app, Parser *p, int current_line) {
    if (!app || !p) return false;
    if (!wbasic_ui_active(app)) {
        runtime_error(app, current_line, "Graphics not available in CLI/headless mode");
        return false;
    }
    if (!video_mode_is_graphics(app->video_mode)) {
        runtime_error(app, current_line, "PSET requires graphics mode");
        return false;
    }

    skip_ws(p);
    if (!consume(p, '(')) { runtime_error(app, current_line, "PSET expects (x,y)"); return false; }

    double xv = 0.0, yv = 0.0;
    if (!parse_expr(app, p, &xv)) { runtime_error(app, current_line, "PSET expects x"); return false; }
    if (!consume(p, ',')) { runtime_error(app, current_line, "PSET expects ','"); return false; }
    if (!parse_expr(app, p, &yv)) { runtime_error(app, current_line, "PSET expects y"); return false; }
    if (!consume(p, ')')) { runtime_error(app, current_line, "PSET missing ')'"); return false; }

    int color = (app->cur_fg >= 0 && app->cur_fg <= 15) ? app->cur_fg : 15;
    skip_ws(p);
    if (consume(p, ',')) {
        double cv = 0.0;
        if (!parse_expr(app, p, &cv)) { runtime_error(app, current_line, "PSET expects color"); return false; }
        color = (int)llround(cv);
    }
    skip_ws(p);
    if (*p->s != '\0') { runtime_error(app, current_line, "Syntax error"); return false; }
    if (color < 0 || color > 15) { runtime_error(app, current_line, "Bad color"); return false; }

    int x = (int)llround(xv);
    int y = (int)llround(yv);
    bool ok = gfx_pset(app, x, y, color);
    if (ok) {
        app->gfx_draw_x = x;
        app->gfx_draw_y = y;
        screen_render(app);
    }
    return ok;
}

static bool exec_line_gfx(App *app, Parser *p, int current_line) {
    if (!app || !p) return false;
    if (!wbasic_ui_active(app)) {
        runtime_error(app, current_line, "Graphics not available in CLI/headless mode");
        return false;
    }
    if (!video_mode_is_graphics(app->video_mode)) {
        runtime_error(app, current_line, "LINE requires graphics mode");
        return false;
    }

    skip_ws(p);
    if (!consume(p, '(')) { runtime_error(app, current_line, "LINE expects (x1,y1)-(x2,y2)"); return false; }

    double x1v = 0.0, y1v = 0.0, x2v = 0.0, y2v = 0.0;
    if (!parse_expr(app, p, &x1v)) { runtime_error(app, current_line, "LINE expects x1"); return false; }
    if (!consume(p, ',')) { runtime_error(app, current_line, "LINE expects ','"); return false; }
    if (!parse_expr(app, p, &y1v)) { runtime_error(app, current_line, "LINE expects y1"); return false; }
    if (!consume(p, ')')) { runtime_error(app, current_line, "LINE missing ')'" ); return false; }
    skip_ws(p);
    if (!consume(p, '-')) { runtime_error(app, current_line, "LINE expects '-'" ); return false; }
    skip_ws(p);
    if (!consume(p, '(')) { runtime_error(app, current_line, "LINE expects (x2,y2)"); return false; }
    if (!parse_expr(app, p, &x2v)) { runtime_error(app, current_line, "LINE expects x2"); return false; }
    if (!consume(p, ',')) { runtime_error(app, current_line, "LINE expects ','"); return false; }
    if (!parse_expr(app, p, &y2v)) { runtime_error(app, current_line, "LINE expects y2"); return false; }
    if (!consume(p, ')')) { runtime_error(app, current_line, "LINE missing ')'" ); return false; }

    int color = (app->cur_fg >= 0 && app->cur_fg <= 15) ? app->cur_fg : 15;
    int draw_mode = 0; /* 0=line, 1=box, 2=boxfill */

    skip_ws(p);
    if (consume(p, ',')) {
        bool color_set = false;
        bool mode_set = false;

        skip_ws(p);

        bool color_omitted = false;

        /* Optional [attribute], including omitted attribute in forms like LINE ...,,B. */
        if (consume(p, ',')) {
            color_omitted = true;
        } else {
            const char *t = p->s;
            if ((t[0] == 'B' || t[0] == 'b') && (t[1] == 'F' || t[1] == 'f') && is_word_boundary(t[2])) {
                draw_mode = 2;
                mode_set = true;
                p->s += 2;
            } else if ((t[0] == 'B' || t[0] == 'b') && is_word_boundary(t[1])) {
                draw_mode = 1;
                mode_set = true;
                p->s += 1;
            } else {
                double cv = 0.0;
                if (!parse_expr(app, p, &cv)) { runtime_error(app, current_line, "LINE expects color"); return false; }
                color = (int)llround(cv);
                color_set = true;
            }
        }

        /* Omitted color form: LINE ...,,B / LINE ...,,BF */
        if (color_omitted && !mode_set) {
            skip_ws(p);
            const char *t = p->s;
            if ((t[0] == 'B' || t[0] == 'b') && (t[1] == 'F' || t[1] == 'f') && is_word_boundary(t[2])) {
                draw_mode = 2;
                mode_set = true;
                p->s += 2;
            } else if ((t[0] == 'B' || t[0] == 'b') && is_word_boundary(t[1])) {
                draw_mode = 1;
                mode_set = true;
                p->s += 1;
            }
        }

        skip_ws(p);
        if (consume(p, ',')) {
            skip_ws(p);

            if (!mode_set) {
                const char *t = p->s;
                if ((t[0] == 'B' || t[0] == 'b') && (t[1] == 'F' || t[1] == 'f') && is_word_boundary(t[2])) {
                    draw_mode = 2;
                    mode_set = true;
                    p->s += 2;
                } else if ((t[0] == 'B' || t[0] == 'b') && is_word_boundary(t[1])) {
                    draw_mode = 1;
                    mode_set = true;
                    p->s += 1;
                } else {
                    double stylev = 0.0;
                    if (!parse_expr(app, p, &stylev)) { runtime_error(app, current_line, "Syntax error"); return false; }
                (void)stylev;
                }
            } else {
                double stylev = 0.0;
                if (!parse_expr(app, p, &stylev)) { runtime_error(app, current_line, "Syntax error"); return false; }
                (void)stylev;
            }

            skip_ws(p);
            if (consume(p, ',')) {
                double stylev = 0.0;
                if (!parse_expr(app, p, &stylev)) { runtime_error(app, current_line, "Syntax error"); return false; }
                (void)stylev;
            }
        } else if (!color_set && mode_set) {
            /* LINE ...,B (or BF) with no style */
        }
    }

    skip_ws(p);
    if (*p->s != '\0') { runtime_error(app, current_line, "Syntax error"); return false; }
    if (color < 0 || color > 15) { runtime_error(app, current_line, "Bad color"); return false; }

    int x1 = (int)llround(x1v);
    int y1 = (int)llround(y1v);
    int x2 = (int)llround(x2v);
    int y2 = (int)llround(y2v);

    if (draw_mode == 0) {
        gfx_line(app, x1, y1, x2, y2, color);
    } else {
        int xmin = (x1 < x2) ? x1 : x2;
        int xmax = (x1 > x2) ? x1 : x2;
        int ymin = (y1 < y2) ? y1 : y2;
        int ymax = (y1 > y2) ? y1 : y2;

        if (draw_mode == 1) {
            gfx_line(app, xmin, ymin, xmax, ymin, color);
            gfx_line(app, xmax, ymin, xmax, ymax, color);
            gfx_line(app, xmax, ymax, xmin, ymax, color);
            gfx_line(app, xmin, ymax, xmin, ymin, color);
        } else {
            for (int y = ymin; y <= ymax; y++) {
                gfx_line(app, xmin, y, xmax, y, color);
            }
        }
    }

    app->gfx_draw_x = x2;
    app->gfx_draw_y = y2;
    screen_render(app);
    return true;
}

/* ---- File I/O helpers ---- */

static BasicFile *file_get(App *app, int n) {
    if (n < 0 || n >= BASIC_MAX_FILES) return NULL;
    return &app->files[n];
}

static void file_close_one(BasicFile *bf) {
    if (!bf) return;
    if (bf->fp) fclose(bf->fp);
    bf->fp = NULL;
    bf->mode = BF_CLOSED;
    bf->eof_latched = false;
}

static void files_close_all(App *app) {
    if (!app) return;
    for (int i = 0; i < BASIC_MAX_FILES; i++) file_close_one(&app->files[i]);
}

static double func_eof(App *app, int n) {
    BasicFile *bf = file_get(app, n);
    if (!bf || !bf->fp || bf->mode != BF_INPUT) return -1.0;
    if (bf->eof_latched) return -1.0;
    int c = fgetc(bf->fp);
    if (c == EOF) { bf->eof_latched = true; return -1.0; }
    ungetc(c, bf->fp);
    return 0.0;
}

static bool exec_open(App *app, Parser *p, int current_line) {
    skip_ws(p);
    char *path = NULL;
    /*
     * GW-BASIC allows OPEN to take a filename expression, including a plain
     * string variable like FN$.
     *
     * In normal operation we parse a full string expression. However, OPEN is
     * common enough (and used heavily by the torture suite) that we include a
     * conservative fallback: if the general string-expression parser fails,
     * accept a bare string variable token and use its current value.
     */
    {
        const char *save = p->s;
        if (!parse_string_value(app, p, &path)) {
            p->s = save;
            char *name = NULL;
            if (parse_identifier(p, &name) && name && name_is_string(name)) {
                /* Only treat as a variable if this isn't immediately a call (FNx$(...)). */
                const char *after = p->s;
                Parser tmp = *p;
                skip_ws(&tmp);
                if (peek(&tmp) != '(') {
                    Var *v = vars_get_or_create(app, name);
                    path = xstrdup((v && v->kind == V_STR && v->str) ? v->str : "");
                    free(name);
                } else {
                    p->s = after;
                    free(name);
                }
            } else {
                if (name) free(name);
            }

            if (!path) {
                runtime_error(app, current_line, "OPEN requires filename");
                return false;
            }
        }
    }

    if (!consume_word_ci(p, "FOR")) {
        free(path);
        runtime_error(app, current_line, "OPEN missing FOR");
        return false;
    }

    BasicFileMode mode = BF_CLOSED;
    if (consume_word_ci(p, "INPUT")) mode = BF_INPUT;
    else if (consume_word_ci(p, "OUTPUT")) mode = BF_OUTPUT;
    else if (consume_word_ci(p, "APPEND")) mode = BF_APPEND;
    else if (consume_word_ci(p, "RANDOM")) mode = BF_RANDOM;
    else {
        free(path);
        runtime_error(app, current_line, "OPEN mode must be INPUT/OUTPUT/APPEND/RANDOM");
        return false;
    }

    if (!consume_word_ci(p, "AS")) {
        free(path);
        runtime_error(app, current_line, "OPEN missing AS");
        return false;
    }

    skip_ws(p);
    consume(p, '#');

    double hv = 0.0;
    if (!parse_expr(app, p, &hv)) {
        free(path);
        runtime_error(app, current_line, "Bad file handle");
        return false;
    }
    int h = (int)llround(hv);
    if (h <= 0 || h >= BASIC_MAX_FILES) {
        free(path);
        runtime_error(app, current_line, "File handle out of range");
        return false;
    }

    int rec_len = 0;
    if (mode == BF_RANDOM) {
        // GW-BASIC style: OPEN "file" FOR RANDOM AS #n LEN=<reclen>
        skip_ws(p);
        if (consume_word_ci(p, "LEN")) {
            skip_ws(p);
            if (!consume(p, '=')) {
                free(path);
                runtime_error(app, current_line, "OPEN RANDOM missing LEN=");
                return false;
            }
            double lv = 0.0;
            if (!parse_expr(app, p, &lv)) {
                free(path);
                runtime_error(app, current_line, "Bad LEN value");
                return false;
            }
            rec_len = (int)llround(lv);
        }
        if (rec_len <= 0) {
            free(path);
            runtime_error(app, current_line, "OPEN RANDOM requires LEN=<record_length>");
            return false;
        }
    }

    BasicFile *bf = file_get(app, h);
    // GW-BASIC: OPEN on an already-open file handle raises ERR 55 (File already open).
    if (bf && bf->fp) {
        free(path);
        runtime_error(app, current_line, "File already open");
        return false;
    }

    const char *fmode = (mode == BF_INPUT)  ? "r"  :
                       (mode == BF_OUTPUT) ? "w"  :
                       (mode == BF_APPEND) ? "a"  :
                       /* BF_RANDOM */        "r+b";

    // Resolve relative paths against the directory of the currently loaded BASIC program.
    // This matches typical GW-BASIC expectations (SAVE/LOAD files live alongside the .BAS).
    char *open_path = NULL;
    if (path && path[0] == '~') {
        const char *home = g_get_home_dir();
        if (path[1] == '/') open_path = g_build_filename(home, path + 2, NULL);
        else open_path = g_build_filename(home, path + 1, NULL);
    } else if (path && path[0] != '/' && app->current_path && app->current_path[0]) {
        char *dir = g_path_get_dirname(app->current_path);
        open_path = g_build_filename(dir, path, NULL);
        g_free(dir);
    } else {
        open_path = g_strdup(path ? path : "");
    }

    bf->fp = fopen(open_path, fmode);
    if (!bf->fp && mode == BF_RANDOM) {
        // Create if missing (read/write binary)
        bf->fp = fopen(open_path, "w+b");
    }
    g_free(open_path);
    if (!bf->fp) {
        free(path);
        runtime_error(app, current_line, "Cannot open file");
        return false;
    }

    bf->mode = mode;
    bf->record_len = (mode == BF_RANDOM) ? rec_len : 0;
    if (mode == BF_RANDOM) {
        field_clear(bf);
        if (bf->record_buf) { free(bf->record_buf); bf->record_buf = NULL; }
        bf->record_buf = (unsigned char*)calloc((size_t)rec_len, 1);
        if (!bf->record_buf) { free(path); runtime_error(app, current_line, "Out of memory"); return false; }
    } else {
        if (bf->record_buf) { free(bf->record_buf); bf->record_buf = NULL; }
        field_clear(bf);
    }
    bf->eof_latched = false;

    free(path);
    return true;
}

// Phase 2: SEEK #n, pos
// - For RANDOM files: pos is 1-based record number; seeks to (pos-1)*record_len
// - For others: pos is 1-based byte position; seeks to (pos-1)
static bool exec_seek(App *app, Parser *p, int current_line) {
    skip_ws(p);
    if (!consume(p, '#')) { runtime_error(app, current_line, "SEEK requires #n"); return false; }
    double hv = 0.0;
    if (!parse_expr(app, p, &hv)) { runtime_error(app, current_line, "Bad file handle"); return false; }
    int h = (int)llround(hv);
    if (h <= 0 || h >= BASIC_MAX_FILES || !app->files[h].fp) { runtime_error(app, 0, "Invalid file handle"); return false; }

    skip_ws(p);
    if (!consume(p, ',')) { runtime_error(app, current_line, "SEEK requires #n, pos"); return false; }

    double pv = 0.0;
    if (!parse_expr(app, p, &pv)) { runtime_error(app, current_line, "Bad SEEK position"); return false; }

    BasicFile *bf = &app->files[h];
    long pos = 0;
    if (bf->mode == BF_RANDOM) {
        if (bf->record_len <= 0) { runtime_error(app, current_line, "RANDOM file missing LEN"); return false; }
        pos = (long)((pv - 1.0) * (double)bf->record_len);
    } else {
        pos = (long)pv - 1;
    }
    if (pos < 0) pos = 0;

    if (fseek(bf->fp, pos, SEEK_SET) != 0) { runtime_error(app, current_line, "SEEK failed"); return false; }
    bf->eof_latched = false;
    field_sync_vars(app, bf);
    return true;
}

// Phase 3: GET #n, rec  (RANDOM files)
static bool exec_get(App *app, Parser *p, int current_line) {
    skip_ws(p);
    if (!consume(p, '#')) { runtime_error(app, current_line, "GET requires #n, rec"); return false; }
    double hv = 0.0;
    if (!parse_expr(app, p, &hv)) { runtime_error(app, current_line, "Bad file handle"); return false; }
    int h = (int)llround(hv);
    if (h <= 0 || h >= BASIC_MAX_FILES || !app->files[h].fp) { runtime_error(app, current_line, "Invalid file handle"); return false; }

    skip_ws(p);
    if (!consume(p, ',')) { runtime_error(app, current_line, "GET requires #n, rec"); return false; }

    double rv = 0.0;
    if (!parse_expr(app, p, &rv)) { runtime_error(app, current_line, "Bad record number"); return false; }
    long rec = (long)llround(rv);
    if (rec <= 0) { runtime_error(app, current_line, "Record number out of range"); return false; }

    BasicFile *bf = &app->files[h];
    if (bf->mode != BF_RANDOM) { runtime_error(app, current_line, "GET only valid for RANDOM files"); return false; }
    if (bf->record_len <= 0) { runtime_error(app, current_line, "RANDOM file missing LEN"); return false; }
    if (!bf->record_buf) {
        bf->record_buf = (unsigned char*)calloc((size_t)bf->record_len, 1);
        if (!bf->record_buf) { runtime_error(app, current_line, "Out of memory"); return false; }
    }

    long off = (rec - 1) * (long)bf->record_len;
    if (fseek(bf->fp, off, SEEK_SET) != 0) { runtime_error(app, current_line, "GET seek failed"); return false; }

    size_t got = fread(bf->record_buf, 1, (size_t)bf->record_len, bf->fp);
    if (got < (size_t)bf->record_len) {
        // Beyond EOF: fill remainder with NUL bytes (0x00) for now.
        memset(bf->record_buf + got, 0, (size_t)bf->record_len - got);
    }

        bf->eof_latched = false;
        field_sync_vars(app, bf);
        return true;
}


// Phase 3: PUT #n, rec  (RANDOM files)
static bool exec_put(App *app, Parser *p, int current_line) {
    skip_ws(p);
    if (!consume(p, '#')) { runtime_error(app, current_line, "PUT requires #n, rec"); return false; }
    double hv = 0.0;
    if (!parse_expr(app, p, &hv)) { runtime_error(app, current_line, "Bad file handle"); return false; }
    int h = (int)llround(hv);
    if (h <= 0 || h >= BASIC_MAX_FILES || !app->files[h].fp) { runtime_error(app, current_line, "Invalid file handle"); return false; }

    skip_ws(p);
    if (!consume(p, ',')) { runtime_error(app, current_line, "PUT requires #n, rec"); return false; }

    double rv = 0.0;
    if (!parse_expr(app, p, &rv)) { runtime_error(app, current_line, "Bad record number"); return false; }
    long rec = (long)llround(rv);
    if (rec <= 0) { runtime_error(app, current_line, "Record number out of range"); return false; }

    BasicFile *bf = &app->files[h];
    if (bf->mode != BF_RANDOM) { runtime_error(app, current_line, "PUT only valid for RANDOM files"); return false; }
    if (bf->record_len <= 0) { runtime_error(app, current_line, "RANDOM file missing LEN"); return false; }
    if (!bf->record_buf) {
        // No FIELD support yet (Phase 4). For Phase 3 we create an all-zero record buffer.
        bf->record_buf = (unsigned char*)calloc((size_t)bf->record_len, 1);
        if (!bf->record_buf) { runtime_error(app, current_line, "Out of memory"); return false; }
    }

    long off = (rec - 1) * (long)bf->record_len;
    if (fseek(bf->fp, off, SEEK_SET) != 0) { runtime_error(app, current_line, "PUT seek failed"); return false; }

    size_t wrote = fwrite(bf->record_buf, 1, (size_t)bf->record_len, bf->fp);
    if (wrote != (size_t)bf->record_len) { runtime_error(app, current_line, "PUT write failed"); return false; }
    fflush(bf->fp);

    bf->eof_latched = false;
    return true;
}

// Phase 4: FIELD #n, <len> AS <var$>, <len> AS <var$>, ...
static bool exec_field(App *app, Parser *p, int current_line) {
    skip_ws(p);
    if (!consume(p, '#')) { runtime_error(app, current_line, "FIELD requires #n, ..."); return false; }
    double hv = 0.0;
    if (!parse_expr(app, p, &hv)) { runtime_error(app, current_line, "Bad file handle"); return false; }
    int h = (int)llround(hv);
    BasicFile *bf = file_get(app, h);
    if (!bf) { runtime_error(app, current_line, "Invalid file handle"); return false; }
    if (bf->mode != BF_RANDOM) { runtime_error(app, current_line, "FIELD only valid for RANDOM files"); return false; }
    if (bf->record_len <= 0) { runtime_error(app, current_line, "RANDOM file missing LEN"); return false; }
    if (!bf->record_buf) {
        bf->record_buf = (unsigned char*)calloc((size_t)bf->record_len, 1);
        if (!bf->record_buf) { runtime_error(app, current_line, "Out of memory"); return false; }
    }

    skip_ws(p);
    if (!consume(p, ',')) { runtime_error(app, current_line, "FIELD requires #n, ..."); return false; }

    // Replace any existing mapping for this file handle
    field_clear(bf);

    int off = 0;
    while (1) {
        double lv = 0.0;
        if (!parse_expr(app, p, &lv)) { runtime_error(app, current_line, "Bad FIELD length"); return false; }
        int len = (int)llround(lv);
        if (len <= 0) { runtime_error(app, current_line, "Bad FIELD length"); return false; }

        skip_ws(p);
        if (!consume_word_ci(p, "AS")) { runtime_error(app, current_line, "FIELD missing AS"); return false; }

        char *vname = NULL;
        if (!parse_identifier(p, &vname)) { runtime_error(app, current_line, "FIELD expected variable"); return false; }
        if (!ident_is_string_var(app, vname)) { free(vname); runtime_error(app, current_line, "FIELD variable must be string"); return false; }

        if (off + len > bf->record_len) { free(vname); runtime_error(app, current_line, "FIELD exceeds record LEN"); return false; }

        if (bf->field_count >= bf->field_cap) {
            int ncap = bf->field_cap ? bf->field_cap * 2 : 8;
            FieldMap *nf = (FieldMap*)realloc(bf->fields, (size_t)ncap * sizeof(FieldMap));
            if (!nf) { free(vname); runtime_error(app, current_line, "Out of memory"); return false; }
            bf->fields = nf;
            bf->field_cap = ncap;
        }

        bf->fields[bf->field_count].name = vname; // already upper
        bf->fields[bf->field_count].offset = off;
        bf->fields[bf->field_count].len = len;
        bf->field_count++;

        off += len;

        skip_ws(p);
        if (consume(p, ',')) continue;
        break;
    }

    // Update string vars to reflect current record buffer
    field_sync_vars(app, bf);
    return true;
}

static bool exec_lset_rset(App *app, Parser *p, int current_line, bool right_justify) {
    char *name = NULL;
    if (!parse_identifier(p, &name)) { runtime_error(app, current_line, "Expected variable"); return false; }
    if (!ident_is_string_var(app, name)) { free(name); runtime_error(app, current_line, "LSET/RSET requires string variable"); return false; }

    skip_ws(p);
    if (!consume(p, '=')) { free(name); runtime_error(app, current_line, "Expected '='"); return false; }

    char *sv = NULL;
    if (!parse_string_value(app, p, &sv)) { free(name); runtime_error_or_pending(app, current_line, "Bad string assignment"); return false; }

    // If this variable is FIELD-mapped, write into the record buffer slice
    BasicFile *bf = NULL;
    FieldMap *fm = NULL;
    if (field_lookup(app, name, &bf, &fm) && bf && fm && bf->record_buf) {
        field_write_slice(bf, fm, sv ? sv : "", right_justify);
        field_sync_vars(app, bf); // keep vars consistent with buffer
        free(sv);
        free(name);
        return true;
    }

    // Not field-mapped: behave like normal string assignment
    Var *v = vars_get_or_create(app, name);
    v->kind = V_STR;
    free(v->str);
    v->str = sv; // take ownership
    free(name);
    return true;
}





static bool exec_close(App *app, Parser *p, int current_line) {
    skip_ws(p);
    if (*p->s == 0) {
        files_close_all(app);
        return true;
    }

    for (;;) {
        skip_ws(p);
        consume(p, '#');

        double hv = 0.0;
        if (!parse_expr(app, p, &hv)) {
            runtime_error(app, current_line, "Bad file handle");
            return false;
        }
        int h = (int)llround(hv);
        if (h <= 0 || h >= BASIC_MAX_FILES) {
            runtime_error(app, current_line, "File handle out of range");
            return false;
        }

        file_close_one(file_get(app, h));

        skip_ws(p);
        if (consume(p, ',')) continue;
        break;
    }

    return true;
}

static bool exec_print_file(App *app, Parser *p, int current_line) {
    skip_ws(p);
    if (!consume(p, '#')) {
        runtime_error(app, current_line, "PRINT missing #");
        return false;
    }

    double hv = 0.0;
    if (!parse_expr(app, p, &hv)) {
        runtime_error(app, current_line, "Bad file handle");
        return false;
    }
    int h = (int)llround(hv);

    // Validate handle first (GW-BASIC: bad file number = ERR 52)
    if (h <= 0 || h >= BASIC_MAX_FILES) {
        runtime_error(app, current_line, "Bad file number");
        return false;
    }

    BasicFile *bf = file_get(app, h);
    if (!bf || !bf->fp) {
        runtime_error(app, current_line, "Bad file number");
        return false;
    }
    // GW-BASIC: attempting output to an INPUT file is "Bad file mode" (ERR 54)
    if (bf->mode != BF_OUTPUT && bf->mode != BF_APPEND) {
        runtime_error(app, current_line, "Bad file mode");
        return false;
    }

    skip_ws(p);
    if (consume(p, ',')) { /* ok */ }
    else if (consume(p, ';')) { /* ok */ }

    char last_sep = 0;

    for (;;) {
        skip_ws(p);
        if (*p->s == 0) break;

        const char *save = p->s;
        char *sv = NULL;
        if (parse_string_value(app, p, &sv)) {
            fputs(sv, bf->fp);
            free(sv);
        } else {
            p->s = save;
            double v = 0.0;
            if (!parse_expr(app, p, &v)) {
                runtime_error(app, current_line, "Bad PRINT# expression");
                return false;
            }
            char buf[64];
            if (fabs(v - llround(v)) < 1e-9) snprintf(buf, sizeof(buf), "%lld", (long long)llround(v));
            else snprintf(buf, sizeof(buf), "%.10g", v);
            fputs(buf, bf->fp);
        }

        skip_ws(p);
        if (consume(p, ';')) { last_sep = ';'; continue; }
        if (consume(p, ',')) { fputc(' ', bf->fp); last_sep = ','; continue; }
        last_sep = 0;
        break;
    }

    if (last_sep == 0) fputc('\n', bf->fp);
    fflush(bf->fp);
    return true;
}

/* ---- WRITE / WRITE # ---- */

typedef struct WriteSink {
    App *app;      /* when to_file == false */
    FILE *fp;      /* when to_file == true */
    bool to_file;
} WriteSink;

static void write_sink_puts(WriteSink *ws, const char *s) {
    if (!ws || !s) return;
    if (ws->to_file) {
        fputs(s, ws->fp);
    } else {
        out_append(ws->app, s);
    }
}

static void write_sink_putc(WriteSink *ws, int c) {
    if (!ws) return;
    if (ws->to_file) {
        fputc(c, ws->fp);
    } else {
        char buf[2];
        buf[0] = (char)c;
        buf[1] = 0;
        out_append(ws->app, buf);
    }
}

static void write_emit_quoted_string(WriteSink *ws, const char *s) {
    /* GW-BASIC WRITE: strings always quoted; internal quotes doubled. */
    write_sink_putc(ws, '"');
    const char *p = (s ? s : "");
    while (*p) {
        if (*p == '"') {
            write_sink_putc(ws, '"');
            write_sink_putc(ws, '"');
        } else {
            write_sink_putc(ws, (unsigned char)*p);
        }
        p++;
    }
    write_sink_putc(ws, '"');
}

static bool exec_write_common(App *app, WriteSink *ws, Parser *p, int current_line) {
    (void)app;
    (void)current_line;
    if (!p || !ws) return false;

    /* WRITE with no args => newline */
    skip_ws(p);
    if (*p->s == 0) {
        write_sink_putc(ws, '\n');
        return true;
    }

    bool need_comma = false;

    for (;;) {
        skip_ws(p);
        if (*p->s == 0) break;

        /* Allow empty fields: WRITE ,A  or WRITE , , */
        if (*p->s == ',' || *p->s == ';') {
            /* Empty field */
            if (need_comma) write_sink_putc(ws, ',');
            /* empty field emits nothing */
            need_comma = true;
            p->s++;
            continue;
        }

        if (need_comma) write_sink_putc(ws, ',');

        const char *save = p->s;
        char *sv = NULL;
        if (parse_string_value(app, p, &sv)) {
            write_emit_quoted_string(ws, sv);
            free(sv);
        } else {
            p->s = save;
            double nv = 0.0;
            if (!parse_expr(app, p, &nv)) return false;
            char buf[96];
            double r = round(nv);
            if (fabs(nv - r) < 1e-12) snprintf(buf, sizeof(buf), "%.0f", r);
            else snprintf(buf, sizeof(buf), "%.12g", nv);
            write_sink_puts(ws, buf);
        }

        need_comma = true;

        skip_ws(p);
        if (*p->s == ',' || *p->s == ';') {
            p->s++;
            continue;
        }
        break;
    }

    write_sink_putc(ws, '\n');
    return true;
}

static bool exec_write(App *app, Parser *p, int current_line) {
    if (!app || !p) return false;
    WriteSink ws;
    ws.app = app;
    ws.fp = NULL;
    ws.to_file = false;
    return exec_write_common(app, &ws, p, current_line);
}

static bool exec_write_file(App *app, Parser *p, int current_line) {
    if (!app || !p) return false;

    skip_ws(p);
    if (!consume(p, '#')) {
        runtime_error(app, current_line, "WRITE missing #");
        return false;
    }

    double hv = 0.0;
    if (!parse_expr(app, p, &hv)) {
        runtime_error(app, current_line, "Bad file handle");
        return false;
    }
    int h = (int)llround(hv);

    if (h <= 0 || h >= BASIC_MAX_FILES) {
        runtime_error(app, current_line, "Bad file number");
        return false;
    }

    BasicFile *bf = file_get(app, h);
    if (!bf || !bf->fp) {
        runtime_error(app, current_line, "Bad file number");
        return false;
    }
    if (bf->mode != BF_OUTPUT && bf->mode != BF_APPEND && bf->mode != BF_RANDOM) {
        /* GW-BASIC: OUTPUT/APPEND are writeable; RANDOM is read/write. */
        runtime_error(app, current_line, "Bad file mode");
        return false;
    }

    skip_ws(p);
    if (consume(p, ',') || consume(p, ';')) { /* ok */ }

    WriteSink ws;
    ws.app = NULL;
    ws.fp = bf->fp;
    ws.to_file = true;

    bool ok = exec_write_common(app, &ws, p, current_line);
    fflush(bf->fp);
    return ok;
}

static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "WBASIC: out of memory\n");
        exit(1);
    }
    return p;
}

static void *xrealloc(void *ptr, size_t n) {
    void *p = realloc(ptr, n);
    if (!p) {
        fprintf(stderr, "WBASIC: out of memory\n");
        exit(1);
    }
    return p;
}


static bool read_line_file(FILE *fp, char **out_line) {
    size_t cap = 256, len = 0;
    char *buf = (char*)xmalloc(cap);
    buf[0] = 0;

    for (;;) {
        int c = fgetc(fp);
        if (c == EOF) break;
        if (c == '\r') continue;
        if (c == '\n') break;
        if (len + 2 > cap) { cap *= 2; buf = (char*)xrealloc(buf, cap); }
        buf[len++] = (char)c;
        buf[len] = 0;
    }

    if (len == 0 && feof(fp)) { free(buf); return false; }
    *out_line = buf;
    return true;
}

static char *trim_ws_copy(const char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    const char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) e--;
    size_t n = (size_t)(e - s);
    char *o = (char*)xmalloc(n + 1);
    memcpy(o, s, n);
    o[n] = 0;
    return o;
}


static char *unescape_doubled_quotes_inner(const char *s) {
    /* Convert doubled quotes ("") -> " in a WRITE/INPUT-quoted field. */
    const char *p = (s ? s : "");
    size_t cap = strlen(p) + 1;
    char *out = (char*)xmalloc(cap);
    size_t oi = 0;
    while (*p) {
        if (p[0] == '"' && p[1] == '"') {
            out[oi++] = '"';
            p += 2;
            continue;
        }
        out[oi++] = *p++;
    }
    out[oi] = 0;
    return out;
}


static bool exec_input_file(App *app, Parser *p, int current_line, bool line_mode) {
    skip_ws(p);
    if (!consume(p, '#')) {
        runtime_error(app, current_line, "INPUT# missing #");
        return false;
    }

    double hv = 0.0;
    if (!parse_expr(app, p, &hv)) {
        runtime_error(app, current_line, "Bad file handle");
        return false;
    }
    int h = (int)llround(hv);

    BasicFile *bf = file_get(app, h);
    if (!bf || !bf->fp || bf->mode != BF_INPUT) {
        runtime_error(app, current_line, "File not open for input");
        return false;
    }

    skip_ws(p);
    if (!consume(p, ',')) {
        runtime_error(app, current_line, "INPUT# missing ','");
        return false;
    }

    char *line = NULL;
    if (!read_line_file(bf->fp, &line)) {
        bf->eof_latched = true;
        runtime_error(app, current_line, "INPUT past EOF");
        return false;
    }

    if (line_mode) {
        char *vname = NULL;
        if (!parse_identifier(p, &vname)) {
            free(line);
            runtime_error(app, current_line, "Bad variable");
            return false;
        }
        if (!ident_is_string_var(app, vname)) {
            free(vname);
            free(line);
            runtime_error(app, current_line, "LINE INPUT requires string variable");
            return false;
        }

        Var *v = vars_get_or_create(app, vname);
        v->kind = V_STR;
        free(v->str);
        v->str = xstrdup(line);

        free(vname);
        free(line);
        return true;
    }

    const char *cur = line;

    for (;;) {
        char *vname = NULL;
        if (!parse_identifier(p, &vname)) {
            free(line);
            runtime_error(app, current_line, "Bad variable");
            return false;
        }

        const char *field_start = cur;
        bool inq = false;
        while (*cur) {
            if (*cur == '"') inq = !inq;
            if (!inq && *cur == ',') break;
            cur++;
        }
        const char *field_end = cur;
        if (*cur == ',') cur++;

        char *field_raw = (char*)xmalloc((size_t)(field_end - field_start) + 1);
        memcpy(field_raw, field_start, (size_t)(field_end - field_start));
        field_raw[field_end - field_start] = 0;
        char *field = trim_ws_copy(field_raw);
        free(field_raw);

        Var *v = vars_get_or_create(app, vname);
        if (name_is_string(vname)) {
            v->kind = V_STR;
            free(v->str);
            size_t L = strlen(field);

if (L >= 2 && field[0] == '"' && field[L-1] == '"') {
    field[L-1] = 0;
    char *un = unescape_doubled_quotes_inner(field + 1);
    v->str = un; /* takes ownership */
} else {
    v->str = xstrdup(field);
}
        } else {
            v->kind = V_NUM;
            double nv = strtod(field, NULL);
            nv = coerce_numeric_store(app, v, vname, nv);
            v->num = nv;
        }

        free(field);
        free(vname);

        skip_ws(p);
        if (consume(p, ',')) continue;
        break;
    }

    free(line);
    return true;
}


/* ---- INPUT ---- */
static void input_echo_update(App *app, const char *txt);

typedef struct InputVarSpec {
    char *name;
    bool is_str;
} InputVarSpec;

static bool parse_input_line_fields(const char *line, char ***out_fields, int *out_n) {
    /* Split by commas, honoring quotes.
       Strictness (GW-BASIC-ish, tuned for WRITE round-trip):
         - Unterminated quoted fields => parse failure (?Redo from start).
         - If a field contains any '"', it must be a quoted field (starts/ends with '"' after trim).
         - Inside a quoted field, interior quotes must be doubled (""). These are unescaped to a single '"'. */

    const char *cur = line ? line : "";

    int cap = 8;
    int n = 0;
    char **fields = (char**)xmalloc((size_t)cap * sizeof(char*));

    for (;;) {
        bool inq = false;
        const char *start = cur;
        while (*cur) {
            if (*cur == '"') inq = !inq;
            if (!inq && *cur == ',') break;
            cur++;
        }

        /* Unterminated quote: reject entire line (GW-BASIC: Redo from start). */
        if (inq && *cur == 0) {
            for (int i = 0; i < n; i++) free(fields[i]);
            free(fields);
            *out_fields = NULL;
            *out_n = 0;
            return false;
        }

        const char *end = cur;
        if (*cur == ',') cur++; /* consume comma */

        /* Copy + trim raw field */
        size_t L = (size_t)(end - start);
        char *raw = (char*)xmalloc(L + 1);
        memcpy(raw, start, L);
        raw[L] = 0;
        char *field = trim_ws_copy(raw);
        free(raw);

        /* Strict quote handling + doubled-quote unescape */
        if (strchr(field, '"')) {
            size_t tl = strlen(field);
            if (tl < 2 || field[0] != '"' || field[tl - 1] != '"') {
                free(field);
                for (int i = 0; i < n; i++) free(fields[i]);
                free(fields);
                *out_fields = NULL;
                *out_n = 0;
                return false;
            }

            char *out = (char*)xmalloc(tl + 1);
            size_t oi = 0;
            for (size_t i = 1; i + 1 < tl; i++) {
                char c = field[i];
                if (c == '"') {
                    if (i + 1 < tl - 1 && field[i + 1] == '"') {
                        out[oi++] = '"';
                        i++; /* consume second quote */
                    } else {
                        free(out);
                        free(field);
                        for (int j = 0; j < n; j++) free(fields[j]);
                        free(fields);
                        *out_fields = NULL;
                        *out_n = 0;
                        return false;
                    }
                } else {
                    out[oi++] = c;
                }
            }
            out[oi] = 0;
            free(field);
            field = out;
        }

        if (n >= cap) {
            cap *= 2;
            fields = (char**)xrealloc(fields, (size_t)cap * sizeof(char*));
        }
        fields[n++] = field;

        if (*end == 0) break;
    }

    *out_fields = fields;
    *out_n = n;
    return true;
}

static void free_input_fields(char **fields, int n) {
    if (!fields) return;
    for (int i = 0; i < n; i++) free(fields[i]);
    free(fields);
}

static bool field_to_number_strict(App *app, const char *field, double *out) {
    // GW-BASIC INPUT numeric parsing: allow leading/trailing spaces (already trimmed),
    // allow empty -> 0, otherwise must parse a valid number.
    const char *s = field ? field : "";
    if (*s == 0) { *out = 0.0; return true; }
    char *endp = NULL;
    double v = strtod(s, &endp);
    if (!endp) return false;
    while (*endp && isspace((unsigned char)*endp)) endp++;
    if (*endp != 0) return false;
    *out = v;
    (void)app;
    return true;
}

static bool exec_input(App *app, Parser *p, int current_line) {
    skip_ws(p);

    // INPUT ["prompt";] var[,var...]
    char *prompt = NULL;
    if (parse_string_literal(p, &prompt)) {
        skip_ws(p);
        // Typically INPUT "Prompt"; A  (semicolon), but allow comma too.
        if (consume(p, ';') || consume(p, ',')) {
            // ok
        }
        skip_ws(p);
    }

    // Parse varlist
    InputVarSpec vars[64];
    int var_n = 0;
    memset(vars, 0, sizeof(vars));

    for (;;) {
        char *name = NULL;
        if (!parse_identifier(p, &name)) {
            // No vars at all is an error
            if (var_n == 0) {
                free(prompt);
                runtime_error(app, current_line, "INPUT expects a variable name");
                return false;
            }
            break;
        }
        bool is_str = ident_is_string_var(app, name);
        if (var_n < (int)(sizeof(vars)/sizeof(vars[0]))) {
            vars[var_n].name = name;
            vars[var_n].is_str = is_str;
            var_n++;
        } else {
            free(name);
            runtime_error(app, current_line, "Too many INPUT variables");
            free(prompt);
            return false;
        }

        skip_ws(p);
        if (consume(p, ',')) { skip_ws(p); continue; }
        break;
    }

    /* Terminal-style inline INPUT using cmd_entry (no modal dialog). */
    if (!app) {
        for (int i = 0; i < var_n; i++) free(vars[i].name);
        free(prompt);
        runtime_error(app, current_line, "INPUT failed (no app)");
        return false;
    }

    while (1) {
        // Print prompt or default "? "
        if (prompt && *prompt) {
            out_append(app, prompt);
            size_t pl = strlen(prompt);
            if (pl > 0 && prompt[pl - 1] != ' ' && prompt[pl - 1] != '	') out_append(app, " ");
        } else {
            out_append(app, "? ");
        }

        // Remember where the user's INPUT text begins so we can live-echo it into the output pane.
        app->input_echo_row = app->out_row;
        app->input_echo_col = app->out_col;
        app->input_echo_len = 0;
        app->input_echo_draw_len = 0;
        app->input_cursor_on = true;
        app->input_cursor_next_toggle_us = g_get_monotonic_time() + 500000; /* 500ms */

        // Prepare to receive input line via command entry.
        app->input_waiting = true;
        app->input_ready = false;
        if (app->input_line) { g_free(app->input_line); app->input_line = NULL; }

        if (app->cmd_entry) {
#ifndef WBASIC_NO_UI
        gtk_widget_grab_focus(app->cmd_entry);
        cmd_entry_set_stealth(app, true);
        gtk_widget_queue_draw(app->cmd_entry);

        GtkEntry *e = GTK_ENTRY(app->cmd_entry);
        gtk_entry_set_invisible_char(e, ' ');
        gtk_entry_set_visibility(e, FALSE);
        gtk_entry_set_text(e, "");
#else
        /* Headless build: no cmd_entry; input comes from stdin. */
        (void)app;
#endif
    }

    set_run_state(app, RUN_WAITING);
        input_echo_update(app, "");

                if (wbasic_ui_active(app) && app->cmd_entry) {
        while (!app->input_ready && !app->stop_flag && !app->quitting) {
            ui_pump_raw(app);
            gint64 now = g_get_monotonic_time();
            if (now >= app->input_cursor_next_toggle_us) {
                app->input_cursor_on = !app->input_cursor_on;
                app->input_cursor_next_toggle_us = now + 500000;
                const char *curtxt = "";
#ifndef WBASIC_NO_UI
                if (app->cmd_entry) curtxt = gtk_entry_get_text(GTK_ENTRY(app->cmd_entry));
#endif
                input_echo_update(app, curtxt ? curtxt : "");
            }
            g_usleep(10 * 1000);
        }

        if (app->cmd_entry) {
#ifndef WBASIC_NO_UI
            GtkEntry *e = GTK_ENTRY(app->cmd_entry);
            gtk_entry_set_visibility(e, TRUE);
            gtk_entry_set_invisible_char(e, 0);
            gtk_entry_set_text(e, "");
#endif
        }
        } else {
            /* Headless: read a line from stdin.
               Note: the prompt (or ? ) has already been written via out_append() above. */
            {
                char buf[4096];
                /* Ensure prompt is visible before blocking for input. */
                fflush(stdout);
                if (!fgets(buf, sizeof(buf), stdin)) {
                    app->input_line = g_strdup("");
                } else {
                    size_t l = strlen(buf);
                    while (l && (buf[l-1] == '\n' || buf[l-1] == '\r')) buf[--l] = 0;
                    app->input_line = g_strdup(buf);
                }
                app->input_ready = true;
            }
        }


        if (!app->stop_flag && !app->quitting) set_run_state(app, RUN_RUNNING);

        if (app->stop_flag || app->quitting) {
            app->input_waiting = false;
            #ifndef WBASIC_NO_UI
cmd_entry_set_stealth(app, false);
#endif

            if (app->input_line) { g_free(app->input_line); app->input_line = NULL; }
            free(prompt);
            for (int i = 0; i < var_n; i++) free(vars[i].name);
            runtime_error(app, current_line, "Break");
            return false;
        }

        const char *txt = app->input_line ? app->input_line : "";

        // Parse fields
        char **fields = NULL;
        int field_n = 0;
        bool fields_ok = parse_input_line_fields(txt, &fields, &field_n);

        bool ok = fields_ok;
        if (ok) for (int i = 0; i < var_n; i++) {
            const char *f = (i < field_n) ? fields[i] : ""; // missing fields -> empty
            Var *v = vars_get_or_create(app, vars[i].name);
            if (vars[i].is_str) {
                v->kind = V_STR;
                v->is_array = false;
                free(v->str);
                v->str = xstrdup(f);
            } else {
                double nv = 0.0;
                if (!field_to_number_strict(app, f, &nv)) { ok = false; break; }
                v->kind = V_NUM;
                v->is_array = false;
                nv = coerce_numeric_store(app, v, vars[i].name, nv);
                v->num = nv;
            }
        }

        free_input_fields(fields, field_n);

        app->input_waiting = false;
        #ifndef WBASIC_NO_UI
cmd_entry_set_stealth(app, false);
#endif

        app->input_ready = false;
        if (app->input_line) { g_free(app->input_line); app->input_line = NULL; }

        if (ok) {
            break;
        }

        // Bad input -> Redo from start
        out_append(app, "\n?Redo from start\n");
        screen_render_now(app);
    }

    free(prompt);
    for (int i = 0; i < var_n; i++) free(vars[i].name);
    return true;
}



/* ---- DEFINT ---- */
static bool exec_defint(App *app, Parser *p, int current_line) {
    // DEFINT A-Z[, ...]  (GW-BASIC style). Sets default type for new numeric vars.
    // Accepts single letters (A) or ranges (A-Z). Comma-separated list.
    skip_ws(p);
    if (*p->s == 0) { runtime_error(app, current_line, "DEFINT expects letter range"); return false; }

    for (;;) {
        skip_ws(p);
        char a = (char)toupper((unsigned char)*p->s);
        if (a < 'A' || a > 'Z') { runtime_error(app, current_line, "DEFINT expects letter range"); return false; }
        p->s++;

        skip_ws(p);
        char b = a;
        if (consume(p, '-')) {
            skip_ws(p);
            b = (char)toupper((unsigned char)*p->s);
            if (b < 'A' || b > 'Z') { runtime_error(app, current_line, "DEFINT expects letter range"); return false; }
            p->s++;
        }

        if (b < a) { char t = a; a = b; b = t; }
        for (char c = a; c <= b; c++) app->def_type[c - 'A'] = (unsigned char)DT_INT;

        skip_ws(p);
        if (consume(p, ',')) continue;
        break;
    }
    return true;
}



/* ---- DEFDBL / DEFSNG / DEFSTR ---- */
static bool exec_defset(App *app, Parser *p, int current_line, DefType t, const char *kwname) {
    // DEFxxx A-Z[, ...]  (GW-BASIC style). Sets default type for vars without explicit suffix.
    skip_ws(p);
    if (*p->s == 0) { 
        char msg[64]; 
        snprintf(msg, sizeof(msg), "%s expects letter range", kwname);
        runtime_error(app, current_line, msg); 
        return false; 
    }

    for (;;) {
        skip_ws(p);
        char a = (char)toupper((unsigned char)*p->s);
        if (a < 'A' || a > 'Z') { 
            char msg[64]; 
            snprintf(msg, sizeof(msg), "%s expects letter range", kwname);
            runtime_error(app, current_line, msg); 
            return false; 
        }
        p->s++;

        skip_ws(p);
        char b = a;
        if (consume(p, '-')) {
            skip_ws(p);
            b = (char)toupper((unsigned char)*p->s);
            if (b < 'A' || b > 'Z') { 
                char msg[64]; 
                snprintf(msg, sizeof(msg), "%s expects letter range", kwname);
                runtime_error(app, current_line, msg); 
                return false; 
            }
            p->s++;
        }

        if (b < a) { char tmp = a; a = b; b = tmp; }
        for (char c = a; c <= b; c++) app->def_type[c - 'A'] = (unsigned char)t;

        skip_ws(p);
        if (consume(p, ',')) continue;
        break;
    }
    return true;
}

static bool exec_defdbl(App *app, Parser *p, int current_line) { return exec_defset(app, p, current_line, DT_DBL, "DEFDBL"); }
static bool exec_defsng(App *app, Parser *p, int current_line) { return exec_defset(app, p, current_line, DT_SNG, "DEFSNG"); }
static bool exec_defstr(App *app, Parser *p, int current_line) { return exec_defset(app, p, current_line, DT_STR, "DEFSTR"); }

/* ---- OPTION BASE ---- */
// (removed stray forward decl)
static bool exec_option(App *app, Parser *p, int current_line) {
    // OPTION BASE 0|1
    skip_ws(p);
    const char *s = p->s;
    if (!(toupper((unsigned char)s[0])=='B' && toupper((unsigned char)s[1])=='A' &&
          toupper((unsigned char)s[2])=='S' && toupper((unsigned char)s[3])=='E' &&
          is_word_boundary(s[4]))) {
        runtime_error(app, current_line, "OPTION expects BASE");
        return false;
    }
    p->s += 4;

    if (app->option_base_locked) { runtime_error(app, current_line, "OPTION BASE must appear before DIM"); return false; }

    double v = 0.0;
    if (!parse_expr(app, p, &v)) { runtime_error(app, current_line, "OPTION BASE expects 0 or 1"); return false; }
    int b = (int)llround(v);
    if (!(b == 0 || b == 1)) { runtime_error(app, current_line, "OPTION BASE must be 0 or 1"); return false; }

    app->option_base = b;
    return true;
}


/* ---- WIDTH ---- */
static bool exec_width(App *app, Parser *p, int current_line) {
    // WIDTH <cols>
    skip_ws(p);
    double v = 0.0;
    if (!parse_expr(app, p, &v)) { runtime_error(app, current_line, "WIDTH expects a number"); return false; }
    int w = (int)llround(v);
    if (w < 20 || w > 255) { runtime_error(app, current_line, "Illegal WIDTH"); return false; }

    // Update width and rebuild the text screen buffers.
    // We clear the screen as GW-BASIC does when WIDTH changes.
    app->screen_cols = w;

    if (app->screen) { free(app->screen); app->screen = NULL; }
    if (app->screen_fg) { free(app->screen_fg); app->screen_fg = NULL; }
    if (app->screen_bg) { free(app->screen_bg); app->screen_bg = NULL; }

    screen_clear(app);
    app->screen_dirty = false;
    screen_render_now(app);
    return true;
}


/* ---- DIM ---- */
static bool exec_dim(App *app, Parser *p, int current_line) {
    // DIM A(10) or DIM A(10,20,...)   (up to 5 dims)
    app->option_base_locked = true;
    for (;;) {
        char *name = NULL;
        if (!parse_identifier(p, &name)) { runtime_error(app, current_line, "DIM expects name"); return false; }
        if (!consume(p, '(')) { free(name); runtime_error(app, current_line, "DIM missing '('"); return false; }

        int nd = 0;
        int dim_max[5] = {0,0,0,0,0};
        while (1) {
            if (nd >= 5) { free(name); runtime_error(app, current_line, "DIM supports up to 5 dimensions"); return false; }
            double d = 0.0;
            if (!parse_expr(app, p, &d)) { free(name); runtime_error(app, current_line, "DIM bad size"); return false; }
            int n = (int)llround(d);
            if (n < 0) { free(name); runtime_error(app, current_line, "DIM size must be >=0"); return false; }
            dim_max[nd++] = n;
            skip_ws(p);
            if (consume(p, ',')) continue;
            if (!consume(p, ')')) { free(name); runtime_error(app, current_line, "DIM missing ')'"); return false; }
            break;
        }

        Var *v = vars_get_or_create(app, name);
        bool okdim = false;
        if (ident_is_string_var(app, name)) okdim = var_define_str_array(v, nd, app->option_base, dim_max);
        else okdim = var_define_num_array(v, nd, app->option_base, dim_max);
        if (!okdim) { free(name); runtime_error(app, current_line, "DIM out of memory"); return false; }

        free(name);

        skip_ws(p);
        if (*p->s == ',') { p->s++; continue; }
        break;
    }
    return true;
}

static bool exec_redim(App *app, Parser *p, int current_line) {
    // REDIM [PRESERVE] A(10) or A$(10,20,...) (up to 5 dims)
    app->option_base_locked = true;
    // With PRESERVE: only last dimension may change; other dims must match.
    bool preserve = false;
    skip_ws(p);
    if (starts_ci(p->s, "PRESERVE") && is_word_boundary(p->s[8])) {
        p->s += 8;
        preserve = true;
    }

    for (;;) {
        char *name = NULL;
        if (!parse_identifier(p, &name)) { runtime_error(app, current_line, "REDIM expects name"); return false; }
        if (!consume(p, '(')) { free(name); runtime_error(app, current_line, "REDIM missing '('"); return false; }

        int nd = 0;
        int dim_max[5] = {0,0,0,0,0};
        while (1) {
            if (nd >= 5) { free(name); runtime_error(app, current_line, "Illegal function call"); return false; }
            double d = 0.0;
            if (!parse_expr(app, p, &d)) { free(name); runtime_error(app, current_line, "Bad expression"); return false; }
            int n = (int)llround(d);
            if (n < app->option_base) { free(name); runtime_error(app, current_line, "Illegal function call"); return false; }
            dim_max[nd++] = n;
            skip_ws(p);
            if (consume(p, ',')) continue;
            if (!consume(p, ')')) { free(name); runtime_error(app, current_line, "REDIM missing ')'"); return false; }
            break;
        }

        Var *v = vars_get_or_create(app, name);
        bool is_str = ident_is_string_var(app, name);

        // Tightened REDIM runtime errors (GW/QBasic-like)
        // - Invalid/unsupported PRESERVE reshapes -> Illegal function call
        // - PRESERVE with wrong target type -> Type mismatch
        // - Allocation failures -> Out of memory
        if (preserve && v->is_array) {
            // Type must match existing array
            if (is_str) {
                if (v->kind != V_STR || !v->sarr) { free(name); runtime_error(app, current_line, "Type mismatch"); return false; }
            } else {
                if (v->kind != V_NUM || !v->arr) { free(name); runtime_error(app, current_line, "Type mismatch"); return false; }
            }
            // PRESERVE rule: same #dims; all dims except last must match exactly; only last dim upper bound may change.
            if (v->arr_dims != nd) { free(name); runtime_error(app, current_line, "Illegal function call"); return false; }
            for (int i = 0; i < nd - 1; i++) {
                if (v->arr_dim_lo[i] != app->option_base || v->arr_dim_max[i] != dim_max[i]) {
                    free(name);
                    runtime_error(app, current_line, "Illegal function call");
                    return false;
                }
            }
            if (v->arr_dim_lo[nd - 1] != app->option_base) { free(name); runtime_error(app, current_line, "Illegal function call"); return false; }
        }

        bool ok = var_redim_array(v, is_str, nd, app->option_base, dim_max, preserve);
        if (!ok) {
            free(name);
            runtime_error(app, current_line, "Out of memory");
            return false;
        }

        free(name);

        skip_ws(p);
        if (*p->s == ',') { p->s++; continue; }
        break;
    }
    return true;
}


/* ---- FOR/NEXT ---- */

static bool find_matching_next_for(App *app, int start_line_idx, int start_stmt_idx, const char *var_name,
                                  int current_line, int *out_line_idx, int *out_stmt_idx);

static bool for_advance_frame(App *app, int fi, bool *out_cont);
static bool exec_for(App *app, Parser *p, int current_line, int cur_line_idx, int cur_stmt_idx,
                     int *line_idx, int *stmt_idx) {
    // FOR var = start TO end [STEP step]
    char *name = NULL;
    if (!parse_identifier(p, &name) || ident_is_string_var(app, name)) { free(name); runtime_error(app, current_line, "FOR expects numeric variable"); return false; }
    if (!consume(p, '=')) { free(name); runtime_error(app, current_line, "FOR missing '='"); return false; }

    double startv = 0.0, endv = 0.0, stepv = 1.0;
    if (!parse_expr(app, p, &startv)) { free(name); runtime_error(app, current_line, "FOR bad start"); return false; }

    skip_ws(p);
    if (!(toupper((unsigned char)p->s[0])=='T' && toupper((unsigned char)p->s[1])=='O' && is_word_boundary(p->s[2]))) {
        free(name); runtime_error(app, current_line, "FOR missing TO"); return false;
    }
    p->s += 2;

    if (!parse_expr(app, p, &endv)) { free(name); runtime_error(app, current_line, "FOR bad end"); return false; }

    skip_ws(p);
    if (toupper((unsigned char)p->s[0])=='S' && toupper((unsigned char)p->s[1])=='T' &&
        toupper((unsigned char)p->s[2])=='E' && toupper((unsigned char)p->s[3])=='P' &&
        is_word_boundary(p->s[4])) {
        p->s += 4;
        if (!parse_expr(app, p, &stepv)) { free(name); runtime_error(app, current_line, "FOR bad STEP"); return false; }
        if (stepv == 0.0) { free(name); runtime_error(app, current_line, "FOR STEP cannot be 0"); return false; }
    }

    Var *v = vars_get_or_create(app, name);
    v->kind = V_NUM;
    v->is_array = false;
    v->num = coerce_numeric_store(app, v, name, startv);

    // GW-BASIC: if the initial loop condition is already false, skip the loop body entirely
    // (do not push a FOR frame). Condition check is inclusive.
    bool enter = (stepv > 0.0) ? (v->num <= endv) : (v->num >= endv);
    if (!enter) {
        // Skip forward to the matching NEXT (respecting nested FOR/NEXT), then continue after it.
        int nli = 0, nsi = 0;
        if (!find_matching_next_for(app, cur_line_idx, cur_stmt_idx, name, current_line, &nli, &nsi)) {
            free(name);
            return false;
        }
        *line_idx = nli;
        *stmt_idx = nsi + 1; // continue after NEXT
        free(name);
        return true;
    }

    if (app->for_sp >= 128) { free(name); runtime_error(app, current_line, "FOR stack overflow"); return false; }

    ForFrame fr;
    fr.var_name = name; // keep allocated, freed on pop/reset
    fr.end_value = endv;
    fr.step_value = stepv;
    // start is the next statement after this FOR statement:
    fr.start_line_idx = cur_line_idx;
    fr.start_stmt_idx = cur_stmt_idx + 1;
    app->for_stack[app->for_sp++] = fr;
    return true;
}

static bool next_stmt_mentions_var(const char *s_after_next, const char *var_name) {
    // Parse NEXT var[,var...] and report whether var_name appears (case-insensitive).
    Parser p = { s_after_next };
    skip_ws(&p);
    bool saw_any = false;
    while (1) {
        char *nm = NULL;
        Parser save = p;
        if (!parse_identifier(&p, &nm)) {
            p = save;
            break;
        }
        saw_any = true;
        bool match = (strcasecmp(nm, var_name) == 0);
        free(nm);
        skip_ws(&p);
        if (match) return true;
        if (consume(&p, ',')) { skip_ws(&p); continue; }
        break;
    }
    // If NEXT has no variables listed, it matches the innermost FOR at this nesting level.
    (void)saw_any;
    return false;
}

static bool find_matching_next_for(App *app, int start_line_idx, int start_stmt_idx, const char *var_name,
                                  int current_line, int *out_line_idx, int *out_stmt_idx) {
    // Scan forward from just after the FOR statement to find the matching NEXT.
    // Handles nesting of FOR/NEXT. Also handles NEXT varlists; if the list contains var_name,
    // it is considered a match at the current nesting level.
    int depth = 0;

    for (int li = start_line_idx; li < (int)app->prog.count; li++) {
        const char *ltxt = app->prog.lines[li].text ? app->prog.lines[li].text : "";
        StmtList sl = split_statements(ltxt);

        int si0 = 0;
        if (li == start_line_idx) si0 = start_stmt_idx + 1;

        for (int si = si0; si < sl.count; si++) {
            char *tmp = xstrdup(sl.stmts[si]);
            char *s = trim(tmp);
            if (*s == 0) { free(tmp); continue; }

            // Ignore REM and apostrophe comments
            if ((starts_ci(s, "REM") && is_word_boundary(s[3])) || (*s == '\'')) { free(tmp); continue; }

            if (starts_ci(s, "FOR") && is_word_boundary(s[3])) {
                depth++;
                free(tmp);
                continue;
            }

            if (starts_ci(s, "NEXT") && is_word_boundary(s[4])) {
                if (depth == 0) {
                    // Match if NEXT has no var list (innermost) OR if var_name appears in the list.
                    const char *after = s + 4;
                    Parser p = { after };
                    skip_ws(&p);
                    bool has_vars = false;
                    Parser save = p;
                    char *nm = NULL;
                    if (parse_identifier(&p, &nm)) { has_vars = true; free(nm); }
                    p = save;

                    bool match = (!has_vars) || next_stmt_mentions_var(after, var_name);
                    if (match) {
                        *out_line_idx = li;
                        *out_stmt_idx = si;
                        free(tmp);
                        stmtlist_free(&sl);
                        return true;
                    }
                }
                // NEXT closes one nesting level (even if it lists multiple vars; for scanning we treat it as one).
                if (depth > 0) depth--;
                free(tmp);
                continue;
            }

            free(tmp);
        }

        stmtlist_free(&sl);
    }

    runtime_error(app, current_line, "FOR without NEXT");
    return false;
}

static bool for_advance_frame(App *app, int fi, bool *out_cont) {
    ForFrame *fr = &app->for_stack[fi];
    Var *v = vars_get_or_create(app, fr->var_name);
    v->kind = V_NUM;
    v->is_array = false;
    v->num = coerce_numeric_store(app, v, fr->var_name, v->num + fr->step_value);
    bool cont = (fr->step_value > 0) ? (v->num <= fr->end_value) : (v->num >= fr->end_value);
    *out_cont = cont;
    return true;
}

static bool exec_next(App *app, Parser *p, int current_line, int *line_idx, int *stmt_idx) {
    // NEXT [var[,var...]]
    // If multiple variables are given, they must be in inside-out order (innermost first).
    // The behavior matches GW-BASIC: process the list left-to-right (innermost-to-outermost).

    char *vars[32];
    int nvars = 0;

    Parser save_all = *p;
    skip_ws(p);
    while (1) {
        Parser save = *p;
        char *nm = NULL;
        if (!parse_identifier(p, &nm)) {
            *p = save;
            break;
        }
        if (ident_is_string_var(app, nm)) { free(nm); runtime_error(app, current_line, "NEXT var must be numeric"); return false; }
        if (nvars < (int)(sizeof(vars)/sizeof(vars[0]))) {
            vars[nvars++] = nm;
        } else {
            free(nm);
            runtime_error(app, current_line, "Too many variables in NEXT");
            return false;
        }
        skip_ws(p);
        if (consume(p, ',')) { skip_ws(p); continue; }
        break;
    }
    // If we didn't parse any identifiers, restore parser so things like NEXT: ... remain ok.
    if (nvars == 0) *p = save_all;

    if (app->for_sp <= 0) {
        for (int i = 0; i < nvars; i++) free(vars[i]);
        runtime_error(app, current_line, "NEXT without FOR");
        return false;
    }

    // No var list: act on top-of-stack.
    if (nvars == 0) {
        int fi = app->for_sp - 1;
        bool cont = false;
        (void)for_advance_frame(app, fi, &cont);
        if (cont) {
            *line_idx = app->for_stack[fi].start_line_idx;
            *stmt_idx = app->for_stack[fi].start_stmt_idx;
            return true;
        }
        free(app->for_stack[fi].var_name);
        app->for_sp = fi;
        return true;
    }

    // Process varlist left-to-right (innermost-to-outermost). If an inner loop continues, jump back to it immediately.
    for (int vi = 0; vi < nvars; vi++) {
        const char *wanted = vars[vi];

        int fi = app->for_sp - 1;
        while (fi >= 0 && strcasecmp(app->for_stack[fi].var_name, wanted) != 0) fi--;
        if (fi < 0) {
            for (int i = 0; i < nvars; i++) free(vars[i]);
            runtime_error(app, current_line, "NEXT doesn't match any FOR");
            return false;
        }

        bool cont = false;
        (void)for_advance_frame(app, fi, &cont);

        if (cont) {
            // Continuing this loop: do not discard its frame (or any outer frames). However, any inner frames
            // above it are no longer valid if the user specified an outer NEXT while inner loops were open.
            // For compatibility, unwind frames above fi.
            for (int j = app->for_sp - 1; j > fi; j--) free(app->for_stack[j].var_name);
            app->for_sp = fi + 1;

            *line_idx = app->for_stack[fi].start_line_idx;
            *stmt_idx = app->for_stack[fi].start_stmt_idx;
            for (int i = 0; i < nvars; i++) free(vars[i]);
            return true;
        }

        // Finished this loop: pop down to fi (inclusive) and continue to next outer variable in the list.
        for (int j = app->for_sp - 1; j >= fi; j--) free(app->for_stack[j].var_name);
        app->for_sp = fi;
    }

    for (int i = 0; i < nvars; i++) free(vars[i]);
    return true;
}



/* ---- WHILE/WEND ---- */

static bool find_matching_wend(App *app, int start_line_idx, int start_stmt_idx, int current_line, int *out_line_idx, int *out_stmt_idx) {
    // Scan forward from just after the WHILE statement to find the matching WEND.
    // Handles nesting of WHILE/WEND. Returns the location (line_idx, stmt_idx) of the matching WEND statement.
    int depth = 0;

    for (int li = start_line_idx; li < (int)app->prog.count; li++) {
        const char *ltxt = app->prog.lines[li].text ? app->prog.lines[li].text : "";
        StmtList sl = split_statements(ltxt);

        int si0 = 0;
        if (li == start_line_idx) si0 = start_stmt_idx + 1;

        for (int si = si0; si < sl.count; si++) {
            char *tmp = xstrdup(sl.stmts[si]);
            char *s = trim(tmp);

            if (*s == 0) { free(tmp); continue; }

            // Ignore REM and apostrophe comments
            if ((starts_ci(s, "REM") && is_word_boundary(s[3])) || (*s == '\'')) { free(tmp); continue; }

            if (starts_ci(s, "WHILE") && is_word_boundary(s[5])) {
                depth++;
                free(tmp);
                continue;
            }
            if (starts_ci(s, "WEND") && is_word_boundary(s[4])) {
                if (depth == 0) {
                    *out_line_idx = li;
                    *out_stmt_idx = si;
                    free(tmp);
                    stmtlist_free(&sl);
                    return true;
                }
                depth--;
                free(tmp);
                continue;
            }

            free(tmp);
        }

        stmtlist_free(&sl);
    }

    runtime_error(app, current_line, "WHILE without WEND");
    return false;
}

static bool find_matching_loop(App *app, int start_line_idx, int start_stmt_idx, int current_line,
                               int *out_line_idx, int *out_stmt_idx) {
    // Scan forward from just after the DO statement to find the matching LOOP.
    // Handles nesting of DO/LOOP. Returns the location (line_idx, stmt_idx) of the matching LOOP statement.
    int depth = 0;

    for (int li = start_line_idx; li < (int)app->prog.count; li++) {
        const char *ltxt = app->prog.lines[li].text ? app->prog.lines[li].text : "";
        StmtList sl = split_statements(ltxt);

        int si0 = 0;
        if (li == start_line_idx) si0 = start_stmt_idx + 1;

        for (int si = si0; si < sl.count; si++) {
            char *tmp = xstrdup(sl.stmts[si]);
            char *s = trim(tmp);

            if (*s == 0) { free(tmp); continue; }

            // Ignore REM and apostrophe comments
            if ((starts_ci(s, "REM") && is_word_boundary(s[3])) || (*s == '\'')) { free(tmp); continue; }

            if (starts_ci(s, "DO") && is_word_boundary(s[2])) {
                depth++;
                free(tmp);
                continue;
            }

            if (starts_ci(s, "LOOP") && is_word_boundary(s[4])) {
                if (depth == 0) {
                    *out_line_idx = li;
                    *out_stmt_idx = si;
                    free(tmp);
                    stmtlist_free(&sl);
                    return true;
                }
                depth--;
                free(tmp);
                continue;
            }

            free(tmp);
        }

        stmtlist_free(&sl);
    }

    runtime_error(app, current_line, "DO without LOOP");
    return false;
}



static bool exec_while(App *app, Parser *p, int current_line, int cur_line_idx, int cur_stmt_idx, int *line_idx, int *stmt_idx) {
    // WHILE <expr>
    double cond = 0.0;
    if (!parse_cond_or(app, p, &cond)) return false;

    if (cond != 0.0) {
        if (app->while_sp >= 128) { runtime_error(app, current_line, "WHILE stack overflow"); return false; }
        app->while_stack[app->while_sp++] = (WhileFrame){ .while_line_idx = cur_line_idx, .while_stmt_idx = cur_stmt_idx };
        return true;
    }

    // Condition false: skip to matching WEND and continue after it.
    int wli = 0, wsi = 0;
    if (!find_matching_wend(app, cur_line_idx, cur_stmt_idx, current_line, &wli, &wsi)) return false;

    *line_idx = wli;
    *stmt_idx = wsi + 1; // continue after WEND
    return true;
}

static bool exec_wend(App *app, int current_line, int *line_idx, int *stmt_idx) {
    if (app->while_sp <= 0) { runtime_error(app, current_line, "WEND without WHILE"); return false; }
    WhileFrame fr = app->while_stack[--app->while_sp];
    *line_idx = fr.while_line_idx;
    *stmt_idx = fr.while_stmt_idx; // jump back to WHILE statement
    return true;
}

/* ---- DO/LOOP ---- */
static bool exec_do(App *app, Parser *p, int current_line, int cur_line_idx, int cur_stmt_idx,
                    int *line_idx, int *stmt_idx) {
    // DO [WHILE <expr>] | DO [UNTIL <expr>]
    skip_ws(p);

    DoPreCondKind kind = DO_PRE_NONE;
    double cond = 0.0;

    if (starts_ci(p->s, "WHILE") && is_word_boundary(p->s[5])) {
        p->s += 5;
        kind = DO_PRE_WHILE;
        if (!parse_cond_or(app, p, &cond)) return false;
    } else if (starts_ci(p->s, "UNTIL") && is_word_boundary(p->s[5])) {
        p->s += 5;
        kind = DO_PRE_UNTIL;
        if (!parse_cond_or(app, p, &cond)) return false;
    }

    bool enter = true;
    if (kind == DO_PRE_WHILE) enter = (cond != 0.0);
    else if (kind == DO_PRE_UNTIL) enter = (cond == 0.0);

    if (!enter) {
        // Skip loop body: jump to matching LOOP and continue after it.
        int lli = 0, lsi = 0;
        if (!find_matching_loop(app, cur_line_idx, cur_stmt_idx, current_line, &lli, &lsi)) return false;
        *line_idx = lli;
        *stmt_idx = lsi + 1;
        return true;
    }

    // Entering loop: push frame so LOOP can jump back here.
    if (app->do_sp >= 128) { runtime_error(app, current_line, "DO stack overflow"); return false; }
    app->do_stack[app->do_sp++] = (DoFrame){
        .do_line_idx = cur_line_idx,
        .do_stmt_idx = cur_stmt_idx,
        .pre_kind = kind
    };
    return true;
}

static bool exec_loop(App *app, Parser *p, int current_line, int cur_line_idx, int cur_stmt_idx, int *line_idx, int *stmt_idx) {
    // LOOP [WHILE <expr>] | LOOP [UNTIL <expr>]
    if (app->do_sp <= 0) { runtime_error(app, current_line, "LOOP without DO"); return false; }

    skip_ws(p);

    bool has_cond = false;
    bool while_form = false;
    double cond = 0.0;

    if (starts_ci(p->s, "WHILE") && is_word_boundary(p->s[5])) {
        p->s += 5;
        has_cond = true;
        while_form = true;
        if (!parse_cond_or(app, p, &cond)) return false;
    } else if (starts_ci(p->s, "UNTIL") && is_word_boundary(p->s[5])) {
        p->s += 5;
        has_cond = true;
        while_form = false;
        if (!parse_cond_or(app, p, &cond)) return false;
    }

    bool again = true;
    if (has_cond) {
        if (while_form) again = (cond != 0.0);
        else again = (cond == 0.0); // UNTIL: repeat while condition is false
    }

    DoFrame fr = app->do_stack[--app->do_sp];

    if (again) {
        *line_idx = fr.do_line_idx;
        *stmt_idx = fr.do_stmt_idx; // jump back to DO statement (re-evaluates pre-test if present)
        return true;
    }

    // Exit loop: continue after this LOOP statement.
    *line_idx = cur_line_idx;
    *stmt_idx = cur_stmt_idx + 1;
    return true;
}


static bool exec_exit_do(App *app, int current_line, int *line_idx, int *stmt_idx) {
    // EXIT DO: exit innermost DO/LOOP
    if (app->do_sp <= 0) { runtime_error(app, current_line, "EXIT DO without DO"); return false; }

    DoFrame fr = app->do_stack[app->do_sp - 1];

    int lli = 0, lsi = 0;
    if (!find_matching_loop(app, fr.do_line_idx, fr.do_stmt_idx, current_line, &lli, &lsi)) return false;

    // Discard the innermost DO frame; we will continue after its matching LOOP.
    app->do_sp--;

    *line_idx = lli;
    *stmt_idx = lsi + 1;
    return true;
}





/* ---- GOSUB/RETURN ---- */
static bool exec_gosub(App *app, Parser *p, int current_line, int cur_line_idx, int cur_stmt_idx, int *line_idx, int *stmt_idx) {
    double ln = 0.0;
    if (!parse_expr(app, p, &ln)) { runtime_error(app, current_line, "GOSUB expects line number"); return false; }
    int target_line = (int)llround(ln);
    int idx = program_find_index(&app->prog, target_line);
    if (idx < 0) { runtime_error(app, current_line, "GOSUB target not found"); return false; }

    if (app->gosub_sp >= 128) { runtime_error(app, current_line, "GOSUB stack overflow"); return false; }
    if (app->chain_active) {
        // Returning into an in-progress chain (typically an IF tail with ':')
        app->gosub_stack[app->gosub_sp++] = (GosubFrame){
            .kind = GOSUB_RET_CHAIN,
            .ret_line_idx = app->chain_base_line_idx,
            .ret_stmt_idx = app->chain_base_stmt_idx,
            .ret_chain_next_si = cur_stmt_idx + 1,
            .ret_chain_text = (app->chain_text ? strdup(app->chain_text) : NULL),
        };
    } else {
        // Normal case: return to the next top-level statement in the same line
        app->gosub_stack[app->gosub_sp++] = (GosubFrame){
            .kind = GOSUB_RET_NORMAL,
            .ret_line_idx = cur_line_idx,
            .ret_stmt_idx = cur_stmt_idx + 1,
            .ret_chain_next_si = 0,
            .ret_chain_text = NULL,
        };
    }

    *line_idx = idx;
    *stmt_idx = 0;
    return true;
}

static bool exec_return(App *app, int current_line, int *line_idx, int *stmt_idx) {
    if (app->gosub_sp <= 0) { runtime_error(app, current_line, "RETURN without GOSUB"); return false; }

    GosubFrame fr = app->gosub_stack[--app->gosub_sp];

    if (fr.kind == GOSUB_RET_CHAIN) {
        // Schedule a resume into a chain at the owning (line_idx, stmt_idx).
        if (app->resume_chain_text) { free(app->resume_chain_text); app->resume_chain_text = NULL; }
        app->resume_chain_pending = true;
        app->resume_chain_line_idx = fr.ret_line_idx;
        app->resume_chain_stmt_idx = fr.ret_stmt_idx;
        app->resume_chain_next_si = fr.ret_chain_next_si;
        app->resume_chain_text = fr.ret_chain_text; // take ownership
        fr.ret_chain_text = NULL;
    }

    if (fr.kind == GOSUB_RET_KEYTRAP) {
        app->on_key_in_progress = false;
    }
    if (fr.kind == GOSUB_RET_TIMERTRAP) {
        app->timer_in_progress = false;
        /* GW-BASIC: if the handler took longer than the interval, missed ticks do not queue;
           schedule the next fire from *now* on RETURN. If the handler disabled the trap
           via TIMER OFF or changed interval/target, respect that. */
        if (app->timer_enabled && !app->timer_stopped && app->on_timer_gosub_line > 0 && app->on_timer_interval > 0.0) {
            double now = timer_now_sec();
            timer_schedule_next(app, now);
        }
    }
    *line_idx = fr.ret_line_idx;
    *stmt_idx = fr.ret_stmt_idx;

    if (fr.ret_chain_text) free(fr.ret_chain_text);
    return true;
}

/* ---- ERASE (arrays) ---- */
static bool exec_erase(App *app, Parser *p, int current_line) {
    // ERASE name[,name...]
    // GW-BASIC: only arrays; name must not include subscripts.
    bool any = false;
    for (;;) {
        char *name = NULL;
        if (!parse_identifier(p, &name)) {
            if (!any) runtime_error(app, current_line, "Bad ERASE");
            return false;
        }
        any = true;

        skip_ws(p);
        if (*p->s == '(') {
            free(name);
            runtime_error(app, current_line, "Bad ERASE");
            return false;
        }

        Var *v = vars_lookup(app, name);
        if (!v || !v->is_array) {
            free(name);
            runtime_error(app, current_line, "Illegal function call");
            return false;
        }

        var_erase_array(v);
        free(name);

        skip_ws(p);
        if (consume(p, ',')) continue;
        break;
    }
    return true;
}

/* ---- SWAP (scalars or array elements) ---- */
typedef struct {
    Var *v;
    bool is_str;
    bool is_arr_elem;
    int nd;
    int idx[5];
    char *name;
} SwapRef;

static bool parse_swap_ref(App *app, Parser *p, int current_line, SwapRef *out) {
    memset(out, 0, sizeof(*out));
    char *name = NULL;
    if (!parse_identifier(p, &name)) return false;
    bool is_str = ident_is_string_var(app, name);

    bool is_arr_elem = false;
    int nd = 0;
    int idx[5] = {0,0,0,0,0};
    skip_ws(p);
    if (consume(p, '(')) {
        is_arr_elem = true;
        if (!parse_array_indices(app, p, &nd, idx)) { free(name); runtime_error(app, current_line, "Bad array index"); return false; }
    }

    Var *v = vars_get_or_create(app, name);

    // GW-BASIC semantics: a bare name (no parentheses) refers to the scalar
    // variable, even if an array of the same base name exists.
    // Therefore, do NOT reject SWAP Q,Q when Q() is DIMed; it's a valid scalar SWAP.

    // Auto-dimension undefined arrays like GW-BASIC when an element is referenced.
    if (is_arr_elem && !v->is_array) {
        app->option_base_locked = true;
        int dim_max[5] = {10,10,10,10,10};
        bool okdim = is_str ? var_define_str_array(v, nd, app->option_base, dim_max)
                            : var_define_num_array(v, nd, app->option_base, dim_max);
        if (!okdim) { free(name); runtime_error(app, current_line, "Out of memory"); return false; }
    }

    out->v = v;
    out->is_str = is_str;
    out->is_arr_elem = is_arr_elem;
    out->nd = nd;
    memcpy(out->idx, idx, sizeof(idx));
    out->name = name; // ownership
    return true;
}

static void free_swap_ref(SwapRef *r) {
    if (r && r->name) free(r->name);
    if (r) r->name = NULL;
}

static bool exec_swap(App *app, Parser *p, int current_line) {
    // SWAP a,b
    SwapRef a = {0}, b = {0};
    if (!parse_swap_ref(app, p, current_line, &a)) { runtime_error(app, current_line, "Bad SWAP"); return false; }
    skip_ws(p);
    if (!consume(p, ',')) { free_swap_ref(&a); runtime_error(app, current_line, "Bad SWAP"); return false; }
    if (!parse_swap_ref(app, p, current_line, &b)) { free_swap_ref(&a); runtime_error(app, current_line, "Bad SWAP"); return false; }

    if (a.is_str != b.is_str) {
        free_swap_ref(&a); free_swap_ref(&b);
        runtime_error(app, current_line, "Type mismatch");
        return false;
    }

    if (a.is_str) {
        char **pa = NULL;
        char **pb = NULL;
        if (a.is_arr_elem) {
            if (!a.v->is_array || !a.v->sarr) { free_swap_ref(&a); free_swap_ref(&b); runtime_error(app, current_line, "Subscript out of range"); return false; }
            size_t offa = 0;
            if (!array_calc_offset(a.v, a.nd, a.idx, &offa)) { free_swap_ref(&a); free_swap_ref(&b); runtime_error(app, current_line, "Subscript out of range"); return false; }
            pa = &a.v->sarr[offa];
            if (!*pa) *pa = xstrdup("");
        } else {
            pa = &a.v->str;
            if (!*pa) *pa = xstrdup("");
        }
        if (b.is_arr_elem) {
            if (!b.v->is_array || !b.v->sarr) { free_swap_ref(&a); free_swap_ref(&b); runtime_error(app, current_line, "Subscript out of range"); return false; }
            size_t offb = 0;
            if (!array_calc_offset(b.v, b.nd, b.idx, &offb)) { free_swap_ref(&a); free_swap_ref(&b); runtime_error(app, current_line, "Subscript out of range"); return false; }
            pb = &b.v->sarr[offb];
            if (!*pb) *pb = xstrdup("");
        } else {
            pb = &b.v->str;
            if (!*pb) *pb = xstrdup("");
        }

        char *tmp = *pa;
        *pa = *pb;
        *pb = tmp;
    } else {
        double *pa = NULL;
        double *pb = NULL;
        if (a.is_arr_elem) {
            if (!a.v->is_array || !a.v->arr) { free_swap_ref(&a); free_swap_ref(&b); runtime_error(app, current_line, "Subscript out of range"); return false; }
            size_t offa = 0;
            if (!array_calc_offset(a.v, a.nd, a.idx, &offa)) { free_swap_ref(&a); free_swap_ref(&b); runtime_error(app, current_line, "Subscript out of range"); return false; }
            pa = &a.v->arr[offa];
        } else {
            pa = &a.v->num;
        }
        if (b.is_arr_elem) {
            if (!b.v->is_array || !b.v->arr) { free_swap_ref(&a); free_swap_ref(&b); runtime_error(app, current_line, "Subscript out of range"); return false; }
            size_t offb = 0;
            if (!array_calc_offset(b.v, b.nd, b.idx, &offb)) { free_swap_ref(&a); free_swap_ref(&b); runtime_error(app, current_line, "Subscript out of range"); return false; }
            pb = &b.v->arr[offb];
        } else {
            pb = &b.v->num;
        }

        double tmp = *pa;
        *pa = *pb;
        *pb = tmp;
    }

    free_swap_ref(&a);
    free_swap_ref(&b);
    return true;
}

/* ---- Assignment (scalar or array element) ---- */
static bool exec_assignment(App *app, Parser *p, int current_line) {
    char *name = NULL;
    if (!parse_identifier(p, &name)) { runtime_error(app, current_line, "Expected variable name"); return false; }
    bool is_str = ident_is_string_var(app, name);

    skip_ws(p);
    bool is_arr_elem = false;
    int nd = 0;
    int idx[5] = {0,0,0,0,0};
    if (consume(p, '(')) {
        is_arr_elem = true;
        if (!parse_array_indices(app, p, &nd, idx)) { free(name); runtime_error(app, current_line, "Bad array index"); return false; }
    }

    if (!consume(p, '=')) { free(name); runtime_error(app, current_line, "Expected '='"); return false; }

    Var *v = vars_get_or_create(app, name);

    if (is_str) {
        char *sv = NULL;
        if (!parse_string_value(app, p, &sv)) { free(name); runtime_error_or_pending(app, current_line, "Bad string assignment"); return false; }

        v->kind = V_STR;
        if (is_arr_elem) {
// Auto-dimension undefined arrays like GW-BASIC (default upper bound 10 per dimension)
if (!v->is_array) {
    app->option_base_locked = true;
    int dim_max[5] = {10,10,10,10,10};
    if (!var_define_str_array(v, nd, app->option_base, dim_max)) { free(sv); free(name); runtime_error(app, current_line, "Out of memory"); return false; }
}
            if (!v->is_array || !v->sarr) { free(sv); free(name); runtime_error(app, current_line, "Subscript out of range"); return false; }
            size_t off = 0;
            if (!array_calc_offset(v, nd, idx, &off)) { free(sv); free(name); runtime_error(app, current_line, "Subscript out of range"); return false; }
            free(v->sarr[off]);
            v->sarr[off] = sv; // take ownership
        } else {
            // Scalar assignment should not destroy an existing array of the same name (GW-BASIC allows both).
            free(v->str);
            v->str = sv;
        }
    } else {
        double nv = 0.0;
        if (!parse_expr(app, p, &nv)) { free(name); runtime_error_or_pending(app, current_line, "Bad numeric assignment"); return false; }
        v->kind = V_NUM;
        if (is_arr_elem) {
// Auto-dimension undefined arrays like GW-BASIC (default upper bound 10 per dimension)
if (!v->is_array) {
    app->option_base_locked = true;
    int dim_max[5] = {10,10,10,10,10};
    if (!var_define_num_array(v, nd, app->option_base, dim_max)) { free(name); runtime_error(app, current_line, "Out of memory"); return false; }
}
            if (!v->is_array || !v->arr) { free(name); runtime_error(app, current_line, "Subscript out of range"); return false; }
            size_t off = 0;
            if (!array_calc_offset(v, nd, idx, &off)) { free(name); runtime_error(app, current_line, "Subscript out of range"); return false; }
            nv = coerce_numeric_store(app, v, name, nv);
            v->arr[off] = nv;
        } else {
            // Scalar assignment should not destroy an existing array of the same name (GW-BASIC allows both).
            nv = coerce_numeric_store(app, v, name, nv);
            v->num = nv;
        }
    }

    free(name);
    return true;
}


/* ===================== MID$ assignment (l-value) ===================== */
static bool exec_mid_assign(App *app, Parser *p, int current_line) {
    // Expect MID$(
    if (!starts_ci(p->s, "MID$")) return false;
    p->s += 4;
    skip_ws(p);
    if (!consume(p, '(')) { runtime_error(app, current_line, "Bad MID$ assignment"); return false; }

    // First argument: string variable (optionally string array element)
    skip_ws(p);
    char *tname = NULL;
    if (!parse_identifier(p, &tname)) { runtime_error(app, current_line, "Bad MID$ assignment"); return false; }
    if (!name_is_string(tname)) { free(tname); runtime_error(app, current_line, "Type mismatch"); return false; }

    Var *tv = vars_get_or_create(app, tname);

    bool targ_is_arr = false;
    int nd = 0;
    int idx[5] = {0,0,0,0,0};
    skip_ws(p);
    if (consume(p, '(')) {
        targ_is_arr = true;
        if (!parse_array_indices(app, p, &nd, idx)) { free(tname); runtime_error(app, current_line, "Bad array index"); return false; }
    }

    // Expect comma after target
    skip_ws(p);
    if (!consume(p, ',')) { free(tname); runtime_error(app, current_line, "Bad MID$ assignment"); return false; }

    // pos expression
    double dpos = 0.0;
    if (!parse_expr(app, p, &dpos)) { free(tname); runtime_error_or_pending(app, current_line, "Bad MID$ assignment"); return false; }
    int pos = (int)llround(dpos);

    // optional len expression
    int len_spec = -1;
    skip_ws(p);
    if (consume(p, ',')) {
        double dlen = 0.0;
        if (!parse_expr(app, p, &dlen)) { free(tname); runtime_error_or_pending(app, current_line, "Bad MID$ assignment"); return false; }
        len_spec = (int)llround(dlen);
    }

    skip_ws(p);
    if (!consume(p, ')')) { free(tname); runtime_error(app, current_line, "Bad MID$ assignment"); return false; }

    skip_ws(p);
    if (!consume(p, '=')) { free(tname); runtime_error(app, current_line, "Expected '='"); return false; }

    // RHS string value
    char *rhs = NULL;
    if (!parse_string_value(app, p, &rhs)) { free(tname); runtime_error_or_pending(app, current_line, "Bad MID$ assignment"); return false; }

    int rhs_len = (int)strlen(rhs);
    /* rlen: if LEN omitted, GW-BASIC uses the remainder of the string from POS to end */
    int rlen = (len_spec >= 0) ? len_spec : -1;

    // Validate indices per GW-BASIC style (additional bounds checked after we know target string length)
    if (pos < 1) { free(rhs); free(tname); runtime_error(app, current_line, "Illegal function call"); return false; }
    if (rlen < -1) { free(rhs); free(tname); runtime_error(app, current_line, "Illegal function call"); return false; }
    if (len_spec >= 0 && rlen == 0) { free(rhs); free(tname); return true; }

    // Resolve target slot (string pointer we own)
    char **slot = NULL;
    if (targ_is_arr) {
        // Auto-dimension undefined string arrays like GW-BASIC (default upper bound 10 per dimension)
        if (!tv->is_array) {
            app->option_base_locked = true;
            int dim_max[5] = {10,10,10,10,10};
            if (!var_define_str_array(tv, nd, app->option_base, dim_max)) {
                free(rhs);
                free(tname);
                runtime_error(app, current_line, "Out of memory");
                return false;
            }
        }
        if (!tv->is_array || !tv->sarr) { free(rhs); free(tname); runtime_error(app, current_line, "Subscript out of range"); return false; }
        size_t off = 0;
        if (!array_calc_offset(tv, nd, idx, &off)) { free(rhs); free(tname); runtime_error(app, current_line, "Subscript out of range"); return false; }
        slot = &tv->sarr[off];
        tv->kind = V_STR;
    } else {
        tv->is_array = false;
        tv->kind = V_STR;
        slot = &tv->str;
    }

    if (!*slot) *slot = xstrdup("");

    const char *old = *slot;
    int old_len = (int)strlen(old);

    // GW-BASIC semantics: POS must be within existing string (1..LEN(A$))
    if (pos > old_len) { free(rhs); free(tname); runtime_error(app, current_line, "Illegal function call"); return false; }

    // If LEN omitted, it runs to end-of-string
    if (rlen < 0) rlen = old_len - (pos - 1);

    // LEN must not run past end-of-string
    if (rlen < 0 || (pos - 1) + rlen > old_len) { free(rhs); free(tname); runtime_error(app, current_line, "Illegal function call"); return false; }
    if (rlen == 0) { free(rhs); free(tname); return true; }

    // Make a mutable copy of the existing string (length unchanged)
    char *ns = xstrdup(old);
    if (!ns) { free(rhs); free(tname); runtime_error(app, current_line, "Out of memory"); return false; }

    // Overwrite only the provided RHS chars (do NOT space-fill the remainder of the slice)
    int start = pos - 1;
    int nwrite = (rhs_len < rlen) ? rhs_len : rlen;
    for (int i = 0; i < nwrite; i++) ns[start + i] = rhs[i];

    free(*slot);
    *slot = ns;
    free(rhs);
    free(tname);
    return true;
}

/* ===================== Execute a single statement ===================== */

static bool exec_single_statement(App *app, const char *stmt, int current_line, int cur_li, int cur_si, int *line_idx, int *stmt_idx) {
    // If we just RETURNed from a GOSUB that occurred inside a ':' chain (usually inside an IF tail),
    // resume execution inside that chain instead of re-running the owning statement from the top.
    if (app->resume_chain_pending &&
        app->resume_chain_line_idx == cur_li &&
        app->resume_chain_stmt_idx == cur_si &&
        app->resume_chain_text) {

        int start_si = app->resume_chain_next_si;
        char *chain = app->resume_chain_text;

        // Clear pending *before* executing (so nested GOSUBs/RETURNs don't clobber state).
        app->resume_chain_pending = false;
        app->resume_chain_text = NULL;

        bool ok = exec_statement_chain_from(app, chain, current_line, cur_li, cur_si, start_si, line_idx, stmt_idx);
        free(chain);
        return ok;
    }

    char *tmp = xstrdup(stmt);
    char *s = trim(tmp);
    if (*s == 0) { free(tmp); return true; }

/* Block IF markers: ELSE / END IF / ENDIF */
if (current_line >= 0) {
    
    if (stmt_is_block_elseif(s)) {
        int k = -1;
        IfBlockMapEntry *e = program_ifmap_find_by_elseif(&app->prog, cur_li, cur_si, &k);
        if (!e) { runtime_error(app, current_line, "ELSEIF without IF"); free(tmp); return false; }
        /* Must have an active IF context */
        if (app->if_sp <= 0) { runtime_error(app, current_line, "ELSEIF without IF context"); free(tmp); return false; }
        IfExecFrame *fr = &app->if_stack[app->if_sp - 1];

        /* If a prior branch has already executed (IF or prior ELSEIF), skip the rest of this IF block */
        if (fr->branch_taken || fr->active_branch >= 0) {
            prog_goto_stmt(app, e->end_pos.li, e->end_pos.si, line_idx, stmt_idx);
            free(tmp);
            return true;
        }

        /* Evaluate ELSEIF condition and require block-form THEN with nothing after THEN */
        const char *ss = s;
        while (*ss == ' ' || *ss == '\t') ss++;
        if (!strncasecmp(ss, "ELSEIF", 6)) ss += 6;
        else if (!strncasecmp(ss, "ELSE IF", 7)) ss += 7;
        else { runtime_error(app, current_line, "Bad ELSEIF"); free(tmp); return false; }

        Parser p = { ss };
        bool cond = false;
        if (!eval_condition(app, &p, &cond)) { runtime_error(app, current_line, "Bad ELSEIF condition"); free(tmp); return false; }
        skip_ws(&p);
        const char *th = p.s;
        if (!(toupper((unsigned char)th[0])=='T' && toupper((unsigned char)th[1])=='H' &&
              toupper((unsigned char)th[2])=='E' && toupper((unsigned char)th[3])=='N' &&
              is_word_boundary(th[4]))) {
            runtime_error(app, current_line, "ELSEIF missing THEN");
            free(tmp);
            return false;
        }
        p.s += 4;
        Parser pt = { p.s };
        skip_ws(&pt);
        if (*pt.s != 0) {
            runtime_error(app, current_line, "ELSEIF must be block form");
            free(tmp);
            return false;
        }

        if (cond) {
            fr->branch_taken = true;
            fr->active_branch = 1 + k; /* 0=IF, 1..N=ELSEIF */
            /* Execute this ELSEIF block by continuing to next statement */
            free(tmp);
            return true;
        }

        /* Condition false: jump to next ELSEIF / ELSE / END IF marker */
        if (k >= 0 && (k + 1) < e->elseif_count) {
            prog_goto_stmt(app, e->elseif_pos[k + 1].li, e->elseif_pos[k + 1].si, line_idx, stmt_idx);
        } else if (e->has_else) {
            prog_goto_stmt(app, e->else_pos.li, e->else_pos.si, line_idx, stmt_idx);
        } else {
            prog_goto_stmt(app, e->end_pos.li, e->end_pos.si, line_idx, stmt_idx);
        }
        free(tmp);
        return true;
    }

if (stmt_is_block_else(s)) {
        IfBlockMapEntry *e = program_ifmap_find_by_else(&app->prog, cur_li, cur_si);
        if (!e) { runtime_error(app, current_line, "ELSE without IF"); free(tmp); return false; }
        if (app->if_sp <= 0) { runtime_error(app, current_line, "ELSE without IF context"); free(tmp); return false; }
        IfExecFrame *fr = &app->if_stack[app->if_sp - 1];

        /* If a prior branch executed (IF or ELSEIF), skip the ELSE block */
        if (fr->branch_taken || fr->active_branch >= 0) {
            prog_goto_stmt(app, e->end_pos.li, e->end_pos.si, line_idx, stmt_idx);
            free(tmp);
            return true;
        }

        /* Otherwise, ELSE becomes the active branch; execute block by continuing */
        fr->branch_taken = true;
        fr->active_branch = -1; /* ELSE */
        free(tmp);
        return true;
    }
    if (stmt_is_block_end_if(s)) {
        /* End of a BLOCK IF. Pop IF execution context if present. */
        if (app->if_sp > 0) app->if_sp--;
        free(tmp);
        return true;
    }
}

    // REM
    if (starts_ci(s, "REM") && is_word_boundary(s[3])) { free(tmp); return true; }


// DATA is non-executable (its contents are scanned at RUN time into the DATA pool)
if (starts_ci(s, "DATA") && is_word_boundary(s[4])) { free(tmp); return true; }


// DEF FNname[(args...)] = <numeric-expression>
if (starts_ci(s, "DEF") && is_word_boundary(s[3])) {
    Parser dp = { s + 3 };
    skip_ws(&dp);
    char *fname = NULL;
    if (!parse_identifier(&dp, &fname)) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
    // In GW-BASIC, user-defined function names begin with FN (e.g., FNSQR).
    // Here we store the whole name (e.g., FNSQR).
    // If the parsed identifier doesn't already start with FN, prepend.
    if (strncasecmp(fname, "FN", 2) != 0) {
        char *full = (char*)malloc(strlen(fname) + 3);
        sprintf(full, "FN%s", fname);
        free(fname);
        fname = full;
    }

    // Optional parameter list
    char **params = NULL;
    int pcount = 0;
    skip_ws(&dp);
    if (consume(&dp, '(')) {
        skip_ws(&dp);
        if (!consume(&dp, ')')) {
            while (1) {
                char *pn = NULL;
                if (!parse_identifier(&dp, &pn)) { free(fname); free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
                params = (char**)realloc(params, (size_t)(pcount + 1) * sizeof(char*));
                params[pcount++] = pn;
                skip_ws(&dp);
                if (consume(&dp, ',')) continue;
                if (!consume(&dp, ')')) { free(fname); free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
                break;
            }
        }
    }

    skip_ws(&dp);
    if (!consume(&dp, '=')) { free(fname); free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
    const char *rhs = dp.s;
    while (*rhs == ' ' || *rhs == '	') rhs++;
    if (*rhs == 0) { free(fname); free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }

    int ret_kind = name_is_string(fname) ? 1 : 0;

    if (!program_fndef_set(&app->prog, fname, params, pcount, rhs, ret_kind)) {
        free(fname);
        if (params) { for (int i=0;i<pcount;i++) free(params[i]); free(params); }
        free(tmp);
        runtime_error(app, current_line, "Out of memory");
        return false;
    }
    free(fname);
    // params consumed by program_fndef_set (it takes ownership)
    if (params) free(params);
    free(tmp);
    return true;
}

// READ var[, var ...]
if (starts_ci(s, "READ") && is_word_boundary(s[4])) {
    Parser rp = { s + 4 };
    for (;;) {
        skip_ws(&rp);
        if (*rp.s == 0) { free(tmp); runtime_error(app, current_line, "Expected variable name"); return false; }

        // parse variable name
        char *name = NULL;
        if (!parse_identifier(&rp, &name)) { free(tmp); runtime_error(app, current_line, "Expected variable name"); return false; }
        bool is_str = ident_is_string_var(app, name);

        // optional array indices
        bool is_arr_elem = false;
        int nd = 0;
        int idx[5] = {0,0,0,0,0};
        skip_ws(&rp);
        if (consume(&rp, '(')) {
            is_arr_elem = true;
            if (!parse_array_indices(app, &rp, &nd, idx)) { free(name); free(tmp); runtime_error(app, current_line, "Bad array index"); return false; }
        }

        // fetch next DATA item
        if (app->prog.data_ptr >= app->prog.data_count) { free(name); free(tmp); runtime_error(app, current_line, "Out of DATA"); return false; }
        DataItem di = app->prog.data[app->prog.data_ptr++];

        Var *v = vars_get_or_create(app, name);

        if (is_str) {
            v->kind = V_STR;
            char *sv = xstrdup(di.text ? di.text : "");
            if (is_arr_elem) {
                // Auto-dimension undefined string arrays like GW-BASIC (default upper bound 10 per dimension)
                if (!v->is_array) {
                    app->option_base_locked = true;
                    int dim_max[5] = {10,10,10,10,10};
                    if (!var_define_str_array(v, nd, app->option_base, dim_max)) {
                        free(sv);
                        free(name);
                        free(tmp);
                        runtime_error(app, current_line, "Out of memory");
                        return false;
                    }
                }
                if (!v->is_array || !v->sarr) { free(sv); free(name); free(tmp); runtime_error(app, current_line, "Subscript out of range"); return false; }
                size_t off = 0;
                if (!array_calc_offset(v, nd, idx, &off)) { free(sv); free(name); free(tmp); runtime_error(app, current_line, "Subscript out of range"); return false; }
                free(v->sarr[off]);
                v->sarr[off] = sv;
            } else {
                v->is_array = false;
                free(v->str);
                v->str = sv;
            }
        } else {
            // numeric target
            // GW-BASIC: blank numeric DATA items (including trailing comma) read as 0.
            // If our DATA parser marked an empty item as quoted (e.g., ""), be permissive and treat "" as 0 too.
            if (di.quoted) {
                const char *qt = di.text ? di.text : "";
                Parser qp = { qt };
                skip_ws(&qp);
                if (*qp.s != 0) { free(name); free(tmp); runtime_error(app, current_line, "Type mismatch"); return false; }
                // quoted-but-empty -> 0
            }

            double nv = 0.0;
            // empty numeric DATA item -> 0 (GW-BASIC behavior)
            const char *t = di.text ? di.text : "";
            Parser np = { t };
            skip_ws(&np);
            if (*np.s == 0) {
                nv = 0.0;
            } else {
                if (!parse_number(&np, &nv)) { free(name); free(tmp); runtime_error(app, current_line, "Type mismatch"); return false; }
                skip_ws(&np);
                if (*np.s != 0) { free(name); free(tmp); runtime_error(app, current_line, "Type mismatch"); return false; }
            }

            v->kind = V_NUM;
            if (is_arr_elem) {
                // Auto-dimension undefined numeric arrays like GW-BASIC (default upper bound 10 per dimension)
                if (!v->is_array) {
                    app->option_base_locked = true;
                    int dim_max[5] = {10,10,10,10,10};
                    if (!var_define_num_array(v, nd, app->option_base, dim_max)) {
                        free(name);
                        free(tmp);
                        runtime_error(app, current_line, "Out of memory");
                        return false;
                    }
                }
                if (!v->is_array || !v->arr) { free(name); free(tmp); runtime_error(app, current_line, "Subscript out of range"); return false; }
                size_t off = 0;
                if (!array_calc_offset(v, nd, idx, &off)) { free(name); free(tmp); runtime_error(app, current_line, "Subscript out of range"); return false; }
                nv = coerce_numeric_store(app, v, name, nv);
                nv = coerce_numeric_store(app, v, name, nv);
                nv = coerce_numeric_store(app, v, name, nv);
                v->arr[off] = nv;
            } else {
                v->is_array = false;
                nv = coerce_numeric_store(app, v, name, nv);
                v->num = nv;
            }
        }

        free(name);

        skip_ws(&rp);
        if (consume(&rp, ',')) continue;
        break;
    }
    free(tmp);
    return true;
}



// RESTORE [line]
if (starts_ci(s, "RESTORE") && is_word_boundary(s[7])) {
    Parser rp = { s + 7 };
    skip_ws(&rp);
    if (*rp.s == 0) {
        app->prog.data_ptr = 0;
        free(tmp);
        return true;
    } else {
        double dv = 0.0;
        if (!parse_number(&rp, &dv)) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
        int ln = (int)dv;
        if (fabs(dv - (double)ln) > 1e-9) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
        // Set DATA pointer to first item at/after this line number (GW-BASIC behavior)
        size_t i = 0;
        while (i < app->prog.data_count && app->prog.data[i].line_no < ln) i++;
        app->prog.data_ptr = i;
        free(tmp);
        return true;
    }
}



// KEY ON / KEY OFF / KEY n, expr$
if (starts_ci(s, "KEY") && is_word_boundary(s[3])) {
    Parser kp = { s + 3 };
    skip_ws(&kp);
    // KEY(n) ON / KEY(n) OFF (per-key enable for ON KEY(n) GOSUB traps)
    if (*kp.s == '(') {
        kp.s++;
        double dvn = 0.0;
        if (!parse_expr(app, &kp, &dvn)) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
        int n = (int)dvn;
        if (fabs(dvn - (double)n) > 1e-9) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
        skip_ws(&kp);
        if (!consume(&kp, ')')) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
        if (n < 1 || n > 10) { free(tmp); runtime_error(app, current_line, "Illegal function call"); return false; }
        skip_ws(&kp);
        if (consume_word_ci(&kp, "ON")) {
            skip_ws(&kp);
            if (*kp.s != 0) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
            app->on_key_enabled[n-1] = true;
            free(tmp);
            return true;
        }
        if (consume_word_ci(&kp, "OFF")) {
            skip_ws(&kp);
            if (*kp.s != 0) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
            app->on_key_enabled[n-1] = false;
            free(tmp);
            return true;
        }
        free(tmp);
        runtime_error(app, current_line, "Syntax error");
        return false;
    }


    // KEY ON
    if (consume_word_ci(&kp, "ON")) {
        skip_ws(&kp);
        if (*kp.s != 0) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
        app->key_trap_enabled = true;
        free(tmp);
        return true;
    }

    // KEY OFF
    if (consume_word_ci(&kp, "OFF")) {
        skip_ws(&kp);
        if (*kp.s != 0) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
        app->key_trap_enabled = false;
        free(tmp);
        return true;
    }

    // KEY n, expr$
    double dv = 0.0;
    if (!parse_expr(app, &kp, &dv)) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
    int n = (int)dv;
    if (fabs(dv - (double)n) > 1e-9) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
    if (n < 1 || n > 10) { free(tmp); runtime_error(app, current_line, "Illegal function call"); return false; }
    skip_ws(&kp);
    if (*kp.s != ',') { free(tmp); runtime_error(app, current_line, "Expected ','"); return false; }
    kp.s++; // comma

    char *macro = NULL;
    if (!parse_string_value(app, &kp, &macro)) { free(tmp); runtime_error(app, current_line, "Type mismatch"); return false; }
    skip_ws(&kp);
    if (*kp.s != 0) { free(tmp); free(macro); runtime_error(app, current_line, "Syntax error"); return false; }

    int idx = n - 1;
    if (app->key_macros[idx]) { free(app->key_macros[idx]); app->key_macros[idx] = NULL; }
    if (macro && *macro) {
        app->key_macros[idx] = macro; // take ownership
    } else {
        free(macro); // treat empty string as unassigned
    }

    free(tmp);
    return true;
}

    
    // RANDOMIZE [expr|TIMER]  (GW-BASIC compatible)
    if (starts_ci(s, "RANDOMIZE") && is_word_boundary(s[9])) {
        const char *a = s + 9;
        while (*a && isspace((unsigned char)*a)) a++;

        unsigned seed = (unsigned)time(NULL);
        if (*a) {
            Parser p = { .s = a };
            double v = 0.0;
            if (!parse_expr(app, &p, &v)) { runtime_error(app, current_line, "Bad RANDOMIZE expression"); free(tmp); return false; }
            skip_ws(&p);
            if (*p.s) { runtime_error(app, current_line, "Bad RANDOMIZE expression"); free(tmp); return false; }

            if (v < 0) v = -v;
            // TIMER is fractional seconds; multiply to spread entropy a bit.
            seed = (unsigned)(v * 1000.0);
            if (seed == 0) seed = (unsigned)time(NULL);
        }
        srand(seed);
        free(tmp);
        return true;
    }

// MID$ assignment: MID$(A$, pos [,len]) = expr$
    if (starts_ci(s, "MID$")) {
        Parser mp = { s };
        bool ok = exec_mid_assign(app, &mp, current_line);
        free(tmp);
        return ok;
    }


    // RUN [line]
    if (starts_ci(s, "RUN") && is_word_boundary(s[3])) {
        bool ok = exec_run_stmt(app, s, current_line, line_idx, stmt_idx);
        free(tmp);
        return ok;
    }


    // STOP (debug halt; variables preserved)
    if (starts_ci(s, "STOP") && is_word_boundary(s[4])) {
        app->stop_flag = true;

    set_run_state(app, RUN_STOPPED);
        return false; // abort execution cleanly
    }

// END / SYSTEM  (GW-BASIC: SYSTEM exits to DOS; WBASIC treats SYSTEM as END for compatibility)
    if (starts_ci(s, "END") && is_word_boundary(s[3])) { free(tmp); return false; }
    if (starts_ci(s, "SYSTEM") && is_word_boundary(s[6])) { free(tmp); return false; }

    // CLS (screen/output clear)
    if (starts_ci(s, "CLS") && is_word_boundary(s[3])) {
        if (video_mode_is_graphics(app->video_mode)) {
            gfx_clear(app, (unsigned char)((app->cur_bg >= 0) ? app->cur_bg : 0));
            gfx_draw_reset_defaults(app);
            screen_clear(app);
            screen_render(app);
        } else {
            out_clear(app, true);
        }
        free(tmp);
        return true;
    }

    if (starts_ci(s, "SCREEN") && is_word_boundary(s[6])) {
        Parser p = { s + 6 };
        bool ok = exec_screen(app, &p, current_line);
        free(tmp);
        return ok;
    }

    if (starts_ci(s, "PSET") && is_word_boundary(s[4])) {
        Parser p = { s + 4 };
        bool ok = exec_pset(app, &p, current_line);
        free(tmp);
        return ok;
    }

    if (starts_ci(s, "LINE") && is_word_boundary(s[4])) {
        char *t = s + 4;
        while (*t && isspace((unsigned char)*t)) t++;
        if (!(starts_ci(t, "INPUT") && is_word_boundary(t[5]))) {
            Parser p = { s + 4 };
            bool ok = exec_line_gfx(app, &p, current_line);
            free(tmp);
            return ok;
        }
    }
    if (starts_ci(s, "CIRCLE") && is_word_boundary(s[6])) {
        Parser p = { s + 6 };
        bool ok = exec_circle_gfx(app, &p, current_line);
        free(tmp);
        return ok;
    }
    if (starts_ci(s, "PAINT") && is_word_boundary(s[5])) {
        Parser p = { s + 5 };
        bool ok = exec_paint_gfx(app, &p, current_line);
        free(tmp);
        return ok;
    }
    if (starts_ci(s, "DRAW") && is_word_boundary(s[4])) {
        Parser p = { s + 4 };
        bool ok = exec_draw_gfx(app, &p, current_line);
        free(tmp);
        return ok;
    }

// CLEAR [expr[,expr[,expr]]]  (GW-BASIC compatibility: clear variables/arrays, close files, reset stacks; ignore sizing args)
if (starts_ci(s, "CLEAR") && is_word_boundary(s[5])) {
    Parser cp = { s + 5 };
    skip_ws(&cp);

    // Parse and ignore up to 3 optional numeric expressions, allowing commas.
    // GW-BASIC uses these for memory sizing (string/stack/array space). WBASIC ignores them but accepts syntax.
    if (*cp.s) {
        int n = 0;
        while (n < 3) {
            skip_ws(&cp);
            if (*cp.s == ',') { cp.s++; n++; continue; }
            double dv = 0.0;
            if (!parse_number(&cp, &dv)) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
            n++;
            skip_ws(&cp);
            if (*cp.s == ',') { cp.s++; continue; }
            break;
        }
        skip_ws(&cp);
        if (*cp.s) { free(tmp); runtime_error(app, current_line, "Syntax error"); return false; }
    }

    // Close all BASIC files
    files_close_all(app);

    // Reset runtime control-flow stacks (FOR/WHILE/GOSUB)
    for (int i = 0; i < app->for_sp; i++) free(app->for_stack[i].var_name);
    app->for_sp = 0;
    app->while_sp = 0;
    app->do_sp = 0;
    app->gosub_sp = 0;

    // Clear variables and arrays
    vars_reset(app);
    app->fn_call_sp = 0;

    // Reset DATA pointer (fresh READs)
    app->prog.data_ptr = 0;

    free(tmp);
    return true;
}

    // DEFINT
    if (starts_ci(s, "DEFINT") && is_word_boundary(s[6])) {
        Parser p = { s + 6 };
        bool ok = exec_defint(app, &p, current_line);
        free(tmp);
        return ok;
    }

    // DEFDBL / DEFSNG / DEFSTR
    if (starts_ci(s, "DEFDBL") && is_word_boundary(s[6])) {
        Parser p = { s + 6 };
        bool ok = exec_defdbl(app, &p, current_line);
        free(tmp);
        return ok;
    }
    if (starts_ci(s, "DEFSNG") && is_word_boundary(s[6])) {
        Parser p = { s + 6 };
        bool ok = exec_defsng(app, &p, current_line);
        free(tmp);
        return ok;
    }
    if (starts_ci(s, "DEFSTR") && is_word_boundary(s[6])) {
        Parser p = { s + 6 };
        bool ok = exec_defstr(app, &p, current_line);
        free(tmp);
        return ok;
    }


// OPEN
    if (starts_ci(s, "OPEN") && is_word_boundary(s[4])) {
        Parser p = { s + 4 };
        bool ok = exec_open(app, &p, current_line);
        free(tmp);
        return ok;
    }

    



// FIELD #n, ...  (RANDOM)
if (starts_ci(s, "FIELD") && is_word_boundary(s[5])) {
    Parser p = { s + 5 };
    bool ok = exec_field(app, &p, current_line);
    free(tmp);
    return ok;
}

// LSET var$ = expr$
if (starts_ci(s, "LSET") && is_word_boundary(s[4])) {
    Parser p = { s + 4 };
    bool ok = exec_lset_rset(app, &p, current_line, false);
    free(tmp);
    return ok;
}

// RSET var$ = expr$
if (starts_ci(s, "RSET") && is_word_boundary(s[4])) {
    Parser p = { s + 4 };
    bool ok = exec_lset_rset(app, &p, current_line, true);
    free(tmp);
    return ok;
}

// GET (graphics) or GET #n,rec (RANDOM)
if (starts_ci(s, "GET") && is_word_boundary(s[3])) {
    Parser p = { s + 3 };
    skip_ws(&p);
    bool ok = false;
    if (*p.s == '#') ok = exec_get(app, &p, current_line);
    else             ok = exec_get_gfx(app, &p, current_line);
    free(tmp);
    return ok;
}

// PUT (graphics) or PUT #n,rec (RANDOM)
if (starts_ci(s, "PUT") && is_word_boundary(s[3])) {
    Parser p = { s + 3 };
    skip_ws(&p);
    bool ok = false;
    if (*p.s == '#') ok = exec_put(app, &p, current_line);
    else             ok = exec_put_gfx(app, &p, current_line);
    free(tmp);
    return ok;
}
// SEEK #n, pos
if (starts_ci(s, "SEEK") && is_word_boundary(s[4])) {
    Parser p = { s + 4 };
    bool ok = exec_seek(app, &p, current_line);
    free(tmp);
    return ok;
}

// CLOSE
    if (starts_ci(s, "CLOSE") && is_word_boundary(s[5])) {
        Parser p = { s + 5 };
        bool ok = exec_close(app, &p, current_line);
        free(tmp);
        return ok;
    }

    
    // LINE INPUT (keyboard): LINE INPUT A$
    if (starts_ci(s, "LINE") && is_word_boundary(s[4])) {
        char *t = s + 4;
        while (*t && isspace((unsigned char)*t)) t++;
        if (starts_ci(t, "INPUT") && is_word_boundary(t[5])) {
            Parser p2 = { t + 5 };
            skip_ws(&p2);
            if (*p2.s != '#') {
                bool ok = exec_input(app, &p2, current_line);
                free(tmp);
                return ok;
            }
        }
    }
// LINE INPUT (file only): LINE INPUT #n, A$
    if (starts_ci(s, "LINE") && is_word_boundary(s[4])) {
        char *t = s + 4;
        while (*t && isspace((unsigned char)*t)) t++;
        if (starts_ci(t, "INPUT") && is_word_boundary(t[5])) {
            Parser p = { t + 5 };
            bool ok = exec_input_file(app, &p, current_line, true);
            free(tmp);
            return ok;
        }
    }

    
    // SPEED n (custom): 0 slowest .. 100 fastest
    if (starts_ci(s, "SPEED") && is_word_boundary(s[5])) {
        Parser p = { s + 5 };
        bool ok = exec_speed(app, &p, current_line);
        free(tmp);
        return ok;
    }

// LOCATE row,col
    if (starts_ci(s, "LOCATE") && is_word_boundary(s[6])) {
        Parser p = { s + 6 };
        bool ok = exec_locate(app, &p, current_line);
        free(tmp);
        return ok;
    }

// COLOR fg,bg[,border]  (text mode only; border ignored)
if (starts_ci(s, "COLOR") && is_word_boundary(s[5])) {
    Parser p = { s + 5 };
    bool ok = exec_color(app, &p, current_line);
    free(tmp);
    return ok;
}


    // OPTION BASE
    if (starts_ci(s, "OPTION") && is_word_boundary(s[6])) {
        Parser p = { s + 6 };
        bool ok = exec_option(app, &p, current_line);
        free(tmp);
        return ok;
    }

    // WIDTH
    if (starts_ci(s, "WIDTH") && is_word_boundary(s[5])) {
        Parser p = { s + 5 };
        bool ok = exec_width(app, &p, current_line);
        free(tmp);
        return ok;
    }

    
// WRITE
if (starts_ci(s, "WRITE") && is_word_boundary(s[5])) {
    Parser p = { s + 5 };
    skip_ws(&p);
    if (*p.s == '#') {
        bool ok = exec_write_file(app, &p, current_line);
        free(tmp);
        return ok;
    }
    bool ok = exec_write(app, &p, current_line);
    if (!ok) runtime_error_or_pending(app, current_line, "Bad WRITE");
    free(tmp);
    return ok;
}

// PRINT
    if (starts_ci(s, "PRINT") && is_word_boundary(s[5])) {
        Parser p = { s + 5 };
        skip_ws(&p);
        if (*p.s == '#') {
            bool ok = exec_print_file(app, &p, current_line);
            /* exec_print_file() raises the correct runtime error itself (e.g. Bad file number/mode).
               Do not raise a secondary generic PRINT error here. */
            free(tmp);
            return ok;
        }
        bool ok = exec_print(app, &p, current_line);
        if (!ok) runtime_error_or_pending(app, current_line, "Bad PRINT");
        free(tmp);
        return ok;
    }


    // BEEP
    if (starts_ci(s, "BEEP") && is_word_boundary(s[4])) {
        bool ok = exec_beep(app);
        free(tmp);
        return ok;
    }

    // INPUT
    if (starts_ci(s, "INPUT") && is_word_boundary(s[5])) {
        Parser p = { s + 5 };
        skip_ws(&p);
        if (*p.s == '#') {
            bool ok = exec_input_file(app, &p, current_line, false);
            free(tmp);
            return ok;
        }
        bool ok = exec_input(app, &p, current_line);
        free(tmp);
        return ok;
    }

        // REDIM
    if (starts_ci(s, "REDIM") && is_word_boundary(s[5])) {
        Parser p = { s + 5 };
        bool ok = exec_redim(app, &p, current_line);
        free(tmp);
        return ok;
    }

// DIM
    if (starts_ci(s, "DIM") && is_word_boundary(s[3])) {
        Parser p = { s + 3 };
        bool ok = exec_dim(app, &p, current_line);
        free(tmp);
        return ok;
    }

    // ERASE
    if (starts_ci(s, "ERASE") && is_word_boundary(s[5])) {
        Parser p = { s + 5 };
        bool ok = exec_erase(app, &p, current_line);
        free(tmp);
        return ok;
    }

    // SWAP
    if (starts_ci(s, "SWAP") && is_word_boundary(s[4])) {
        Parser p = { s + 4 };
        bool ok = exec_swap(app, &p, current_line);
        free(tmp);
        return ok;
    }

    // TIMER ON | TIMER OFF | TIMER STOP  (GW-BASIC event control for ON TIMER)
    if (starts_ci(s, "TIMER") && is_word_boundary(s[5])) {
        Parser p = { s + 5 };
        skip_ws(&p);
        if (starts_ci(p.s, "ON") && is_word_boundary(p.s[2])) {
            p.s += 2; skip_ws(&p);
            if (*p.s != 0) { runtime_error(app, current_line, "Syntax error"); free(tmp); return false; }
            app->timer_enabled = true;
            app->timer_stopped = false;
            if (app->on_timer_gosub_line > 0 && app->on_timer_interval > 0.0) timer_schedule_next(app, timer_now_sec());
            free(tmp);
            return true;
        }
        if (starts_ci(p.s, "OFF") && is_word_boundary(p.s[3])) {
            p.s += 3; skip_ws(&p);
            if (*p.s != 0) { runtime_error(app, current_line, "Syntax error"); free(tmp); return false; }
            app->timer_enabled = false;
            app->timer_stopped = false;
            free(tmp);
            return true;
        }
        if (starts_ci(p.s, "STOP") && is_word_boundary(p.s[4])) {
            p.s += 4; skip_ws(&p);
            if (*p.s != 0) { runtime_error(app, current_line, "Syntax error"); free(tmp); return false; }
            app->timer_stopped = true;
            free(tmp);
            return true;
        }
        runtime_error(app, current_line, "Syntax error");
        free(tmp);
        return false;
    }



    
    // ON ERROR GOTO <line>  (GW-BASIC)
    if (starts_ci(s, "ON") && is_word_boundary(s[2])) {
        Parser p = { s + 2 };
        skip_ws(&p);
        if (starts_ci(p.s, "ERROR") && is_word_boundary(p.s[5])) {
            p.s += 5;
            skip_ws(&p);
            if (!starts_ci(p.s, "GOTO") || !is_word_boundary(p.s[4])) {
                runtime_error(app, current_line, "ON ERROR expects GOTO");
                free(tmp);
                return false;
            }
            p.s += 4;
            skip_ws(&p);

            double ln = 0.0;
            if (!parse_expr(app, &p, &ln)) {
                runtime_error(app, current_line, "ON ERROR GOTO expects line number");
                free(tmp);
                return false;
            }
            int target = (int)llround(ln);
            // ON ERROR GOTO 0 disables trapping (GW-BASIC)
            app->on_error_goto_line = target;
            app->in_error_handler = false;
            free(tmp);
            return true;
        }

        // ON KEY(n) GOSUB <line>
        if (starts_ci(p.s, "KEY") && is_word_boundary(p.s[3])) {
            p.s += 3;
            skip_ws(&p);
            if (!consume(&p, '(')) { runtime_error(app, current_line, "Syntax error"); free(tmp); return false; }
            double dvn = 0.0;
            if (!parse_expr(app, &p, &dvn)) { runtime_error(app, current_line, "Syntax error"); free(tmp); return false; }
            int n = (int)dvn;
            if (fabs(dvn - (double)n) > 1e-9) { runtime_error(app, current_line, "Syntax error"); free(tmp); return false; }
            skip_ws(&p);
            if (!consume(&p, ')')) { runtime_error(app, current_line, "Syntax error"); free(tmp); return false; }
            if (n < 1 || n > 10) { runtime_error(app, current_line, "Illegal function call"); free(tmp); return false; }
            skip_ws(&p);
            if (!starts_ci(p.s, "GOSUB") || !is_word_boundary(p.s[5])) { runtime_error(app, current_line, "ON KEY expects GOSUB"); free(tmp); return false; }
            p.s += 5;
            skip_ws(&p);
            double ln = 0.0;
            if (!parse_expr(app, &p, &ln)) { runtime_error(app, current_line, "ON KEY expects line number"); free(tmp); return false; }
            int target_line = (int)llround(ln);
            int idx = program_find_index(&app->prog, target_line);
            if (idx < 0) { runtime_error(app, current_line, "ON KEY target not found"); free(tmp); return false; }
            skip_ws(&p);
            if (*p.s != 0) { runtime_error(app, current_line, "Syntax error"); free(tmp); return false; }
            app->on_key_gosub_line[n-1] = target_line;
            free(tmp);
            return true;
        }


        // ON TIMER(interval) GOSUB <line>
        if (starts_ci(p.s, "TIMER") && is_word_boundary(p.s[5])) {
            p.s += 5;
            skip_ws(&p);
            if (!consume(&p, '(')) { runtime_error(app, current_line, "Syntax error"); free(tmp); return false; }
            double interval = 0.0;
            if (!parse_expr(app, &p, &interval)) { runtime_error(app, current_line, "Syntax error"); free(tmp); return false; }
            skip_ws(&p);
            if (!consume(&p, ')')) { runtime_error(app, current_line, "Syntax error"); free(tmp); return false; }
            skip_ws(&p);
            if (!starts_ci(p.s, "GOSUB") || !is_word_boundary(p.s[5])) { runtime_error(app, current_line, "ON TIMER expects GOSUB"); free(tmp); return false; }
            p.s += 5;
            skip_ws(&p);
            double ln = 0.0;
            if (!parse_expr(app, &p, &ln)) { runtime_error(app, current_line, "ON TIMER expects line number"); free(tmp); return false; }
            int target_line = (int)llround(ln);
            skip_ws(&p);
            if (*p.s != 0) { runtime_error(app, current_line, "Syntax error"); free(tmp); return false; }

            if (target_line <= 0) {
                // Disable
                app->on_timer_gosub_line = 0;
                app->on_timer_interval = 0.0;
                app->timer_enabled = false;
                app->timer_stopped = false;
                app->timer_in_progress = false;
                app->timer_next_fire = 0.0;
                free(tmp);
                return true;
            }

            // Arm (but do not implicitly enable; requires TIMER ON like GW-BASIC)
            int idx = program_find_index(&app->prog, target_line);
            if (idx < 0) { runtime_error(app, current_line, "ON TIMER target not found"); free(tmp); return false; }
            app->on_timer_gosub_line = target_line;
            app->on_timer_interval = interval;
            /* next fire scheduled on RETURN from handler (no queued ticks) */
            free(tmp);
            return true;
        }
    }

// RESUME [<line>] | RESUME NEXT  (Phase 3: implement RESUME NEXT in addition to plain RESUME and RESUME <line>)
if (starts_ci(s, "RESUME") && is_word_boundary(s[6])) {
    Parser p = { s + 6 };
    skip_ws(&p);

    
bool resume_next = false;

/* Accept 'RESUME NEXT' (case-insensitive) with any mix of whitespace, including NBSP (0xA0). */
while (*p.s && (isspace((unsigned char)*p.s) || (unsigned char)*p.s == 0xA0)) p.s++;

if (starts_ci(p.s, "NEXT") && is_word_boundary(p.s[4])) {
    resume_next = true;
    p.s += 4;
    while (*p.s && (isspace((unsigned char)*p.s) || (unsigned char)*p.s == 0xA0)) p.s++;
    if (*p.s != 0) {
        runtime_error(app, current_line, "RESUME NEXT takes no argument");
        free(tmp);
        return false;
    }
} else {
    /* Not NEXT: leave p.s as-is so RESUME <line> parsing sees the token, or plain RESUME sees end-of-line. */
    skip_ws(&p);
}
// RESUME is only valid while in an ON ERROR handler.
    if (!app->in_error_handler) {
        runtime_error(app, current_line, "RESUME without error");
        free(tmp);
        return false;
    }

    
int target_idx = -1;
int target_stmt = 0;

/* Decide RESUME variant:
   - RESUME NEXT: resume at statement after the faulting one (across ':' chains / next line)
   - RESUME (no arg): resume at the exact faulting statement
   - RESUME <line>: resume at the start of that line
*/
if (resume_next) {
    if (!app->err_origin_valid || app->err_origin_line_idx < 0 || app->err_origin_stmt_idx < 0) {
        runtime_error(app, current_line, "RESUME cannot determine error origin");
        free(tmp);
        return false;
    }
    /* Use the interpreter's canonical next-statement helper so we respect ':' chains and line boundaries. */
    prog_next_stmt(app, app->err_origin_line_idx, app->err_origin_stmt_idx, &target_idx, &target_stmt);
    if (target_idx < 0) {
        /* Past program end: resume will naturally finish the run loop. */
        target_idx = (int)app->prog.count;
        target_stmt = 0;
    }
} else if (*p.s == 0) {
    /* Plain RESUME (no argument) */
    if (!app->err_origin_valid || app->err_origin_line_idx < 0 || app->err_origin_stmt_idx < 0) {
        runtime_error(app, current_line, "RESUME cannot determine error origin");
        free(tmp);
        return false;
    }
    target_idx  = app->err_origin_line_idx;
    target_stmt = app->err_origin_stmt_idx;

    /* If the error occurred inside an IF/ELSE ':' tail chain, plain RESUME must retry
       the owning statement (so the IF condition is re-evaluated), not restart inside the tail. */
    if (app->err_origin_in_chain &&
        app->err_origin_chain_base_line_idx >= 0 &&
        app->err_origin_chain_base_stmt_idx >= 0) {
        target_idx  = app->err_origin_chain_base_line_idx;
        target_stmt = app->err_origin_chain_base_stmt_idx;
    }
} else {
    /* RESUME <line> */
    double ln = 0.0;
    if (!parse_expr(app, &p, &ln)) {
        runtime_error(app, current_line, "RESUME expects line number");
        free(tmp);
        return false;
    }
    int target_ln = (int)llround(ln);
    int idx = program_find_index(&app->prog, target_ln);
    if (idx < 0) {
        runtime_error(app, current_line, "target not found");
        free(tmp);
        return false;
    }
    target_idx  = idx;
    target_stmt = 0;
}

// Clear error/trap state and resume execution at target cursor.

    err_clear(app);
app->in_error_handler = false;

/* If the error occurred inside a ':' chain (e.g., IF THEN/ELSE tail),
   RESUME and RESUME NEXT must continue within that chain, not after the owning statement. */
if (resume_next && app->err_origin_in_chain && app->err_origin_chain_text) {
    int start_si = app->err_origin_chain_stmt_idx;
    if (resume_next) start_si = app->err_origin_chain_stmt_idx + 1;

    if (app->resume_chain_text) { free(app->resume_chain_text); app->resume_chain_text = NULL; }
    app->resume_chain_pending = true;
    app->resume_chain_line_idx = app->err_origin_chain_base_line_idx;
    app->resume_chain_stmt_idx = app->err_origin_chain_base_stmt_idx;
    app->resume_chain_next_si = start_si;
    app->resume_chain_text = app->err_origin_chain_text; /* take ownership */
    app->err_origin_chain_text = NULL;

    app->err_origin_valid = false;
    app->err_origin_in_chain = false;
    app->err_origin_chain_base_line_idx = -1;
    app->err_origin_chain_base_stmt_idx = -1;
    app->err_origin_chain_stmt_idx = -1;

    *line_idx = app->resume_chain_line_idx;
    *stmt_idx = app->resume_chain_stmt_idx;
    free(tmp);
    return true;
}

app->err_origin_valid = false;

*line_idx = target_idx;
*stmt_idx = target_stmt;
free(tmp);
return true;
}


// ON <expr> GOTO/GOSUB <line-list>
    if (starts_ci(s, "ON") && is_word_boundary(s[2])) {
        Parser p = { s + 2 };
        double dv = 0.0;
        if (!parse_expr(app, &p, &dv)) { runtime_error(app, current_line, "ON expects selector expression"); free(tmp); return false; }
        int sel = (int)dv; // truncation, GW-BASIC style
        skip_ws(&p);

        bool is_gosub = false;
        if (starts_ci(p.s, "GOTO") && is_word_boundary(p.s[4])) { is_gosub = false; p.s += 4; }
        else if (starts_ci(p.s, "GOSUB") && is_word_boundary(p.s[5])) { is_gosub = true; p.s += 5; }
        else { runtime_error(app, current_line, "ON expects GOTO or GOSUB"); free(tmp); return false; }

        int targets[64];
        int nt = 0;
        for (;;) {
            skip_ws(&p);
            double ln = 0.0;
            if (!parse_expr(app, &p, &ln)) { runtime_error(app, current_line, "ON expects line number list"); free(tmp); return false; }
            if (nt < 64) targets[nt++] = (int)llround(ln);
            skip_ws(&p);
            if (*p.s == ',') { p.s++; continue; }
            break;
        }

        if (sel >= 1 && sel <= nt) {
            int target_line = targets[sel - 1];
            int idx = program_find_index(&app->prog, target_line);
            if (idx < 0) { runtime_error(app, current_line, "ON target not found"); free(tmp); return false; }
            if (is_gosub) {
                int cur_si = (*stmt_idx > 0) ? (*stmt_idx - 1) : 0;
                if (app->gosub_sp >= 128) { runtime_error(app, current_line, "GOSUB stack overflow"); free(tmp); return false; }
                app->gosub_stack[app->gosub_sp++] = (GosubFrame){ .kind = GOSUB_RET_NORMAL, .ret_line_idx = *line_idx, .ret_stmt_idx = cur_si + 1, .ret_chain_next_si = 0, .ret_chain_text = NULL };
            }
            *line_idx = idx;
            *stmt_idx = 0;
        }
        free(tmp);
        return true; // out-of-range is no-op
    }

    // GOTO
    if (starts_ci(s, "GOTO") && is_word_boundary(s[4])) {
        Parser p = { s + 4 };
        double ln = 0.0;
        if (!parse_expr(app, &p, &ln)) { runtime_error(app, current_line, "GOTO expects line number"); free(tmp); return false; }
        int target = (int)llround(ln);
        int idx = program_find_index(&app->prog, target);
        if (idx < 0) { runtime_error(app, current_line, "GOTO target not found"); free(tmp); return false; }
        *line_idx = idx;
        *stmt_idx = 0;
        free(tmp);
        return true;
    }

    // IF
    if (starts_ci(s, "IF") && is_word_boundary(s[2])) {
        Parser p = { s + 2 };
        bool cond = false;
        if (!eval_condition(app, &p, &cond)) { runtime_error(app, current_line, "Bad IF condition"); free(tmp); return false; }
        skip_ws(&p);
        const char *th = p.s;
        if (!(toupper((unsigned char)th[0])=='T' && toupper((unsigned char)th[1])=='H' &&
              toupper((unsigned char)th[2])=='E' && toupper((unsigned char)th[3])=='N' &&
              is_word_boundary(th[4]))) {
            runtime_error(app, current_line, "IF missing THEN");
            free(tmp);
            return false;
        }
        p.s += 4;

/* Block IF form: IF <expr> THEN   (nothing follows THEN on this statement) */
{
    Parser pt = { p.s };
    skip_ws(&pt);
    if (*pt.s == 0) {
        if (current_line < 0 || cur_li < 0 || cur_si < 0) {
            runtime_error(app, current_line, "Block IF not valid in immediate mode");
            free(tmp);
            return false;
        }
        IfBlockMapEntry *e = program_ifmap_find_by_if(&app->prog, cur_li, cur_si);
        if (!e) { runtime_error(app, current_line, "IF without END IF"); free(tmp); return false; }


        /* Phase 2 ELSEIF runtime: establish IF execution context for this block.
           We only push a context if control will later encounter END IF (i.e., IF taken or ELSE present). */
        if ((cond) || (e->has_else) || (e->elseif_count > 0)) {
            if (app->if_sp < 0) app->if_sp = 0;
            if (app->if_sp >= 128) { runtime_error(app, current_line, "IF stack overflow"); free(tmp); return false; }
            IfExecFrame fr;
            memset(&fr, 0, sizeof(fr));
            fr.if_entry_index = (int)(e - app->prog.ifmap);
            fr.else_entry_index = -1;
            fr.end_entry_index = -1;
            fr.condition_true = cond;
            fr.branch_taken = cond;
            fr.active_branch = cond ? 0 : -2;
            app->if_stack[app->if_sp++] = fr;
        }

        if (!cond) {
            if (e->elseif_count > 0) {
                prog_goto_stmt(app, e->elseif_pos[0].li, e->elseif_pos[0].si, line_idx, stmt_idx);
            } else if (e->has_else) {
                prog_goto_stmt(app, e->else_pos.li, e->else_pos.si, line_idx, stmt_idx);
            } else {
                prog_goto_stmt(app, e->end_pos.li, e->end_pos.si, line_idx, stmt_idx);
            }
        }
        free(tmp);
        return true;
    }
}

        // THEN: line number or statement
        Parser p2 = { p.s };
        double ln = 0.0;
        if (parse_number(&p2, &ln)) {
            if (cond) {
                int target = (int)llround(ln);
                int idx = program_find_index(&app->prog, target);
                if (idx < 0) { runtime_error(app, current_line, "THEN target not found"); free(tmp); return false; }
                *line_idx = idx;
                *stmt_idx = 0;
                free(tmp);
                return true;
            }

            /* cond is false: support single-line ELSE <line#> */
            skip_ws(&p2);
            if (starts_ci(p2.s, "ELSE") && is_word_boundary(p2.s[4])) {
                p2.s += 4;
                skip_ws(&p2);
                double ln2 = 0.0;
                if (parse_number(&p2, &ln2)) {
                    int target = (int)llround(ln2);
                    int idx = program_find_index(&app->prog, target);
                    if (idx < 0) { runtime_error(app, current_line, "ELSE target not found"); free(tmp); return false; }
                    *line_idx = idx;
                    *stmt_idx = 0;
                    free(tmp);
                    return true;
                }
            }

            free(tmp);
            return true; /* no ELSE target */
        } else {
            // execute the rest as a statement (supports single-line ELSE)
            char *rest = xstrdup(p.s);
            char *rt = trim(rest);

            char *else_kw = find_else_kw(rt);
            if (else_kw) {
                *else_kw = 0; // terminate THEN-part
                char *then_part = trim(rt);
                char *ep0 = else_kw + 4;
                while (*ep0 == ' ' || *ep0 == '	' || *ep0 == ':') ep0++; /* allow ELSE:IF and ELSE : IF */
                char *else_part = trim(ep0);

                                bool ok = true;
                if (cond) {
                    if (*then_part) ok = exec_statement_chain_from(app, then_part, current_line, cur_li, cur_si, 0, line_idx, stmt_idx);
                } else {
                    if (*else_part) ok = exec_statement_chain_from(app, else_part, current_line, cur_li, cur_si, 0, line_idx, stmt_idx);
                }


                free(rest);
                free(tmp);
                return ok;
            }

            bool ok = cond ? exec_statement_chain_from(app, rt, current_line, cur_li, cur_si, 0, line_idx, stmt_idx) : true;
            free(rest);
            free(tmp);
            return ok;
        }
    }

    // FOR
    if (starts_ci(s, "FOR") && is_word_boundary(s[3])) {
        Parser p = { s + 3 };
        int cur_si = (*stmt_idx > 0) ? (*stmt_idx - 1) : 0;
        bool ok = exec_for(app, &p, current_line, *line_idx, cur_si, line_idx, stmt_idx);
        free(tmp);
        return ok;
    }

    // NEXT
    if (starts_ci(s, "NEXT") && is_word_boundary(s[4])) {
        Parser p = { s + 4 };
        bool ok = exec_next(app, &p, current_line, line_idx, stmt_idx);
        free(tmp);
        return ok;
    }



    // EXIT DO
    if (starts_ci(s, "EXIT") && is_word_boundary(s[4])) {
        char *t = trim(s + 4);
        if (starts_ci(t, "DO") && is_word_boundary(t[2])) {
            bool ok = exec_exit_do(app, current_line, line_idx, stmt_idx);
            free(tmp);
            return ok;
        }
    }



// DO
if (starts_ci(s, "DO") && is_word_boundary(s[2])) {
    Parser p = { s + 2 };
    int cur_si = (*stmt_idx > 0) ? (*stmt_idx - 1) : 0;
    bool ok = exec_do(app, &p, current_line, *line_idx, cur_si, line_idx, stmt_idx);
    free(tmp);
    return ok;
}

// LOOP
if (starts_ci(s, "LOOP") && is_word_boundary(s[4])) {
    Parser p = { s + 4 };
    int cur_si = (*stmt_idx > 0) ? (*stmt_idx - 1) : 0;
    bool ok = exec_loop(app, &p, current_line, *line_idx, cur_si, line_idx, stmt_idx);
    free(tmp);
    return ok;
}

// WHILE
if (starts_ci(s, "WHILE") && is_word_boundary(s[5])) {
    Parser p = { s + 5 };
    int cur_si = (*stmt_idx > 0) ? (*stmt_idx - 1) : 0;
    bool ok = exec_while(app, &p, current_line, *line_idx, cur_si, line_idx, stmt_idx);
    free(tmp);
    return ok;
}

// WEND
if (starts_ci(s, "WEND") && is_word_boundary(s[4])) {
    bool ok = exec_wend(app, current_line, line_idx, stmt_idx);
    free(tmp);
    return ok;
}

    // GOSUB
    // IMPORTANT: Use the *actual* current statement indices passed into exec_single_statement.
    // Do NOT derive them from *stmt_idx, because exec_statement_chain may pre/post-increment it.
    if (starts_ci(s, "GOSUB") && is_word_boundary(s[5])) {
        Parser p = { s + 5 };
        bool ok = exec_gosub(app, &p, current_line, cur_li, cur_si, line_idx, stmt_idx);
        free(tmp);
        return ok;
    }

    // RETURN
    if (starts_ci(s, "RETURN") && is_word_boundary(s[6])) {
        bool ok = exec_return(app, current_line, line_idx, stmt_idx);
        free(tmp);
        return ok;
    }

    /* Second-chance graphics command dispatch (prevents fall-through to assignment parser). */
    if (starts_ci(s, "SCREEN") && is_word_boundary(s[6])) {
        Parser p = { s + 6 };
        bool ok = exec_screen(app, &p, current_line);
        free(tmp);
        return ok;
    }
    if (starts_ci(s, "PSET") && is_word_boundary(s[4])) {
        Parser p = { s + 4 };
        bool ok = exec_pset(app, &p, current_line);
        free(tmp);
        return ok;
    }
    if (starts_ci(s, "LINE") && is_word_boundary(s[4])) {
        char *t = s + 4;
        while (*t && isspace((unsigned char)*t)) t++;
        if (!(starts_ci(t, "INPUT") && is_word_boundary(t[5]))) {
            Parser p = { s + 4 };
            bool ok = exec_line_gfx(app, &p, current_line);
            free(tmp);
            return ok;
        }
    }
    if (starts_ci(s, "CIRCLE") && is_word_boundary(s[6])) {
        Parser p = { s + 6 };
        bool ok = exec_circle_gfx(app, &p, current_line);
        free(tmp);
        return ok;
    }
    if (starts_ci(s, "PAINT") && is_word_boundary(s[5])) {
        Parser p = { s + 5 };
        bool ok = exec_paint_gfx(app, &p, current_line);
        free(tmp);
        return ok;
    }
    if (starts_ci(s, "DRAW") && is_word_boundary(s[4])) {
        Parser p = { s + 4 };
        bool ok = exec_draw_gfx(app, &p, current_line);
        free(tmp);
        return ok;
    }

    // LET (optional)
    if (starts_ci(s, "LET") && is_word_boundary(s[3])) {
        s = trim(s + 3);
    }

    // assignment (including array element)
    {
        Parser p = { s };
        bool ok = exec_assignment(app, &p, current_line);
        free(tmp);
        return ok;
    }
}

/* ===================== Listing / File IO ===================== */


#ifndef WBASIC_NO_UI
static bool file_load_into_editor(App *app, const char *path) {
    gchar *contents = NULL;
    gsize len = 0;
    GError *err = NULL;
    if (!g_file_get_contents(path, &contents, &len, &err)) {
        out_printf(app, "LOAD failed: %s\n", err->message);
        g_error_free(err);
        return false;
    }
    app->suppress_dirty = true;
    editor_set_text(app->editor_buf, contents);
    app->suppress_dirty = false;
    g_free(contents);
    set_current_path(app, path);
    recent_add(app, path);
    mark_dirty(app, false);
    return true;
}
#else
static bool file_load_into_editor(App *app, const char *path) { (void)app; (void)path; return false; }
#endif /* WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static bool file_save_from_editor(App *app, const char *path) {
    char *text = editor_get_text(app->editor_buf);
    GError *err = NULL;
    if (!g_file_set_contents(path, text, -1, &err)) {
        out_printf(app, "SAVE failed: %s\n", err->message);
        g_error_free(err);
        g_free(text);
        return false;
    }
    g_free(text);
    return true;
}
#else
static bool file_save_from_editor(App *app, const char *path) { (void)app; (void)path; return false; }
#endif /* WBASIC_NO_UI */


/* ===================== Block IF map (build-time) ===================== */

static void program_ifmap_clear(Program *p) {
    if (!p) return;
    if (p->ifmap) { free(p->ifmap); p->ifmap = NULL; }
    p->ifmap_count = 0;
    p->ifmap_cap = 0;
}

static void program_ifmap_add(Program *p, IfBlockMapEntry e) {
    if (!p) return;
    if (p->ifmap_count + 1 > p->ifmap_cap) {
        size_t nc = p->ifmap_cap ? (p->ifmap_cap * 2) : 64;
        p->ifmap = (IfBlockMapEntry*)xrealloc(p->ifmap, nc * sizeof(IfBlockMapEntry));
        p->ifmap_cap = nc;
    }
    p->ifmap[p->ifmap_count++] = e;
}

static IfBlockMapEntry *program_ifmap_find_by_if(Program *p, int li, int si) {
    if (!p) return NULL;
    for (size_t i = 0; i < p->ifmap_count; i++) {
        if (p->ifmap[i].if_pos.li == li && p->ifmap[i].if_pos.si == si) return &p->ifmap[i];
    }
    return NULL;
}

static IfBlockMapEntry *program_ifmap_find_by_else(Program *p, int li, int si) {
    if (!p) return NULL;
    for (size_t i = 0; i < p->ifmap_count; i++) {
        if (p->ifmap[i].has_else && p->ifmap[i].else_pos.li == li && p->ifmap[i].else_pos.si == si) return &p->ifmap[i];
    }
    return NULL;
}
static WB_UNUSED IfBlockMapEntry *program_ifmap_find_by_end(Program *p, int li, int si) {
    if (!p) return NULL;
    for (size_t i = 0; i < p->ifmap_count; i++) {
        if (p->ifmap[i].end_pos.li == li && p->ifmap[i].end_pos.si == si) return &p->ifmap[i];
    }
    return NULL;
}

static IfBlockMapEntry *program_ifmap_find_by_elseif(Program *p, int li, int si, int *out_elseif_index) {
    if (out_elseif_index) *out_elseif_index = -1;
    if (!p) return NULL;
    for (size_t i = 0; i < p->ifmap_count; i++) {
        for (int k = 0; k < p->ifmap[i].elseif_count; k++) {
            if (p->ifmap[i].elseif_pos[k].li == li && p->ifmap[i].elseif_pos[k].si == si) {
                if (out_elseif_index) *out_elseif_index = k;
                return &p->ifmap[i];
            }
        }
    }
    return NULL;
}


static bool is_space_only(const char *s) {
    while (*s) { if (!isspace((unsigned char)*s)) return false; s++; }
    return true;
}

/* Find keyword (e.g., THEN) outside quotes, with word-boundary checks. Returns pointer to the keyword or NULL. */
static const char *find_kw_outside_quotes_ci(const char *s, const char *kw) {
    size_t klen = strlen(kw);
    bool inq = false;
    for (const char *p = s; *p; p++) {
        if (*p == '"') inq = !inq;
        if (inq) continue;
        if (strncasecmp(p, kw, klen) == 0) {
            char before = (p == s) ? ' ' : p[-1];
            char after  = p[klen];
            if (!is_word_char_(before) && !is_word_char_(after)) return p;
        }
    }
    return NULL;
}

static bool stmt_is_block_if_open(const char *stmt) {
    if (!stmt) return false;
    const char *s = stmt;
    while (*s && isspace((unsigned char)*s)) s++;
    if (!(starts_ci(s, "IF") && is_word_boundary(s[2]))) return false;

    const char *thenp = find_kw_outside_quotes_ci(s, "THEN");
    if (!thenp) return false;

    /* Require nothing after THEN (whitespace only) => block opener */
    const char *after = thenp + 4;
    return is_space_only(after);
}

static bool stmt_is_block_else(const char *stmt) {
    if (!stmt) return false;
    const char *s = stmt;
    while (*s && isspace((unsigned char)*s)) s++;
    if (!(starts_ci(s, "ELSE") && is_word_boundary(s[4]))) return false;
    return is_space_only(s + 4);
}


static bool stmt_is_block_elseif(const char *stmt)
{
    const char *s = stmt;
    while (*s && isspace((unsigned char)*s)) s++;

    bool else_if_form = false;
    if (starts_ci(s, "ELSEIF") && is_word_boundary(s[6])) {
        s += 6;
    } else if (starts_ci(s, "ELSE") && is_word_boundary(s[4])) {
        const char *t = s + 4;
        while (*t && isspace((unsigned char)*t)) t++;
        if (starts_ci(t, "IF") && is_word_boundary(t[2])) {
            else_if_form = true;
            s = t + 2;
        } else {
            return false;
        }
    } else {
        return false;
    }

    /* Must have a condition and a THEN, and must be block form: nothing after THEN */
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == 0) return false; /* no condition */

    const char *thenp = wbasic_ascii_strcasestr(s, "THEN");
    if (!thenp) return false;
    /* Ensure something before THEN (condition not empty) */
    const char *q = thenp;
    while (q > s && isspace((unsigned char)q[-1])) q--;
    if (q == s) return false;

    /* After THEN must be whitespace only (block form) */
    const char *aft = thenp + 4;
    while (*aft && isspace((unsigned char)*aft)) aft++;
    if (*aft != 0) return false;

    (void)else_if_form;
    return true;
}

static bool stmt_is_block_end_if(const char *stmt) {
    if (!stmt) return false;
    const char *s = stmt;
    while (*s && isspace((unsigned char)*s)) s++;

    /* ENDIF */
    if (starts_ci(s, "ENDIF") && is_word_boundary(s[5]) && is_space_only(s + 5)) return true;

    /* END IF (allow multiple spaces) */
    if (starts_ci(s, "END") && is_word_boundary(s[3])) {
        s += 3;
        while (*s && isspace((unsigned char)*s)) s++;
        if (starts_ci(s, "IF") && is_word_boundary(s[2]) && is_space_only(s + 2)) return true;
    }
    return false;
}

typedef struct {
    size_t entry_index; /* index in p->ifmap */
    bool has_else;
} IfStackItem;

static bool program_build_ifmap(App *app) {
    Program *p = &app->prog;
    program_ifmap_clear(p);

    IfStackItem *stack = NULL;
    size_t sp = 0, scap = 0;

    for (size_t li = 0; li < p->count; li++) {
        const char *text = p->lines[li].text ? p->lines[li].text : "";
        StmtList sl = split_statements(text);

        for (int si = 0; si < sl.count; si++) {
            const char *st = sl.stmts[si];

            if (stmt_is_block_if_open(st)) {
                IfBlockMapEntry e;
                memset(&e, 0, sizeof(e));
                e.if_pos = (StmtPos){ (int)li, si };
                e.has_else = false;
                e.else_pos = (StmtPos){ -1, -1 };
                e.end_pos  = (StmtPos){ -1, -1 };
                e.elseif_count = 0;
                program_ifmap_add(p, e);

                if (sp + 1 > scap) {
                    size_t nc = scap ? scap * 2 : 64;
                    stack = (IfStackItem*)xrealloc(stack, nc * sizeof(IfStackItem));
                    scap = nc;
                }
                stack[sp++] = (IfStackItem){ .entry_index = p->ifmap_count - 1, .has_else = false };
            } else 
            if (stmt_is_block_elseif(st)) {
                if (sp == 0) {
                    runtime_error(app, p->lines[li].line_no, "ELSEIF without IF");
                    stmtlist_free(&sl);
                    free(stack);
                    program_ifmap_clear(p);
                    return false;
                }
                IfStackItem *top = &stack[sp - 1];
                IfBlockMapEntry *e = &p->ifmap[top->entry_index];
                if (e->has_else) {
                    runtime_error(app, p->lines[li].line_no, "ELSEIF after ELSE in IF block");
                    stmtlist_free(&sl);
                    free(stack);
                    program_ifmap_clear(p);
                    return false;
                }
                if (e->elseif_count >= (int)(sizeof(e->elseif_pos)/sizeof(e->elseif_pos[0]))) {
                    runtime_error(app, p->lines[li].line_no, "Too many ELSEIF in IF block");
                    stmtlist_free(&sl);
                    free(stack);
                    program_ifmap_clear(p);
                    return false;
                }
                e->elseif_pos[e->elseif_count++] = (StmtPos){ (int)li, si };
                continue;
            }

if (stmt_is_block_else(st)) {
                if (sp == 0) {
                    runtime_error(app, p->lines[li].line_no, "ELSE without IF");
                    stmtlist_free(&sl);
                    free(stack);
                    program_ifmap_clear(p);
                    return false;
                }
                IfStackItem *top = &stack[sp - 1];
                IfBlockMapEntry *e = &p->ifmap[top->entry_index];
                if (e->has_else) {
                    runtime_error(app, p->lines[li].line_no, "Multiple ELSE in IF block");
                    stmtlist_free(&sl);
                    free(stack);
                    program_ifmap_clear(p);
                    return false;
                }
                e->has_else = true;
                e->else_pos = (StmtPos){ (int)li, si };
            } else if (stmt_is_block_end_if(st)) {
                if (sp == 0) {
                    runtime_error(app, p->lines[li].line_no, "END IF without IF");
                    stmtlist_free(&sl);
                    free(stack);
                    program_ifmap_clear(p);
                    return false;
                }
                IfStackItem top = stack[--sp];
                IfBlockMapEntry *e = &p->ifmap[top.entry_index];
                e->end_pos = (StmtPos){ (int)li, si };
            }
        }

        stmtlist_free(&sl);
    }

    if (sp != 0) {
        /* Report the last unmatched IF line number for clarity */
        IfStackItem top = stack[sp - 1];
        IfBlockMapEntry *e = &p->ifmap[top.entry_index];
        int ln = p->lines[e->if_pos.li].line_no;
        runtime_error(app, ln, "IF without END IF");
        free(stack);
        program_ifmap_clear(p);
        return false;
    }

    free(stack);
    return true;
}
static WB_UNUSED void prog_next_stmt(App *app, int li, int si, int *out_li, int *out_si) {
    if (!app || !out_li || !out_si) return;
    Program *p = &app->prog;
    if (li < 0 || li >= (int)p->count) { *out_li = li; *out_si = si; return; }

    StmtList sl = split_statements(p->lines[li].text ? p->lines[li].text : "");
    if (si + 1 < sl.count) {
        *out_li = li;
        *out_si = si + 1;
    } else {
        *out_li = li + 1;
        *out_si = 0;
    }
    stmtlist_free(&sl);
}

static void prog_goto_stmt(App *app, int li, int si, int *out_li, int *out_si)
{
    (void)app;
    if (!out_li || !out_si) return;
    *out_li = li;
    *out_si = si;
}


/* ===================== Program execution ===================== */

// Apply interpreter-wide pacing based on the Output speed slider.
//  - output_speed = 1.0 => no delay
//  - output_speed = 0.0 => max delay per executed statement
static inline void exec_apply_pacing(App *app) {
    double s = app->output_speed;
    if (s < 0.0) s = 0.0;
    if (s > 1.0) s = 1.0;

    // Tuned power curve: smooth control near Fast, with midpoint ~5x faster than Slow.
//   s=1.0 ("Fast") -> curve=0
//   s=0.0 ("Slow") -> curve=1
// We want at mid slider: delay ~= 0.2 * Slow => (0.5)^e ~= 0.2 => e ~= 2.32
    double t = 1.0 - s;
    double curve = pow(t, 2.32);

    // Max delay per statement at Slow end (microseconds).
    // Tune this to taste; 500us avg at Slow (implemented as ~1ms every 2 statements).
    const double max_us_per_stmt = 500;

    app->exec_pace_accum_us += curve * max_us_per_stmt;

    // Sleep only in coalesced chunks to reduce overhead.
    if (app->exec_pace_accum_us >= 1000) {  // coalesce to ~1ms chunks
        int us = (int)(app->exec_pace_accum_us);
        // Cap sleeps so UI stays responsive.
        if (us > 20000) us = 20000;
        g_usleep((gulong)us);
        app->exec_pace_accum_us -= (double)us;
    }
}



static void runtime_reset(App *app) {
    app->on_key_pending = -1;
    app->on_key_in_progress = false;

    app->inkey_ready = false;
    app->stop_flag = false;
        app->pause_flag = false;
    app->option_base = 0;
    app->option_base_locked = false;
    for (int i = 0; i < 26; i++) app->def_type[i] = (unsigned char)DT_SNG;
    app->exec_pace_accum_us = 0.0;

    /* Reset SPEED on each RUN (uses CLI/embedded/default baseline) */
    app->output_speed = app->default_output_speed;
    app->print_throttle_carry_ms = 0.0;

    // clear stacks
    for (int i = 0; i < app->for_sp; i++) free(app->for_stack[i].var_name);
    app->for_sp = 0;
    app->while_sp = 0;
    app->gosub_sp = 0;

    if (app->resume_chain_text) { free(app->resume_chain_text); app->resume_chain_text = NULL; }
    app->resume_chain_pending = false;
    app->chain_active = false;
    app->chain_text = NULL;


    // ON ERROR GOTO trap state
    app->on_error_goto_line = 0;
    app->error_trap_pending = false;
    app->error_trap_line_idx = 0;
    app->error_trap_stmt_idx = 0;
    app->in_error_handler = false;
    app->last_err_line = -1;
    app->last_err_code = 0;
    app->exec_cursor_valid = false;
    app->err_origin_valid = false;
    app->err_origin_line_idx = -1;
    app->err_origin_stmt_idx = -1;
    app->err_origin_in_chain = false;
    app->err_origin_chain_base_line_idx = -1;
    app->err_origin_chain_base_stmt_idx = -1;
    app->err_origin_chain_stmt_idx = -1;
    if (app->err_origin_chain_text) { free(app->err_origin_chain_text); app->err_origin_chain_text = NULL; }

    // clear variables
    vars_reset(app);
}

static void maybe_do_pending_load(App *app);

static void do_exec_from(App *app, int start_line_idx, int start_stmt_idx, bool clear_output, bool reset_vars) {
    if (clear_output) {
        out_clear(app, false);
        screen_render_flush(app);
    }
    if (!editor_to_program(app)) return;

    /* Build Block IF map once per RUN (parse/build-time, no runtime scanning) */
    if (!program_build_ifmap(app)) {
        /* runtime_error() already reported; treat as a stop */
        set_run_state(app, RUN_STOPPED);
        return;
    }


    if (reset_vars) {
        runtime_reset(app);
        program_build_data_pool(&app->prog);
    } else {
        // Keep everything (vars, stacks, output). Just ensure we are not in STOP state.
        app->stop_flag = false;
    }

    app->exec_cursor_valid = false;
    set_run_state(app, RUN_RUNNING);

    int line_idx = start_line_idx;
    int stmt_idx = start_stmt_idx;

    while (line_idx >= 0 && line_idx < (int)app->prog.count) {
        // Allow GTK to process window close/Quit and other UI events.
        ui_pump(app);
        if (app->quitting) return;
        if (app->stop_flag) {
            // Execution was interrupted (user STOP/BREAK or a deferred stop for LOAD).

    set_run_state(app, RUN_STOPPED);
            {
                int stop_line_no = -1;
                if (line_idx >= 0 && line_idx < (int)app->prog.count) stop_line_no = app->prog.lines[line_idx].line_no;
                else if (app->ui_last_exec_line > 0) stop_line_no = app->ui_last_exec_line;
                char sbuf[96];
                if (stop_line_no > 0) {
                    snprintf(sbuf, sizeof(sbuf), "\n**** STOPPED AT LINE %d ****\n", stop_line_no);
                } else {
                    snprintf(sbuf, sizeof(sbuf), "\n**** STOPPED ****\n");
                }
                out_append(app, sbuf);
            }
            // If a LOAD was requested while running, perform it now (stop -> clear output -> load -> idle).
            if (app->pending_load_path) {
                set_run_state(app, RUN_IDLE);
                maybe_do_pending_load(app);
            }
            return;
        }

        int current_line = app->prog.lines[line_idx].line_no;
        app->ui_last_exec_line = current_line;
        /* While executing an ON ERROR handler, we intentionally keep in_error_handler=true
           across multiple handler lines until a successful RESUME (or ON ERROR GOTO 0) clears it.
           This prevents recursive trapping and matches GW-BASIC expectations that the handler can
           span multiple lines. */



/* If an ON KEY(n) GOSUB trap is pending, dispatch it here (safe point). */
if (app->key_trap_enabled && app->on_key_pending >= 0) {
    int k = app->on_key_pending;
    app->on_key_pending = -1;
    if (k >= 0 && k < 10 && app->on_key_enabled[k] && app->on_key_gosub_line[k] > 0 && !app->on_key_in_progress) {
        int idx = program_find_index(&app->prog, app->on_key_gosub_line[k]);
        if (idx >= 0) {
            if (app->gosub_sp >= 128) { runtime_error(app, current_line, "GOSUB stack overflow"); }
            else {
                app->gosub_stack[app->gosub_sp++] = (GosubFrame){
                    .kind = GOSUB_RET_KEYTRAP,
                    .ret_line_idx = line_idx,
                    .ret_stmt_idx = stmt_idx,
                    .ret_chain_next_si = 0,
                    .ret_chain_text = NULL,
                };
                app->on_key_in_progress = true;
                line_idx = idx;
                stmt_idx = 0;
                continue;
            }
        }
    }
}
/* If a KEY macro was pressed while running, execute it here (safe point in interpreter loop). */
if (app->key_trap_enabled && app->runtime_key_macro && app->runtime_key_macro[0] != '\0') {
    char *km = app->runtime_key_macro;
    app->runtime_key_macro = NULL;
    exec_statement_chain(app, km, current_line, &line_idx, &stmt_idx);
    g_free(km);
    /* After macro execution, continue loop with possibly updated line_idx/stmt_idx. */
    continue;
} else if (app->runtime_key_macro) {
    /* empty macro */
    g_free(app->runtime_key_macro);
    app->runtime_key_macro = NULL;
}

        /* If an ON TIMER(interval) GOSUB trap is armed, dispatch it at this safe point.
           GW-BASIC semantics: no re-entrancy; firing is best-effort at statement boundaries. */
        if (app->timer_enabled && !app->timer_stopped && app->on_timer_gosub_line > 0 && app->on_timer_interval > 0.0 && !app->timer_in_progress) {
            double now = timer_now_sec();
            if (timer_reached(now, app->timer_next_fire)) {
                int idx = program_find_index(&app->prog, app->on_timer_gosub_line);
                if (idx >= 0) {
                    if (app->gosub_sp >= 128) { runtime_error(app, current_line, "GOSUB stack overflow"); }
                    else {
                        app->gosub_stack[app->gosub_sp++] = (GosubFrame){
                            .kind = GOSUB_RET_TIMERTRAP,
                            .ret_line_idx = line_idx,
                            .ret_stmt_idx = stmt_idx,
                            .ret_chain_next_si = 0,
                            .ret_chain_text = NULL,
                        };
                        app->timer_in_progress = true;
                        /* next fire scheduled on RETURN from handler (no queued ticks) */
                        line_idx = idx;
                        stmt_idx = 0;
                        continue;
                    }
                } else {
                    // If the handler line doesn't exist, disable the trap to avoid looping.
                    app->on_timer_gosub_line = 0;
                }
            }
        }

        StmtList sl = split_statements(app->prog.lines[line_idx].text);

        if (stmt_idx >= sl.count) {
            stmtlist_free(&sl);
            line_idx++;
            stmt_idx = 0;
            continue;
        }

        const char *stmt = sl.stmts[stmt_idx];

        int next_line_idx = line_idx;
        int next_stmt_idx = stmt_idx + 1;

                /* Phase 0 (RESUME groundwork): publish the current execution cursor so
           runtime_error() can capture the precise (line_idx, stmt_idx) origin. */
        app->exec_cursor_valid = true;
        app->exec_line_idx = line_idx;
        app->exec_stmt_idx = stmt_idx;
        app->exec_line_no = current_line;

bool cont = exec_single_statement(app, stmt, current_line, line_idx, stmt_idx, &next_line_idx, &next_stmt_idx);

        stmtlist_free(&sl);

        // Interpreter-wide speed control (not just output pacing)
        exec_apply_pacing(app);

        /* If a runtime error was trapped via ON ERROR GOTO, override control flow now. */
        if (app->error_trap_pending) {
            next_line_idx = app->error_trap_line_idx;
            next_stmt_idx = app->error_trap_stmt_idx;
            app->error_trap_pending = false;
            cont = true;
        }


        // exec_single_statement() returns false for END (normal termination) and also for
        // certain error/stop paths. Ensure we leave RUNNING when a program terminates.
        // If an error/STOP already set RUN_STOPPED, keep it.
        if (!cont) {
            if (app->run_state == RUN_RUNNING) set_run_state(app, RUN_IDLE);
    maybe_do_pending_load(app);
            return;
        }

        line_idx = next_line_idx;
        stmt_idx = next_stmt_idx;

        ui_pump(app);
        if (app->quitting) return;
    }
    set_run_state(app, RUN_IDLE);
}

static void key_macro_queue_clear(App *app);

static void run_apply_default_screen0(App *app) {
    if (!app) return;
    app->video_mode = WB_VIDEO_TEXT;
    screen_clear(app);
    screen_render(app);
}

static void do_run(App *app) {

    run_apply_default_screen0(app);

        // Ensure each RUN starts with Preferences exact colors (until BASIC COLOR is used).
        app->cur_fg = 16;
        app->cur_bg = 16;
        if (!wbasic_ui_active(app) && headless_stdout_is_tty()) {
            headless_stdout_prepare_ansi();
            /* COLOR with no args => ANSI reset */
            fputs("\x1b[0m", stdout);
        }

    app->print_throttle_carry_ms = 0.0;

    key_macro_queue_clear(app);

    // RUN clears output and resets variables (classic BASIC behavior)
    do_exec_from(app, 0, 0, true, true);
}

/* ------------------------------------------------------------
 * RUN statement (GW-BASIC): restart program execution.
 *
 * Supported forms:
 *   RUN
 *   RUN <line-number>
 *
 * Semantics:
 *   - reset runtime state (vars/stacks/DATA pointer/options)
 *   - clear output (WBASIC policy; programs typically CLS before RUN)
 *   - jump to first statement (or the requested line)
 *
 * Note: Implemented as an in-loop control transfer (not a nested do_exec_from)
 * so it works safely inside ':' chains like "CLS:RUN".
 * ------------------------------------------------------------ */
static bool exec_run_stmt(App *app, const char *s, int current_line, int *line_idx, int *stmt_idx)
{
    /* Parse optional line number */
    Parser rp = { s + 3 };
    skip_ws(&rp);

    int target_line_no = 0;
    bool has_target = false;
    if (*rp.s) {
        double dv = 0.0;
        if (!parse_expr(app, &rp, &dv)) {
            runtime_error(app, current_line, "Bad RUN target");
            return false;
        }
        int n = (int)dv;
        if (fabs(dv - (double)n) > 1e-9 || n < 0) {
            runtime_error(app, current_line, "Bad RUN target");
            return false;
        }
        skip_ws(&rp);
        if (*rp.s != 0) {
            runtime_error(app, current_line, "Syntax error");
            return false;
        }
        target_line_no = n;
        has_target = true;
    }

    /* Rebuild Block IF map (safe; program may have changed before RUN) */
    if (!program_build_ifmap(app)) {
        /* runtime_error already reported */
        set_run_state(app, RUN_STOPPED);
        return false;
    }

    /* Reset colors to preference defaults until BASIC COLOR is used */
    run_apply_default_screen0(app);
    app->cur_fg = 16;
    app->cur_bg = 16;
        if (!wbasic_ui_active(app) && headless_stdout_is_tty()) {
            headless_stdout_prepare_ansi();
            /* COLOR with no args => ANSI reset */
            fputs("\x1b[0m", stdout);
        }

    app->print_throttle_carry_ms = 0.0;
    key_macro_queue_clear(app);

    /* WBASIC RUN policy: clear output and reset runtime state */
    out_clear(app, false);
    runtime_reset(app);
    program_build_data_pool(&app->prog);

    int start_idx = 0;
    if (has_target && target_line_no > 0) {
        int idx = program_find_index(&app->prog, target_line_no);
        if (idx < 0) {
            runtime_error(app, current_line, "Undefined line number");
            return false;
        }
        start_idx = idx;
    }

    /* Jump execution */
    *line_idx = start_idx;
    *stmt_idx = 0;
    return true;
}

static WB_UNUSED void do_stop(App *app) {
    if (!app) return;
    app->pause_flag = false;
    app->stop_flag = true;
    set_run_state(app, RUN_STOPPED);
}

static WB_UNUSED void do_pause_toggle(App *app) {
    if (!app) return;
    // Only meaningful while executing or waiting for INPUT.
    if (app->run_state == RUN_IDLE || app->run_state == RUN_STOPPED) return;
    if (!app->pause_flag) {
        app->pause_flag = true;
        app->pre_pause_state = app->run_state;
        set_run_state(app, RUN_PAUSED);
    } else {
        app->pause_flag = false;
        // ui_pause_wait() will restore RUN_RUNNING at next pump.
        set_run_state(app, RUN_RUNNING);
    }
}


/* ===================== Immediate mode ===================== */

static WB_UNUSED bool parse_quoted_filename(const char *s, char **out) {
    Parser p = { s };
    char *v = NULL;
    if (!parse_string_literal(&p, &v)) return false;
    *out = v;
    return true;
}

static void do_new(App *app) {
    // Stop anything in progress and clear any deferred actions.
    app->stop_flag = false;
    if (app->pending_load_path) { g_free(app->pending_load_path); app->pending_load_path = NULL; }
    app->pending_load_clear_output = false;

    // Reset interpreter runtime state (variables/stacks/options, INKEY buffer, etc.)
    runtime_reset(app);

    // Reset program + UI
    program_free(&app->prog);
    program_init(&app->prog);

    out_clear(app, false);

#ifndef WBASIC_NO_UI
    /* UI/editor state only exists in the GTK interpreter. */
    app->suppress_dirty = true;
    editor_set_text(app->editor_buf, "");
    if (app->cmd_entry) gtk_entry_set_text(GTK_ENTRY(app->cmd_entry), "");
    app->suppress_dirty = false;
#endif

    // Reset AUTO mode
    app->auto_mode = false;
    app->auto_line = 10;
    app->auto_step = 10;

#ifndef WBASIC_NO_UI
    set_current_path(app, NULL);
    mark_dirty(app, false);
#endif

    // NEW should always return the status indicator to Idle.
    set_run_state(app, RUN_IDLE);
}

static void do_list(App *app) {
    // LIST should refresh/rebuild the editor contents from the current program,
    // not dump the listing to the output pane.
    if (!editor_to_program(app)) return;
    program_to_editor(app);
}

// Ensure a filename ends with .bas (case-insensitive). Returns newly-allocated string (g_free when done).
static WB_UNUSED char *ensure_bas_suffix(const char *fn) {
    if (!fn) return NULL;
    size_t n = strlen(fn);
    if (n >= 4 && g_ascii_strcasecmp(fn + (n - 4), ".bas") == 0) {
        return g_strdup(fn);
    }
    return g_strconcat(fn, ".bas", NULL);
}

// Add a "*.bas" filter (and an "All files" fallback) to a GtkFileChooser dialog.
#ifndef WBASIC_NO_UI
static void add_bas_file_filters(GtkWidget *dlg) {
    GtkFileFilter *fbas = gtk_file_filter_new();
    gtk_file_filter_set_name(fbas, "BASIC programs (*.bas)");
    gtk_file_filter_add_pattern(fbas, "*.bas");
    gtk_file_filter_add_pattern(fbas, "*.BAS");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), fbas);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dlg), fbas);

    GtkFileFilter *fall = gtk_file_filter_new();
    gtk_file_filter_set_name(fall, "All files (*)");
    gtk_file_filter_add_pattern(fall, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), fall);
}
#endif /* !WBASIC_NO_UI */

static void maybe_do_pending_load(App *app) {
    if (!app->pending_load_path) return;

    // We are idle now; clear any stop request.
    app->stop_flag = false;

    if (app->pending_load_clear_output) out_clear(app, false);
    file_load_into_editor(app, app->pending_load_path);

    g_free(app->pending_load_path);
    app->pending_load_path = NULL;
    app->pending_load_clear_output = false;

    // After a program load, status should be Idle.
    set_run_state(app, RUN_IDLE);
}

#ifndef WBASIC_NO_UI
static void do_load(App *app, const char *arg) {
    char *fn = NULL;
    if (arg && *arg) {
        if (!parse_quoted_filename(arg, &fn)) {
            out_append(app, "LOAD expects: LOAD \"file.bas\"\n");
            return;
        }
        // If currently executing (or waiting), stop first and defer LOAD until idle.
        if (app->run_state == RUN_RUNNING || (app->run_state == RUN_WAITING && !app->input_waiting)) {
            app->stop_flag = true;

    set_run_state(app, RUN_STOPPED);
            if (app->pending_load_path) g_free(app->pending_load_path);
            app->pending_load_path = g_strdup(fn);
            app->pending_load_clear_output = true;
        } else {
            out_clear(app, false);
            file_load_into_editor(app, fn);
            app->stop_flag = false;
            set_run_state(app, RUN_IDLE);
        }
        free(fn);
        return;
    }
    GtkWidget *dlg = gtk_file_chooser_dialog_new(
        "Load BASIC Program", GTK_WINDOW(app->win), GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL
    );
    attach_windows_dark_titlebar(dlg);
    add_bas_file_filters(dlg);
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
        // If currently executing (or waiting), stop first and defer LOAD until idle.
        if (app->run_state == RUN_RUNNING || app->run_state == RUN_WAITING) {
            app->stop_flag = true;

    set_run_state(app, RUN_STOPPED);
            if (app->pending_load_path) g_free(app->pending_load_path);
            app->pending_load_path = g_strdup(filename);
            app->pending_load_clear_output = true;
        } else {
            out_clear(app, false);
            file_load_into_editor(app, filename);
            app->stop_flag = false;
            set_run_state(app, RUN_IDLE);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dlg);
}
#else
static void do_load(App *app, const char *arg) { (void)arg; if (app) out_append(app, "LOAD is not available in headless builds.\n"); }
#endif /* WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void do_save(App *app, const char *arg) {
    char *fn = NULL;
    if (arg && *arg) {
        if (!parse_quoted_filename(arg, &fn)) {
            out_append(app, "SAVE expects: SAVE \"file.bas\"\n");
            return;
        }
        if (file_save_from_editor(app, fn)) {
            set_current_path(app, fn);
            mark_dirty(app, false);
        }
        free(fn);
        return;
    }
    GtkWidget *dlg = gtk_file_chooser_dialog_new(
        "Save BASIC Program", GTK_WINDOW(app->win), GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL
    );
    attach_windows_dark_titlebar(dlg);
    add_bas_file_filters(dlg);
    if (app->current_path && *app->current_path) {
        /* If a program is already open, default to that filename */
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), app->current_path);
    } else {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), "Program.bas");
    }
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
        char *fixed = ensure_bas_suffix(filename);
        g_free(filename);
        if (file_save_from_editor(app, fixed)) {
            set_current_path(app, fixed);
            mark_dirty(app, false);
        }
        g_free(fixed);
    }
    gtk_widget_destroy(dlg);
}
#else
static void do_save(App *app, const char *arg) { (void)arg; if (app) out_append(app, "SAVE is not available in headless builds.\n"); }
#endif /* WBASIC_NO_UI */


/* ===================== UI Save helpers ===================== */

#ifndef WBASIC_NO_UI
static bool ui_save_as_prompt(App *app)
{
    if (!app) return false;
    GtkWidget *dlg = gtk_file_chooser_dialog_new(
        "Save BASIC Program", GTK_WINDOW(app->win), GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL
    );
    attach_windows_dark_titlebar(dlg);
    add_bas_file_filters(dlg);

    if (app->current_path && *app->current_path) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), app->current_path);
    } else {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), "Program.bas");
    }
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);

    bool ok = false;
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
        char *fixed = ensure_bas_suffix(filename);
        g_free(filename);

        if (file_save_from_editor(app, fixed)) {
            set_current_path(app, fixed);
            mark_dirty(app, false);
            ok = true;
        }
        g_free(fixed);
    }
    gtk_widget_destroy(dlg);
    return ok;
}
#else
static bool ui_save_as_prompt(App *app) { (void)app; return false; }
#endif /* WBASIC_NO_UI */

static WB_UNUSED bool ui_save_current_or_prompt(App *app)
{
    if (!app) return false;
    if (app->current_path && *app->current_path) {
        if (file_save_from_editor(app, app->current_path)) {
            mark_dirty(app, false);
            return true;
        }
        return false;
    }
    return ui_save_as_prompt(app);
}

#ifndef WBASIC_NO_UI
static bool ui_confirm_save_if_dirty(App *app)
{
    if (!app) return true;
    if (!app->dirty) return true;

    const char *name = (app->current_path && *app->current_path) ? app->current_path : "Untitled";
    char msg[1024];
    snprintf(msg, sizeof(msg), "Save changes to \"%s\" before closing?", name);

    GtkWidget *dlg = gtk_message_dialog_new(
        GTK_WINDOW(app->win),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_NONE,
        "%s", msg
    );
    attach_windows_dark_titlebar(dlg);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg),
        "If you don’t save, your changes will be lost.");

    gtk_dialog_add_button(GTK_DIALOG(dlg), "_Cancel", GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button(GTK_DIALOG(dlg), "_Don't Save", GTK_RESPONSE_REJECT);
    gtk_dialog_add_button(GTK_DIALOG(dlg), "_Save", GTK_RESPONSE_ACCEPT);

    int resp = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);

    if (resp == GTK_RESPONSE_CANCEL || resp == GTK_RESPONSE_DELETE_EVENT) {
        return false;
    }
    if (resp == GTK_RESPONSE_REJECT) {
        return true; /* proceed without saving */
    }

    /* Save */
    return ui_save_current_or_prompt(app);
}
#else
static bool ui_confirm_save_if_dirty(App *app) { (void)app; return true; }
#endif /* WBASIC_NO_UI */

static WB_UNUSED void request_quit(App *app)
{
    if (!app) return;
    if (!ui_confirm_save_if_dirty(app)) return;

    app->quitting = true;
    app->stop_flag = true;
    if (app->win) gtk_widget_destroy(app->win);
}

/* ===================== AUTO / RENUM helpers ===================== */

static bool __attribute__((unused)) parse_int_arg(const char *s, int *out) {
    if (!s) return false;
    while (isspace((unsigned char)*s)) s++;
    if (!isdigit((unsigned char)*s) && *s != '+' && *s != '-') return false;
    char *endp = NULL;
    long v = strtol(s, &endp, 10);
    if (!endp || endp == s) return false;
    *out = (int)v;
    return true;
}

static char *renum_rewrite_text(const char *in, const int *old_nums, const int *new_nums, size_t nmap);

static int renum_lookup(const int *old_nums, const int *new_nums, size_t nmap, int oldv) {
    // old_nums is sorted ascending
    size_t lo = 0, hi = nmap;
    while (lo < hi) {
        size_t mid = lo + (hi - lo)/2;
        int v = old_nums[mid];
        if (v == oldv) return new_nums[mid];
        if (v < oldv) lo = mid + 1;
        else hi = mid;
    }
    return -1;
}

static bool is_word_char_(char c) { return isalnum((unsigned char)c) || c == '_'; }

static bool __attribute__((unused)) match_kw_ci(const char *s, const char *kw) {
    // Match kw case-insensitive at s, ensuring word boundaries
    size_t k = strlen(kw);
    if (strncasecmp(s, kw, k) != 0) return false;
    char before = (s > kw ? s[-1] : ' '); // not reliable, but we only call at token starts
    (void)before;
    char after = s[k];
    if (is_word_char_(after)) return false;
    return true;
}

static char *renum_rewrite_text(const char *in, const int *old_nums, const int *new_nums, size_t nmap) {
    const char *kw = NULL;
    int kw_len = 0;
    (void)kw; (void)kw_len;

    if (!in) return xstrdup("");
    size_t len = strlen(in);
    char *out = (char*)malloc(len * 2 + 64); // generous
    if (!out) return xstrdup(in);
    size_t oi = 0;
    bool in_str = false;

    // Helper to append
    #define APPCH(ch) do { out[oi++] = (ch); } while(0)
    #define APPSTR(str) do { const char *_t = (str); while(*_t) out[oi++] = *_t++; } while(0)

    for (size_t i = 0; i < len; ) {
        char c = in[i];

        // Handle strings
        if (c == '"') {
            APPCH(c);
            i++;
            in_str = !in_str;
            continue;
        }
        if (in_str) {
            APPCH(c);
            i++;
            continue;
        }

        // Comments: REM or apostrophe -> copy rest and break
        if (c == '\'') {
            APPSTR(in + i);
            break;
        }

        // Token start? We'll look for REM at word boundary too.

        // Apostrophe inline comment: stop renumber scanning (leave rest of line untouched)
        if (c == '\'' && !in_str) {
            APPSTR(in + i);
            break;
        }
        if (isalpha((unsigned char)c)) {
            // Check REM
            if (strncasecmp(in + i, "REM", 3) == 0 && !is_word_char_(in[i+3])) {
                APPSTR(in + i);
                break;
            }

            // Keywords that take a line number
            int kw_len = 0;
            enum { KW_NONE, KW_GOTO, KW_GOSUB, KW_THEN, KW_ELSE, KW_RESTORE } kind = KW_NONE;

            if (strncasecmp(in + i, "GOTO", 4) == 0 && !is_word_char_(in[i+4])) { kw="GOTO"; kw_len=4; kind=KW_GOTO; }
            else if (strncasecmp(in + i, "GOSUB", 5) == 0 && !is_word_char_(in[i+5])) { kw="GOSUB"; kw_len=5; kind=KW_GOSUB; }
            else if (strncasecmp(in + i, "THEN", 4) == 0 && !is_word_char_(in[i+4])) { kw="THEN"; kw_len=4; kind=KW_THEN; }
            else if (strncasecmp(in + i, "ELSE", 4) == 0 && !is_word_char_(in[i+4])) { kw="ELSE"; kw_len=4; kind=KW_ELSE; }
            else if (strncasecmp(in + i, "RESTORE", 7) == 0 && !is_word_char_(in[i+7])) { kw="RESTORE"; kw_len=7; kind=KW_RESTORE; }

            if (kind != KW_NONE) {
                // Copy keyword exactly as typed
                for (int k=0;k<kw_len;k++) APPCH(in[i+k]);
                i += kw_len;

                // Copy whitespace
                while (i < len && isspace((unsigned char)in[i])) { APPCH(in[i]); i++; }

                // THEN can be followed by a statement; only rewrite if an integer follows
                if (i < len && isdigit((unsigned char)in[i])) {
                    char *endp = NULL;
                    long oldv = strtol(in + i, &endp, 10);
                    if (endp && endp != in + i) {
                        int newv = renum_lookup(old_nums, new_nums, nmap, (int)oldv);
                        char buf[32];
                        if (newv >= 0) {
                            snprintf(buf, sizeof(buf), "%d", newv);
                        } else {
                            snprintf(buf, sizeof(buf), "%ld", oldv);
                        }
                        APPSTR(buf);
                        i = (size_t)(endp - in);
                        continue;
                    }
                }
                // no number; keep going
                continue;
            }
        }

        // Default: copy char
        APPCH(c);
        i++;
    }

    out[oi] = 0;
    #undef APPCH
    #undef APPSTR
    return out;
}

static void do_renum(App *app, int start, int step) {
    if (!app) return;
    if (start <= 0 || step <= 0) { out_append(app, "ERROR: Illegal function call\n"); return; }

    if (!editor_to_program(app)) return;
    Program *p = &app->prog;
    if (p->count == 0) { out_append(app, "OK\n"); return; }

    // Build mapping
    size_t n = p->count;
    int *old_nums = (int*)malloc(n * sizeof(int));
    int *new_nums = (int*)malloc(n * sizeof(int));
    if (!old_nums || !new_nums) { free(old_nums); free(new_nums); out_append(app, "ERROR: Out of memory\n"); return; }

    int cur = start;
    for (size_t i=0;i<n;i++) {
        old_nums[i] = p->lines[i].line_no;
        new_nums[i] = cur;
        cur += step;
    }

    // Rewrite line texts with updated targets
    Line *new_lines = (Line*)calloc(n, sizeof(Line));
    if (!new_lines) { free(old_nums); free(new_nums); out_append(app, "ERROR: Out of memory\n"); return; }

    for (size_t i=0;i<n;i++) {
        new_lines[i].line_no = new_nums[i];
        new_lines[i].text = renum_rewrite_text(p->lines[i].text, old_nums, new_nums, n);
        if (!new_lines[i].text) new_lines[i].text = xstrdup("");
    }

    // Replace program
    program_free(p);
    p->lines = new_lines;
    p->count = n;
    p->cap = n;

    program_to_editor(app);

    free(old_nums);
    free(new_nums);
    out_append(app, "OK\n");
}
static void do_immediate(App *app, const char *cmdline) {
    app->exec_cursor_valid = false;

    char *tmp = xstrdup(cmdline);
    char *s = trim(tmp);

    /* AUTO_EXIT_FIX: If AUTO is active and the user enters nothing after the line number,
       exit AUTO (GW-BASIC behavior). */
    if (app->auto_mode) {
        char *p = s;
        while (*p && isdigit((unsigned char)*p)) p++;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') {
            app->auto_mode = false;
            if (app->cmd_entry) gtk_entry_set_text(GTK_ENTRY(app->cmd_entry), "");
            out_append(app, "OK\n");
            free(tmp);
            return;
        }
    }

    if (*s == 0) { free(tmp); return; }

    // Immediate BEEP
    if (starts_ci(s, "BEEP") && is_word_boundary(s[4])) {
        GdkDisplay *d = gdk_display_get_default();
        if (d) gdk_display_beep(d);
        out_append(app, "OK\n");
        free(tmp);
        return;
    }


    // line-number edit: <n> <text> or <n> alone to delete
    char *endp = NULL;
    long ln = strtol(s, &endp, 10);
    if (endp != s && (isspace((unsigned char)*endp) || *endp == 0)) {
        int line_no = (int)ln;
        char *rest = trim(endp);
        if (!editor_to_program(app)) return;
        program_set_line(&app->prog, line_no, rest);
        program_to_editor(app);
        free(tmp);
        return;
    }

    // commands
    if (starts_ci(s, "RUN")  && is_word_boundary(s[3])) { free(tmp); do_run(app); return; }
    if (starts_ci(s, "LIST") && is_word_boundary(s[4])) { free(tmp); do_list(app); return; }
    if (starts_ci(s, "NEW")  && is_word_boundary(s[3])) { free(tmp); do_new(app); return; }

    if (starts_ci(s, "STOP") && is_word_boundary(s[4])) {
        // Immediate STOP: halt a running program (variables preserved for inspection).
        if (app->run_state == RUN_RUNNING || app->run_state == RUN_WAITING) {
            app->stop_flag = true;

    set_run_state(app, RUN_STOPPED);
            {
                int stop_line_no = (app->ui_last_exec_line > 0) ? app->ui_last_exec_line : -1;
                char sbuf[96];
                if (stop_line_no > 0) {
                    snprintf(sbuf, sizeof(sbuf), "\n**** STOPPED AT LINE %d ****\n", stop_line_no);
                } else {
                    snprintf(sbuf, sizeof(sbuf), "\n**** STOPPED ****\n");
                }
                out_append(app, sbuf);
            }
        } else {
            out_append(app, "OK\n");
        }
        free(tmp);
        return;
    }

    if (starts_ci(s, "AUTO") && is_word_boundary(s[4])) {
        // AUTO [start[,step]]
        char *a = trim(s+4);
        int start = 10, step = 10;
        if (*a) {
            // parse start
            start = (int)strtol(a, &endp, 10);
            if (endp == a) { out_append(app, "ERROR: Illegal function call\n"); free(tmp); return; }
            a = trim(endp);
            if (*a == ',') {
                a = trim(a+1);
                step = (int)strtol(a, &endp, 10);
                if (endp == a) { out_append(app, "ERROR: Illegal function call\n"); free(tmp); return; }
            }
        }
        if (start <= 0 || step <= 0) { out_append(app, "ERROR: Illegal function call\n"); free(tmp); return; }
        app->auto_mode = true;
        app->auto_line = start;
        app->auto_step = step;
        // Prime the entry with the first line number
        if (app->cmd_entry) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d ", app->auto_line);
            gtk_entry_set_text(GTK_ENTRY(app->cmd_entry), buf);
            gtk_editable_set_position(GTK_EDITABLE(app->cmd_entry), -1);
        }
        out_append(app, "OK\n");
        free(tmp);
        return;
    }
    if (starts_ci(s, "RENUM") && is_word_boundary(s[5])) {
        // RENUM [start[,step]]  (simple form)
        char *a = trim(s+5);
        int start = 10, step = 10;
        if (*a) {
            start = (int)strtol(a, &endp, 10);
            if (endp == a) { out_append(app, "ERROR: Illegal function call\n"); free(tmp); return; }
            a = trim(endp);
            if (*a == ',') {
                a = trim(a+1);
                step = (int)strtol(a, &endp, 10);
                if (endp == a) { out_append(app, "ERROR: Illegal function call\n"); free(tmp); return; }
            }
        }
        do_renum(app, start, step);
        free(tmp);
        return;
    }
    if (starts_ci(s, "LOAD") && is_word_boundary(s[4])) {
        char *a = trim(s+4);
        char *acpy = (*a) ? xstrdup(a) : NULL;
        free(tmp);
        do_load(app, acpy ? acpy : NULL);
        free(acpy);
        return;
    }
    if (starts_ci(s, "SAVE") && is_word_boundary(s[4])) {
        char *a = trim(s+4);
        char *acpy = (*a) ? xstrdup(a) : NULL;
        free(tmp);
        do_save(app, acpy ? acpy : NULL);
        free(acpy);
        return;
    }
    if (starts_ci(s, "RANDOMIZE") && is_word_boundary(s[9])) {
        char *a = trim(s+9);
        unsigned seed = (unsigned)time(NULL);
        if (*a) {
            char *endp2 = NULL;
            double v = strtod(a, &endp2);
            if (endp2 && endp2 != a) seed = (unsigned)fabs(v);
        }
        srand(seed);
        app->have_last_rnd = false;
        out_append(app, "OK\n");
        free(tmp);
        return;
    }

    if ((starts_ci(s, "CLS") && is_word_boundary(s[3])) ||
        (starts_ci(s, "CLEAR")&& is_word_boundary(s[5]))) {
        if (starts_ci(s, "CLS") && (video_mode_is_graphics(app->video_mode))) {
            gfx_clear(app, (unsigned char)((app->cur_bg >= 0) ? app->cur_bg : 0));
            gfx_draw_reset_defaults(app);
            screen_clear(app);
            screen_render(app);
        } else {
            out_clear(app, true);
        }
        free(tmp);
        return;
    }

    // immediate statement (single line, may contain ':')
    // Execute statement-by-statement. If a statement performs flow-control (GOTO/GOSUB/RETURN)
    // into the stored program, we transfer execution to the program runner starting at that target.
    StmtList sl = split_statements(s);
    int si = 0;
    while (si < sl.count) {
        int nli = 0;
        int nsi = si + 1;

        bool cont = exec_single_statement(app, sl.stmts[si], -1, -1, -1, &nli, &nsi);
        if (!cont) break;

        if (nli != 0) {
            // Jump into program execution without clearing output and without resetting variables
            stmtlist_free(&sl);
            free(tmp);
            do_exec_from(app, nli, nsi, false, false);
            return;
        }

        si = nsi;
        ui_pump(app);
        if (app->quitting) break;
    }
    stmtlist_free(&sl);
    free(tmp);
}

/* ===================== UI callbacks ===================== */

#ifndef WBASIC_NO_UI
static void on_menu_new(GtkMenuItem *mi, gpointer user_data)
{
    (void)mi;
    App *app = (App*)user_data;
    if (!ui_confirm_save_if_dirty(app)) return;
    do_new(app);
}
#endif /* !WBASIC_NO_UI */
#ifndef WBASIC_NO_UI
static void on_menu_open(GtkMenuItem *mi, gpointer user_data)
{
    (void)mi;
    App *app = (App*)user_data;
    if (!ui_confirm_save_if_dirty(app)) return;
    do_load(app, NULL);
}
#endif /* !WBASIC_NO_UI */
#ifndef WBASIC_NO_UI
static void on_menu_save(GtkMenuItem *mi, gpointer user_data) { (void)mi; ui_save_current_or_prompt((App*)user_data); }
#ifndef WBASIC_NO_UI
#endif /* !WBASIC_NO_UI */
static void on_menu_save_as(GtkMenuItem *mi, gpointer user_data) { (void)mi; ui_save_as_prompt((App*)user_data); }
#ifndef WBASIC_NO_UI
#endif /* !WBASIC_NO_UI */
static void on_menu_quit(GtkMenuItem *mi, gpointer user_data) { (void)mi; request_quit((App*)user_data); }
#ifndef WBASIC_NO_UI
#endif /* !WBASIC_NO_UI */
static void on_menu_run(GtkMenuItem *mi, gpointer user_data)  { (void)mi; do_run((App*)user_data); }
#ifndef WBASIC_NO_UI
#endif /* !WBASIC_NO_UI */
static void on_menu_stop(GtkMenuItem *mi, gpointer user_data) { (void)mi; do_stop((App*)user_data); }
#ifndef WBASIC_NO_UI
#endif /* !WBASIC_NO_UI */
static void on_menu_pause(GtkMenuItem *mi, gpointer user_data) { (void)mi; do_pause_toggle((App*)user_data); }
#ifndef WBASIC_NO_UI
#endif /* !WBASIC_NO_UI */
static void on_menu_list(GtkMenuItem *mi, gpointer user_data) { (void)mi; do_list((App*)user_data); }
#ifndef WBASIC_NO_UI
#endif /* !WBASIC_NO_UI */
static void on_menu_renum(GtkMenuItem *mi, gpointer user_data) { (void)mi; do_immediate((App*)user_data, "RENUM"); }

#endif /* !WBASIC_NO_UI */
static void undo_stack_free(UndoStack *u);

#ifndef WBASIC_NO_UI
static void pref_on_fg(GtkColorButton *b, gpointer user_data) {
    App *app = (App*)user_data;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(b), &app->fg_color);
    app->have_fg = true;
    apply_theme(app);
    if (app) { undo_stack_free(app->editor_undo); app->editor_undo = NULL; }

    prefs_save(app);
}
#endif /* !WBASIC_NO_UI */
#ifndef WBASIC_NO_UI
static void pref_on_bg(GtkColorButton *b, gpointer user_data) {
    App *app = (App*)user_data;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(b), &app->bg_color);
    app->have_bg = true;
    apply_theme(app);
    prefs_save(app);
}
#endif /* !WBASIC_NO_UI */
#ifndef WBASIC_NO_UI
static void pref_on_font(GtkFontButton *b, gpointer user_data) {
    App *app = (App*)user_data;
    // GtkFontButton implements GtkFontChooser
    gchar *fn = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(b));
    if (fn && *fn) {
        free(app->font_name);
        app->font_name = xstrdup(fn);
        apply_font_css(app);
        g_free(fn);
        apply_theme(app);
        prefs_save(app);

        // Force redraw so the new CSS is reflected immediately
        if (app->editor_view) { gtk_widget_queue_draw(app->editor_view); gtk_widget_queue_resize(app->editor_view); }
        if (app->output_view) { gtk_widget_queue_draw(app->output_view); gtk_widget_queue_resize(app->output_view); }
        gtk_style_context_reset_widgets(gdk_screen_get_default());
    } else if (fn) {
        g_free(fn);
    }
}
#endif /* !WBASIC_NO_UI */
#ifndef WBASIC_NO_UI
/* deleted unused static function: pref_on_split */
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void pref_on_reset(GtkButton *btn, gpointer user_data) {
    (void)btn;
    App *app = (App*)user_data;
    app->have_fg = false;
    app->have_bg = false;
    apply_theme(app);
    prefs_save(app);
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void pref_on_show_splash(GtkToggleButton *btn, gpointer user_data)
{
    App *app = (App *)user_data;
    if (!app) return;
    app->show_splash = gtk_toggle_button_get_active(btn) ? true : false;
    prefs_save(app);
}

static void show_preferences(App *app) {
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "Preferences",
        GTK_WINDOW(app->win),
        GTK_DIALOG_MODAL,
        "_Close", GTK_RESPONSE_CLOSE,
    NULL
    );
    attach_windows_dark_titlebar(dlg);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
    gtk_box_pack_start(GTK_BOX(content), grid, TRUE, TRUE, 0);

    GtkWidget *lbl_fg = gtk_label_new("Foreground:");
    gtk_widget_set_halign(lbl_fg, GTK_ALIGN_START);
    GtkWidget *btn_fg = gtk_color_button_new();
    if (app->have_fg) gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(btn_fg), &app->fg_color);
    g_signal_connect(btn_fg, "color-set", G_CALLBACK(pref_on_fg), app);

    GtkWidget *lbl_bg = gtk_label_new("Background:");
    gtk_widget_set_halign(lbl_bg, GTK_ALIGN_START);
    GtkWidget *btn_bg = gtk_color_button_new();
    if (app->have_bg) gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(btn_bg), &app->bg_color);
    g_signal_connect(btn_bg, "color-set", G_CALLBACK(pref_on_bg), app);

    GtkWidget *lbl_font = gtk_label_new("Font:");
    gtk_widget_set_halign(lbl_font, GTK_ALIGN_START);
    GtkWidget *btn_font = gtk_font_button_new();
    gtk_font_button_set_show_size(GTK_FONT_BUTTON(btn_font), TRUE);
    gtk_font_button_set_use_font(GTK_FONT_BUTTON(btn_font), TRUE);
    if (app->font_name && *app->font_name) gtk_font_chooser_set_font(GTK_FONT_CHOOSER(btn_font), app->font_name);
    g_signal_connect(btn_font, "font-set", G_CALLBACK(pref_on_font), app);

        // Endpoint labels

    GtkWidget *btn_reset = gtk_button_new_with_label("Reset Colors");
    g_signal_connect(btn_reset, "clicked", G_CALLBACK(pref_on_reset), app);

    gtk_grid_attach(GTK_GRID(grid), lbl_fg, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_fg, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_bg, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_bg, 1, 1, 1, 1);
    /* Reset Colors belongs with the color pickers */
    gtk_grid_attach(GTK_GRID(grid), btn_reset, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_font, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_font, 1, 3, 1, 1);
    GtkWidget *sep_splash = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_hexpand(sep_splash, TRUE);
    gtk_widget_set_margin_top(sep_splash, 10);
    gtk_widget_set_margin_bottom(sep_splash, 4);

    GtkWidget *chk_splash = gtk_check_button_new_with_label("Show splash screen at startup");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_splash), app->show_splash ? TRUE : FALSE);
    g_signal_connect(chk_splash, "toggled", G_CALLBACK(pref_on_show_splash), app);

    gtk_grid_attach(GTK_GRID(grid), sep_splash, 0, 4, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), chk_splash, 1, 5, 1, 1);

    gtk_widget_show_all(dlg);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}
#endif /* !WBASIC_NO_UI */


#ifndef WBASIC_NO_UI
static void on_menu_prefs(GtkMenuItem *mi, gpointer user_data) { (void)mi; show_preferences((App*)user_data); }

#endif /* !WBASIC_NO_UI */

/* Edit menu clipboard helpers */
typedef enum { EDIT_CUT=1, EDIT_COPY=2, EDIT_PASTE=3 } EditAction;

#ifndef WBASIC_NO_UI
static WB_UNUSED void do_edit_clipboard(App *app, EditAction act)
{
    if (!app || !app->win) return;

    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(app->win));
    if (!focus) focus = app->editor_view ? app->editor_view : app->cmd_entry;

    /* GtkEntry (and other editables) */
    if (focus && GTK_IS_EDITABLE(focus)) {
        GtkEditable *e = GTK_EDITABLE(focus);
        if (act == EDIT_CUT)   gtk_editable_cut_clipboard(e);
        if (act == EDIT_COPY)  gtk_editable_copy_clipboard(e);
        if (act == EDIT_PASTE) gtk_editable_paste_clipboard(e);
        return;
    }

    /* GtkTextView */
    if (focus && GTK_IS_TEXT_VIEW(focus)) {
        GtkTextView *tv = GTK_TEXT_VIEW(focus);
        GtkTextBuffer *buf = gtk_text_view_get_buffer(tv);
        GtkClipboard *cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

        gboolean editable = gtk_text_view_get_editable(tv);
        if (act == EDIT_COPY) {
            gtk_text_buffer_copy_clipboard(buf, cb);
        } else if (act == EDIT_CUT) {
            if (editable) gtk_text_buffer_cut_clipboard(buf, cb, TRUE);
        } else if (act == EDIT_PASTE) {
            if (editable) gtk_text_buffer_paste_clipboard(buf, cb, NULL, TRUE);
        }
        return;
    }
}
#else
static WB_UNUSED void do_edit_clipboard(App *app, EditAction act) { (void)app; (void)act; }
#endif /* WBASIC_NO_UI */


#ifndef WBASIC_NO_UI
static WB_UNUSED void do_edit_select_all(App *app)
{
    if (!app || !app->win) return;

    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(app->win));
    if (!focus) focus = app->editor_view ? app->editor_view : app->cmd_entry;

    if (focus && GTK_IS_EDITABLE(focus)) {
        gtk_editable_select_region(GTK_EDITABLE(focus), 0, -1);
        return;
    }
    if (focus && GTK_IS_TEXT_VIEW(focus)) {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(focus));
        GtkTextIter a, b;
        gtk_text_buffer_get_start_iter(buf, &a);
        gtk_text_buffer_get_end_iter(buf, &b);
        gtk_text_buffer_select_range(buf, &a, &b);
    }
}
#else
static WB_UNUSED void do_edit_select_all(App *app) { (void)app; }
#endif /* WBASIC_NO_UI */


typedef struct UndoStack {
    GPtrArray *states;   // array of gchar* snapshots
    int index;           // current state index within states
    int max_states;      // safety cap
    bool in_apply;       // guard to avoid recording while applying undo/redo
} UndoStack;

#ifndef WBASIC_NO_UI
static char *buffer_get_all_text(GtkTextBuffer *buf)
{
    GtkTextIter a, b;
    gtk_text_buffer_get_start_iter(buf, &a);
    gtk_text_buffer_get_end_iter(buf, &b);
    return gtk_text_buffer_get_text(buf, &a, &b, FALSE);
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void buffer_set_all_text(GtkTextBuffer *buf, const char *s)
{
    gtk_text_buffer_set_text(buf, s ? s : "", -1);
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static UndoStack *undo_stack_new_for_buffer(GtkTextBuffer *buf)
{
    UndoStack *u = g_new0(UndoStack, 1);
    u->states = g_ptr_array_new_with_free_func(g_free);
    u->index = 0;
    u->max_states = 1000;
    u->in_apply = false;

    char *t = buffer_get_all_text(buf);
    g_ptr_array_add(u->states, t ? t : g_strdup(""));
    u->index = 0;
    return u;
}
#endif /* !WBASIC_NO_UI */
static WB_UNUSED void undo_stack_free(UndoStack *u)
{
    if (!u) return;
    if (u->states) g_ptr_array_free(u->states, TRUE);
    g_free(u);
}

#ifndef WBASIC_NO_UI
static void undo_stack_record_state(UndoStack *u, GtkTextBuffer *buf)
{
    if (!u || u->in_apply) return;

    char *t = buffer_get_all_text(buf);
    if (!t) t = g_strdup("");

    const char *cur = (u->states && u->states->len > 0) ? (const char*)g_ptr_array_index(u->states, u->index) : "";
    if (cur && strcmp(cur, t) == 0) { g_free(t); return; }

    // Drop any redo history
    while ((int)u->states->len - 1 > u->index) {
        g_ptr_array_remove_index(u->states, u->states->len - 1);
    }

    g_ptr_array_add(u->states, t);
    u->index = (int)u->states->len - 1;

    // Enforce cap by trimming from the front
    while ((int)u->states->len > u->max_states) {
        g_ptr_array_remove_index(u->states, 0);
        u->index--;
        if (u->index < 0) u->index = 0;
    }
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void on_editor_buffer_changed(GtkTextBuffer *buf, gpointer user_data)
{
    App *app = (App*)user_data;
    if (!app || !app->editor_undo) return;
    undo_stack_record_state(app->editor_undo, buf);
}
#endif /* !WBASIC_NO_UI */

static WB_UNUSED bool undo_stack_can_undo(UndoStack *u) { return u && u->states && u->index > 0; }
static WB_UNUSED bool undo_stack_can_redo(UndoStack *u) { return u && u->states && u->index < (int)u->states->len - 1; }

#ifndef WBASIC_NO_UI
static void undo_stack_apply_index(UndoStack *u, GtkTextBuffer *buf, int new_index)
{
    if (!u || !u->states || new_index < 0 || new_index >= (int)u->states->len) return;
    u->in_apply = true;
    const char *s = (const char*)g_ptr_array_index(u->states, new_index);
    buffer_set_all_text(buf, s);
    u->index = new_index;
    u->in_apply = false;
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static WB_UNUSED void do_edit_undo_redo(App *app, bool is_redo)
{
    if (!app || !app->win) return;

    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(app->win));
    if (!focus) focus = app->editor_view ? app->editor_view : app->cmd_entry;

    // WBASIC uses a plain GtkTextView editor (no GtkSourceView dependency).
    // Undo/Redo applies only to the main editor buffer.
    if (focus == app->editor_view && app->editor_buf && app->editor_undo) {
        if (!is_redo) {
            if (undo_stack_can_undo(app->editor_undo)) {
                undo_stack_apply_index(app->editor_undo, app->editor_buf, app->editor_undo->index - 1);
            }
        } else {
            if (undo_stack_can_redo(app->editor_undo)) {
                undo_stack_apply_index(app->editor_undo, app->editor_buf, app->editor_undo->index + 1);
            }
        }
    }
}
#else
static WB_UNUSED void do_edit_undo_redo(App *app, bool is_redo) { (void)app; (void)is_redo; }
#endif /* WBASIC_NO_UI */


#ifndef WBASIC_NO_UI
static void on_menu_undo(GtkMenuItem *mi, gpointer user_data)       { (void)mi; do_edit_undo_redo((App*)user_data, false); }
#ifndef WBASIC_NO_UI
#endif /* !WBASIC_NO_UI */
static void on_menu_redo(GtkMenuItem *mi, gpointer user_data)       { (void)mi; do_edit_undo_redo((App*)user_data, true); }
#ifndef WBASIC_NO_UI
#endif /* !WBASIC_NO_UI */
static void on_menu_select_all(GtkMenuItem *mi, gpointer user_data) { (void)mi; do_edit_select_all((App*)user_data); }

#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void on_menu_cut(GtkMenuItem *mi, gpointer user_data)   { (void)mi; do_edit_clipboard((App*)user_data, EDIT_CUT); }
#ifndef WBASIC_NO_UI
#endif /* !WBASIC_NO_UI */
static void on_menu_copy(GtkMenuItem *mi, gpointer user_data)  { (void)mi; do_edit_clipboard((App*)user_data, EDIT_COPY); }
#ifndef WBASIC_NO_UI
#endif /* !WBASIC_NO_UI */
static void on_menu_paste(GtkMenuItem *mi, gpointer user_data) { (void)mi; do_edit_clipboard((App*)user_data, EDIT_PASTE); }

#endif /* !WBASIC_NO_UI */
#ifndef WBASIC_NO_UI
static void on_menu_about(GtkMenuItem *mi, gpointer user_data) {
    (void)mi;
    App *app = (App*)user_data;

/* About text (V1.07) */
const char *about_line2 = "Version V" WBASIC_VERSION_STR;
const char *about_line3 = "February 13, 2026";

/* Custom About dialog (non-deprecated APIs) */
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        "About WBASIC",
        GTK_WINDOW(app->win),
        GTK_DIALOG_MODAL,
        "_OK",
        GTK_RESPONSE_OK,
        NULL
    );
    attach_windows_dark_titlebar(dlg);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(content), hbox, TRUE, TRUE, 0);

    GdkPixbuf *logo = ui_load_wbasic_icon_pixbuf();
    if (logo) {
        /* Left logo */
        GtkWidget *img = gtk_image_new_from_pixbuf(logo);
        gtk_box_pack_start(GTK_BOX(hbox), img, FALSE, FALSE, 0);

        /* Window icon */
        gtk_window_set_icon(GTK_WINDOW(dlg), logo);
    }

    /* Right text */
GtkWidget *rv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_valign(rv, GTK_ALIGN_CENTER);
gtk_box_pack_start(GTK_BOX(hbox), rv, TRUE, TRUE, 0);

GtkWidget *title = gtk_label_new(NULL);
gtk_label_set_markup(GTK_LABEL(title),
                     "<span size='xx-large' weight='bold'>Wally&#39;s Basic (Wbasic)</span>");
gtk_widget_set_halign(title, GTK_ALIGN_START);
gtk_box_pack_start(GTK_BOX(rv), title, FALSE, FALSE, 0);

GtkWidget *l2 = gtk_label_new(about_line2);
gtk_widget_set_halign(l2, GTK_ALIGN_START);
gtk_box_pack_start(GTK_BOX(rv), l2, FALSE, FALSE, 0);

GtkWidget *l3 = gtk_label_new(about_line3);
gtk_widget_set_halign(l3, GTK_ALIGN_START);
gtk_box_pack_start(GTK_BOX(rv), l3, FALSE, FALSE, 0);

    gtk_widget_show_all(dlg);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);

    if (logo) g_object_unref(logo);


}
#endif /* !WBASIC_NO_UI */


#ifndef WBASIC_NO_UI
static gboolean on_win_configure(GtkWidget *w, GdkEvent *event, gpointer user_data) {
    (void)w;
    (void)event;
    App *app = (App*)user_data;
    if (!app) return FALSE;

    // Track window size continuously so we can restore it reliably.
    // Some window managers may report "maximized" early; we still record the
    // last known size so it persists across restarts.
    if (app->win) {
        int ww = 0, wh = 0;
        if (event && event->type == GDK_CONFIGURE) {
            GdkEventConfigure *ce = (GdkEventConfigure*)event;
            ww = ce->width; wh = ce->height;
        } else {
            gtk_window_get_size(GTK_WINDOW(w), &ww, &wh);
        }
        if (ww > 0 && wh > 0) {
            app->have_win_size = true;
            app->win_w = ww;
            app->win_h = wh;
        }
        // Track window position as well (x/y in root window coordinates).
        // We only update when GTK provides configure-event coordinates.
        if (event && event->type == GDK_CONFIGURE) {
            GdkEventConfigure *ce2 = (GdkEventConfigure*)event;
            int wx = ce2->x;
            int wy = ce2->y;
            // Some WMs may report extreme negative values when minimized; ignore those.
            if (wx > -10000 && wx < 100000 && wy > -10000 && wy < 100000) {
                app->have_win_pos = true;
                app->win_x = wx;
                app->win_y = wy;
            }
        }
    }
    return FALSE; // propagate
}
#endif /* !WBASIC_NO_UI */




#ifndef WBASIC_NO_UI
static void on_win_size_allocate(GtkWidget *w, GtkAllocation *alloc, gpointer user_data) {
    (void)w;
    App *app = (App*)user_data;
    if (!app || !alloc) return;

    // Fires reliably; use it to persist last known window size.
    if (alloc->width > 0 && alloc->height > 0) {
        app->have_win_size = true;
        app->win_w = alloc->width;
        app->win_h = alloc->height;
    }
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static gboolean on_win_delete(GtkWidget *w, GdkEvent *event, gpointer user_data) {
    (void)w;
    (void)event;
    App *app = (App*)user_data;
    if (!app) return FALSE;
    if (!ui_confirm_save_if_dirty(app)) {
        return TRUE; /* cancel close */
    }
    app->quitting = true;
    app->stop_flag = true;
    if (app->paned) { app->have_paned_pos = true; app->paned_pos = gtk_paned_get_position(GTK_PANED(app->paned)); }
    prefs_save(app);
    return FALSE; /* allow close */
}
#endif /* !WBASIC_NO_UI */


#ifndef WBASIC_NO_UI
static void on_win_destroy(GtkWidget *w, gpointer user_data) {
    (void)w;
    App *app = (App*)user_data;
    if (app) {
        /* Ensure interpreter loops stop touching GTK after teardown begins. */
        app->quitting = true;
        app->stop_flag = true;
        app->ui_destroyed = true;
        /* Null out widget pointers so any late-running code can safely bail. */
        app->win = NULL;
        app->cmd_entry = NULL;
        app->editor_view = NULL;
        app->output_view = NULL;
        app->status_led = NULL;
        app->status_label = NULL;
    }
    // Save prefs on final teardown (window size already tracked during runtime).
    if (app && app->paned) {
        app->have_paned_pos = true;
        app->paned_pos = gtk_paned_get_position(GTK_PANED(app->paned));
    }

    prefs_save(app);
    gtk_main_quit();
}
#endif /* !WBASIC_NO_UI */




/* INPUT inline live-echo support: mirror current cmd_entry text into output screen buffer */
static void input_echo_update(App *app, const char *txt) {
    if (!app) return;
    if (!(app->run_state == RUN_WAITING && app->input_waiting)) return;

    screen_ensure(app);
    if (!app->screen) return;

    /* Save current output cursor, but (by design) we keep the visible cursor at the end of INPUT text. */
    int save_r = app->out_row;
    int save_c = app->out_col;

    const char *s = (txt ? txt : "");
    int tlen = (int)strlen(s);
    int new_draw_len = tlen + (app->input_cursor_on ? 1 : 0);
    int old_draw_len = app->input_echo_draw_len;

    /* Rewind to where INPUT text begins (right after the prompt). */
    app->out_row = app->input_echo_row;
    app->out_col = app->input_echo_col;

    /* Clear previously drawn echo (text + cursor), overwriting with spaces. */
    if (old_draw_len > 0) {
        for (int i = 0; i < old_draw_len; i++) screen_write(app, " ");
        app->out_row = app->input_echo_row;
        app->out_col = app->input_echo_col;
    }

    /* Write the current text. */
    if (tlen > 0) screen_write(app, s);

    /* Draw the simulated cursor glyph immediately after the text. */
    if (app->input_cursor_on) screen_write(app, "_");

    app->input_echo_len = tlen;
    app->input_echo_draw_len = new_draw_len;

    screen_render(app);

    /* Leave cursor at end of echoed input (terminal-like). */
    (void)save_r; (void)save_c;
}

/* INPUT$(n) keyboard read (Phase: Binary I/O Tier-1)
   NOTE: In GTK UI builds, this uses the existing terminal-style input line mechanism
   (waits for Enter) and returns the first N chars of the entered line.
*/
static char *inputdollar_read_keyboard(App *app, int n, int current_line) {
    if (n <= 0) return xstrdup("");
#ifdef WBASIC_NO_UI
    char *buf = (char*)malloc((size_t)n + 1);
    if (!buf) { runtime_error(app, current_line, "Out of memory"); return NULL; }
    size_t got = fread(buf, 1, (size_t)n, stdin);
    buf[got] = 0;
    return buf;
#else
    if (!app) return xstrdup("");

    app->input_waiting = true;
    app->input_ready = false;
    if (app->input_line) { g_free(app->input_line); app->input_line = NULL; }

    if (app->cmd_entry) gtk_widget_grab_focus(app->cmd_entry);

    set_run_state(app, RUN_WAITING);
    input_echo_update(app, "");

    while (!app->input_ready && !app->stop_flag && !app->quitting) {
        ui_pump_raw(app);
        ui_delay_ms(app, 5);
    }

    app->input_waiting = false;
    if (app->stop_flag || app->quitting) return xstrdup("");

    const char *line = app->input_line ? app->input_line : "";
    size_t L = strlen(line);
    if ((size_t)n > L) n = (int)L;
    char *out = (char*)malloc((size_t)n + 1);
    if (!out) { runtime_error(app, current_line, "Out of memory"); return NULL; }
    memcpy(out, line, (size_t)n);
    out[n] = 0;
    return out;
#endif
}



#ifndef WBASIC_NO_UI
static void on_cmd_changed(GtkEditable *editable, gpointer user_data) {
    App *app = (App*)user_data;
    if (!app) return;
    if (app->ui_destroyed) return;
    if (app->suppress_cmd_changed) return;
    if (!(app->run_state == RUN_WAITING && app->input_waiting)) return;
    if (!editable || !GTK_IS_EDITABLE(editable) || !GTK_IS_ENTRY(editable)) return;
    const char *txt = gtk_entry_get_text(GTK_ENTRY(editable));
    input_echo_update(app, txt ? txt : "");
}
#endif /* !WBASIC_NO_UI */
#ifndef WBASIC_NO_UI
static void on_cmd_activate(GtkEntry *entry, gpointer user_data) {
    App *app = (App*)user_data;
    if (!app || app->ui_destroyed) return;
    if (!entry || !GTK_IS_ENTRY(entry)) return;
    const char *cmd = gtk_entry_get_text(entry);

    /* If a program is waiting on INPUT, treat this entry as the INPUT line. */
    if (app && app->run_state == RUN_WAITING && app->input_waiting) {
        const char *txt = cmd ? cmd : "";
        if (app->input_line) { g_free(app->input_line); app->input_line = NULL; }
        app->input_line = g_strdup(txt);
        app->input_ready = true;

        /* Remove the blinking cursor glyph before committing the newline. */
        app->input_cursor_on = false;
        input_echo_update(app, txt ? txt : "");

        /* The typed line is already live-echoed; just commit a newline. */
        screen_newline(app);
        screen_render(app);
        app->suppress_cmd_changed = true;
        gtk_entry_set_text(entry, "");
        app->suppress_cmd_changed = false;
        return;
    }

    // AUTO mode: prefill next line number after each line entry.
    if (app && app->auto_mode) {
        char *tmp = xstrdup(cmd ? cmd : "");
        char *s = trim(tmp);

        /* If the user cleared the entry and hit Enter, exit AUTO. */
        if (*s == 0) {
            app->auto_mode = false;
            gtk_entry_set_text(entry, "");
            out_append(app, "OK\n");
            free(tmp);
            return;
        }

        /* If user typed a non-line-number command, exit AUTO and process normally. */
        if (!isdigit((unsigned char)*s)) {
            app->auto_mode = false;
            free(tmp);
            do_immediate(app, cmd);
            gtk_entry_set_text(entry, "");
            return;
        }

        /* Parse the user-provided line number. */
        char *endp = NULL;
        long ln = strtol(s, &endp, 10);
        if (endp == s) {
            /* Shouldn't happen due to digit check, but be safe. */
            app->auto_mode = false;
            free(tmp);
            do_immediate(app, cmd);
            gtk_entry_set_text(entry, "");
            return;
        }

        /* GW-BASIC behavior: if the user enters ONLY the line number (no text after it),
           exit AUTO immediately (do not advance to the next line number). */
        {
            const char *p = endp;
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '\0') {
                app->auto_mode = false;
                gtk_entry_set_text(entry, "");
                out_append(app, "OK\n");
                free(tmp);
                return;
            }
        }

        /* Process the line normally. */
        do_immediate(app, cmd);

        /* Advance AUTO line number from what user actually entered. */
        app->auto_line = (int)ln + app->auto_step;

        char buf[32];
        snprintf(buf, sizeof(buf), "%d ", app->auto_line);
        gtk_entry_set_text(entry, buf);
        gtk_editable_set_position(GTK_EDITABLE(entry), -1);

        free(tmp);
        return;
    }

    do_immediate(app, cmd);
    // If AUTO was just enabled by this command, keep the primed line number.
    if (!(app && app->auto_mode)) {
        gtk_entry_set_text(entry, "");
    } else {
        gtk_editable_set_position(GTK_EDITABLE(entry), -1);
    }
}
#endif /* !WBASIC_NO_UI */

/* Forward decl: used by menu builder */
#ifndef WBASIC_NO_UI
static void on_menu_export_standalone(GtkMenuItem *mi, gpointer user_data);

/* ===================== UI construction ===================== */

#ifndef WBASIC_NO_UI
static GtkWidget *make_menu_bar(App *app) {
    GtkWidget *menubar = gtk_menu_bar_new();

    // File
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *file = gtk_menu_item_new_with_label("File");
    GtkWidget *mi_new  = gtk_menu_item_new_with_label("New");
    gtk_widget_add_accelerator(mi_new, "activate", app->accel, GDK_KEY_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    GtkWidget *mi_open = gtk_menu_item_new_with_label("Open...");
    gtk_widget_add_accelerator(mi_open, "activate", app->accel, GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    GtkWidget *mi_save = gtk_menu_item_new_with_label("Save");
    
    app->mi_save = mi_save;
gtk_widget_add_accelerator(mi_save, "activate", app->accel, GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    GtkWidget *mi_save_as = gtk_menu_item_new_with_label("Save As...");
    GtkWidget *mi_export_standalone = gtk_menu_item_new_with_label("Export Standalone...");
    gtk_widget_add_accelerator(mi_export_standalone, "activate", app->accel, GDK_KEY_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    
    app->mi_save_as = mi_save_as;
gtk_widget_add_accelerator(mi_save_as, "activate", app->accel, GDK_KEY_s, GDK_SHIFT_MASK|GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    GtkWidget *mi_quit = gtk_menu_item_new_with_label("Quit");
    gtk_widget_add_accelerator(mi_quit, "activate", app->accel, GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_open);
    
    /* Recent Files submenu */
    GtkWidget *mi_recent = gtk_menu_item_new_with_label("Recent Files");
    GtkWidget *recent_menu = gtk_menu_new();
    app->mi_recent_menu = recent_menu;
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_recent), recent_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_recent);
    g_signal_connect(mi_recent, "activate", G_CALLBACK(on_recent_menu_activate), app);
    recent_menu_rebuild(app);
gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_save);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_save_as);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_export_standalone);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_quit);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), file_menu);
    g_signal_connect(mi_new,  "activate", G_CALLBACK(on_menu_new),  app);
    g_signal_connect(mi_open, "activate", G_CALLBACK(on_menu_open), app);
    g_signal_connect(mi_save,    "activate", G_CALLBACK(on_menu_save),    app);
    g_signal_connect(mi_save_as, "activate", G_CALLBACK(on_menu_save_as), app);
    g_signal_connect(mi_export_standalone, "activate", G_CALLBACK(on_menu_export_standalone), app);
    g_signal_connect(mi_quit,    "activate", G_CALLBACK(on_menu_quit),    app);

    // Program
    GtkWidget *prog_menu = gtk_menu_new();
    GtkWidget *prog = gtk_menu_item_new_with_label("Program");
    GtkWidget *mi_list = gtk_menu_item_new_with_label("List");
    gtk_widget_add_accelerator(mi_list, "activate", app->accel, GDK_KEY_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    GtkWidget *mi_run  = gtk_menu_item_new_with_label("Run");
    gtk_widget_add_accelerator(mi_run, "activate", app->accel, GDK_KEY_F5, 0, GTK_ACCEL_VISIBLE);
    GtkWidget *mi_stop = gtk_menu_item_new_with_label("Stop");
    gtk_widget_add_accelerator(mi_stop, "activate", app->accel, GDK_KEY_Escape, 0, GTK_ACCEL_VISIBLE);
    GtkWidget *mi_pause = gtk_menu_item_new_with_label("Pause/Resume");
    gtk_widget_add_accelerator(mi_pause, "activate", app->accel, GDK_KEY_F6, 0, GTK_ACCEL_VISIBLE);
    GtkWidget *mi_sep_prog = gtk_separator_menu_item_new();
    GtkWidget *mi_renum = gtk_menu_item_new_with_label("Renum");
    gtk_menu_shell_append(GTK_MENU_SHELL(prog_menu), mi_run);
    gtk_menu_shell_append(GTK_MENU_SHELL(prog_menu), mi_stop);
    gtk_menu_shell_append(GTK_MENU_SHELL(prog_menu), mi_pause);
    gtk_menu_shell_append(GTK_MENU_SHELL(prog_menu), mi_sep_prog);
    gtk_menu_shell_append(GTK_MENU_SHELL(prog_menu), mi_list);
    gtk_menu_shell_append(GTK_MENU_SHELL(prog_menu), mi_renum);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(prog), prog_menu);
    g_signal_connect(mi_list, "activate", G_CALLBACK(on_menu_list), app);
    g_signal_connect(mi_renum, "activate", G_CALLBACK(on_menu_renum), app);
    g_signal_connect(mi_run,  "activate", G_CALLBACK(on_menu_run),  app);
    g_signal_connect(mi_stop, "activate", G_CALLBACK(on_menu_stop), app);
    g_signal_connect(mi_pause,"activate", G_CALLBACK(on_menu_pause), app);
    // Edit
    GtkWidget *edit_menu = gtk_menu_new();
    GtkWidget *edit = gtk_menu_item_new_with_label("Edit");


GtkWidget *mi_undo = gtk_menu_item_new_with_label("Undo");
GtkWidget *mi_redo = gtk_menu_item_new_with_label("Redo");
GtkWidget *mi_select_all = gtk_menu_item_new_with_label("Select All");
gtk_widget_add_accelerator(mi_undo, "activate", app->accel, GDK_KEY_z, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
// Redo: Ctrl+Y and Ctrl+Shift+Z
gtk_widget_add_accelerator(mi_redo, "activate", app->accel, GDK_KEY_y, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
gtk_widget_add_accelerator(mi_redo, "activate", app->accel, GDK_KEY_z, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
gtk_widget_add_accelerator(mi_select_all, "activate", app->accel, GDK_KEY_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_undo);
gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_redo);
gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());
gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_select_all);
gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());
g_signal_connect(mi_undo, "activate", G_CALLBACK(on_menu_undo), app);
g_signal_connect(mi_redo, "activate", G_CALLBACK(on_menu_redo), app);
g_signal_connect(mi_select_all, "activate", G_CALLBACK(on_menu_select_all), app);

    GtkWidget *mi_cut = gtk_menu_item_new_with_label("Cut");
    GtkWidget *mi_copy = gtk_menu_item_new_with_label("Copy");
    GtkWidget *mi_paste = gtk_menu_item_new_with_label("Paste");
    gtk_widget_add_accelerator(mi_cut, "activate", app->accel, GDK_KEY_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(mi_copy, "activate", app->accel, GDK_KEY_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(mi_paste, "activate", app->accel, GDK_KEY_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_cut);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_copy);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_paste);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());
    g_signal_connect(mi_cut, "activate", G_CALLBACK(on_menu_cut), app);
    g_signal_connect(mi_copy, "activate", G_CALLBACK(on_menu_copy), app);
    g_signal_connect(mi_paste, "activate", G_CALLBACK(on_menu_paste), app);

    GtkWidget *mi_prefs = gtk_menu_item_new_with_label("Preferences...");
gtk_widget_add_accelerator(mi_prefs, "activate", app->accel, GDK_KEY_comma, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_prefs);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit), edit_menu);
    g_signal_connect(mi_prefs, "activate", G_CALLBACK(on_menu_prefs), app);

    // Help
    GtkWidget *help_menu = gtk_menu_new();
    GtkWidget *help = gtk_menu_item_new_with_label("Help");
    GtkWidget *mi_about = gtk_menu_item_new_with_label("About");
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), mi_about);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help), help_menu);
    g_signal_connect(mi_about, "activate", G_CALLBACK(on_menu_about), app);


    // Top-level menu order: File, Edit, Program, Run, Help
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), edit);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), prog);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help);

    return menubar;
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static GtkWidget *make_scrolled_text_view(GtkTextBuffer **out_buf, GtkWidget **out_view, bool editable, bool monospace) {
    GtkWidget *tv = gtk_text_view_new();
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), editable);

    // NOTE:
    // Do *not* force monospace here.
    // If GtkTextView is set to monospace=TRUE, it overrides CSS, which prevents the
    // Preferences dialog font selection from taking effect.
    // We keep the "monospace" parameter only as a hint for defaults; the actual
    // font is controlled by CSS in apply_theme() (and defaults to "Monospace 12").
    (void)monospace;

    if (!editable) {
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_WORD_CHAR);
        g_object_set_data(G_OBJECT(buf), "xbasic_output_view", tv);
    }

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tv);
    if (out_buf) *out_buf = buf;
    if (out_view) *out_view = tv;
    return sw;
}
#endif /* !WBASIC_NO_UI */


#ifndef WBASIC_NO_UI
static GtkWidget *make_scrolled_editor_view(GtkTextBuffer **out_buf, GtkWidget **out_view) {
    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_NONE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tv);

    if (out_buf) *out_buf = buf;
    if (out_view) *out_view = tv;
    return sw;
}
#endif /* !WBASIC_NO_UI */




#ifndef WBASIC_NO_UI
static gboolean on_cmd_key_press(GtkWidget *w, GdkEventKey *e, gpointer user_data);
static gboolean on_win_key_press(GtkWidget *w, GdkEventKey *e, gpointer user_data);

static void update_window_title(App *app)
{
    if (!app || !app->win) return;

    const char *disp = "Untitled";
    gchar *base = NULL;

    if (app->current_path && *app->current_path) {
        base = g_path_get_basename(app->current_path);
        if (base && *base) disp = base;
    }

    gchar *title = g_strdup_printf("%s%s - Wally's Basic", disp, (app->dirty ? "*" : ""));
    gtk_window_set_title(GTK_WINDOW(app->win), title);
    g_free(title);
    if (base) g_free(base);
}
#endif /* !WBASIC_NO_UI */
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void set_current_path(App *app, const char *path_or_null)
{
    if (!app) return;
    if (app->current_path) { free(app->current_path); app->current_path = NULL; }
    if (path_or_null && *path_or_null) app->current_path = xstrdup(path_or_null);
    update_window_title(app);
}
#else
static WB_UNUSED void set_current_path(App *app, const char *path_or_null) { if (!app) return; if (app->current_path) { free(app->current_path); app->current_path = NULL; } if (path_or_null && *path_or_null) app->current_path = xstrdup(path_or_null); }
#endif /* WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void mark_dirty(App *app, bool dirty)
{
    if (!app) return;
    app->dirty = dirty;
    
    if (app->mi_save) gtk_widget_set_sensitive(app->mi_save, app->dirty);
update_window_title(app);
}
#else
static void mark_dirty(App *app, bool dirty) { if (app) app->dirty = dirty; }
#endif /* WBASIC_NO_UI */

#ifndef WBASIC_NO_UI
static void on_editor_buf_changed(GtkTextBuffer *buf, gpointer user_data)
{
    (void)buf;
    App *app = (App*)user_data;
    if (!app || app->suppress_dirty) return;
    if (!app->dirty) mark_dirty(app, true);
    // If we were stopped, editing the program returns to Idle.
    if (app->run_state == RUN_STOPPED) set_run_state(app, RUN_IDLE);
}
#endif /* !WBASIC_NO_UI */

#ifndef WBASIC_NO_UI

#ifndef WBASIC_NO_UI
static gboolean ui_splash_timeout_cb(gpointer user_data);
static gboolean ui_splash_show_idle(gpointer user_data);

static void ui_splash_destroy(App *app) {
    if (!app) return;
    if (app->splash_dlg && GTK_IS_WIDGET(app->splash_dlg)) {
        GtkWidget *w = app->splash_dlg;
        app->splash_dlg = NULL;
        gtk_widget_destroy(w);
    }
    /* After splash dismiss, perform any deferred startup actions exactly once. */
    if (app->deferred_startup_file) {
        char *p = app->deferred_startup_file;
        app->deferred_startup_file = NULL;
        file_load_into_editor(app, p);
        free(p);
    }
    if (app->deferred_autorun) {
        app->deferred_autorun = false;
        do_run(app);
    }
}


static gboolean ui_splash_on_key(GtkWidget *w, GdkEventKey *e, gpointer user_data) {
    (void)w; (void)e;
    App *app = (App*)user_data;
    ui_splash_destroy(app);
    return TRUE;
}
static gboolean ui_splash_on_button(GtkWidget *w, GdkEventButton *e, gpointer user_data) {
    (void)w; (void)e;
    App *app = (App*)user_data;
    ui_splash_destroy(app);
    return TRUE;
}

static gboolean ui_splash_timeout_cb(gpointer user_data) {
    App *app = (App*)user_data;
    ui_splash_destroy(app);
    return G_SOURCE_REMOVE;
}

static gboolean ui_splash_show_idle(gpointer user_data) {
    App *app = (App*)user_data;
    if (!app || !app->win) return G_SOURCE_REMOVE;
    if (!app->show_splash) return G_SOURCE_REMOVE;
    if (app->splash_dlg) return G_SOURCE_REMOVE;

    GtkWidget *dlg = gtk_dialog_new();
    attach_windows_dark_titlebar(dlg);
    gtk_window_set_title(GTK_WINDOW(dlg), "Wally's Basic");
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(app->win));
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_decorated(GTK_WINDOW(dlg), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
    gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER_ON_PARENT);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 18);

    GtkWidget *v = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(content), v, TRUE, TRUE, 0);

    GdkPixbuf *pix = ui_load_wbasic_icon_pixbuf();
    if (pix) {
        GtkWidget *img = gtk_image_new_from_pixbuf(pix);
        gtk_widget_set_halign(img, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(v), img, FALSE, FALSE, 0);
        g_object_unref(pix);
    }

    GtkWidget *lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl), "<span size='xx-large' weight='bold'>Wally&#39;s Basic</span>");
    gtk_widget_set_halign(lbl, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(v), lbl, FALSE, FALSE, 0);

    gtk_widget_add_events(dlg, GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK);
    g_signal_connect(dlg, "key-press-event", G_CALLBACK(ui_splash_on_key), app);
    g_signal_connect(dlg, "button-press-event", G_CALLBACK(ui_splash_on_button), app);

    app->splash_dlg = dlg;
    gtk_widget_show_all(dlg);

    g_timeout_add(2000, ui_splash_timeout_cb, app);
    return G_SOURCE_REMOVE;
}
#endif /* !WBASIC_NO_UI */

static void build_ui(App *app) {
    app->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    attach_windows_dark_titlebar(app->win);
    // Window title is driven by current filename
    update_window_title(app);

    // Keyboard accelerators
    app->accel = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(app->win), app->accel);
    if (app->have_win_size && app->win_w > 0 && app->win_h > 0) {
        gtk_window_set_default_size(GTK_WINDOW(app->win), app->win_w, app->win_h);
    } else {
        gtk_window_set_default_size(GTK_WINDOW(app->win), 980, 720);
    }

    // Restore window position (best-effort). Must be done before the window is shown.
    if (app->have_win_pos) {
        gtk_window_move(GTK_WINDOW(app->win), app->win_x, app->win_y);
    }
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app->win), vbox);

    gtk_box_pack_start(GTK_BOX(vbox), make_menu_bar(app), FALSE, FALSE, 0);

    app->paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    GtkWidget *paned = app->paned;

    /* Experiment: make editor/output divider wider */
    gtk_paned_set_wide_handle(GTK_PANED(paned), TRUE);
    

    /* Make paned handle ~50% thicker */
    {
        GtkCssProvider *prov = gtk_css_provider_new();
        gtk_css_provider_load_from_data(prov,
            "paned > separator { min-width: 12px; min-height: 12px; }",
            -1, NULL);
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(prov),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(prov);
    }
gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);

    GtkWidget *editor_sw = make_scrolled_editor_view(&app->editor_buf, &app->editor_view);
    // Plain GtkTextView editor: provide WBASIC undo/redo via lightweight snapshot stack.
    app->editor_undo = undo_stack_new_for_buffer(app->editor_buf);
    g_signal_connect(app->editor_buf, "changed", G_CALLBACK(on_editor_buffer_changed), app);
    g_signal_connect(app->editor_buf, "changed", G_CALLBACK(on_editor_buf_changed), app);
    gtk_paned_pack1(GTK_PANED(paned), editor_sw, TRUE, FALSE);

    app->output_sw = make_scrolled_text_view(&app->output_buf, &app->output_view, false, true);

    /* Pane "border": add internal padding so text never prints flush to the edge.
       Use the same thickness as the paned separator (~12px). */
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(app->editor_view), 12);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(app->editor_view), 12);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(app->editor_view), 12);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(app->editor_view), 12);

    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(app->output_view), 12);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(app->output_view), 12);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(app->output_view), 12);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(app->output_view), 12);
    /* Initialize optimized output scrollback (GTK).
       We keep transcript scrollback lines above a stable mark, and re-render the current screen below it. */
    app->out_scrollback_lines = 0;
    app->out_scrollback_max_lines = 1000;
    if (app->output_buf) {
        GtkTextIter it0;
        gtk_text_buffer_get_start_iter(app->output_buf, &it0);
        GtkTextMark *m = gtk_text_buffer_get_mark(app->output_buf, "out_screen_start");
        if (!m) {
            m = gtk_text_buffer_create_mark(app->output_buf, "out_screen_start", &it0, TRUE /* left gravity: keep screen boundary */);
        } else {
            gtk_text_buffer_move_mark(app->output_buf, m, &it0);
        }
        app->out_screen_start_mark = m;
    }


    // Names used by CSS theming
    if (app->editor_view) gtk_widget_set_name(app->editor_view, "wbasic_editor");
    if (app->output_view) gtk_widget_set_name(app->output_view, "wbasic_output");

    // Apply saved theme (colors + font) now that views are named
    apply_theme(app);
    app->output_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(app->output_stack), GTK_STACK_TRANSITION_TYPE_NONE);
    gtk_stack_add_named(GTK_STACK(app->output_stack), app->output_sw, "text");

    app->gfx_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(app->gfx_area, TRUE);
    gtk_widget_set_vexpand(app->gfx_area, TRUE);
    gtk_widget_set_name(app->gfx_area, "wbasic_gfx");
    g_signal_connect(app->gfx_area, "draw", G_CALLBACK(on_gfx_area_draw), app);
    gtk_stack_add_named(GTK_STACK(app->output_stack), app->gfx_area, "gfx");

    ui_update_output_mode(app);
    gtk_paned_pack2(GTK_PANED(paned), app->output_stack, TRUE, FALSE);
    if (app->have_paned_pos && app->paned_pos > 0) gtk_paned_set_position(GTK_PANED(paned), app->paned_pos);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Command:"), FALSE, FALSE, 8);
    app->cmd_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), app->cmd_entry, TRUE, TRUE, 8);

    // Status indicator (text + LED) in lower-right corner
    GtkWidget *status_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(status_box, GTK_ALIGN_END);

    app->status_label = gtk_label_new("Idle");
    gtk_label_set_xalign(GTK_LABEL(app->status_label), 1.0); // right-justify text within label
    gtk_widget_set_halign(app->status_label, GTK_ALIGN_END);

    app->status_led = gtk_drawing_area_new();
    gtk_widget_set_size_request(app->status_led, 16, 16);
    gtk_widget_set_halign(app->status_led, GTK_ALIGN_END);
    g_signal_connect(app->status_led, "draw", G_CALLBACK(on_status_led_draw), app);

    app->run_state = RUN_IDLE;

    // Pack label to the left, LED to the far right (corner)
    gtk_box_pack_start(GTK_BOX(status_box), app->status_label, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(status_box), app->status_led, FALSE, FALSE, 0);

    gtk_box_pack_end(GTK_BOX(hbox), status_box, FALSE, FALSE, 4);
    g_signal_connect(app->cmd_entry, "activate", G_CALLBACK(on_cmd_activate), app);
    g_signal_connect(app->cmd_entry, "changed", G_CALLBACK(on_cmd_changed), app);
    g_signal_connect(app->cmd_entry, "key-press-event", G_CALLBACK(on_cmd_key_press), app);
    g_signal_connect(app->cmd_entry, "focus-in-event", G_CALLBACK(on_cmd_focus_in), app);
    g_signal_connect(app->cmd_entry, "focus-out-event", G_CALLBACK(on_cmd_focus_out), app);

    // Start with no BASIC program in memory
    editor_set_text(app->editor_buf, "");

    g_signal_connect(app->win, "delete-event", G_CALLBACK(on_win_delete), app);
    g_signal_connect(app->win, "key-press-event", G_CALLBACK(on_win_key_press), app);
    g_signal_connect(app->win, "size-allocate", G_CALLBACK(on_win_size_allocate), app);
    g_signal_connect(app->win, "configure-event", G_CALLBACK(on_win_configure), app);
    g_signal_connect(app->win, "destroy", G_CALLBACK(on_win_destroy), app);
}
#endif /* !WBASIC_NO_UI */



/* ===================== TRUE Option C export support ===================== */

static void free_key(gpointer data);
static void free_val(gpointer data);

#ifndef WBASIC_NO_UI
static WB_UNUSED char *get_editor_text_dup(App *app) {
    if (!app || !app->editor_buf) return NULL;
    GtkTextIter a, b;
    gtk_text_buffer_get_start_iter(app->editor_buf, &a);
    gtk_text_buffer_get_end_iter(app->editor_buf, &b);
    return gtk_text_buffer_get_text(app->editor_buf, &a, &b, FALSE); /* g_free */
}
#else
static WB_UNUSED char *get_editor_text_dup(App *app) { (void)app; return NULL; }
#endif /* WBASIC_NO_UI */

/* deleted unused static function: c_emit_escaped */

/* Like c_emit_escaped(), but for a non-NUL-terminated slice. */
static void c_emit_escaped_len(FILE *fp, const char *s, size_t n) {
    const unsigned char *p = (const unsigned char*)s;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = p[i];
        if (c == '\\') fputs("\\\\", fp);
        else if (c == '"') fputs("\\\"", fp);
        else if (c == '\n') fputs("\\n", fp);
        else if (c == '\r') {}
        else if (c == '\t') fputs("\\t", fp);
        else fputc(c, fp);
    }
}

static WB_UNUSED int export_standalone_from_text(const char *bas_text, const char *out_exe, int embed_speed_0_100, bool embed_include_speed, char **out_buildlog) {
    if (out_buildlog) *out_buildlog = NULL;
    if (!bas_text || !out_exe) return 1;

    char stub_path[4096];
    snprintf(stub_path, sizeof(stub_path), "%s_export.c", out_exe);

    FILE *fp = fopen(stub_path, "wb");
    if (!fp) return 1;

    /* Exported program runs headless by default (no window). */
    fprintf(fp, "#define WBASIC_EMBEDDED_BUILD 1\n");
    fprintf(fp, "#define WBASIC_NO_UI 1\n");

    if (embed_include_speed) {
        if (embed_speed_0_100 < 0) embed_speed_0_100 = 0;
        if (embed_speed_0_100 > 100) embed_speed_0_100 = 100;
        fprintf(fp, "#define WBASIC_EMBEDDED_OUTPUT_SPEED_0_100 %d\n", embed_speed_0_100);
    }

    fprintf(fp, "#include \"%s\"\n\n", WBASIC_SOURCE_FILE);
    fprintf(fp, "static const char *embedded_bas =\n");

    const char *cur = bas_text;
    while (*cur) {
        const char *line_end = cur;
        while (*line_end && *line_end != '\n') line_end++;
        size_t len = (size_t)(line_end - cur);
        fputs("\"", fp);
        c_emit_escaped_len(fp, cur, len);
        fputs("\\n\"\n", fp);
        cur = (*line_end == '\n') ? (line_end + 1) : line_end;
    }
    fputs(";\n\nint main(int argc, char **argv){ return wbasic_run_embedded(argc, argv, embedded_bas); }\n", fp);
    fclose(fp);

    char log_path[4096];
    snprintf(log_path, sizeof(log_path), "%s_export_build.log", out_exe);

    char cmd[8192];
    int n = snprintf(cmd, sizeof(cmd),
                     "sh -c 'gcc -DWBASIC_EMBEDDED_BUILD -O2 -s \"%s\" -o \"%s\" $(pkg-config --cflags --libs gtk+-3.0) -lm >\"%s\" 2>&1'",
                     stub_path, out_exe, log_path);
    int rc = 0;
    if (n < 0 || (size_t)n >= sizeof(cmd)) {
        do { FILE *lf = fopen(log_path, "ab"); if (lf) { fputs("Export failed: command too long (choose a shorter export path).\n", lf); fclose(lf); } } while(0);
        rc = 1;
    } else {
        rc = system(cmd);
    }
if (out_buildlog) {
        gchar *contents = NULL;
        gsize len = 0;
        if (g_file_get_contents(log_path, &contents, &len, NULL) && contents) {
            *out_buildlog = contents; /* caller g_free */
        }
    }

    return rc == 0 ? 0 : 1;
}

/* Embedded runner entrypoint used by exported standalone programs. */

/* Run BASIC source text in headless CLI mode (no GTK). */
static int wbasic_run_headless_from_text(int argc, char **argv, const char *source_text) {
    App app;
    memset(&app, 0, sizeof(app));

    /* Default speed: Fast (may be overridden by embedded define or --speed). */
    app.output_speed = 1.0;
    app.default_output_speed = app.output_speed;

    /* Phase 3: PRINT-statement throttling tickle (UI pumps only during delays). */
    app.tickle.fn = NULL;
    app.tickle.user = NULL;
    app.embedded_text = source_text ? source_text : "";

#ifdef WBASIC_EMBEDDED_OUTPUT_SPEED_0_100
    /* Embedded default PRINT throttle speed from exporter (0..100). */
    {
        int sp = (int)WBASIC_EMBEDDED_OUTPUT_SPEED_0_100;
        if (sp < 0) sp = 0;
        if (sp > 100) sp = 100;
        app.output_speed = ((double)sp) / 100.0;
    }
#endif

    /* CLI options:
     -h, --help                  Show help
     -s N, --speed N, --speed=N  Override embedded PRINT throttle speed (0..100)
*/
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
        }

        if (!strcmp(argv[i], "--speed") || !strcmp(argv[i], "-S") || !strcmp(argv[i], "-s")) {
            if (i + 1 < argc) {
                int sp = atoi(argv[i + 1]);
                if (sp < 0) sp = 0;
                if (sp > 100) sp = 100;
                app.output_speed = ((double)sp) / 100.0;
                i++;
                continue;
            } else {
                fprintf(stderr, "Missing value for %s\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        }

        if (!strncmp(argv[i], "--speed=", 8)) {
            int sp = atoi(argv[i] + 8);
            if (sp < 0) sp = 0;
            if (sp > 100) sp = 100;
            app.output_speed = ((double)sp) / 100.0;
            continue;
        }

        if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            fprintf(stderr, "Unexpected argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    app.default_output_speed = app.output_speed;

    app.headless_tty_fd = -1;
    app.headless_tty_inited = false;
    app.headless_tty_using_stdin = false;
    app.headless_cursor_dirty = false;

    app.key_trap_enabled = true;
    app.on_key_pending = -1;
    app.on_key_in_progress = false;
    app.export_include_speed = false;
    app.show_splash = true;
    app.splash_dlg = NULL;

    app.out_row = 1;
    app.out_col = 1;
    app.screen_rows = 25;
    app.screen_cols = 80;
    app.screen = NULL;
    app.screen_fg = NULL;
    app.screen_bg = NULL;
    app.video_mode = WB_VIDEO_TEXT;
    app.gfx_width = 0;
    app.gfx_height = 0;
    app.gfx_pixels = NULL;
    app.cur_fg = 16;
    app.cur_bg = 16;
    app.option_base = 0;
    app.option_base_locked = false;
    err_clear(&app);

    gettimeofday(&app.start_tv, NULL);
    srand((unsigned)time(NULL));
    app.have_last_rnd = false;

    program_init(&app.prog);
    app.vars = g_hash_table_new_full(g_str_hash, g_str_equal, free_key, free_val);

    /* Run the embedded program once and exit. */
    do_exec_from(&app, 0, 0, true, true);

    headless_tty_shutdown(&app);

    runtime_reset(&app);
    program_free(&app.prog);
    g_hash_table_destroy(app.vars);
    free(app.screen);
    free(app.screen_fg);
    free(app.screen_bg);
    gfx_free(&app);
    return 0;
}

int wbasic_run_embedded(int argc, char **argv, const char *source_text) {
#ifdef WBASIC_NO_UI
    return wbasic_run_headless_from_text(argc, argv, source_text);
#else
    /* Use caller-provided argc/argv to keep GTK/GLib argument parsing happy. */
    if (argc <= 0 || argv == NULL || argv[0] == NULL) {
        static char argv0[] = "program_export";
        static char *fallback_argv[] = { argv0, NULL };
        int fallback_argc = 1;
        char **argvp = fallback_argv;
        gtk_init(&fallback_argc, &argvp);
    } else {
        char **argvp = argv;
        gtk_init(&argc, &argvp);
    }
    apply_windows_dark_mode_preference();

    App app;
    memset(&app, 0, sizeof(app));
    app.key_trap_enabled = true;
    app.on_key_pending = -1;
    app.on_key_in_progress = false;
    app.current_path = NULL;
    app.dirty = false;
    app.suppress_dirty = false;
    app.show_splash = true;
    app.splash_dlg = NULL;
    app.deferred_startup_file = NULL;
    app.deferred_autorun = false;
    app.show_splash = true;
    app.splash_dlg = NULL;
    app.deferred_startup_file = NULL;
    app.deferred_autorun = false;

    app.font_css = gtk_css_provider_new();
    app.output_speed = 1.0;
    app.default_output_speed = app.output_speed;
    app.export_include_speed = false;
    app.out_row = 1;
    app.out_col = 1;
    app.screen_rows = 25;
    app.screen_cols = 80;
    app.screen = NULL;
    app.video_mode = WB_VIDEO_TEXT;
    app.gfx_width = 0;
    app.gfx_height = 0;
    app.gfx_pixels = NULL;
    app.option_base = 0;
    app.option_base_locked = false;
    app.font_name = xstrdup("Monospace 12");
    err_clear(&app);

    gettimeofday(&app.start_tv, NULL);
    srand((unsigned)time(NULL));
    app.have_last_rnd = false;
    prefs_load(&app);
    recent_load(&app);
    program_init(&app.prog);
    app.vars = g_hash_table_new_full(g_str_hash, g_str_equal, free_key, free_val);

    build_ui(&app);
    gtk_widget_show_all(app.win);

    if (app.show_splash) {
        g_idle_add(ui_splash_show_idle, &app);
    }

    app.suppress_dirty = true;
    editor_set_text(app.editor_buf, source_text ? source_text : "");
    app.suppress_dirty = false;

    if (app.show_splash) {
        app.deferred_autorun = true;
    } else {
        do_run(&app);
    }
 gtk_main();

    runtime_reset(&app);
    program_free(&app.prog);
    g_hash_table_destroy(app.vars);
    if (app.css_provider) g_object_unref(app.css_provider);
    if (app.scrollback_lines) {
        scrollback_clear(&app);
        free(app.scrollback_lines);
        app.scrollback_lines = NULL;
    }
    if (app.screen) free(app.screen);
    if (app.screen_fg) free(app.screen_fg);
    if (app.screen_bg) free(app.screen_bg);
    gfx_free(&app);
    if (app.accel) g_object_unref(app.accel);
    free(app.font_name);
    return 0;
#endif
}

#ifndef WBASIC_NO_UI
/* deleted unused static function: truncate_for_dialog */

static void on_menu_export_standalone(GtkMenuItem *mi, gpointer user_data) {
    (void)mi;
    App *app = (App*)user_data;
    if (!app) return;

    GtkWidget *dlg = gtk_file_chooser_dialog_new(
        "Export Standalone Executable",
        GTK_WINDOW(app->win),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Export", GTK_RESPONSE_ACCEPT,
    NULL
    );
    attach_windows_dark_titlebar(dlg);
    GtkFileChooser *fc = GTK_FILE_CHOOSER(dlg);
    gtk_file_chooser_set_do_overwrite_confirmation(fc, TRUE);
    const char *default_name = "program";
    gchar *base = NULL;
    if (app->current_path && *app->current_path) {
        base = g_path_get_basename(app->current_path);
        if (base && *base) {
            char *dot = strrchr(base, '.');
            if (dot && g_ascii_strcasecmp(dot, ".bas") == 0) *dot = '\0';
            if (*base) default_name = base;
        }
    }
    gtk_file_chooser_set_current_name(fc, default_name);
    if (base) g_free(base);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(dlg);
        return;
    }
    char *out_path = gtk_file_chooser_get_filename(fc);
    gtk_widget_destroy(dlg);
    if (!out_path) return;

    char *bas = get_editor_text_dup(app);
    if (!bas) { g_free(out_path); return; }

    char *buildlog = NULL;
    int rc = export_standalone_from_text(bas, out_path, (int)llround(app->default_output_speed * 100.0), app->export_include_speed, &buildlog);

    GtkWidget *msg;
    if (rc == 0) {
        msg = gtk_message_dialog_new(GTK_WINDOW(app->win), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                     "Export complete.\n\nOutput: %s", out_path);
    } else {
        /* The exporter writes a build log next to the output executable. */
        char log_path[4096];
        snprintf(log_path, sizeof(log_path), "%s_export_build.log", out_path);
        const char *log_disp = log_path;
        const char *slash = strrchr(log_disp, '/');
        if (slash && slash[1]) log_disp = slash + 1;

        msg = gtk_message_dialog_new(GTK_WINDOW(app->win), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                     "Export failed.\n\nSee build log: %s", log_disp);
        /* Keep the dialog clean; full details are in the build log file. */
    }
    attach_windows_dark_titlebar(msg);

    /* Center the text block relative to the icon. */
    GtkWidget *msg_area = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(msg));
    if (msg_area) {
        gtk_widget_set_valign(msg_area, GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(msg_area, 6);
        gtk_widget_set_margin_bottom(msg_area, 6);
    }

/* Brand the export result dialog with the embedded WBASIC icon (no external files). */
    GdkPixbuf *pix = ui_load_wbasic_icon_pixbuf();
    if (pix) {
        GtkWidget *imgw = gtk_image_new_from_pixbuf(pix);
        gtk_widget_show(imgw);
        /* Avoid deprecated gtk_message_dialog_get_image/gtk_message_dialog_set_image by using the "image" property. */
        g_object_set(G_OBJECT(msg), "image", imgw, NULL);
        gtk_window_set_icon(GTK_WINDOW(msg), pix);
    }

    gtk_window_set_resizable(GTK_WINDOW(msg), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(msg), 520, -1);
    gtk_dialog_run(GTK_DIALOG(msg));
    if (pix) g_object_unref(pix);
    gtk_widget_destroy(msg);

    if (buildlog) g_free(buildlog);
    g_free(bas);
    g_free(out_path);
}
#endif /* !WBASIC_NO_UI */


/* ===================== main ===================== */

static void free_key(gpointer data) { free(data); }
static void free_val(gpointer data) { var_free((Var*)data); }

#ifndef WBASIC_NO_UI
int wbasic_main(int argc, char **argv) {
    /* TRUE Option C CLI: --export <input.bas> <output_exe> */
    if (argc >= 4 && (!strcmp(argv[1], "--export") || !strcmp(argv[1], "-E"))) {
        const char *in_bas = argv[2];
        const char *out_exe = argv[3];
        gchar *contents = NULL;
        gsize len = 0;
        if (!g_file_get_contents(in_bas, &contents, &len, NULL) || !contents) {
            fprintf(stderr, "Failed to read %s\n", in_bas);
            return 1;
        }
        char *buildlog = NULL;
        int rc = export_standalone_from_text(contents, out_exe, 100, false, &buildlog);
        if (rc != 0) {
            fprintf(stderr, "Export failed.\n");
            if (buildlog) { fprintf(stderr, "%s\n", buildlog); g_free(buildlog); }
            g_free(contents);
            return 1;
        }
        if (buildlog) g_free(buildlog);
        g_free(contents);
        printf("Exported standalone executable: %s\n", out_exe);
        return 0;
    }

    
    /* Unified binary experiment: allow --cli/--headless to run in terminal mode. */
    bool want_cli = false;
#ifndef _WIN32
    bool force_gtk = false;
#endif
    for (int i = 1; i < argc; i++) {
        if (!argv[i]) continue;
        if (!strcmp(argv[i], "--cli") || !strcmp(argv[i], "--headless") || !strcmp(argv[i], "-c") || !strcmp(argv[i], "-C")) want_cli = true;
#ifndef _WIN32
        if (!strcmp(argv[i], "--gtk")) force_gtk = true;
#endif
    }

#ifndef _WIN32
    /* Auto-fallback: if no GUI display is available, default to CLI unless --gtk forced. */
    if (!want_cli && !force_gtk) {
        const char *disp = getenv("DISPLAY");
        const char *wdisp = getenv("WAYLAND_DISPLAY");
        if ((!disp || !*disp) && (!wdisp || !*wdisp)) want_cli = true;
    }
#endif

    if (want_cli) {
        /* CLI mode: wbasic --cli <file.bas> [-s N] */
        if (argc < 2) {
            fprintf(stderr,
                    "Usage: %s [--cli|--headless] <file.bas> [-s N]\n"
                    "  -s N, --speed N, --speed=N   Set PRINT throttle speed (0=slowest, 100=fastest).\n"
                    "  -h, --help                   Show help.\n",
                    (argc > 0 && argv[0]) ? argv[0] : "wbasic");
            return 1;
        }

        const char *in_bas = NULL;
        for (int i = 1; i < argc; i++) {
            const char *a = argv[i];
            if (!a) continue;
            if (a[0] == '-') {
                if (!strcmp(a, "-s") || !strcmp(a, "--speed")) { i++; continue; }
                continue;
            }
            in_bas = a;
            break;
        }

        if (!in_bas) {
            fprintf(stderr, "No input .bas file provided.\n");
            return 1;
        }

        gchar *contents = NULL;
        gsize len = 0;
        if (!g_file_get_contents(in_bas, &contents, &len, NULL) || !contents) {
            fprintf(stderr, "Failed to read: %s\n", in_bas);
            return 1;
        }

        /* Strip the input filename and --cli/--headless/--gtk from argv before calling runner. */
        char *argv2[256];
        int argc2 = 0;
        argv2[argc2++] = argv[0];
        for (int i = 1; i < argc && argc2 < 255; i++) {
            const char *a = argv[i];
            if (!a) continue;
            if (in_bas && !strcmp(a, in_bas)) continue;
            if (!strcmp(a, "--cli") || !strcmp(a, "--headless") || !strcmp(a, "-c") || !strcmp(a, "-C")) continue;
            if (!strcmp(a, "--gtk")) continue;
            argv2[argc2++] = argv[i];
        }
        argv2[argc2] = NULL;

        int rc = wbasic_run_headless_from_text(argc2, argv2, contents);
        g_free(contents);
        return rc;
    }

gtk_init(&argc, &argv);
    apply_windows_dark_mode_preference();

    const char *startup_file = NULL;
    int autorun = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
        }
        if (!strcmp(argv[i], "-r")) {
            autorun = 1;
            continue;
        }
        if (argv[i][0] != '-' && !startup_file) {
            startup_file = argv[i];
        }
    }
    if (argc >= 2) {
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
            print_usage(argv[0]);
            return 0;
        }
        /* First non-option argument is treated as a BASIC program to load. */
        if (argv[1][0] != '-') startup_file = argv[1];
    }

    App app;
    memset(&app, 0, sizeof(app));
    // GW-BASIC KEY macro defaults
    app.key_trap_enabled = true;
    app.on_key_pending = -1;
    app.on_key_in_progress = false;
    app.current_path = NULL;
    app.dirty = false;
    app.suppress_dirty = false;
    app.show_splash = true;
    app.splash_dlg = NULL;
    app.deferred_startup_file = NULL;
    app.deferred_autorun = false;

    app.font_css = gtk_css_provider_new();
    // Defaults (may be overridden by prefs_load)
    app.output_speed = 1.0; // Fast
    app.default_output_speed = app.output_speed;
    app.export_include_speed = false;
    // Output cursor starts at 1,1 (top-left) for LOCATE.
    app.out_row = 1;
    app.out_col = 1;
app.screen_rows = 25;
app.screen_cols = 80;
app.screen = NULL;
    app.video_mode = WB_VIDEO_TEXT;
    app.gfx_width = 0;
    app.gfx_height = 0;
    app.gfx_pixels = NULL;
    app.option_base = 0;  // OPTION BASE default
    app.option_base_locked = false;
    // Default font (can be overridden by prefs_load and Preferences dialog)
    // Keep this in sync with apply_theme() comments.
    app.font_name = xstrdup("Monospace 12");
    err_clear(&app);

    
    gettimeofday(&app.start_tv, NULL);
    srand((unsigned)time(NULL));
    app.have_last_rnd = false;
    prefs_load(&app);
    recent_load(&app);
    program_init(&app.prog);

    app.vars = g_hash_table_new_full(g_str_hash, g_str_equal, free_key, free_val);

    build_ui(&app);
    gtk_widget_show_all(app.win);

    /* Splash: show after GTK main loop starts. If enabled, defer startup load/autorun until splash dismiss. */
    if (app.show_splash) {
        if (startup_file) app.deferred_startup_file = xstrdup(startup_file);
        app.deferred_autorun = autorun;
        g_idle_add(ui_splash_show_idle, &app);
    } else {
        if (startup_file) {
            file_load_into_editor(&app, startup_file);
            if (autorun) do_run(&app);
        }
    }
    gtk_main();

    // cleanup
    runtime_reset(&app);
    program_free(&app.prog);
    g_hash_table_destroy(app.vars);
    if (app.css_provider) g_object_unref(app.css_provider);
    if (app.scrollback_lines) {
        scrollback_clear(&app);
        free(app.scrollback_lines);
        app.scrollback_lines = NULL;
    }
    if (app.screen) free(app.screen);
    if (app.screen_fg) free(app.screen_fg);
    if (app.screen_bg) free(app.screen_bg);
    gfx_free(&app);
    if (app.accel) g_object_unref(app.accel);
    free(app.font_name);
    return 0;
}
#else
int wbasic_main(int argc, char **argv) {
    // Headless CLI: wbasic_cli <file.bas> [-s N]
    if (argc < 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        fprintf(stderr,
                "Usage: %s <file.bas> [-s N]\n"
                "  -s N, --speed N, --speed=N   Set PRINT throttle speed (0=slowest, 100=fastest).\n"
                "  -h, --help                   Show this help and exit.\n",
                (argc > 0 && argv[0]) ? argv[0] : "wbasic_cli");
        return 1;
    }

    const char *in_bas = NULL;
    // Find first non-option argument as the input BASIC file.
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!a) continue;
        if (a[0] == '-') {
            // Skip option argument for -s N or --speed N
            if (!strcmp(a, "-s") || !strcmp(a, "--speed")) { i++; continue; }
            continue;
        }
        in_bas = a;
        break;
    }

    if (!in_bas) {
        fprintf(stderr, "No input .bas file provided.\n");
        return 1;
    }

    gchar *contents = NULL;
    gsize len = 0;
    if (!g_file_get_contents(in_bas, &contents, &len, NULL) || !contents) {
        fprintf(stderr, "Failed to read: %s\n", in_bas);
        return 1;
    }

    // wbasic_run_embedded parses runtime flags like -s/--speed. It does NOT expect the input filename
    // to appear in argv, so strip it out before calling.
    char *argv2[256];
    int argc2 = 0;
    argv2[argc2++] = argv[0];
    for (int i = 1; i < argc && argc2 < 255; i++) {
        const char *a = argv[i];
        if (!a) continue;
        if (in_bas && !strcmp(a, in_bas)) continue; // drop input file
        argv2[argc2++] = argv[i];
    }
    argv2[argc2] = NULL;

    int rc = wbasic_run_embedded(argc2, argv2, contents);
    g_free(contents);
    return rc;
}
#endif /* WBASIC_NO_UI */
#ifndef WBASIC_EMBEDDED_BUILD
int main(int argc, char **argv) {
    return wbasic_main(argc, argv);
}
#endif



/* ===================== GW-BASIC KEY (F1..F10) support ===================== */

static gboolean on_key_macro_idle(gpointer user_data) {
    App *app = (App *)user_data;

    /* This idle handler must run exactly once per scheduled macro. */
    app->key_macro_idle_scheduled = FALSE;
    app->key_macro_idle_id = 0;

    if (!app->key_macro_pending || app->key_macro_pending[0] == '\0') {
        if (app->key_macro_pending) { g_free(app->key_macro_pending); app->key_macro_pending = NULL; }
        return G_SOURCE_REMOVE;
    }

    char *macro = app->key_macro_pending;
    app->key_macro_pending = NULL;

    /* Execute macro as immediate statements (safe outside key event handler). */
    do_immediate(app, macro);

    g_free(macro);
    return G_SOURCE_REMOVE;
}



static WB_UNUSED void queue_key_macro(App *app, const char *macro) {
    if (!macro || !*macro) return;

    /* Replace any pending macro with the latest. */
    if (app->key_macro_pending) {
        g_free(app->key_macro_pending);
        app->key_macro_pending = NULL;
    }
    app->key_macro_pending = g_strdup(macro);

    if (!app->key_macro_idle_scheduled) {
        app->key_macro_idle_scheduled = TRUE;
        app->key_macro_idle_id = g_idle_add(on_key_macro_idle, app);
    }
}


static void key_macro_queue_clear(App *app) {
    if (!app) return;
    if (app->key_macro_idle_id) {
        g_source_remove(app->key_macro_idle_id);
        app->key_macro_idle_id = 0;
    }
    app->key_macro_idle_scheduled = FALSE;
    if (app->key_macro_pending) {
        g_free(app->key_macro_pending);
        app->key_macro_pending = NULL;
    }
}




/* ========================================================================== */
#ifndef WBASIC_NO_UI
static gboolean on_win_key_press(GtkWidget *w, GdkEventKey *e, gpointer user_data) {
    (void)w;
    App *app = (App*)user_data;
    if (!app || !e) return FALSE;

    /* F6 Pause/Resume hotkey: handle before BASIC KEY macro / INKEY$ swallowing */
    if (e->keyval == GDK_KEY_F6 && ((e->state & (GDK_CONTROL_MASK|GDK_MOD1_MASK|GDK_SHIFT_MASK|GDK_SUPER_MASK)) == 0)) {
        do_pause_toggle(app);
        return TRUE;
    }

// GW-BASIC KEY trap (F1..F10): when enabled, function keys execute stored macros.
// We queue macro execution to run safely outside the key event handler.
if (app->key_trap_enabled && !(app->run_state == RUN_WAITING && app->input_waiting)) {
    guint st = e->state;
    if ((st & (GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_SUPER_MASK)) == 0) {
        if (e->keyval >= GDK_KEY_F1 && e->keyval <= GDK_KEY_F10) {
            int k = (int)(e->keyval - GDK_KEY_F1); // 0..9
            // ON KEY(n) GOSUB trap has priority over KEY macro
            if (k >= 0 && k < 10 && app->on_key_enabled[k] && app->on_key_gosub_line[k] > 0) {
                app->on_key_pending = k;
                return TRUE; // swallow
            }
            // ON KEY(n) GOSUB trap has priority over KEY macro
            if (k >= 0 && k < 10 && app->on_key_enabled[k] && app->on_key_gosub_line[k] > 0) {
                app->on_key_pending = k;
                return TRUE; // swallow
            }
            if (k >= 0 && k < 10 && app->key_macros[k]) {
                if (app->run_state == RUN_RUNNING || app->run_state == RUN_WAITING) {
                if (app->runtime_key_macro) { g_free(app->runtime_key_macro); app->runtime_key_macro = NULL; }
                app->runtime_key_macro = g_strdup(app->key_macros[k]);
            } else {
                queue_key_macro(app, app->key_macros[k]);
            }
                return TRUE; // swallow
            }
        }
    }
}

    
    /* If a program is waiting on runtime INPUT, allow normal typing into the command entry.
       We still honor ESC to stop the program, and F6 is handled above. */
    if (app->run_state == RUN_WAITING && app->input_waiting) {
        if (e->keyval == GDK_KEY_Escape) {
            do_stop(app);
            return TRUE;
        }
        return FALSE; /* let the GtkEntry receive the keystroke */
    }

// While a program is running or waiting for INKEY$, capture keys at the window level
    // and swallow them so they never reach the editor.
    if (app->run_state == RUN_RUNNING || app->run_state == RUN_WAITING) {
        // ESC should always stop a running program. Because we swallow keypresses
        // at the window level for INKEY$, GTK accelerators won't see Escape.
        if (e->keyval == GDK_KEY_Escape) {
            do_stop(app);
            return TRUE;
        }

        // GW-BASIC KEY trap (F1..F10) - queue macro execution and swallow event
if (app->key_trap_enabled) {
    if (e->keyval >= GDK_KEY_F1 && e->keyval <= GDK_KEY_F10) {
        int k = (int)(e->keyval - GDK_KEY_F1); // 0..9
        if (k >= 0 && k < 10 && app->key_macros[k]) {
            if (app->run_state == RUN_RUNNING || app->run_state == RUN_WAITING) {
                if (app->runtime_key_macro) { g_free(app->runtime_key_macro); app->runtime_key_macro = NULL; }
                app->runtime_key_macro = g_strdup(app->key_macros[k]);
            } else {
                queue_key_macro(app, app->key_macros[k]);
            }
            return TRUE; // swallow
        }
    }
}

guint state = e->state;
        if (state & (GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_SUPER_MASK)) {
            return FALSE; // allow shortcuts
        }

        gunichar uc = gdk_keyval_to_unicode(e->keyval);
        if (uc >= 0x20 && uc <= 0x7E) {
            app->inkey_char = (char)uc;
            app->inkey_ready = TRUE;
            return TRUE; // swallow event
        }
        return TRUE;
    }
    return FALSE;
}
#endif /* !WBASIC_NO_UI */
