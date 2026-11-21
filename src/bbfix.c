// BBLIT2 Fix — Pernalonga e Taz: O Furacão do Tempo (bblit2.exe PT-BR)
// Proxy dinput.dll: o jogo carrega este arquivo da pasta bin no lugar do dinput.dll do sistema.
// Nada no exe é alterado no disco; as correções são aplicadas em memória ao iniciar.
//  - Vídeo: tela cheia na resolução do monitor sem trocar o modo de vídeo (e sem exigir 16 bits), widescreen Hor+,
//    menus 2D ampliados em 4:3, MSAA + supersampling via FBO, mipmaps/anisotrópico, distância de visão maior.
//  - Controle: XInput (Xbox e compatíveis) somado ao teclado DirectInput do jogo, câmera no analógico direito.
//  - Idioma pelas falas do CD (PT-BR = 05), jogo em segundo plano sem se suspender.
// Licença MIT.
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>
#include "hud_math.h"
#include "pad_cursor.h"
#include "pad_icons.h"
#include "menu_center.h"

#define FIX_VERSION "0.12.1"

// ---------------------------------------------------------------- config / log
static struct {
    int enabled, log_gl, aspect, msaa, vsync, xinput, unlock_joy, a_enter, deadzone, cam_stick, swap_ab, language, game_res, background, debug, inv_stick, inv_trig, draw_dist, ssaa, mipmaps, aniso, actor_dist, auto_quality, lod_dist, force_gl, dump_tex, hd_tex, pad_prompts, menu_fill, hires_menu, menu_pad, menu_hd, hud_probe, center_menus;
} cfg = { 1, 1, 2, 4, 1, 1, 0, 1, 30, 1, 0, -1, 1, 1, 0, 1, 1, 400, 2, 1, 16, 200, 1, 400, 1, 0, 1, 2, 1, 1, 1, 1, 0, 1 };
// pós-processamento [postfx]: valores em %
static struct { int on, bloom, threshold, sharpen, saturation, contrast, vignette; } fx = { 1, 25, 70, 40, 110, 105, 20 };

static char g_dir[MAX_PATH];
static FILE *g_log;
static CRITICAL_SECTION g_lock;

static void logf_(const char *fmt, ...) {
    if (!g_log) return;
    EnterCriticalSection(&g_lock);
    SYSTEMTIME t; GetLocalTime(&t);
    fprintf(g_log, "%02d:%02d:%02d.%03d ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    va_list ap; va_start(ap, fmt); vfprintf(g_log, fmt, ap); va_end(ap);
    fputc('\n', g_log); fflush(g_log);
    LeaveCriticalSection(&g_lock);
}

static int ini_int(const char *sec, const char *key, int def) {
    char path[MAX_PATH]; snprintf(path, sizeof path, "%s\\bblit2fix.ini", g_dir);
    return GetPrivateProfileIntA(sec, key, def, path);
}

static void load_config(void) {
    cfg.enabled   = ini_int("fix", "enabled", 1);
    cfg.log_gl    = ini_int("fix", "log_gl", 1);
    cfg.aspect    = ini_int("video", "aspect", 2);
    cfg.msaa      = ini_int("video", "msaa", 4);
    cfg.vsync     = ini_int("video", "vsync", 1);
    cfg.xinput    = ini_int("pad", "xinput", 1);
    cfg.unlock_joy= ini_int("pad", "unlock_dinput_joystick", 0);
    cfg.a_enter   = ini_int("pad", "a_is_enter", 1);
    cfg.deadzone  = ini_int("pad", "deadzone", 30);
    cfg.cam_stick = ini_int("pad", "right_stick_camera", 1);
    cfg.swap_ab   = ini_int("pad", "swap_ab", 0);
    cfg.menu_pad  = ini_int("pad", "menu_navigation", 1);
    cfg.language  = ini_int("game", "language", -1);
    cfg.game_res  = ini_int("video", "native_res", 1);
    cfg.background = ini_int("video", "background", 1);
    cfg.debug     = ini_int("fix", "debug", 0);
    cfg.hud_probe = ini_int("debug", "hud_probe", 0);
    cfg.center_menus = ini_int("game", "center_menus", 1);
    cfg.inv_stick = ini_int("pad", "invert_camera_stick", 1);
    cfg.inv_trig  = ini_int("pad", "invert_camera_triggers", 1);
    cfg.draw_dist = ini_int("video", "draw_distance", 400);
    cfg.ssaa      = ini_int("video", "supersampling", 2);
    if (cfg.ssaa < 1) cfg.ssaa = 1;
    if (cfg.ssaa > 4) cfg.ssaa = 4;
    cfg.mipmaps   = ini_int("video", "mipmaps", 1);
    cfg.aniso     = ini_int("video", "anisotropy", 16);
    cfg.actor_dist = ini_int("video", "object_distance", 200);
    if (cfg.actor_dist < 100) cfg.actor_dist = 100;
    if (cfg.actor_dist > 400) cfg.actor_dist = 400;
    cfg.auto_quality = ini_int("video", "auto_quality", 1);
    cfg.lod_dist  = ini_int("video", "lod_distance", 400);
    if (cfg.lod_dist < 100) cfg.lod_dist = 100;
    if (cfg.lod_dist > 1000) cfg.lod_dist = 1000;
    cfg.force_gl  = ini_int("video", "force_opengl", 1);
    cfg.dump_tex  = ini_int("fix", "dump_textures", 0);
    cfg.hd_tex    = ini_int("video", "hd_textures", 1);
    cfg.menu_fill = ini_int("video", "menu_fill", 3);
    cfg.menu_hd = ini_int("video", "menu_hd", 1);
    cfg.hires_menu = ini_int("video", "hires_menu", 1);
    cfg.pad_prompts = ini_int("pad", "button_prompts", 2);
    fx.on         = ini_int("postfx", "enabled", 1);
    fx.bloom      = ini_int("postfx", "bloom", 25);
    fx.threshold  = ini_int("postfx", "bloom_threshold", 70);
    fx.sharpen    = ini_int("postfx", "sharpen", 40);
    fx.saturation = ini_int("postfx", "saturation", 110);
    fx.contrast   = ini_int("postfx", "contrast", 105);
    fx.vignette   = ini_int("postfx", "vignette", 20);
    #define CLAMP_(v, a, b) do { if ((v) < (a)) (v) = (a); if ((v) > (b)) (v) = (b); } while (0)
    CLAMP_(fx.bloom, 0, 100); CLAMP_(fx.threshold, 0, 100); CLAMP_(fx.sharpen, 0, 100);
    CLAMP_(fx.saturation, 0, 200); CLAMP_(fx.contrast, 50, 150); CLAMP_(fx.vignette, 0, 100);
    if (cfg.draw_dist < 50) cfg.draw_dist = 50;
    if (cfg.draw_dist > 400) cfg.draw_dist = 400;
}

// ---------------------------------------------------------------- host check / memory patch
static int host_ok;
static int patch_bytes(DWORD va, const char *expect, int n, const char *repl) {
    BYTE *p = (BYTE *)va;
    if (memcmp(p, expect, n) != 0) { logf_("patch %08lx: bytes inesperados, ignorado", va); return 0; }
    DWORD old; VirtualProtect(p, n, PAGE_EXECUTE_READWRITE, &old);
    memcpy(p, repl, n);
    VirtualProtect(p, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, n);
    logf_("patch %08lx aplicado", va);
    return 1;
}

static DWORD crc32_buf(DWORD crc, const BYTE *p, size_t n) {
    static DWORD tab[256]; static int init;
    if (!init) { for (DWORD i = 0; i < 256; i++) { DWORD c = i; for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1; tab[i] = c; } init = 1; }
    crc = ~crc;
    while (n--) crc = tab[(crc ^ *p++) & 0xff] ^ (crc >> 8);
    return ~crc;
}
// Só a versão brasileira (bblit2.exe 1.0, CRC32 0x647dd079) foi mapeada. Outras versões (ex.: a americana 1.0.3.0)
// têm o mesmo tamanho e o mesmo código nos primeiros endereços, mas os dados ficam em outro lugar: com qualquer
// outro arquivo o mod fica desligado (o proxy do DirectInput continua funcionando).
#define HOST_CRC 0x647dd079u
static int check_host(void) {
    char exe[MAX_PATH]; GetModuleFileNameA(NULL, exe, MAX_PATH);
    HANDLE h = CreateFileA(exe, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    DWORD size = 0, crc = 0;
    if (h != INVALID_HANDLE_VALUE) {
        size = GetFileSize(h, NULL);
        if (size == 880640) {
            BYTE *buf = HeapAlloc(GetProcessHeap(), 0, size); DWORD got = 0;
            if (buf && ReadFile(h, buf, size, &got, NULL) && got == size) crc = crc32_buf(0, buf, size);
            if (buf) HeapFree(GetProcessHeap(), 0, buf);
        }
        CloseHandle(h);
    }
    int ok = size == 880640 && crc == HOST_CRC;
    logf_("exe: %s (%lu bytes, crc32 %08lx) -> %s", exe, size, crc, ok ? "bblit2.exe PT-BR reconhecido" : "versao NAO suportada: mod desligado (so o proxy dinput fica ativo)");
    return ok;
}

// ---------------------------------------------------------------- IAT hooks
static void *hook_iat(HMODULE mod, const char *dll, const char *fn, void *repl) {
    BYTE *base = (BYTE *)mod;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_DATA_DIRECTORY dd = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dd.VirtualAddress) return NULL;
    for (IMAGE_IMPORT_DESCRIPTOR *d = (void *)(base + dd.VirtualAddress); d->Name; d++) {
        if (_stricmp((char *)(base + d->Name), dll) != 0) continue;
        IMAGE_THUNK_DATA *oft = (void *)(base + (d->OriginalFirstThunk ? d->OriginalFirstThunk : d->FirstThunk));
        IMAGE_THUNK_DATA *ft = (void *)(base + d->FirstThunk);
        for (; oft->u1.AddressOfData; oft++, ft++) {
            if (IMAGE_SNAP_BY_ORDINAL(oft->u1.Ordinal)) continue;
            IMAGE_IMPORT_BY_NAME *ibn = (void *)(base + oft->u1.AddressOfData);
            if (strcmp((char *)ibn->Name, fn) != 0) continue;
            void *orig = (void *)ft->u1.Function;
            DWORD old; VirtualProtect(&ft->u1.Function, 4, PAGE_READWRITE, &old);
            ft->u1.Function = (DWORD)repl;
            VirtualProtect(&ft->u1.Function, 4, old, &old);
            return orig;
        }
    }
    return NULL;
}

// ---------------------------------------------------------------- window / aspect state
// O jogo acha que a tela tem g_gw x g_gh (640x480 nos menus, a resolução das opções nas fases).
// Na verdade a janela cobre o monitor inteiro na resolução nativa e tudo é ampliado para o retângulo
// g_vx,g_vy,g_vw,g_vh (4:3 centralizado, ou a tela toda com aspect=0). O modo de vídeo nunca é trocado.
static HWND g_hwnd;
static int g_gw = 640, g_gh = 480;  // tamanho que o jogo pensa ter
static int g_winw, g_winh;          // tamanho real da área de desenho
static int g_vx, g_vy, g_vw, g_vh;  // retângulo da imagem dentro dela
static RECT g_mon;                  // monitor da janela
static int g_nat_w, g_nat_h;        // resolução 4:3 nativa

static void monitor_rect(void) {
    HMONITOR m = MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi; mi.cbSize = sizeof mi;
    if (m && GetMonitorInfoA(m, &mi)) g_mon = mi.rcMonitor;
    else SetRect(&g_mon, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
}

static int g_gl_on;      // há contexto OpenGL ativo na janela: o GL é quem mostra a imagem
static void update_rect(void) {
    if (!g_hwnd) return;
    RECT r; if (!GetClientRect(g_hwnd, &r)) return;
    int w = r.right - r.left, h = r.bottom - r.top;
    if (w <= 0 || h <= 0 || g_gw <= 0 || g_gh <= 0) return;
    static int lw, lh, lgw, lgh;
    g_winw = w; g_winh = h;
    // mantém a proporção que o jogo está usando (4:3 nos menus, a da resolução nas fases); aspect=0 estica
    int keep = cfg.aspect != 0 && !(!g_gl_on && cfg.menu_fill == 2);   // menu_fill=2: telas 2D esticadas
    if (keep && (long long)w * g_gh > (long long)h * g_gw) { g_vh = h; g_vw = (int)((long long)h * g_gw / g_gh); g_vx = (w - g_vw) / 2; g_vy = 0; }
    else if (keep && (long long)w * g_gh < (long long)h * g_gw) { g_vw = w; g_vh = (int)((long long)w * g_gh / g_gw); g_vx = 0; g_vy = (h - g_vh) / 2; }
    else { g_vx = 0; g_vy = 0; g_vw = w; g_vh = h; }
    if (w != lw || h != lh || g_gw != lgw || g_gh != lgh) {
        lw = w; lh = h; lgw = g_gw; lgh = g_gh;
        logf_("jogo %dx%d -> janela %dx%d, imagem %dx%d em (%d,%d)", g_gw, g_gh, w, h, g_vw, g_vh, g_vx, g_vy);
    }
}
static HMODULE g_gl;                // opengl32 que o jogo carregou
static int g_S = 1;                 // fator do framebuffer interno (supersampling) enquanto o FBO está ativo
static int scaling_active(void) { return g_hwnd && g_winw && (g_S != 1 || g_vw != g_gw || g_vh != g_gh || g_vx || g_vy); }
// GL: retângulo em pixels do jogo (origem embaixo) -> pixels reais
static void map_rect(GLint *x, GLint *y, GLsizei *w, GLsizei *h) {
    if (!scaling_active()) return;
    double sx = (double)g_vw * g_S / g_gw, sy = (double)g_vh * g_S / g_gh;
    int x0 = g_vx * g_S + (int)floor(*x * sx + 0.5), x1 = g_vx * g_S + (int)floor((*x + *w) * sx + 0.5);
    int y0 = g_vy * g_S + (int)floor(*y * sy + 0.5), y1 = g_vy * g_S + (int)floor((*y + *h) * sy + 0.5);
    *x = x0; *y = y0; *w = x1 - x0; *h = y1 - y0;
}
static HDC g_vdc;
static void present_2d(int force);
static void bg_release(void);
static int is_game_dc(HDC dc) { return g_hwnd && dc && WindowFromDC(dc) == g_hwnd; }
static int is_game_wnd(HWND w) { return g_hwnd && w == g_hwnd; }

static void fit_window(void) {
    if (!g_hwnd) return;
    monitor_rect();
    SetWindowLongA(g_hwnd, GWL_STYLE, (GetWindowLongA(g_hwnd, GWL_STYLE) & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_DLGFRAME)) | WS_POPUP);
    SetWindowPos(g_hwnd, NULL, g_mon.left, g_mon.top, g_mon.right - g_mon.left, g_mon.bottom - g_mon.top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    update_rect();
}
static void paint_black(void) {
    if (!g_hwnd) return;
    HDC dc = GetDC(g_hwnd); RECT r; GetClientRect(g_hwnd, &r);
    FillRect(dc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    ReleaseDC(g_hwnd, dc);
}

// ---------------------------------------------------------------- GL hooks
typedef void (APIENTRY *PFN_Viewport)(GLint, GLint, GLsizei, GLsizei);
typedef void (APIENTRY *PFN_Ortho)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void (APIENTRY *PFN_LoadMatrixf)(const GLfloat *);
typedef void (APIENTRY *PFN_LoadMatrixd)(const GLdouble *);
typedef void (APIENTRY *PFN_MatrixMode)(GLenum);
typedef void (APIENTRY *PFN_Clear)(GLbitfield);
typedef void (APIENTRY *PFN_ReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
typedef void (APIENTRY *PFN_CopyTex2D)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
typedef void (APIENTRY *PFN_CopyTexSub2D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
typedef void (APIENTRY *PFN_Enable)(GLenum);
typedef void (APIENTRY *PFN_DrawPixels)(GLsizei, GLsizei, GLenum, GLenum, const void *);
typedef void (APIENTRY *PFN_RasterPos2)(GLint, GLint);
typedef BOOL (WINAPI *PFN_wglSwap)(HDC);
typedef BOOL (WINAPI *PFN_wglMakeCurrent)(HDC, HGLRC);
typedef HGLRC (WINAPI *PFN_wglCreateContext)(HDC);
typedef int  (WINAPI *PFN_ChoosePF)(HDC, const PIXELFORMATDESCRIPTOR *);
typedef BOOL (WINAPI *PFN_SetPF)(HDC, int, const PIXELFORMATDESCRIPTOR *);
typedef PROC (WINAPI *PFN_wglGetProc)(LPCSTR);

static PFN_Viewport r_Viewport, r_Scissor;
static PFN_Ortho r_Ortho, r_Frustum;
static PFN_LoadMatrixf r_LoadMatrixf;
static PFN_LoadMatrixd r_LoadMatrixd;
static PFN_MatrixMode r_MatrixMode;
static PFN_Clear r_Clear;
static PFN_ReadPixels r_ReadPixels;
static PFN_CopyTex2D r_CopyTexImage2D;
static PFN_CopyTexSub2D r_CopyTexSubImage2D;
static PFN_Enable r_Enable, r_Disable;
static PFN_DrawPixels r_DrawPixels;
static PFN_wglSwap r_wglSwapBuffers, r_SwapBuffersGdi;
static PFN_wglMakeCurrent r_wglMakeCurrent;
static PFN_wglCreateContext r_wglCreateContext;
static PFN_ChoosePF r_wglChoosePixelFormat, r_ChoosePixelFormatGdi;
static PFN_SetPF r_wglSetPixelFormat, r_SetPixelFormatGdi;
static PFN_wglGetProc r_wglGetProcAddress;

static int n_vp, n_sc, n_or, n_fr, n_lm, n_rp, n_ct, n_dp;
static GLenum g_mode = GL_MODELVIEW;
static int pr_proj;                                  // diagnóstico do HUD: última projeção (1 glOrtho, 2 glFrustum, 3 glLoadMatrix)
static void pr_tick(void);
static int g_scissor_on;
static long g_frames;
#define LOGN(counter, lim, ...) do { if (cfg.log_gl && (counter)++ < (lim)) logf_(__VA_ARGS__); } while (0)

static void APIENTRY h_Viewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    update_rect();
    LOGN(n_vp, 40, "glViewport(%d,%d,%d,%d) jogo %dx%d janela %dx%d", x, y, w, h, g_gw, g_gh, g_winw, g_winh);
    map_rect(&x, &y, &w, &h); r_Viewport(x, y, w, h);
}
static void APIENTRY h_Scissor(GLint x, GLint y, GLsizei w, GLsizei h) {
    LOGN(n_sc, 20, "glScissor(%d,%d,%d,%d)", x, y, w, h);
    map_rect(&x, &y, &w, &h); r_Scissor(x, y, w, h);
}
static void APIENTRY h_Enable(GLenum c) { if (c == GL_SCISSOR_TEST) g_scissor_on = 1; r_Enable(c); }
static void APIENTRY h_Disable(GLenum c) { if (c == GL_SCISSOR_TEST) g_scissor_on = 0; r_Disable(c); }
static void APIENTRY h_Ortho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    pr_proj = 1;
    LOGN(n_or, 30, "glOrtho(%g,%g,%g,%g,%g,%g) modo %04x", l, r, b, t, n, f, g_mode);
    r_Ortho(l, r, b, t, n, f);
}
static void APIENTRY h_Frustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    pr_proj = 2;
    LOGN(n_fr, 30, "glFrustum(%g,%g,%g,%g,%g,%g) aspecto %.3f", l, r, b, t, n, f, (r - l) / (t - b));
    r_Frustum(l, r, b, t, n, f);
}
static void APIENTRY h_MatrixMode(GLenum m) { g_mode = m; r_MatrixMode(m); }
static void APIENTRY h_LoadMatrixf(const GLfloat *m) {
    if (g_mode == GL_PROJECTION) pr_proj = 3;
    if (g_mode == GL_PROJECTION)
        LOGN(n_lm, 30, "glLoadMatrixf(PROJ) %g %g %g %g | %g %g %g %g | %g %g %g %g | %g %g %g %g",
             m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]);
    r_LoadMatrixf(m);
}
static void APIENTRY h_LoadMatrixd(const GLdouble *m) {
    if (g_mode == GL_PROJECTION)
        LOGN(n_lm, 30, "glLoadMatrixd(PROJ) %g %g %g %g | %g %g %g %g", m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13]);
    r_LoadMatrixd(m);
}
static void APIENTRY h_Clear(GLbitfield mask) { r_Clear(mask); }
static void APIENTRY h_ReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum t, void *p) {
    LOGN(n_rp, 20, "glReadPixels(%d,%d,%d,%d)", x, y, w, h);
    r_ReadPixels(x, y, w, h, f, t, p);
}
static void hd_forget_bound(void);
static void APIENTRY h_CopyTexImage2D(GLenum tg, GLint lv, GLenum fmt, GLint x, GLint y, GLsizei w, GLsizei h, GLint b) {
    LOGN(n_ct, 20, "glCopyTexImage2D(%d,%d,%d,%d)", x, y, w, h);
    if (lv == 0 && tg == GL_TEXTURE_2D) hd_forget_bound();
    r_CopyTexImage2D(tg, lv, fmt, x, y, w, h, b);
}
static void APIENTRY h_CopyTexSubImage2D(GLenum tg, GLint lv, GLint xo, GLint yo, GLint x, GLint y, GLsizei w, GLsizei h) {
    LOGN(n_ct, 20, "glCopyTexSubImage2D(%d,%d,%d,%d)", x, y, w, h);
    r_CopyTexSubImage2D(tg, lv, xo, yo, x, y, w, h);
}
static void APIENTRY h_DrawPixels(GLsizei w, GLsizei h, GLenum f, GLenum t, const void *p) {
    LOGN(n_dp, 20, "glDrawPixels(%d,%d)", w, h);
    r_DrawPixels(w, h, f, t, p);
}

static HGLRC g_fbo_rc; static int g_fbo_ok;
static void fbo_setup(HGLRC rc);
static volatile LONG g_kr, g_klod;
static void debug_keys(void);
static void icons_sync(void);
static void hd_reset(void);
static HGLRC g_hd_rc;                               // contexto dono das texturas marcadas como HD
static void mc_frame_end(void);
static void after_swap(void) {
    g_frames++;
    pr_tick();
    mc_frame_end();
    if (scaling_active() && r_Clear && r_Scissor && (g_vx > 0 || g_vy > 0)) {
        // limpa as barras para o próximo quadro (o jogo pode não limpar a cor)
        int S = g_S;
        GLboolean on = g_scissor_on;
        GLfloat cc[4]; glGetFloatv(GL_COLOR_CLEAR_VALUE, cc);
        glClearColor(0, 0, 0, 1);
        r_Enable(GL_SCISSOR_TEST);
        if (g_vx > 0) {
            r_Scissor(0, 0, g_vx * S, g_winh * S); r_Clear(GL_COLOR_BUFFER_BIT);
            r_Scissor((g_vx + g_vw) * S, 0, (g_winw - g_vx - g_vw) * S, g_winh * S); r_Clear(GL_COLOR_BUFFER_BIT);
        } else {
            r_Scissor(0, 0, g_winw * S, g_vy * S); r_Clear(GL_COLOR_BUFFER_BIT);
            r_Scissor(0, (g_vy + g_vh) * S, g_winw * S, (g_winh - g_vy - g_vh) * S); r_Clear(GL_COLOR_BUFFER_BIT);
        }
        glClearColor(cc[0], cc[1], cc[2], cc[3]);
        if (on) r_Enable(GL_SCISSOR_TEST); else r_Disable(GL_SCISSOR_TEST);
    }
    static DWORD t0, a0; static long f0, af0; static int slow; DWORD now = GetTickCount();
    if (!t0) { t0 = a0 = now; f0 = af0 = g_frames; }
    if (now - t0 >= 10000) { logf_("fps: %.1f (jogo %dx%d, janela %dx%d, interno x%d)", (g_frames - f0) * 1000.0 / (now - t0), g_gw, g_gh, g_winw, g_winh, g_S); t0 = now; f0 = g_frames; }
    // qualidade automática: com foco no jogo, se não segurar ~30 fps por 5 janelas de 2 s (10 s), reduz o antisserrilhado
    if (now - a0 >= 2000) {
        double fps = (g_frames - af0) * 1000.0 / (now - a0);
        a0 = now; af0 = g_frames;
        int focused = 0; { HWND fg = GetForegroundWindow(); DWORD pid = 0; if (fg) GetWindowThreadProcessId(fg, &pid); focused = pid == GetCurrentProcessId(); }
        int ingame = *(volatile int *)0x4d0800 == 2;   // só durante o jogo (vídeos e menus rodam a outro ritmo)
        if (cfg.auto_quality && focused && ingame && g_fbo_ok && fps >= 8 && fps < 26.5) slow++; else slow = 0;
        if (slow >= 5) {
            slow = 0;
            if (cfg.ssaa > 1) cfg.ssaa = 1; else if (cfg.msaa > 2) cfg.msaa = 2; else if (cfg.msaa) cfg.msaa = 0; else fx.on = 0;
            logf_("qualidade automatica: %.1f fps -> supersampling %dx, MSAA %dx, efeitos %s", fps, cfg.ssaa, cfg.msaa, fx.on ? "sim" : "nao");
            fbo_setup(wglGetCurrentContext());   // libera o FBO atual e cria o novo
        }
    }
    icons_sync();
    if (cfg.debug) debug_keys();
}

// ---- framebuffer interno: MSAA + supersampling sem mexer no formato de pixel da janela
#define GL_FRAMEBUFFER_EXT          0x8D40
#define GL_READ_FRAMEBUFFER_EXT     0x8CA8
#define GL_DRAW_FRAMEBUFFER_EXT     0x8CA9
#define GL_RENDERBUFFER_EXT         0x8D41
#define GL_COLOR_ATTACHMENT0_EXT    0x8CE0
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_DEPTH24_STENCIL8_EXT     0x88F0
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
#define GL_MAX_SAMPLES_EXT          0x8D57
#define GL_RGBA8_                   0x8058
typedef void (APIENTRY *PFN_GenX)(GLsizei, GLuint *);
typedef void (APIENTRY *PFN_BindX)(GLenum, GLuint);
typedef void (APIENTRY *PFN_FBRB)(GLenum, GLenum, GLenum, GLuint);
typedef void (APIENTRY *PFN_RBSM)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
typedef void (APIENTRY *PFN_Blit)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
typedef GLenum (APIENTRY *PFN_Check)(GLenum);
typedef void (APIENTRY *PFN_GenMip)(GLenum);
typedef void (APIENTRY *PFN_DelX)(GLsizei, const GLuint *);
static PFN_GenX x_GenFramebuffers, x_GenRenderbuffers;
static PFN_BindX x_BindFramebuffer, x_BindRenderbuffer;
static PFN_FBRB x_FramebufferRenderbuffer;
static PFN_RBSM x_RenderbufferStorageMultisample;
static PFN_Blit x_BlitFramebuffer;
static PFN_Check x_CheckFramebufferStatus;
static PFN_GenMip x_GenerateMipmap;
static PFN_DelX x_DeleteFramebuffers, x_DeleteRenderbuffers;
static GLuint g_fbo, g_res, g_rb[3];   // renderbuffers: cor e profundidade do FBO, cor do resolve
static int g_fbo_w, g_fbo_h;
static GLfloat g_max_aniso;

static void load_gl_ext(void) {
    static int tried; if (tried) return;
    if (!wglGetCurrentContext()) return;           // sem contexto o wglGetProcAddress devolve NULL
    PROC (WINAPI *gp)(LPCSTR) = r_wglGetProcAddress;
    if (!gp && g_gl) gp = (void *)GetProcAddress(g_gl, "wglGetProcAddress");
    if (!gp) return;
    tried = 1;
    x_GenFramebuffers = (void *)gp("glGenFramebuffers");
    x_GenRenderbuffers = (void *)gp("glGenRenderbuffers");
    x_BindFramebuffer = (void *)gp("glBindFramebuffer");
    x_BindRenderbuffer = (void *)gp("glBindRenderbuffer");
    x_FramebufferRenderbuffer = (void *)gp("glFramebufferRenderbuffer");
    x_RenderbufferStorageMultisample = (void *)gp("glRenderbufferStorageMultisample");
    x_BlitFramebuffer = (void *)gp("glBlitFramebuffer");
    x_CheckFramebufferStatus = (void *)gp("glCheckFramebufferStatus");
    x_GenerateMipmap = (void *)gp("glGenerateMipmap");
    x_DeleteFramebuffers = (void *)gp("glDeleteFramebuffers");
    x_DeleteRenderbuffers = (void *)gp("glDeleteRenderbuffers");
    int fbo = x_GenFramebuffers && x_GenRenderbuffers && x_BindFramebuffer && x_BindRenderbuffer && x_FramebufferRenderbuffer
              && x_RenderbufferStorageMultisample && x_BlitFramebuffer;
    if (!fbo) x_BlitFramebuffer = NULL;            // sem o conjunto completo, nada de FBO
    const char *ext = (const char *)glGetString(GL_EXTENSIONS);
    if (ext && strstr(ext, "GL_EXT_texture_filter_anisotropic")) glGetFloatv(0x84FF /* MAX_TEXTURE_MAX_ANISOTROPY */, &g_max_aniso);
    logf_("GL extra: FBO %s, mipmap %s, anisotropia max %.0fx", fbo ? "ok" : "indisponivel", x_GenerateMipmap ? "ok" : "indisponivel", g_max_aniso);
}

static void del_fbo(GLuint *fb, GLuint *rb, int nrb) {
    if (*fb && x_DeleteFramebuffers) x_DeleteFramebuffers(1, fb);
    for (int i = 0; i < nrb; i++) if (rb[i] && x_DeleteRenderbuffers) x_DeleteRenderbuffers(1, &rb[i]);
    *fb = 0; for (int i = 0; i < nrb; i++) rb[i] = 0;
}
// FBO com renderbuffers próprios (rb[0] cor, rb[1] profundidade). Falta de memória ou formato recusado = 0.
static GLuint make_fbo(int w, int h, int samples, int depth, GLuint *rb) {
    GLuint fb = 0;
    while (glGetError() != GL_NO_ERROR) {}
    x_GenFramebuffers(1, &fb);
    x_BindFramebuffer(GL_FRAMEBUFFER_EXT, fb);
    x_GenRenderbuffers(depth ? 2 : 1, rb);
    x_BindRenderbuffer(GL_RENDERBUFFER_EXT, rb[0]);
    x_RenderbufferStorageMultisample(GL_RENDERBUFFER_EXT, samples, GL_RGBA8_, w, h);
    x_FramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, rb[0]);
    if (depth) {
        x_BindRenderbuffer(GL_RENDERBUFFER_EXT, rb[1]);
        x_RenderbufferStorageMultisample(GL_RENDERBUFFER_EXT, samples, GL_DEPTH24_STENCIL8_EXT, w, h);
        x_FramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER_EXT, rb[1]);
    }
    GLenum err = glGetError();
    GLenum st = x_CheckFramebufferStatus ? x_CheckFramebufferStatus(GL_FRAMEBUFFER_EXT) : GL_FRAMEBUFFER_COMPLETE_EXT;
    if (st != GL_FRAMEBUFFER_COMPLETE_EXT || err == 0x0505 /* GL_OUT_OF_MEMORY */) {
        logf_("FBO %dx%d x%d recusado (status %04x, erro %04x)", w, h, samples, st, err);
        x_BindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
        del_fbo(&fb, rb, depth ? 2 : 1);
        return 0;
    }
    return fb;
}


// ---- pós-processamento: a imagem interna vira textura e passa por shaders antes de ir para a janela
//   1) resolve do MSAA para textura   2) redução do supersampling (média SxS)   3) bloom em 1/4 da tela
//   4) passe final: nitidez CAS + bloom + contraste/saturação + vinheta
#define GL_FRAGMENT_SHADER_ 0x8B30
#define GL_VERTEX_SHADER_   0x8B31
#define GL_COMPILE_STATUS_  0x8B81
#define GL_LINK_STATUS_     0x8B82
#define GL_TEXTURE0_        0x84C0
#define GL_CLAMP_TO_EDGE_   0x812F
typedef GLuint (APIENTRY *PFN_CrSh)(GLenum);
typedef void (APIENTRY *PFN_ShSrc)(GLuint, GLsizei, const char *const *, const GLint *);
typedef void (APIENTRY *PFN_Obj)(GLuint);
typedef void (APIENTRY *PFN_GetObjiv)(GLuint, GLenum, GLint *);
typedef void (APIENTRY *PFN_GetLog)(GLuint, GLsizei, GLsizei *, char *);
typedef GLuint (APIENTRY *PFN_CrProg)(void);
typedef void (APIENTRY *PFN_Attach)(GLuint, GLuint);
typedef GLint (APIENTRY *PFN_GetUni)(GLuint, const char *);
typedef void (APIENTRY *PFN_U1i)(GLint, GLint);
typedef void (APIENTRY *PFN_U1f)(GLint, GLfloat);
typedef void (APIENTRY *PFN_U2f)(GLint, GLfloat, GLfloat);
typedef void (APIENTRY *PFN_ActTex)(GLenum);
typedef void (APIENTRY *PFN_FBTex)(GLenum, GLenum, GLenum, GLuint, GLint);
static PFN_CrSh y_CreateShader; static PFN_ShSrc y_ShaderSource; static PFN_Obj y_CompileShader, y_LinkProgram, y_UseProgram;
static PFN_GetObjiv y_GetShaderiv, y_GetProgramiv; static PFN_GetLog y_GetShaderInfoLog, y_GetProgramInfoLog;
static PFN_CrProg y_CreateProgram; static PFN_Attach y_AttachShader; static PFN_GetUni y_GetUniformLocation;
static PFN_U1i y_Uniform1i; static PFN_U1f y_Uniform1f; static PFN_U2f y_Uniform2f;
static PFN_ActTex y_ActiveTexture; static PFN_FBTex y_FramebufferTexture2D;

enum { FX_FULL, FX_HALF, FX_B1, FX_B2, FX_N };    // texturas: interna, tela, bloom A, bloom B
static GLuint g_fx_fb[FX_N], g_fx_tex[FX_N];
static int g_fx_w[FX_N], g_fx_h[FX_N];
static GLuint g_fx_down, g_fx_bright, g_fx_blur, g_fx_final;
static HGLRC g_fx_prog_rc;
static int g_fx_ok, g_fx_bypass;   // bypass: debug F11 mostra a imagem sem os efeitos

static const char *fx_vs =
    "#version 110\n"
    "varying vec2 uv;\n"
    "void main() { uv = gl_Vertex.xy * 0.5 + 0.5; gl_Position = vec4(gl_Vertex.xy, 0.0, 1.0); }\n";
static const char *fx_fs_down =                    // média de SxS texels da imagem interna
    "#version 110\n"
    "uniform sampler2D tex; uniform vec2 px; uniform float S; varying vec2 uv;\n"
    "void main() {\n"
    "  vec2 base = uv - px * (S * 0.5 - 0.5); vec4 a = vec4(0.0);\n"
    "  for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++)\n"
    "    if (float(x) < S && float(y) < S) a += texture2D(tex, base + px * vec2(float(x), float(y)));\n"
    "  gl_FragColor = a / (S * S);\n"
    "}\n";
static const char *fx_fs_bright =                  // reduz 4x4 e guarda só o que é claro (joelho suave)
    "#version 110\n"
    "uniform sampler2D tex; uniform vec2 px; uniform float thr; varying vec2 uv;\n"
    "void main() {\n"
    "  vec3 c = (texture2D(tex, uv + px * vec2(-1.0, -1.0)).rgb + texture2D(tex, uv + px * vec2(1.0, -1.0)).rgb +\n"
    "            texture2D(tex, uv + px * vec2(-1.0, 1.0)).rgb + texture2D(tex, uv + px * vec2(1.0, 1.0)).rgb) * 0.25;\n"
    "  float l = max(c.r, max(c.g, c.b));\n"
    "  gl_FragColor = vec4(c * smoothstep(thr, thr + 0.25, l), 1.0);\n"
    "}\n";
static const char *fx_fs_blur =                    // gaussiana de 9 amostras em 5 leituras lineares
    "#version 110\n"
    "uniform sampler2D tex; uniform vec2 dir; varying vec2 uv;\n"
    "void main() {\n"
    "  vec3 c = texture2D(tex, uv).rgb * 0.2270270270;\n"
    "  c += (texture2D(tex, uv + dir * 1.3846153846).rgb + texture2D(tex, uv - dir * 1.3846153846).rgb) * 0.3162162162;\n"
    "  c += (texture2D(tex, uv + dir * 3.2307692308).rgb + texture2D(tex, uv - dir * 3.2307692308).rgb) * 0.0702702703;\n"
    "  gl_FragColor = vec4(c, 1.0);\n"
    "}\n";
static const char *fx_fs_final =
    "#version 110\n"
    "uniform sampler2D tex; uniform sampler2D btex; uniform vec2 px;\n"
    "uniform float sharp, bloom, con, sat, vig; varying vec2 uv;\n"
    "vec3 T(float x, float y) { return texture2D(tex, uv + px * vec2(x, y)).rgb; }\n"
    "void main() {\n"
    "  vec3 e = T(0.0, 0.0), c = e;\n"
    "  if (sharp > 0.0) {\n"                          // AMD FidelityFX CAS (contrast adaptive sharpening)
    "    vec3 a = T(-1.0, -1.0), b = T(0.0, -1.0), cc = T(1.0, -1.0), d = T(-1.0, 0.0), f = T(1.0, 0.0);\n"
    "    vec3 g = T(-1.0, 1.0), h = T(0.0, 1.0), i = T(1.0, 1.0);\n"
    "    vec3 mn = min(min(min(d, e), min(f, b)), h); mn += min(mn, min(min(a, cc), min(g, i)));\n"
    "    vec3 mx = max(max(max(d, e), max(f, b)), h); mx += max(mx, max(max(a, cc), max(g, i)));\n"
    "    vec3 amp = sqrt(clamp(min(mn, 2.0 - mx) / max(mx, vec3(0.0001)), 0.0, 1.0));\n"
    "    vec3 w = amp * (-1.0 / mix(8.0, 5.0, sharp));\n"
    "    c = clamp(((b + d + f + h) * w + e) / (1.0 + 4.0 * w), 0.0, 1.0);\n"
    "  }\n"
    "  c += bloom * texture2D(btex, uv).rgb;\n"
    "  c = (c - 0.5) * con + 0.5;\n"
    "  float l = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"
    "  c = mix(vec3(l), c, sat);\n"
    "  c *= 1.0 - vig * smoothstep(0.7, 1.45, length((uv - 0.5) * 2.0));\n"
    "  gl_FragColor = vec4(clamp(c, 0.0, 1.0), 1.0);\n"
    "}\n";

static int fx_load(void) {
    static int tried, ok; if (tried) return ok;
    PROC (WINAPI *gp)(LPCSTR) = r_wglGetProcAddress;
    if (!gp && g_gl) gp = (void *)GetProcAddress(g_gl, "wglGetProcAddress");
    if (!gp || !wglGetCurrentContext()) return 0;
    tried = 1;
    #define L(v, n) v = (void *)gp(n)
    L(y_CreateShader, "glCreateShader"); L(y_ShaderSource, "glShaderSource"); L(y_CompileShader, "glCompileShader");
    L(y_GetShaderiv, "glGetShaderiv"); L(y_GetShaderInfoLog, "glGetShaderInfoLog"); L(y_CreateProgram, "glCreateProgram");
    L(y_AttachShader, "glAttachShader"); L(y_LinkProgram, "glLinkProgram"); L(y_GetProgramiv, "glGetProgramiv");
    L(y_GetProgramInfoLog, "glGetProgramInfoLog"); L(y_UseProgram, "glUseProgram"); L(y_GetUniformLocation, "glGetUniformLocation");
    L(y_Uniform1i, "glUniform1i"); L(y_Uniform1f, "glUniform1f"); L(y_Uniform2f, "glUniform2f");
    L(y_ActiveTexture, "glActiveTexture"); L(y_FramebufferTexture2D, "glFramebufferTexture2D");
    #undef L
    ok = y_CreateShader && y_ShaderSource && y_CompileShader && y_GetShaderiv && y_GetShaderInfoLog && y_CreateProgram &&
         y_AttachShader && y_LinkProgram && y_GetProgramiv && y_GetProgramInfoLog && y_UseProgram && y_GetUniformLocation &&
         y_Uniform1i && y_Uniform1f && y_Uniform2f && y_ActiveTexture && y_FramebufferTexture2D;
    if (!ok) logf_("pos-processamento: placa sem shaders GLSL, desligado");
    return ok;
}
static GLuint fx_shader(GLenum type, const char *src, const char *name) {
    GLuint sh = y_CreateShader(type);
    y_ShaderSource(sh, 1, &src, NULL);
    y_CompileShader(sh);
    GLint ok = 0; y_GetShaderiv(sh, GL_COMPILE_STATUS_, &ok);
    if (!ok) { char log[1024] = ""; y_GetShaderInfoLog(sh, sizeof log, NULL, log); logf_("shader %s nao compilou: %s", name, log); return 0; }
    return sh;
}
static GLuint fx_program(const char *fs, const char *name) {
    GLuint v = fx_shader(GL_VERTEX_SHADER_, fx_vs, "vertices"), f = fx_shader(GL_FRAGMENT_SHADER_, fs, name);
    if (!v || !f) return 0;
    GLuint p = y_CreateProgram();
    y_AttachShader(p, v); y_AttachShader(p, f); y_LinkProgram(p);
    GLint ok = 0; y_GetProgramiv(p, GL_LINK_STATUS_, &ok);
    if (!ok) { char log[1024] = ""; y_GetProgramInfoLog(p, sizeof log, NULL, log); logf_("programa %s nao ligou: %s", name, log); return 0; }
    return p;
}
static void fx_free(void) {
    for (int i = 0; i < FX_N; i++) {
        if (g_fx_fb[i] && x_DeleteFramebuffers) x_DeleteFramebuffers(1, &g_fx_fb[i]);
        if (g_fx_tex[i]) glDeleteTextures(1, &g_fx_tex[i]);
        g_fx_fb[i] = g_fx_tex[i] = 0;
    }
    g_fx_ok = 0;
}
// textura + FBO; nomes de textura fixos e altos para não cruzar com os do jogo
static int fx_target(int i, int w, int h) {
    g_fx_tex[i] = 0x7B1B0100 + i; g_fx_w[i] = w; g_fx_h[i] = h;
    glBindTexture(GL_TEXTURE_2D, g_fx_tex[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8_, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE_);
    x_GenFramebuffers(1, &g_fx_fb[i]);
    x_BindFramebuffer(GL_FRAMEBUFFER_EXT, g_fx_fb[i]);
    y_FramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, g_fx_tex[i], 0);
    GLenum st = x_CheckFramebufferStatus ? x_CheckFramebufferStatus(GL_FRAMEBUFFER_EXT) : GL_FRAMEBUFFER_COMPLETE_EXT;
    return st == GL_FRAMEBUFFER_COMPLETE_EXT;
}
static int fx_setup(HGLRC rc, int iw, int ih, int S) {
    fx_free();
    if (!fx.on || !fx_load()) return 0;
    if (g_fx_prog_rc != rc) {
        g_fx_down = fx_program(fx_fs_down, "reducao"); g_fx_bright = fx_program(fx_fs_bright, "brilho");
        g_fx_blur = fx_program(fx_fs_blur, "desfoque"); g_fx_final = fx_program(fx_fs_final, "final");
        g_fx_prog_rc = rc;
    }
    if (!g_fx_down || !g_fx_bright || !g_fx_blur || !g_fx_final) { fx.on = 0; return 0; }
    GLint oldtex = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldtex);
    while (glGetError() != GL_NO_ERROR) {}
    int bw = g_winw / 4 > 1 ? g_winw / 4 : 1, bh = g_winh / 4 > 1 ? g_winh / 4 : 1;
    int ok = fx_target(FX_FULL, iw, ih) && (S == 1 || fx_target(FX_HALF, g_winw, g_winh)) &&
             fx_target(FX_B1, bw, bh) && fx_target(FX_B2, bw, bh);
    if (glGetError() == 0x0505) ok = 0;
    glBindTexture(GL_TEXTURE_2D, (GLuint)oldtex);
    x_BindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
    if (!ok) { logf_("pos-processamento: texturas recusadas, desligado"); fx_free(); return 0; }
    g_fx_ok = 1;
    logf_("pos-processamento: bloom %d%% (limiar %d%%), nitidez %d%%, saturacao %d%%, contraste %d%%, vinheta %d%%",
          fx.bloom, fx.threshold, fx.sharpen, fx.saturation, fx.contrast, fx.vignette);
    return 1;
}
// prepara um passe: destino, programa e textura de entrada na unidade 0
static GLuint fx_pass(GLuint prog, GLuint fb, GLuint src, int dst_w, int dst_h) {
    x_BindFramebuffer(GL_FRAMEBUFFER_EXT, fb);
    glViewport(0, 0, dst_w, dst_h);
    y_UseProgram(prog);
    glBindTexture(GL_TEXTURE_2D, src);
    y_Uniform1i(y_GetUniformLocation(prog, "tex"), 0);
    return prog;
}
static void fx_quad(void) { glRectf(-1, -1, 1, 1); }
#define U1(p, n, v) y_Uniform1f(y_GetUniformLocation(p, n), (GLfloat)(v))
#define U2(p, n, a, b) y_Uniform2f(y_GetUniformLocation(p, n), (GLfloat)(a), (GLfloat)(b))
// roda com a imagem interna já resolvida em g_fx_tex[FX_FULL]; termina desenhando na janela (framebuffer 0)
static void fx_run(void) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_SCISSOR_TEST); glDisable(GL_DEPTH_TEST); glDisable(GL_STENCIL_TEST); glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST); glDisable(GL_CULL_FACE); glDisable(GL_COLOR_LOGIC_OP); glDisable(GL_DITHER);
    glColorMask(1, 1, 1, 1); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    y_ActiveTexture(GL_TEXTURE0_ + 1); GLint tex1 = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex1);
    y_ActiveTexture(GL_TEXTURE0_);
    GLuint P, src = g_fx_tex[FX_FULL]; int sw = g_fx_w[FX_FULL], sh = g_fx_h[FX_FULL];
    if (g_S > 1) {                                   // supersampling: média SxS até o tamanho da janela
        P = fx_pass(g_fx_down, g_fx_fb[FX_HALF], src, g_winw, g_winh);
        U2(P, "px", 1.0 / sw, 1.0 / sh); U1(P, "S", g_S);
        fx_quad();
        src = g_fx_tex[FX_HALF]; sw = g_winw; sh = g_winh;
    }
    int bw = g_fx_w[FX_B1], bh = g_fx_h[FX_B1];
    int on = !g_fx_bypass;
    if (fx.bloom > 0 && on) {                        // bloom: claros em 1/4 da tela, desfoque 2x (H+V)
        P = fx_pass(g_fx_bright, g_fx_fb[FX_B1], src, bw, bh);
        U2(P, "px", 1.0 / sw, 1.0 / sh); U1(P, "thr", fx.threshold / 100.0);
        fx_quad();
        for (int k = 0; k < 2; k++) {
            P = fx_pass(g_fx_blur, g_fx_fb[FX_B2], g_fx_tex[FX_B1], bw, bh); U2(P, "dir", 1.0 / bw, 0); fx_quad();
            P = fx_pass(g_fx_blur, g_fx_fb[FX_B1], g_fx_tex[FX_B2], bw, bh); U2(P, "dir", 0, 1.0 / bh); fx_quad();
        }
    }
    y_ActiveTexture(GL_TEXTURE0_ + 1); glBindTexture(GL_TEXTURE_2D, g_fx_tex[FX_B1]);
    y_ActiveTexture(GL_TEXTURE0_);
    P = fx_pass(g_fx_final, 0, src, g_winw, g_winh);
    y_Uniform1i(y_GetUniformLocation(P, "btex"), 1);
    U2(P, "px", 1.0 / sw, 1.0 / sh);
    U1(P, "sharp", on ? fx.sharpen / 100.0 : 0); U1(P, "bloom", on ? fx.bloom / 100.0 : 0);
    U1(P, "con", on ? fx.contrast / 100.0 : 1); U1(P, "sat", on ? fx.saturation / 100.0 : 1); U1(P, "vig", on ? fx.vignette / 100.0 : 0);
    fx_quad();
    y_UseProgram(0);
    y_ActiveTexture(GL_TEXTURE0_ + 1); glBindTexture(GL_TEXTURE_2D, (GLuint)tex1);
    y_ActiveTexture(GL_TEXTURE0_);
    glPopAttrib();
}
#undef U1
#undef U2

// libera os objetos do FBO (precisa do contexto dono atual)
static void fbo_free(void) {
    if (!g_fbo && !g_res && !g_fx_ok) return;
    x_BindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
    del_fbo(&g_fbo, g_rb, 2);
    del_fbo(&g_res, g_rb + 2, 1);
    fx_free();
}

// Cria a imagem interna. Se a placa recusar (memória, limites), tenta algo mais leve: supersampling menor,
// depois MSAA menor, até desligar. Monitores 4K ficam sem supersampling (já são nítidos, e seria pesado demais).
static void fbo_setup(HGLRC rc) {
    if (rc == g_fbo_rc) fbo_free();
    else { g_fbo = g_res = 0; memset(g_rb, 0, sizeof g_rb); memset(g_fx_fb, 0, sizeof g_fx_fb); memset(g_fx_tex, 0, sizeof g_fx_tex); g_fx_ok = 0; }
    g_S = 1; g_fbo_ok = 0; g_fbo_rc = NULL;
    load_gl_ext();
    if ((cfg.msaa < 2 && cfg.ssaa < 2 && !fx.on) || !x_BlitFramebuffer) { g_fbo_rc = rc; return; }
    update_rect();
    if (!g_winw || !g_winh) return;                 // janela ainda sem tamanho: tenta de novo depois
    g_fbo_rc = rc;
    GLint maxs = 0; glGetIntegerv(GL_MAX_SAMPLES_EXT, &maxs);
    GLint maxrb = 0; glGetIntegerv(0x84E8 /* GL_MAX_RENDERBUFFER_SIZE */, &maxrb);
    int samples = cfg.msaa < 2 ? 0 : (cfg.msaa > maxs ? maxs : cfg.msaa);
    if (samples < 2) samples = 0;
    int S = cfg.ssaa < 1 ? 1 : cfg.ssaa > 4 ? 4 : cfg.ssaa;
    while (S > 1 && (long long)g_winw * S * g_winh * S > 5120LL * 2880) S--;   // teto: 2x de 2560x1440
    while (S > 1 && maxrb && (g_winw * S > maxrb || g_winh * S > maxrb)) S--;
    while (S > 1 || samples || fx.on) {
        int w = g_winw * S, h = g_winh * S;
        g_fbo = make_fbo(w, h, samples, 1, g_rb);
        int fxok = g_fbo && fx.on && fx_setup(rc, w, h, S);
        if (g_fbo && !fxok && !(S > 1 || samples)) { fbo_free(); break; }   // sem efeitos e sem AA: o FBO não serve
        g_res = (g_fbo && !fxok && S > 1 && samples) ? make_fbo(w, h, 0, 0, g_rb + 2) : 0;   // resolve do MSAA antes de reduzir
        if (g_fbo && (fxok || !(S > 1 && samples && !g_res))) {
            x_BindFramebuffer(GL_FRAMEBUFFER_EXT, g_fbo);
            g_fbo_w = w; g_fbo_h = h; g_fbo_ok = 1; g_S = S;
            cfg.ssaa = S; cfg.msaa = samples;       // a qualidade automática parte do que está em uso
            logf_("imagem interna %dx%d, MSAA %dx, supersampling %dx (janela %dx%d)%s", w, h, samples, S, g_winw, g_winh,
                  fxok ? ", com pos-processamento" : "");
            return;
        }
        fbo_free();
        if (S > 1) S--; else if (samples > 2) samples = 2; else if (samples) samples = 0; else fx.on = 0;
        logf_("FBO: tentando supersampling %dx, MSAA %dx, efeitos %s", S, samples, fx.on ? "sim" : "nao");
    }
    cfg.ssaa = 1; cfg.msaa = 0;
    logf_("FBO desligado: imagem direto na janela");
}

static void fbo_present(void) {
    if (!g_fbo_ok || wglGetCurrentContext() != g_fbo_rc) return;
    GLboolean sc = glIsEnabled(GL_SCISSOR_TEST);
    if (sc) glDisable(GL_SCISSOR_TEST);
    if (g_fx_ok) {
        x_BindFramebuffer(GL_READ_FRAMEBUFFER_EXT, g_fbo); x_BindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, g_fx_fb[FX_FULL]);
        x_BlitFramebuffer(0, 0, g_fbo_w, g_fbo_h, 0, 0, g_fbo_w, g_fbo_h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        fx_run();
        if (sc) glEnable(GL_SCISSOR_TEST);
        return;
    }
    if (g_S > 1 && g_res) {
        x_BindFramebuffer(GL_READ_FRAMEBUFFER_EXT, g_fbo); x_BindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, g_res);
        x_BlitFramebuffer(0, 0, g_fbo_w, g_fbo_h, 0, 0, g_fbo_w, g_fbo_h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        x_BindFramebuffer(GL_READ_FRAMEBUFFER_EXT, g_res);
    } else {
        x_BindFramebuffer(GL_READ_FRAMEBUFFER_EXT, g_fbo);
    }
    x_BindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, 0);
    x_BlitFramebuffer(0, 0, g_fbo_w, g_fbo_h, 0, 0, g_winw, g_winh, GL_COLOR_BUFFER_BIT, g_S > 1 ? GL_LINEAR : GL_NEAREST);
    if (sc) glEnable(GL_SCISSOR_TEST);
}
static void fbo_rebind(void) {
    if (g_fbo_ok && wglGetCurrentContext() == g_fbo_rc) x_BindFramebuffer(GL_FRAMEBUFFER_EXT, g_fbo);
}

// F12: captura da tela do jogo na resolução da janela, sem perdas (bin\capturas\*.bmp). Lê a imagem final
// (depois do antisserrilhado e dos efeitos), antes da troca de buffers.
static void shot_check(void) {
    static int prev;
    int down = (GetAsyncKeyState(VK_F12) & 0x8000) != 0;
    int fire = down && !prev; prev = down;
    if (!fire || !g_gl_on || g_winw <= 0 || g_winh <= 0) return;
    DWORD pid = 0; HWND fg = GetForegroundWindow();             // só com o jogo em foco (F12 em outro programa não conta)
    if (fg) GetWindowThreadProcessId(fg, &pid);
    if (pid != GetCurrentProcessId()) return;
    int w = g_winw, h = g_winh, stride = (w * 3 + 3) & ~3;
    BYTE *px = malloc((size_t)stride * h);
    if (!px) return;
    GLint rfb = 0, rb = 0, pa = 4;
    if (x_BindFramebuffer) { glGetIntegerv(0x8CAA /* GL_READ_FRAMEBUFFER_BINDING */, &rfb); x_BindFramebuffer(GL_READ_FRAMEBUFFER_EXT, 0); }
    glGetIntegerv(0x0C02 /* GL_READ_BUFFER */, &rb); glGetIntegerv(0x0D05 /* GL_PACK_ALIGNMENT */, &pa);
    glReadBuffer(GL_BACK); glPixelStorei(0x0D05, 4);
    glReadPixels(0, 0, w, h, 0x80E0 /* GL_BGR */, GL_UNSIGNED_BYTE, px);
    glPixelStorei(0x0D05, pa); glReadBuffer((GLenum)rb);
    if (x_BindFramebuffer) x_BindFramebuffer(GL_READ_FRAMEBUFFER_EXT, (GLuint)rfb);
    char dir[MAX_PATH], path[MAX_PATH];
    snprintf(dir, sizeof dir, "%s\\capturas", g_dir); CreateDirectoryA(dir, NULL);
    SYSTEMTIME st; GetLocalTime(&st);
    snprintf(path, sizeof path, "%s\\bblit2_%04d%02d%02d_%02d%02d%02d_%03d.bmp", dir, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    FILE *f = fopen(path, "wb");
    if (f) {
        BITMAPFILEHEADER fh = { 0x4d42, 0, 0, 0, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) };
        BITMAPINFOHEADER ih = { sizeof ih, w, h, 1, 24, BI_RGB, (DWORD)stride * h, 2835, 2835, 0, 0 };
        fh.bfSize = fh.bfOffBits + ih.biSizeImage;
        fwrite(&fh, sizeof fh, 1, f); fwrite(&ih, sizeof ih, 1, f); fwrite(px, 1, (size_t)stride * h, f); fclose(f);
        logf_("captura: %s (%dx%d)", path, w, h);
    }
    if (cfg.debug && w >= 640 && h >= 360) {           // debug: cópia pequena (640x360) com nome fixo, para conferir rápido
        int sw = 640, sh = 360, ss = sw * 3;
        BYTE *sp = malloc((size_t)ss * sh);
        if (sp) {
            for (int y = 0; y < sh; y++) for (int x = 0; x < sw; x++) {
                int x0 = x * w / sw, x1 = (x + 1) * w / sw, y0 = y * h / sh, y1 = (y + 1) * h / sh, n = 0, a[3] = { 0, 0, 0 };
                for (int yy = y0; yy < y1; yy++) for (int xx = x0; xx < x1; xx++, n++) { const BYTE *q = px + (size_t)yy * stride + xx * 3; a[0] += q[0]; a[1] += q[1]; a[2] += q[2]; }
                BYTE *d = sp + (size_t)y * ss + x * 3; for (int c = 0; c < 3; c++) d[c] = (BYTE)(n ? a[c] / n : 0);
            }
            snprintf(path, sizeof path, "%s\\ultima.bmp", dir);
            if ((f = fopen(path, "wb")) != NULL) {
                BITMAPFILEHEADER fh = { 0x4d42, 0, 0, 0, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) };
                BITMAPINFOHEADER ih = { sizeof ih, sw, sh, 1, 24, BI_RGB, (DWORD)ss * sh, 2835, 2835, 0, 0 };
                fh.bfSize = fh.bfOffBits + ih.biSizeImage;
                fwrite(&fh, sizeof fh, 1, f); fwrite(&ih, sizeof ih, 1, f); fwrite(sp, 1, (size_t)ss * sh, f); fclose(f);
            }
            free(sp);
        }
    }
    free(px);
}
static BOOL WINAPI h_wglSwapBuffers(HDC dc) { fbo_present(); shot_check(); BOOL r = r_wglSwapBuffers(dc); fbo_rebind(); after_swap(); return r; }
static BOOL WINAPI h_SwapBuffersGdi(HDC dc) { fbo_present(); shot_check(); BOOL r = r_SwapBuffersGdi(dc); fbo_rebind(); after_swap(); return r; }

// ----- MSAA: escolhe um formato multisample via WGL_ARB_pixel_format com um contexto temporário
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_ACCELERATION_ARB   0x2003
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB  0x2011
#define WGL_PIXEL_TYPE_ARB     0x2013
#define WGL_COLOR_BITS_ARB     0x2014
#define WGL_ALPHA_BITS_ARB     0x201B
#define WGL_DEPTH_BITS_ARB     0x2022
#define WGL_STENCIL_BITS_ARB   0x2023
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB      0x202B
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB        0x2042
#define GL_MULTISAMPLE_ARB     0x809D
typedef BOOL (WINAPI *PFN_wglChoosePFARB)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);
typedef BOOL (WINAPI *PFN_wglSwapInterval)(int);

static int g_msaa_pf;

static int __attribute__((unused)) find_msaa_format(HDC dc, const PIXELFORMATDESCRIPTOR *want) {
    if (cfg.msaa < 2 || !g_gl) return 0;
    PFN_wglCreateContext cc = (void *)GetProcAddress(g_gl, "wglCreateContext");
    PFN_wglMakeCurrent mc = (void *)GetProcAddress(g_gl, "wglMakeCurrent");
    BOOL (WINAPI *del)(HGLRC) = (void *)GetProcAddress(g_gl, "wglDeleteContext");
    PFN_wglGetProc gp = (void *)GetProcAddress(g_gl, "wglGetProcAddress");
    if (!cc || !mc || !del || !gp) return 0;
    // janela temporária: o pixel format de uma janela só pode ser definido uma vez
    HWND tw = CreateWindowA("STATIC", "", WS_POPUP, 0, 0, 16, 16, NULL, NULL, NULL, NULL);
    if (!tw) return 0;
    HDC tdc = GetDC(tw);
    PIXELFORMATDESCRIPTOR pfd; memset(&pfd, 0, sizeof pfd);
    pfd.nSize = sizeof pfd; pfd.nVersion = 1; pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    int pf = ChoosePixelFormat(tdc, &pfd), result = 0;
    if (pf && SetPixelFormat(tdc, pf, &pfd)) {
        HGLRC rc = cc(tdc);
        if (rc && mc(tdc, rc)) {
            PFN_wglChoosePFARB choose = (void *)gp("wglChoosePixelFormatARB");
            if (choose) {
                int depth = want && want->cDepthBits ? (want->cDepthBits < 24 ? 24 : want->cDepthBits) : 24;
                for (int s = cfg.msaa; s >= 2 && !result; s /= 2) {
                    int at[] = { WGL_DRAW_TO_WINDOW_ARB, 1, WGL_SUPPORT_OPENGL_ARB, 1, WGL_DOUBLE_BUFFER_ARB, 1,
                                 WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB, WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
                                 WGL_COLOR_BITS_ARB, 32, WGL_DEPTH_BITS_ARB, depth, WGL_STENCIL_BITS_ARB, 8,
                                 WGL_SAMPLE_BUFFERS_ARB, 1, WGL_SAMPLES_ARB, s, 0 };
                    int out = 0; UINT n = 0;
                    if (choose(dc, at, NULL, 1, &out, &n) && n) { result = out; logf_("MSAA %dx: formato %d", s, out); }
                }
            } else logf_("MSAA: driver sem wglChoosePixelFormatARB");
            mc(NULL, NULL);
        }
        if (rc) del(rc);
    }
    ReleaseDC(tw, tdc); DestroyWindow(tw);
    return result;
}

static void note_window_from_dc(HDC dc) {
    HWND w = WindowFromDC(dc);
    if (w && !g_hwnd) { g_hwnd = w; update_rect(); }
}
static int WINAPI h_ChoosePF_common(PFN_ChoosePF real, HDC dc, const PIXELFORMATDESCRIPTOR *p, const char *who) {
    note_window_from_dc(dc);
    logf_("%s: cor %d prof %d stencil %d flags %08lx", who, p ? p->cColorBits : 0, p ? p->cDepthBits : 0, p ? p->cStencilBits : 0, p ? p->dwFlags : 0);
    if (g_msaa_pf) return g_msaa_pf;
    int r = real(dc, p);
    logf_("%s -> formato %d", who, r);
    return r;
}
static int WINAPI h_wglChoosePixelFormat(HDC dc, const PIXELFORMATDESCRIPTOR *p) { return h_ChoosePF_common(r_wglChoosePixelFormat, dc, p, "wglChoosePixelFormat"); }
static int WINAPI h_ChoosePixelFormatGdi(HDC dc, const PIXELFORMATDESCRIPTOR *p) { return h_ChoosePF_common(r_ChoosePixelFormatGdi, dc, p, "ChoosePixelFormat"); }
static BOOL WINAPI h_wglSetPixelFormat(HDC dc, int pf, const PIXELFORMATDESCRIPTOR *p) {
    BOOL r = r_wglSetPixelFormat(dc, pf, p); logf_("wglSetPixelFormat(%d) -> %d", pf, r); return r;
}
static BOOL WINAPI h_SetPixelFormatGdi(HDC dc, int pf, const PIXELFORMATDESCRIPTOR *p) {
    BOOL r = r_SetPixelFormatGdi(dc, pf, p); logf_("SetPixelFormat(%d) -> %d", pf, r); return r;
}
typedef int (WINAPI *PFN_DPF)(HDC, int, UINT, PIXELFORMATDESCRIPTOR *);
static PFN_DPF r_DescribePF_gdi, r_DescribePF_wgl;
static int dpf_common(PFN_DPF real, HDC dc, int pf, UINT n, PIXELFORMATDESCRIPTOR *p) {
    int r = real(dc, pf, n, p);
    static int k; if (p && k++ < 6) logf_("DescribePixelFormat(%d) cor %d prof %d stencil %d flags %08lx -> %d", pf, p->cColorBits, p->cDepthBits, p->cStencilBits, p->dwFlags, r);
    return r;
}
static int WINAPI h_DescribePF_gdi(HDC dc, int pf, UINT n, PIXELFORMATDESCRIPTOR *p) { return dpf_common(r_DescribePF_gdi, dc, pf, n, p); }
static int WINAPI h_DescribePF_wgl(HDC dc, int pf, UINT n, PIXELFORMATDESCRIPTOR *p) { return dpf_common(r_DescribePF_wgl, dc, pf, n, p); }
typedef BOOL (WINAPI *PFN_wglDel)(HGLRC);
static PFN_wglDel r_wglDeleteContext;
static BOOL WINAPI h_wglDeleteContext(HGLRC rc) {
    if (rc && rc == g_hd_rc) { hd_reset(); g_hd_rc = NULL; }   // nomes de textura valem por contexto
    if (rc && rc == g_fbo_rc) {
        if (wglGetCurrentContext() == rc) fbo_free();
        g_fbo = g_res = 0; memset(g_rb, 0, sizeof g_rb);
        g_fbo_ok = 0; g_S = 1; g_fbo_rc = NULL;
    }
    return r_wglDeleteContext(rc);
}
static BOOL WINAPI h_wglMakeCurrent(HDC dc, HGLRC rc) {
    BOOL r = r_wglMakeCurrent(dc, rc);
    if (r) {
        int on = rc && dc && WindowFromDC(dc) == g_hwnd;
        if (on != g_gl_on) { g_gl_on = on; logf_("OpenGL %s na janela", on ? "ativo" : "inativo"); if (on) bg_release(); else present_2d(1); }
        if (!on) { g_S = 1; g_fbo_ok = 0; }
        else if (rc != g_fbo_rc) fbo_setup(rc);     // uma tentativa por contexto (com degraus se falhar)
        else fbo_rebind();
    }
    if (r && rc) {
        static int once;
        note_window_from_dc(dc);
        if (!once++) {
            logf_("GL: %s | %s | %s", (const char *)glGetString(GL_VENDOR), (const char *)glGetString(GL_RENDERER), (const char *)glGetString(GL_VERSION));
        }
        if (g_msaa_pf) glEnable(GL_MULTISAMPLE_ARB);
        if (cfg.vsync >= 0 && r_wglGetProcAddress) {
            PFN_wglSwapInterval si = (void *)r_wglGetProcAddress("wglSwapIntervalEXT");
            if (si) si(cfg.vsync);
        }
    }
    return r;
}


typedef void (APIENTRY *PFN_TexParf)(GLenum, GLenum, GLfloat);
typedef void (APIENTRY *PFN_TexPari)(GLenum, GLenum, GLint);
typedef void (APIENTRY *PFN_TexImg)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
typedef void (APIENTRY *PFN_TexSub)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *);
typedef void (APIENTRY *PFN_Buf)(GLenum);
static PFN_TexParf r_TexParameterf; static PFN_TexPari r_TexParameteri;
static PFN_TexImg r_TexImage2D; static PFN_TexSub r_TexSubImage2D;
static PFN_Buf r_DrawBuffer, r_ReadBuffer;
#define GL_LINEAR_MIPMAP_LINEAR_ 0x2703
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
static int tex_min_param(GLenum pname, GLint *v) {
    if (cfg.mipmaps && pname == GL_TEXTURE_MIN_FILTER && *v == GL_LINEAR) {
        load_gl_ext();
        if (!x_GenerateMipmap) return 0;           // sem gerar os níveis a textura ficaria preta: mantém o filtro
        *v = GL_LINEAR_MIPMAP_LINEAR_; return 1;
    }
    return 0;
}
static void tex_aniso(GLenum target) {
    if (cfg.aniso < 2 || !r_TexParameterf || g_max_aniso < 2) return;
    r_TexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, cfg.aniso > g_max_aniso ? g_max_aniso : (GLfloat)cfg.aniso);
}
static void gen_mips(GLenum t);
// filtro trocado para mipmap numa textura que já tem imagem: gera os níveis agora (senão ela fica incompleta = preta)
static void mips_if_loaded(GLenum t) {
    if (t != GL_TEXTURE_2D) return;
    GLint w = 0; glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    if (w > 0) gen_mips(t);
}
static void APIENTRY h_TexParameterf(GLenum t, GLenum pn, GLfloat v) {
    GLint iv = (GLint)v;
    if (pn == GL_TEXTURE_MIN_FILTER && tex_min_param(pn, &iv)) { r_TexParameterf(t, pn, (GLfloat)iv); tex_aniso(t); mips_if_loaded(t); return; }
    r_TexParameterf(t, pn, v);
}
static void APIENTRY h_TexParameteri(GLenum t, GLenum pn, GLint v) {
    if (pn == GL_TEXTURE_MIN_FILTER && tex_min_param(pn, &v)) { r_TexParameteri(t, pn, v); tex_aniso(t); mips_if_loaded(t); return; }
    r_TexParameteri(t, pn, v);
}
static void gen_mips(GLenum t) {
    if (!cfg.mipmaps || t != GL_TEXTURE_2D) return;
    load_gl_ext();
    if (!x_GenerateMipmap) return;
    GLint mf = 0; glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &mf);
    if (mf == GL_LINEAR_MIPMAP_LINEAR_) x_GenerateMipmap(GL_TEXTURE_2D);
}

// ---------------------------------------------------------------- texturas: despejo (debug) e versões HD (xBR 4x)
// Letras, HUD e desenhos 2D são pixel art pequena (64x64, 128x128...). As da lista (por hash do conteúdo, que é o
// mesmo em qualquer cópia do jogo) são redesenhadas em 4x com xBR (detecção de bordas com mistura) quando o jogo as
// carrega: contornos lisos em vez de "escadinha". Nenhuma imagem do jogo sai do PC: o mod refaz isso na hora.
#define GL_BGRA_     0x80E1
#define GL_BGR_      0x80E0
#define GL_US_4444   0x8033
#define GL_US_5551   0x8034
#define GL_US_565    0x8363
#define GL_US_4444R  0x8365
#define GL_US_1555R  0x8366
#define GL_UNPACK_ROW_LENGTH_  0x0CF2
#define GL_UNPACK_SKIP_ROWS_   0x0CF3
#define GL_UNPACK_SKIP_PIXELS_ 0x0CF4
#define GL_UNPACK_ALIGNMENT_   0x0CF5
static int tex_bpp(GLenum f, GLenum ty) {
    if (ty == GL_UNSIGNED_BYTE) {
        if (f == GL_RGBA || f == GL_BGRA_) return 4;
        if (f == GL_RGB || f == GL_BGR_) return 3;
        if (f == GL_LUMINANCE_ALPHA) return 2;
        if (f == GL_LUMINANCE || f == GL_ALPHA) return 1;
        return 0;
    }
    if (ty == GL_US_4444 || ty == GL_US_5551 || ty == GL_US_565 || ty == GL_US_4444R || ty == GL_US_1555R) return 2;
    return 0;
}
// converte para RGBA8 (bytes r,g,b,a); transparente vira 0 (comparações exatas e mistura sem franja escura)
static int tex_to_rgba(int w, int h, GLenum f, GLenum ty, const void *px, DWORD *o) {
    const BYTE *b = px; const WORD *u = px; size_t n = (size_t)w * h;
    #define X5(v) ((((v) & 31) << 3) | (((v) & 31) >> 2))
    #define X4(v) (((v) & 15) * 17)
    #define X6(v) ((((v) & 63) << 2) | (((v) & 63) >> 4))
    #define PK(r, g, bb, a) ((DWORD)(r) | (DWORD)(g) << 8 | (DWORD)(bb) << 16 | (DWORD)(a) << 24)
    if (ty == GL_UNSIGNED_BYTE && f == GL_RGBA) memcpy(o, b, n * 4);
    else if (ty == GL_UNSIGNED_BYTE && f == GL_BGRA_) for (size_t i = 0; i < n; i++) o[i] = PK(b[4*i+2], b[4*i+1], b[4*i], b[4*i+3]);
    else if (ty == GL_UNSIGNED_BYTE && f == GL_RGB) for (size_t i = 0; i < n; i++) o[i] = PK(b[3*i], b[3*i+1], b[3*i+2], 255);
    else if (ty == GL_UNSIGNED_BYTE && f == GL_BGR_) for (size_t i = 0; i < n; i++) o[i] = PK(b[3*i+2], b[3*i+1], b[3*i], 255);
    else if (ty == GL_UNSIGNED_BYTE && f == GL_LUMINANCE_ALPHA) for (size_t i = 0; i < n; i++) o[i] = PK(b[2*i], b[2*i], b[2*i], b[2*i+1]);
    else if (ty == GL_UNSIGNED_BYTE && f == GL_LUMINANCE) for (size_t i = 0; i < n; i++) o[i] = PK(b[i], b[i], b[i], 255);
    else if (ty == GL_UNSIGNED_BYTE && f == GL_ALPHA) for (size_t i = 0; i < n; i++) o[i] = PK(255, 255, 255, b[i]);
    else if (ty == GL_US_5551 && f == GL_RGBA) for (size_t i = 0; i < n; i++) { WORD v = u[i]; o[i] = PK(X5(v >> 11), X5(v >> 6), X5(v >> 1), (v & 1) * 255); }
    else if (ty == GL_US_1555R && f == GL_BGRA_) for (size_t i = 0; i < n; i++) { WORD v = u[i]; o[i] = PK(X5(v >> 10), X5(v >> 5), X5(v), (v >> 15) * 255); }
    else if (ty == GL_US_4444 && f == GL_RGBA) for (size_t i = 0; i < n; i++) { WORD v = u[i]; o[i] = PK(X4(v >> 12), X4(v >> 8), X4(v >> 4), X4(v)); }
    else if (ty == GL_US_4444R && f == GL_BGRA_) for (size_t i = 0; i < n; i++) { WORD v = u[i]; o[i] = PK(X4(v >> 8), X4(v >> 4), X4(v), X4(v >> 12)); }
    else if (ty == GL_US_565 && f == GL_RGB) for (size_t i = 0; i < n; i++) { WORD v = u[i]; o[i] = PK(X5(v >> 11), X6(v >> 5), X5(v), 255); }
    else return 0;
    for (size_t i = 0; i < n; i++) if (!(o[i] >> 24)) o[i] = 0;
    return 1;
}
// Assinatura da textura que não depende das cores finais: cada pixel entra como (alfa, número da cor na ordem em
// que ela aparece). O menu Vídeo do jogo muda matiz/saturação/brilho das paletas ao carregar, então as cores mudam
// de um jogador para outro, mas o desenho (quais pixels têm a mesma cor) e a transparência continuam iguais.
static unsigned long long tex_lhash(const DWORD *p, int w, int h) {
    size_t n = (size_t)w * h, cap = 64;
    while (cap < n * 2) cap <<= 1;
    DWORD *key = malloc(cap * 4), *val = malloc(cap * 4);
    unsigned long long x = 1469598103934665603ULL;
    x = (x ^ (unsigned)w) * 1099511628211ULL; x = (x ^ (unsigned)h) * 1099511628211ULL;
    x = (x ^ 0x4c42414cu) * 1099511628211ULL;
    if (!key || !val) { free(key); free(val); return 0; }
    memset(val, 0xff, cap * 4);
    DWORD nl = 0;
    for (size_t i = 0; i < n; i++) {
        DWORD c = p[i]; if (!(c >> 24)) c = 0;
        size_t k = (c * 2654435761u) & (cap - 1);
        while (val[k] != 0xffffffffu && key[k] != c) k = (k + 1) & (cap - 1);
        if (val[k] == 0xffffffffu) { key[k] = c; val[k] = nl++; }
        x = (x ^ ((c & 0xff000000u) | (val[k] & 0xffffffu))) * 1099511628211ULL;
    }
    free(key); free(val);
    return x;
}

// ---- xBR 4x (algoritmo de Hyllian: regras de borda com pesos em YUV; implementação própria)
static int xd(DWORD p, DWORD q) {
    int dr = (int)(p & 255) - (int)(q & 255), dg = (int)(p >> 8 & 255) - (int)(q >> 8 & 255), db = (int)(p >> 16 & 255) - (int)(q >> 16 & 255);
    int y = 299 * dr + 587 * dg + 114 * db, u = -169 * dr - 331 * dg + 500 * db, v = 500 * dr - 419 * dg - 81 * db;
    return (48 * abs(y) + 7 * abs(u) + 6 * abs(v)) / 1000 + 48 * abs((int)(p >> 24) - (int)(q >> 24));
}
#define XEQ(a, b) (xd(a, b) < 155)
// mistura w/256 de b sobre a, com alfa pré-multiplicado
static DWORD xmix(DWORD a, DWORD b, int w) {
    int aa = a >> 24, ba = b >> 24, oa = (aa * (256 - w) + ba * w) >> 8;
    if (oa <= 0) return 0;
    int r = ((int)(a & 255) * aa * (256 - w) + (int)(b & 255) * ba * w) / (oa * 256);
    int g = ((int)(a >> 8 & 255) * aa * (256 - w) + (int)(b >> 8 & 255) * ba * w) / (oa * 256);
    int c = ((int)(a >> 16 & 255) * aa * (256 - w) + (int)(b >> 16 & 255) * ba * w) / (oa * 256);
    if (r > 255) r = 255; if (g > 255) g = 255; if (c > 255) c = 255;
    return PK(r, g, c, oa);
}
static void xbr4x(const DWORD *src, int w, int h, DWORD *dst) {
    #define SP(x, y) src[((y) < 0 ? 0 : (y) >= h ? h - 1 : (y)) * w + ((x) < 0 ? 0 : (x) >= w ? w - 1 : (x))]
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        DWORD E = SP(x, y), o[16];
        for (int k = 0; k < 16; k++) o[k] = E;
        for (int c = 0; c < 4; c++) {                 // os 4 cantos por espelhamento do canto inferior direito
            int sx = (c & 1) ? -1 : 1, sy = (c & 2) ? -1 : 1;
            #define Q(dx, dy) SP(x + sx * (dx), y + sy * (dy))
            #define O(ox, oy) o[((sy > 0) ? (oy) : 3 - (oy)) * 4 + ((sx > 0) ? (ox) : 3 - (ox))]
            DWORD F = Q(1, 0), H = Q(0, 1);
            if (E == H || E == F) continue;
            DWORD I = Q(1, 1), C = Q(1, -1), G = Q(-1, 1), B = Q(0, -1), D = Q(-1, 0);
            DWORD F4 = Q(2, 0), I4 = Q(2, 1), H5 = Q(0, 2), I5 = Q(1, 2);
            int e = xd(E, C) + xd(E, G) + xd(I, H5) + xd(I, F4) + 4 * xd(H, F);
            int i = xd(H, D) + xd(H, I5) + xd(F, I4) + xd(F, B) + 4 * xd(E, I);
            DWORD px = xd(E, F) <= xd(E, H) ? F : H;
            if (e < i && ((!XEQ(F, B) && !XEQ(H, D)) || (XEQ(E, I) && !XEQ(F, I4) && !XEQ(H, I5)) || XEQ(E, G) || XEQ(E, C))) {
                int ke = xd(F, G), ki = xd(H, C);
                int ex2 = E != C && B != C, ex3 = E != G && D != G;
                if (2 * ke <= ki && ex3 && ke >= 2 * ki && ex2) {          // borda rasa e íngreme ao mesmo tempo
                    O(1, 3) = xmix(O(1, 3), px, 192); O(0, 3) = xmix(O(0, 3), px, 64);
                    O(3, 3) = O(2, 3) = O(3, 2) = px;
                    O(2, 2) = O(3, 0) = O(0, 3); O(3, 1) = O(1, 3);
                } else if (2 * ke <= ki && ex3) {                           // borda rasa
                    O(3, 3) = O(2, 3) = px;
                    O(3, 2) = xmix(O(3, 2), px, 192); O(1, 3) = xmix(O(1, 3), px, 192);
                    O(2, 2) = xmix(O(2, 2), px, 64); O(0, 3) = xmix(O(0, 3), px, 64);
                } else if (ke >= 2 * ki && ex2) {                           // borda íngreme
                    O(3, 3) = O(3, 2) = px;
                    O(2, 3) = xmix(O(2, 3), px, 192); O(3, 1) = xmix(O(3, 1), px, 192);
                    O(2, 2) = xmix(O(2, 2), px, 64); O(3, 0) = xmix(O(3, 0), px, 64);
                } else {                                                    // diagonal
                    O(3, 3) = px;
                    O(3, 2) = xmix(O(3, 2), px, 128); O(2, 3) = xmix(O(2, 3), px, 128);
                }
            } else if (e <= i) {
                O(3, 3) = xmix(O(3, 3), px, 128);
            }
            #undef Q
            #undef O
        }
        for (int k = 0; k < 16; k++) dst[(size_t)(y * 4 + k / 4) * (w * 4) + x * 4 + k % 4] = o[k];
    }
    #undef SP
}
// cor dos pixels transparentes = média dos vizinhos visíveis (o filtro linear não puxa preto para as bordas)
static void tex_dilate(DWORD *p, int w, int h) {
    for (int pass = 0; pass < 2; pass++)
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
            DWORD *q = &p[(size_t)y * w + x];
            if (*q >> 24) continue;
            int r = 0, g = 0, b = 0, n = 0;
            for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
                int xx = x + dx, yy = y + dy;
                if (xx < 0 || yy < 0 || xx >= w || yy >= h) continue;
                DWORD v = p[(size_t)yy * w + xx];
                if (!(v >> 24) && !(v & 0xffffff)) continue;
                r += v & 255; g += v >> 8 & 255; b += v >> 16 & 255; n++;
            }
            if (n) *q = PK(r / n, g / n, b / n, 0);
        }
}


// ---- ícones do Xbox desenhados pelo mod, no estilo dos ícones do PS1 que sobraram na fonte (botão escuro com aro e
// símbolo coloridos). Os do PS1 (✕ ○ □ △ L1 R1 "START") nunca aparecem na versão de PC e viram A B X Y LB RB BACK;
// as teclas Enter e Esc viram A e Menu só enquanto o controle está conectado.
static const char *k_btn_name[BTN_N] = { "A", "B", "X", "Y", "LB", "RB", "LT", "RT", "START", "BACK" };
static volatile int g_pad_on;   // há controle XInput conectado
static volatile DWORD g_f4_prompt_tick;   // última vez que uma dica pediu o nome do F4 ("menu anterior" na tela)
static volatile LONG g_f4_prompt_n;       // diagnóstico: quantas vezes
static const struct { unsigned long long hash; int btn; int cond; } k_icon_tex[] = {
    { 0x079f890026d2ae47ULL, BTN_A, 0 }, { 0x72807a011ac598afULL, BTN_B, 0 }, { 0x17d83bb848a1fdbdULL, BTN_X, 0 },
    { 0xa145955d605231e4ULL, BTN_Y, 0 }, { 0xb9e89ce9a01b0e7cULL, BTN_LB, 0 }, { 0x6098e087052818c0ULL, BTN_RB, 0 },
    { 0xc195fc291c0c4324ULL, BTN_BACK, 0 },
    { 0x0f3e3e1a4d624fcaULL, -1, 1 },          // tecla Enter -> botão que faz o Enter (A, ou B com swap_ab)
    { 0x1d1b9916169e0d2fULL, BTN_START, 1 },   // tecla Esc -> Menu
};
static HBITMAP icon_dib(HDC dc, int w, int h, DWORD **bits) {
    BITMAPINFO bi; memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize = sizeof bi.bmiHeader; bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    return CreateDIBSection(dc, &bi, DIB_RGB_COLORS, (void **)bits, NULL, 0);
}
// texto em máscara (só a tinta, recortada)
static float *text_ink(const char *str, int fh, int *ow, int *oh) {
    HDC dc = CreateCompatibleDC(NULL);
    HFONT f = CreateFontA(-fh, 0, 0, 0, FW_HEAVY, 0, 0, 0, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, "Arial Black");
    HGDIOBJ of = SelectObject(dc, f);
    SIZE sz; GetTextExtentPoint32A(dc, str, (int)strlen(str), &sz);
    int W = sz.cx + fh, H = sz.cy + fh; DWORD *bits = NULL;
    HBITMAP bmp = icon_dib(dc, W, H, &bits);
    float *m = NULL;
    if (bmp && bits) {
        HGDIOBJ ob = SelectObject(dc, bmp);
        memset(bits, 0, (size_t)W * H * 4);
        SetTextColor(dc, RGB(255, 255, 255)); SetBkMode(dc, TRANSPARENT);
        TextOutA(dc, fh / 2, fh / 2, str, (int)strlen(str));
        GdiFlush();
        int x0 = W, y0 = H, x1 = -1, y1 = -1;
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if ((bits[(size_t)y * W + x] & 255) > 6) { if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y; }
        if (x1 >= x0) {
            *ow = x1 - x0 + 1; *oh = y1 - y0 + 1;
            m = malloc((size_t)*ow * *oh * sizeof *m);
            if (m) for (int y = 0; y < *oh; y++) for (int x = 0; x < *ow; x++) m[(size_t)y * *ow + x] = (bits[(size_t)(y0 + y) * W + x0 + x] & 255) / 255.0f;
        }
        SelectObject(dc, ob); DeleteObject(bmp);
    }
    SelectObject(dc, of); DeleteObject(f); DeleteDC(dc);
    return m;
}
typedef struct { float r, g, b, a; } PX4;
static void px_over(PX4 *d, float r, float g, float b, float a) {   // "sobre", cor não pré-multiplicada na entrada
    d->r = r * a + d->r * (1 - a); d->g = g * a + d->g * (1 - a); d->b = b * a + d->b * (1 - a); d->a = a + d->a * (1 - a);
}
// texto na caixa (x0..x1, y0..y1) do canvas; escala para caber (pode estreitar até 60%); align 0 = centro, 1 = esquerda
static void canvas_text(PX4 *cv, int CW, int CH, const char *str, float x0, float y0, float x1, float y1, int align,
                        float r, float g, float b, float shx, float shy) {
    float bw = x1 - x0, bh = y1 - y0;
    int ow = 0, oh = 0; float *m = text_ink(str, (int)(bh * 1.4f), &ow, &oh);
    if (!m) return;
    float sy = bh / oh, sx = sy;
    if (ow * sx > bw) { sx = bw / ow; if (sx < sy * 0.6f) { sy = sx / 0.6f; } }
    float tw = ow * sx, th = oh * sy;
    float ox = align ? x0 : x0 + (bw - tw) / 2, oy = y0 + (bh - th) / 2;
    for (int pass = (shx > 0 || shy > 0) ? 0 : 1; pass < 2; pass++) {
        float dx = pass == 0 ? shx : 0, dy = pass == 0 ? shy : 0;
        int X0 = (int)(ox + dx), Y0 = (int)(oy + dy), X1 = (int)(ox + dx + tw) + 1, Y1 = (int)(oy + dy + th) + 1;
        for (int Y = Y0; Y < Y1; Y++) for (int X = X0; X < X1; X++) {
            if (X < 0 || Y < 0 || X >= CW || Y >= CH) continue;
            float u = (X + 0.5f - ox - dx) / sx, v = (Y + 0.5f - oy - dy) / sy;   // amostra bilinear da máscara
            if (u < 0 || v < 0 || u > ow - 1 || v > oh - 1) continue;
            int ui = (int)u, vi = (int)v; float fu = u - ui, fv = v - vi;
            int u1 = ui + 1 < ow ? ui + 1 : ui, v1 = vi + 1 < oh ? vi + 1 : vi;
            float a = (m[(size_t)vi * ow + ui] * (1 - fu) + m[(size_t)vi * ow + u1] * fu) * (1 - fv) +
                      (m[(size_t)v1 * ow + ui] * (1 - fu) + m[(size_t)v1 * ow + u1] * fu) * fv;
            if (a <= 0.003f) continue;
            if (pass == 0) px_over(&cv[(size_t)Y * CW + X], 0, 0, 0, a);
            else px_over(&cv[(size_t)Y * CW + X], r, g, b, a);
        }
    }
    free(m);
}
// botão redondo: aro preto fino, aro colorido, miolo escuro com degradê
static void canvas_button(PX4 *cv, int CW, int CH, float cx, float cy, float rad, float rr, float rg, float rb) {
    for (int y = (int)(cy - rad) - 1; y <= (int)(cy + rad) + 1; y++) for (int x = (int)(cx - rad) - 1; x <= (int)(cx + rad) + 1; x++) {
        if (x < 0 || y < 0 || x >= CW || y >= CH) continue;
        float dx = x + 0.5f - cx, dy = y + 0.5f - cy, d = sqrtf(dx * dx + dy * dy);
        if (d > rad) continue;
        PX4 *p = &cv[(size_t)y * CW + x];
        if (d > rad * 0.92f) px_over(p, 0, 0, 0, 1);
        else if (d > rad * 0.79f) px_over(p, rr, rg, rb, 1);
        else { float t = d / (rad * 0.79f), v = 0.20f - 0.11f * t, hl = dy < 0 ? 0.06f * (-dy / rad) : 0; px_over(p, v + hl, v + hl, v + 0.02f + hl, 1); }
    }
}
static void btn_color(int b, float *r, float *g, float *bb) {
    switch (b) {
    case BTN_A: *r = 0.45f; *g = 0.82f; *bb = 0.22f; break;
    case BTN_B: *r = 0.95f; *g = 0.27f; *bb = 0.22f; break;
    case BTN_X: *r = 0.25f; *g = 0.58f; *bb = 1.00f; break;
    case BTN_Y: *r = 1.00f; *g = 0.80f; *bb = 0.15f; break;
    default:    *r = 0.82f; *g = 0.82f; *bb = 0.84f; break;
    }
}
// Desenha o ícone na textura HD (W x H = 4x a original). A fonte é desenhada no espaço 512x240 do PS1, em que cada
// linha da textura aparece 1,6x mais alta que uma coluna é larga: o desenho é feito num canvas com essa proporção e
// depois comprimido na vertical. Posições em texels da textura original (de cima para baixo), como as letras: corpo
// das maiúsculas nas linhas 7..14.
// retângulo (contorno de espessura t, e miolo opcional) no canvas
static void canvas_rect(PX4 *cv, int CW, int CH, float x0, float y0, float x1, float y1, float t, float fill) {
    for (int y = (int)y0; y <= (int)y1; y++) for (int x = (int)x0; x <= (int)x1; x++) {
        if (x < 0 || y < 0 || x >= CW || y >= CH) continue;
        float px = x + 0.5f, py = y + 0.5f;
        if (px < x0 || px > x1 || py < y0 || py > y1) continue;
        int edge = px < x0 + t || px > x1 - t || py < y0 + t || py > y1 - t;
        if (edge) px_over(&cv[(size_t)y * CW + x], 0.95f, 0.95f, 0.95f, 1);
        else if (fill >= 0) px_over(&cv[(size_t)y * CW + x], fill, fill, fill + 0.02f, 1);
    }
}
static void icon_render(int btn, int w, int h, DWORD *out) {
    const int SS = 4, W = w * 4, H = h * 4;
    ICON_GEOM geo; int round = pad_icon_geom(btn, w, h, &geo);
    const float AY = round ? geo.ay : 1.6f;              // ícones redondos: proporção do quadro em que o jogo os desenha
    int CW = W * SS, CH = (int)(H * SS * AY + 0.5f);
    PX4 *cv = calloc((size_t)CW * CH, sizeof *cv);
    if (!cv) { memset(out, 0, (size_t)W * H * 4); return; }
    #define TX(t) ((t) * 4.0f * SS)
    #define TY(t) ((t) * 4.0f * SS * AY)
    float r, g, b; btn_color(btn, &r, &g, &b);
    if (round) {                                                       // Ⓐ Ⓑ Ⓧ Ⓨ e o View (BACK)
        float cx = TX(geo.cx), cy = TY(geo.cy), rad = TX(geo.rad);
        canvas_button(cv, CW, CH, cx, cy, rad, r, g, b);
        if (btn == BTN_BACK) {                                         // ⧉: dois retângulos sobrepostos
            float t = rad * 0.085f;
            canvas_rect(cv, CW, CH, cx - rad * 0.42f, cy - rad * 0.38f, cx + rad * 0.16f, cy + rad * 0.10f, t, -1);
            canvas_rect(cv, CW, CH, cx - rad * 0.16f, cy - rad * 0.10f, cx + rad * 0.42f, cy + rad * 0.38f, t, 0.16f);
        } else if (btn == BTN_START) {                                 // ≡
            for (int k = -1; k <= 1; k++) {
                float yy = cy + k * rad * 0.27f, hh = rad * 0.075f, ww = rad * 0.42f;
                for (int y = (int)(yy - hh); y <= (int)(yy + hh); y++) for (int x = (int)(cx - ww); x <= (int)(cx + ww); x++)
                    if (x >= 0 && y >= 0 && x < CW && y < CH) px_over(&cv[(size_t)y * CW + x], 0.95f, 0.95f, 0.95f, 1);
            }
        } else {
            const char *t = btn == BTN_A ? "A" : btn == BTN_B ? "B" : btn == BTN_X ? "X" : "Y";
            float lh = rad * 0.95f;
            canvas_text(cv, CW, CH, t, cx - lh, cy - lh * 0.55f, cx + lh, cy + lh * 0.55f, 0, r, g, b, 0, 0);
        }
    } else if (btn == BTN_LB || btn == BTN_RB || btn == BTN_BACK) {        // texto branco com sombra, como "L1" e "START"
        const char *t = btn == BTN_LB ? "LB" : btn == BTN_RB ? "RB" : "BACK";
        float x1 = btn == BTN_BACK ? TX(50) : TX(13.5f);
        canvas_text(cv, CW, CH, t, TX(0), TY(7), x1, TY(14.2f), btn == BTN_BACK, 1, 1, 1, TX(0.9f), TY(0.9f));
    } else {
        float cx, cy, rad;
        if (h >= 32) { cx = TX(8.5f); cy = TY(10.8f); rad = TX(8.2f); }          // dentro da linha, como uma letra
        else { cx = TX(w / 2.0f); cy = TY(7.5f); rad = TY(7.4f); if (rad > TX(w / 2.0f)) rad = TX(w / 2.0f); }   // Enter/Esc
        canvas_button(cv, CW, CH, cx, cy, rad, r, g, b);
        if (btn == BTN_START) {                                        // ≡
            for (int k = -1; k <= 1; k++) {
                float yy = cy + k * rad * 0.27f, hh = rad * 0.075f, ww = rad * 0.42f;
                for (int y = (int)(yy - hh); y <= (int)(yy + hh); y++) for (int x = (int)(cx - ww); x <= (int)(cx + ww); x++)
                    if (x >= 0 && y >= 0 && x < CW && y < CH) px_over(&cv[(size_t)y * CW + x], 0.95f, 0.95f, 0.95f, 1);
            }
        } else {
            const char *t = btn == BTN_A ? "A" : btn == BTN_B ? "B" : btn == BTN_X ? "X" : "Y";
            float lh = rad * 0.95f;
            canvas_text(cv, CW, CH, t, cx - lh, cy - lh * 0.55f, cx + lh, cy + lh * 0.55f, 0, r, g, b, 0, 0);
        }
    }
    #undef TX
    #undef TY
    for (int y = 0; y < H; y++) {                                       // reduz e volta para o espaço da textura
        int ya = (int)(y * SS * AY), yb = (int)((y + 1) * SS * AY); if (yb > CH) yb = CH; if (yb <= ya) yb = ya + 1;
        for (int x = 0; x < W; x++) {
            float pr = 0, pg = 0, pb = 0, pa = 0; int n = 0;
            for (int j = ya; j < yb; j++) for (int i = 0; i < SS; i++) { PX4 *q = &cv[(size_t)j * CW + x * SS + i]; pr += q->r; pg += q->g; pb += q->b; pa += q->a; n++; }
            pa /= n;
            DWORD o = 0;
            if (pa > 0.002f) {
                int R = (int)(pr / n / pa * 255 + 0.5f), G = (int)(pg / n / pa * 255 + 0.5f), B = (int)(pb / n / pa * 255 + 0.5f), A = (int)(pa * 255 + 0.5f);
                if (R > 255) R = 255; if (G > 255) G = 255; if (B > 255) B = 255; if (A > 255) A = 255;
                o = PK(R, G, B, A);
            }
            out[(size_t)(H - 1 - y) * W + x] = o;                     // a textura do GL fica de baixo para cima
        }
    }
    free(cv);
    tex_dilate(out, W, H);
}
static int icon_index(unsigned long long h) { for (size_t i = 0; i < sizeof k_icon_tex / sizeof *k_icon_tex; i++) if (k_icon_tex[i].hash == h) return (int)i; return -1; }
// texturas que trocam com o controle (Enter / Esc): nome do GL e as duas versões
static struct { GLuint name; int w, h; GLint ifmt; DWORD *orig, *icon; } g_swap[8]; static int g_swap_n, g_swap_state = -1;
static int enter_btn(void) { return cfg.swap_ab ? BTN_B : BTN_A; }
static GLuint g_hd_names[512]; static int g_hd_names_n;
static void swap_forget(GLuint name) {
    for (int i = 0; i < g_swap_n; i++) if (g_swap[i].name == name) { free(g_swap[i].orig); free(g_swap[i].icon); g_swap[i] = g_swap[--g_swap_n]; return; }
}

// controle conectou/desconectou: as texturas Enter/Esc trocam de imagem (no thread do GL, depois do quadro)
static void upload_rgba(GLenum t, GLint ifmt, int W, int H, const DWORD *p);
static void icons_sync(void) {
    if (!g_swap_n || g_swap_state == g_pad_on) return;
    {
        GLint cur = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &cur);
        for (int i = 0; i < g_swap_n; i++) {
            glBindTexture(GL_TEXTURE_2D, g_swap[i].name);
            upload_rgba(GL_TEXTURE_2D, g_swap[i].ifmt, g_swap[i].w, g_swap[i].h, g_pad_on ? g_swap[i].icon : g_swap[i].orig);
            gen_mips(GL_TEXTURE_2D);
        }
        glBindTexture(GL_TEXTURE_2D, (GLuint)cur);
        g_swap_state = g_pad_on;
        logf_("controle %s: icones Enter/Esc %s", g_pad_on ? "conectado" : "desconectado", g_pad_on ? "do Xbox" : "do teclado");
    }
}

// lista de texturas redesenhadas (hash do conteúdo RGBA): embutida + bin\bblit2fix_hd.txt (uma por linha, hex)
// fonte do jogo (todos os caracteres e símbolos) e HUD/pausa (números, silhuetas, barras, espelho, medalhas, claquete,
// cenoura): 143 texturas identificadas pelo despejo da versão PT-BR (assinatura de desenho, ver tex_lhash)
static const unsigned long long k_hd_builtin[] = {
    0x0301d9c626521068ULL, 0x053433e4b2ec2089ULL, 0x08757f739ab6f819ULL, 0x0a533baf0b80483eULL,
    0x0a9945d47d1e74dcULL, 0x0c2d3c4a4be7c8c8ULL, 0x0cd96f880397140eULL, 0x1133637f906822d7ULL,
    0x115f6b07e5b398cfULL, 0x11e7d4253dc0f27dULL, 0x1244c0c5ad1cf26eULL, 0x12564ad76d9f35ecULL,
    0x139d09e0a1221af1ULL, 0x16463c6abc682e5cULL, 0x1773bbcebaff6d1fULL, 0x1831bfeeaf582b53ULL,
    0x1cdb8f624aa4c960ULL, 0x1f549588991b3e03ULL, 0x20d674438ca46896ULL, 0x2263ff8efc8e2c56ULL,
    0x22fc0536c8641f08ULL, 0x2394d92b3345ca55ULL, 0x252b1b44d340ce34ULL, 0x277a86b012af02b5ULL,
    0x283fc4277130b429ULL, 0x287c486ff2ba69e6ULL, 0x28b75e2c6863a939ULL, 0x2af594735f0939b3ULL,
    0x324e7c9d068e0a71ULL, 0x3390e8fa8e7291ceULL, 0x34b05d1a3cbbb422ULL, 0x35c2c5150b74b0eaULL,
    0x37549199b3ed3353ULL, 0x37ba6833ec45134aULL, 0x3976d58b79b04030ULL, 0x3abd3f8592615d20ULL,
    0x3e3984257d7f505aULL, 0x3ff28889dc2f9625ULL, 0x455b345b63177fb0ULL, 0x48b3a3ee49d533c1ULL,
    0x4c1b97f6b6f77249ULL, 0x4c6bdfe92ed18a2dULL, 0x4d131c8ecf566382ULL, 0x4e782ae4a5e5aa16ULL,
    0x4f1dd2973af8b13eULL, 0x4f9ad6b9e366c682ULL, 0x52404156074102eeULL, 0x550cd72b17915a6fULL,
    0x55c077bd77cc11eeULL, 0x55d13c075fbc8ad2ULL, 0x564c98084158291fULL, 0x56793acc501f39b4ULL,
    0x56b65736a62fa453ULL, 0x593232224dc3f8c3ULL, 0x59f8e33e8c711edeULL, 0x5e0ceaf077778bb8ULL,
    0x5e5ef1e7c52a58a8ULL, 0x5ef749858b0c5889ULL, 0x6153c0570df99ae9ULL, 0x64da2ddc2dc0c20fULL,
    0x66085af0d71d90d2ULL, 0x6da516a4789678d7ULL, 0x7052f27c1a3671b8ULL, 0x71aa86f8f34baaa3ULL,
    0x7273c113187a5441ULL, 0x75483015d52c4802ULL, 0x75cc4f48bee38bacULL, 0x76e79fa1ab5f0475ULL,
    0x7759b001f2a635e1ULL, 0x78d8db723ff9d23fULL, 0x7a3aab6ff9988113ULL, 0x7b922a218f546b7fULL,
    0x7bfb007a033fcfcbULL, 0x7d8d41fe7a61e850ULL, 0x81186b738d5b2629ULL, 0x814438235e46e7fcULL,
    0x839627140482a809ULL, 0x83ccf82caee5e12aULL, 0x86120f68a540b8c0ULL, 0x8812b8ce92add9b7ULL,
    0x88d23ca06bc73a6cULL, 0x8a24a8a3dc058391ULL, 0x8a6f24338165a487ULL, 0x8bba8b66fc8286dbULL,
    0x8e5c0efda9ae0b24ULL, 0x8fd84f5e07fadca0ULL, 0x90f156bd20a70ee7ULL, 0x92580ff79274a8f4ULL,
    0x9353e13cd13d3d26ULL, 0x9ab410dd2ec32d95ULL, 0x9ad00533accb12b2ULL, 0x9e67d847c1b96678ULL,
    0x9f99c3526e3e26aeULL, 0x9fceb0741c842792ULL, 0xa0239ce51e365752ULL, 0xa1099bbeaffa0693ULL,
    0xa1daa4b73abbffd1ULL, 0xa4c0af7ed2b2c95bULL, 0xa938f8c51a176782ULL, 0xab7e49f8e5c9978aULL,
    0xac249a5eceea0e7eULL, 0xb4f2a4ef7ad79896ULL, 0xb9e5017491cd6bd5ULL, 0xba8bc21808321dd7ULL,
    0xbef4c66e2a96838bULL, 0xc0a22ec388f2bd23ULL, 0xc0bd46e6152bc4baULL, 0xc410d9cf05561cb7ULL,
    0xc50ebc7bed24c0f8ULL, 0xc5363aa3dd02343aULL, 0xc6622c013d5cb515ULL, 0xc71b14dfca3365ceULL,
    0xcc5dc467b1f36752ULL, 0xcdfbd6edf13ed09eULL, 0xce70b68de0f583d4ULL, 0xcef62c3b523bca08ULL,
    0xd1cd3b1eed4c0abeULL, 0xd33b8759d616831cULL, 0xd36d78d6e72fd9c2ULL, 0xd3727e38ef56f24eULL,
    0xd60f246eed512e05ULL, 0xd61e820769ad6ef9ULL, 0xd834c77f430011b4ULL, 0xd98e2397ab08ae38ULL,
    0xdb0a957bf8a45c7eULL, 0xdb2316d9be39d826ULL, 0xdb893ad4aed1e0ddULL, 0xdbae5673e5fc5661ULL,
    0xdcd166d99a4b576dULL, 0xde79efc1187854cbULL, 0xdf776ca73b3f49baULL, 0xe15c4b1849d82390ULL,
    0xe3c0d45605988aeaULL, 0xe72f6db3dc68fc82ULL, 0xe77f01975c84d402ULL, 0xe89d91b7a472ef99ULL,
    0xe9a22e515cd4c17eULL, 0xec486b43c48aa0b3ULL, 0xf07de88f84ea054aULL, 0xf4070c09d2c559dcULL,
    0xf7844ad6589cfc2bULL, 0xf7eeee695f2cadd8ULL, 0xf9116385d1b888e3ULL,
};
static unsigned long long *g_hd; static int g_hd_n, g_hd_cap;
static void hd_add(unsigned long long h) {
    if (!h) return;
    for (int i = 0; i < g_hd_n; i++) if (g_hd[i] == h) return;
    if (g_hd_n == g_hd_cap) { int nc = g_hd_cap ? g_hd_cap * 2 : 256; unsigned long long *n = realloc(g_hd, nc * sizeof *n); if (!n) return; g_hd = n; g_hd_cap = nc; }
    g_hd[g_hd_n++] = h;
}
static int hd_has(unsigned long long h) { for (int i = 0; i < g_hd_n; i++) if (g_hd[i] == h) return 1; return 0; }
static void hd_load_list(void) {
    if (!cfg.hd_tex) return;
    for (size_t i = 0; i < sizeof k_hd_builtin / sizeof *k_hd_builtin; i++) hd_add(k_hd_builtin[i]);
    char path[MAX_PATH]; snprintf(path, sizeof path, "%s\\bblit2fix_hd.txt", g_dir);
    FILE *f = fopen(path, "r");
    if (f) { char line[128]; while (fgets(line, sizeof line, f)) { if (line[0] == '#' || line[0] == ';') continue; unsigned long long h = strtoull(line, NULL, 16); hd_add(h); } fclose(f); }
    logf_("texturas HD: %d na lista", g_hd_n);
}
// texturas do GL que estão em HD (para os glTexSubImage2D seguintes): g_hd_names, declarado acima
static void hd_mark(GLuint t, int on) {
    for (int i = 0; i < g_hd_names_n; i++) if (g_hd_names[i] == t) { if (!on) g_hd_names[i] = g_hd_names[--g_hd_names_n]; return; }
    if (on && g_hd_names_n < 512) { g_hd_names[g_hd_names_n++] = t; g_hd_rc = wglGetCurrentContext(); }
}
static int hd_is(GLuint t) { for (int i = 0; i < g_hd_names_n; i++) if (g_hd_names[i] == t) return 1; return 0; }
static void hd_reset(void) {
    g_hd_names_n = 0;
    for (int i = 0; i < g_swap_n; i++) { free(g_swap[i].orig); free(g_swap[i].icon); }
    g_swap_n = 0; g_swap_state = -1;
}

// despejo: bin\texdump.bin = registros {"TEX1", w, h, formato, tipo, interno, hash(8)} + RGBA8, uma vez por hash
static FILE *g_dump; static unsigned long long *g_dumped; static int g_dumped_n, g_dumped_cap; static int g_dump_seq;
static void tex_dump(unsigned long long h, int w, int hh, GLenum f, GLenum ty, GLint ifmt, const DWORD *rgba) {
    for (int i = 0; i < g_dumped_n; i++) if (g_dumped[i] == h) return;
    if (g_dumped_n == g_dumped_cap) { int nc = g_dumped_cap ? g_dumped_cap * 2 : 512; unsigned long long *n = realloc(g_dumped, nc * sizeof *n); if (!n) return; g_dumped = n; g_dumped_cap = nc; }
    g_dumped[g_dumped_n++] = h;
    if (!g_dump) { char path[MAX_PATH]; snprintf(path, sizeof path, "%s\\texdump.bin", g_dir); g_dump = fopen(path, "ab"); if (!g_dump) return; }
    GLint name = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &name);
    DWORD hdr[8] = { 0x31584554 /* TEX1 */, (DWORD)w, (DWORD)hh, f, ty, (DWORD)ifmt, (DWORD)h, (DWORD)(h >> 32) };
    fwrite(hdr, 4, 8, g_dump); fwrite(rgba, 4, (size_t)w * hh, g_dump); fflush(g_dump);
    logf_("textura %d: %016llx %dx%d formato %04x tipo %04x interno %d nome %d", ++g_dump_seq, h, w, hh, f, ty, ifmt, name);
}
// devolve 1 se já enviou a versão HD
// pixels de origem em linhas contíguas, respeitando GL_UNPACK_ROW_LENGTH / SKIP_* / ALIGNMENT do jogo
static const void *tex_gather(int w, int h, GLenum f, GLenum ty, const void *px, void **tmp) {
    *tmp = NULL;
    int bpp = tex_bpp(f, ty);
    GLint rl = 0, sr = 0, sp = 0, al = 4;
    glGetIntegerv(GL_UNPACK_ROW_LENGTH_, &rl); glGetIntegerv(GL_UNPACK_SKIP_ROWS_, &sr);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS_, &sp); glGetIntegerv(GL_UNPACK_ALIGNMENT_, &al);
    if (al < 1) al = 1;
    size_t row = (size_t)(rl > 0 ? rl : w) * bpp, stride = (row + al - 1) / al * al, tight = (size_t)w * bpp;
    if (!sr && !sp && stride == tight) return px;
    BYTE *o = malloc(tight * h);
    if (!o) return NULL;
    for (int y = 0; y < h; y++) memcpy(o + (size_t)y * tight, (const BYTE *)px + (size_t)(sr + y) * stride + (size_t)sp * bpp, tight);
    *tmp = o;
    return o;
}
static void upload_rgba(GLenum t, GLint ifmt, int W, int H, const DWORD *p) {
    GLint al = 4, rl = 0, sr = 0, sp = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT_, &al); glGetIntegerv(GL_UNPACK_ROW_LENGTH_, &rl);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS_, &sr); glGetIntegerv(GL_UNPACK_SKIP_PIXELS_, &sp);
    glPixelStorei(GL_UNPACK_ALIGNMENT_, 4); glPixelStorei(GL_UNPACK_ROW_LENGTH_, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS_, 0); glPixelStorei(GL_UNPACK_SKIP_PIXELS_, 0);
    r_TexImage2D(t, 0, ifmt, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, p);
    glPixelStorei(GL_UNPACK_ALIGNMENT_, al); glPixelStorei(GL_UNPACK_ROW_LENGTH_, rl);
    glPixelStorei(GL_UNPACK_SKIP_ROWS_, sr); glPixelStorei(GL_UNPACK_SKIP_PIXELS_, sp);
}
// envia a versão HD (xBR) ou o ícone do controle no lugar da textura, se ela estiver na lista; 1 = enviou
static int hd_replace(GLenum t, GLint ifmt, int w, int h, const DWORD *rgba, unsigned long long hs, GLuint name) {
    int ic = (cfg.xinput && cfg.pad_prompts == 2) ? icon_index(hs) : -1;
    int W = w * 4, H = h * 4;
    if (ic >= 0) {
        DWORD *icon = malloc((size_t)W * H * 4), *orig = NULL;
        if (!icon) return 0;
        int btn = k_icon_tex[ic].btn < 0 ? enter_btn() : k_icon_tex[ic].btn;
        icon_render(btn, w, h, icon);
        const DWORD *up = icon;
        int kept = 0;
        if (k_icon_tex[ic].cond) {                  // Enter/Esc: guarda também a original em HD para trocar depois
            orig = malloc((size_t)W * H * 4);
            if (orig) { xbr4x(rgba, w, h, orig); tex_dilate(orig, W, H); if (!g_pad_on) up = orig; }
            if (g_swap_n < 8 && orig) { g_swap[g_swap_n].name = name; g_swap[g_swap_n].w = W; g_swap[g_swap_n].h = H; g_swap[g_swap_n].ifmt = ifmt; g_swap[g_swap_n].orig = orig; g_swap[g_swap_n].icon = icon; g_swap_n++; g_swap_state = g_pad_on; kept = 1; }
        }
        upload_rgba(t, ifmt, W, H, up);
        if (cfg.debug) {                            // debug: guarda os ícones gerados para conferência
            char path[MAX_PATH]; snprintf(path, sizeof path, "%s\\icons_debug.bin", g_dir);
            static int first = 1; FILE *f = fopen(path, first ? "wb" : "ab"); first = 0;
            if (f) { DWORD hdr[4] = { 0x314e4349, (DWORD)W, (DWORD)H, (DWORD)btn }; fwrite(hdr, 4, 4, f); fwrite(icon, 4, (size_t)W * H, f); fclose(f); }
        }
        if (!kept) { free(orig); free(icon); }
        hd_mark(name, 1);
        static int nlog; if (nlog++ < 12) logf_("icone do controle: %s na textura %u", k_icon_tex[ic].btn < 0 ? "Enter" : k_btn_name[k_icon_tex[ic].btn], name);
        return 1;
    }
    if (!hd_has(hs)) return 0;
    DWORD *big = malloc((size_t)W * H * 4);
    if (!big) return 0;
    xbr4x(rgba, w, h, big);
    tex_dilate(big, W, H);
    upload_rgba(t, ifmt, W, H, big);
    free(big);
    hd_mark(name, 1);
    return 1;
}
// glTexImage2D nível 0: decide se vai a versão HD. O nome da textura sempre perde a marca HD antes (nomes são
// reaproveitados pelo jogo e entre contextos: uma marca velha estragaria texturas atualizadas por pedaços).
static unsigned long long g_dbg_hash; static int g_dbg_name;
static int tex_hooks_on(void) { return g_hd_n || cfg.dump_tex || cfg.hud_probe || (cfg.xinput && cfg.pad_prompts == 2); }
// tipo de HUD de cada textura do jogo (HUD_NONE/SPRITE/BAR_FILL/3D), por nome, marcado no envio
static unsigned char g_hud_kind_by_name[8192];
static unsigned long long g_tex_hash[8192]; static unsigned short g_tex_w[8192], g_tex_h[8192];   // hash e tamanho por nome (diagnóstico)
static void hud_mark_tex(GLuint name, int kind) { if (name < sizeof g_hud_kind_by_name) g_hud_kind_by_name[name] = (unsigned char)kind; }
static int hud_tex_kind(GLuint name) { return name < sizeof g_hud_kind_by_name ? g_hud_kind_by_name[name] : HUD_NONE; }
static int tex_upload_hd(GLenum t, GLint ifmt, GLsizei w, GLsizei h, GLenum f, GLenum ty, const void *px) {
    if (t != GL_TEXTURE_2D) return 0;
    GLint name = 0;
    g_dbg_hash = 0;
    if (g_hd_names_n || g_swap_n || tex_hooks_on()) {
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &name);
        g_dbg_name = name;
        hd_mark((GLuint)name, 0); swap_forget((GLuint)name); hud_mark_tex((GLuint)name, HUD_NONE);
    }
    if (!px || w <= 0 || h <= 0 || w > 1024 || h > 1024) return 0;
    if (!tex_hooks_on()) return 0;
    if (!tex_bpp(f, ty)) { static int n; if (n++ < 8) logf_("textura %dx%d formato %04x tipo %04x: nao suportado", w, h, f, ty); return 0; }
    void *tmp; const void *src = tex_gather(w, h, f, ty, px, &tmp);
    DWORD *rgba = src ? malloc((size_t)w * h * 4) : NULL;
    int done = 0;
    if (rgba && tex_to_rgba(w, h, f, ty, src, rgba)) {
        unsigned long long hs = tex_lhash(rgba, w, h);
        g_dbg_hash = hs;
        hud_mark_tex((GLuint)name, hud_kind(hs));
        if ((GLuint)name < 8192) { g_tex_hash[name] = hs; g_tex_w[name] = (unsigned short)w; g_tex_h[name] = (unsigned short)h; }
        if (cfg.dump_tex) tex_dump(hs, w, h, f, ty, ifmt, rgba);
        done = hd_replace(t, ifmt, w, h, rgba, hs, (GLuint)name);
    }
    free(rgba); free(tmp);
    return done;
}
// glTexSubImage2D nível 0. O jogo acha que a textura tem o tamanho que ele enviou; se ela está em HD, o tamanho
// real é 4x. Pedaço de textura HD: redesenha o pedaço na posição x4. Textura inteira (o jogo reaproveita texturas
// do mesmo tamanho assim): vale como um glTexImage2D novo — vai em HD se estiver na lista, senão volta ao 1x.
static int tex_sub_hd(GLenum t, GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, const void *px) {
    if (t != GL_TEXTURE_2D || !px || w <= 0 || h <= 0 || w > 1024 || h > 1024) return 0;
    if (!g_hd_names_n && !tex_hooks_on()) return 0;
    if (!tex_bpp(f, ty)) return 0;
    GLint name = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &name);
    int is_hd = hd_is((GLuint)name);
    if (!is_hd && (x || y)) return 0;               // pedaço de uma textura comum
    GLint TW = 0, TH = 0;
    glGetTexLevelParameteriv(t, 0, GL_TEXTURE_WIDTH, &TW); glGetTexLevelParameteriv(t, 0, GL_TEXTURE_HEIGHT, &TH);
    int LW = is_hd ? TW / 4 : TW, LH = is_hd ? TH / 4 : TH;            // tamanho do ponto de vista do jogo
    if (is_hd && (x + w > LW || y + h > LH)) {      // não cabe: a textura não é mais a nossa
        static int nl; if (nl++ < 20) logf_("textura %d: marca HD velha descartada (pedaco %d,%d %dx%d, textura %dx%d)", name, x, y, w, h, TW, TH);
        hd_mark((GLuint)name, 0); swap_forget((GLuint)name);
        return 0;
    }
    int full = x == 0 && y == 0 && w == LW && h == LH;
    if (!is_hd && !full) return 0;
    void *tmp; const void *src = tex_gather(w, h, f, ty, px, &tmp);
    DWORD *rgba = src ? malloc((size_t)w * h * 4) : NULL;
    int ok = 0;
    if (rgba && tex_to_rgba(w, h, f, ty, src, rgba)) {
        if (!full) {                                // pedaço de textura HD
            DWORD *big = malloc((size_t)w * h * 64);
            if (big) {
                xbr4x(rgba, w, h, big); tex_dilate(big, w * 4, h * 4);
                GLint al = 4, rl = 0, sr = 0, sp = 0;
                glGetIntegerv(GL_UNPACK_ALIGNMENT_, &al); glGetIntegerv(GL_UNPACK_ROW_LENGTH_, &rl);
                glGetIntegerv(GL_UNPACK_SKIP_ROWS_, &sr); glGetIntegerv(GL_UNPACK_SKIP_PIXELS_, &sp);
                glPixelStorei(GL_UNPACK_ALIGNMENT_, 4); glPixelStorei(GL_UNPACK_ROW_LENGTH_, 0);
                glPixelStorei(GL_UNPACK_SKIP_ROWS_, 0); glPixelStorei(GL_UNPACK_SKIP_PIXELS_, 0);
                r_TexSubImage2D(t, 0, x * 4, y * 4, w * 4, h * 4, GL_RGBA, GL_UNSIGNED_BYTE, big);
                glPixelStorei(GL_UNPACK_ALIGNMENT_, al); glPixelStorei(GL_UNPACK_ROW_LENGTH_, rl);
                glPixelStorei(GL_UNPACK_SKIP_ROWS_, sr); glPixelStorei(GL_UNPACK_SKIP_PIXELS_, sp);
                free(big); ok = 1;
            }
        } else {                                    // textura inteira
            unsigned long long hs = tex_lhash(rgba, w, h);
            if (cfg.dump_tex) tex_dump(hs, w, h, f, ty, 0, rgba);
            GLint ifmt = 0; glGetTexLevelParameteriv(t, 0, GL_TEXTURE_INTERNAL_FORMAT, &ifmt);
            if (ifmt < 5) ifmt = GL_RGBA;           // 1-4 = componentes; usa o formato genérico
            hd_mark((GLuint)name, 0); swap_forget((GLuint)name); hud_mark_tex((GLuint)name, hud_kind(hs));
            ok = hd_replace(t, ifmt, w, h, rgba, hs, (GLuint)name);
            if (!ok && is_hd) {                     // conteúdo comum numa textura que estava em HD: volta ao 1x
                r_TexImage2D(t, 0, ifmt, w, h, 0, f, ty, px);
                ok = 1;
            }
            static int nl; if (cfg.debug && (ok || is_hd) && nl++ < 30) logf_("textura %d: reenviada inteira (%dx%d) por glTexSubImage2D: %s", name, w, h, is_hd && !hd_is((GLuint)name) ? "voltou ao 1x" : "HD");
        }
    }
    free(rgba); free(tmp);
    return ok;
}
static void hd_forget_bound(void) {
    if (!g_hd_names_n && !g_swap_n) return;
    GLint name = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &name);
    hd_mark((GLuint)name, 0); swap_forget((GLuint)name);
}
typedef void (APIENTRY *PFN_DelTex)(GLsizei, const GLuint *);
static PFN_DelTex r_DeleteTextures;
static void APIENTRY h_DeleteTextures(GLsizei n, const GLuint *t) {
    if (t && (g_hd_names_n || g_swap_n)) {
        int was = 0;
        for (GLsizei i = 0; i < n; i++) { was += hd_is(t[i]); hd_mark(t[i], 0); swap_forget(t[i]); }
        static int nl; if (cfg.debug && was && nl++ < 20) logf_("glDeleteTextures: %d texturas (%d estavam em HD)", (int)n, was);
    }
    r_DeleteTextures(n, t);
}
static void APIENTRY h_TexImage2D(GLenum t, GLint lv, GLint ifmt, GLsizei w, GLsizei h, GLint b, GLenum f, GLenum ty, const void *px) {
    if (lv == 0 && b == 0 && tex_upload_hd(t, ifmt, w, h, f, ty, px)) {
        static int nd; if (cfg.debug >= 2 && nd++ < 600) logf_("TI %d %dx%d f%04x t%04x i%d px%d HD %016llx", g_dbg_name, w, h, f, ty, ifmt, px != NULL, g_dbg_hash);
        gen_mips(t); return;
    }
    { static int nd; if (cfg.debug >= 2 && nd++ < 600) logf_("TI %d lv%d %dx%d f%04x t%04x i%d px%d -- %016llx", g_dbg_name, lv, w, h, f, ty, ifmt, px != NULL, g_dbg_hash); }
    r_TexImage2D(t, lv, ifmt, w, h, b, f, ty, px);
    static int n; if (cfg.debug && n++ < 20) logf_("glTexImage2D %dx%d fmt %d", w, h, ifmt);
    if (lv == 0) gen_mips(t);
}
static void APIENTRY h_TexSubImage2D(GLenum t, GLint lv, GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, const void *px) {
    if (cfg.debug >= 2) { static int nd; GLint nm = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &nm); if (nd++ < 300) logf_("TS %d lv%d %d,%d %dx%d f%04x t%04x hd%d", nm, lv, x, y, w, h, f, ty, hd_is((GLuint)nm)); }
    if (lv == 0 && tex_sub_hd(t, x, y, w, h, f, ty, px)) { gen_mips(t); return; }
    r_TexSubImage2D(t, lv, x, y, w, h, f, ty, px);
    if (lv == 0) gen_mips(t);
}
static GLenum map_buf(GLenum m) { return (g_fbo_ok && (m == GL_BACK || m == GL_FRONT || m == GL_FRONT_AND_BACK || m == GL_BACK_LEFT || m == GL_FRONT_LEFT)) ? GL_COLOR_ATTACHMENT0_EXT : m; }
static void APIENTRY h_DrawBuffer(GLenum m) { r_DrawBuffer(map_buf(m)); }
static void APIENTRY h_ReadBuffer(GLenum m) { r_ReadBuffer(map_buf(m)); }

// ---------------------------------------------------------------- GetProcAddress / LoadLibrary hooks
typedef FARPROC (WINAPI *PFN_GPA)(HMODULE, LPCSTR);
typedef HMODULE (WINAPI *PFN_LLA)(LPCSTR);
static PFN_GPA r_GetProcAddress;
static PFN_LLA r_LoadLibraryA;

// ---------------------------------------------------------------- diagnóstico do HUD ([debug] hud_probe=1)
// Mede por onde o jogo desenha o HUD: conta as funções GL chamadas com uma textura do HUD ligada e registra os
// vértices. Com hud_probe=0 nenhum destes ganchos é instalado.
static GLuint pr_tex; static int pr_kind, pr_in, pr_mode, pr_nv;
static float pr_v[16][4], pr_tc[16][2], pr_cur_tc[2];
enum { PR_BEGIN, PR_V2F, PR_V2FV, PR_V2I, PR_V2S, PR_V3F, PR_V3FV, PR_V3I, PR_V4F, PR_V4FV, PR_TC2F, PR_TC2FV,
       PR_DRAWARRAYS, PR_DRAWELEMENTS, PR_CALLLIST, PR_CALLLISTS, PR_RECTF, PR_RECTI, PR_TCX, PR_COLOR, PR_N };
static const char *const pr_names[PR_N] = { "Begin", "Vertex2f", "Vertex2fv", "Vertex2i", "Vertex2s", "Vertex3f",
    "Vertex3fv", "Vertex3i", "Vertex4f", "Vertex4fv", "TexCoord2f", "TexCoord2fv", "DrawArrays", "DrawElements",
    "CallList", "CallLists", "Rectf", "Recti", "TexCoord(outra)", "Color" };
// F10 (com debug=1): registra todos os desenhos 2D (|z| < 50) do próximo quadro, com textura, hash, cor e retângulo
static int pr_frame_dump;                            // 1 = armado, 2 = registrando o quadro atual
static float pr_col[4] = { 1, 1, 1, 1 };
static unsigned pr_cnt[PR_N][4];
static struct { GLint size; GLenum type; GLsizei stride; const void *ptr; } pr_vp, pr_tp;
static void (APIENTRY *r_BindTexture)(GLenum, GLuint);
static void (APIENTRY *r_Begin)(GLenum);
static void (APIENTRY *r_End)(void);
static void (APIENTRY *r_Vertex2f)(GLfloat, GLfloat);
static void (APIENTRY *r_Vertex2fv)(const GLfloat *);
static void (APIENTRY *r_Vertex2i)(GLint, GLint);
static void (APIENTRY *r_Vertex2s)(GLshort, GLshort);
static void (APIENTRY *r_Vertex3f)(GLfloat, GLfloat, GLfloat);
static void (APIENTRY *r_Vertex3fv)(const GLfloat *);
static void (APIENTRY *r_Vertex3i)(GLint, GLint, GLint);
static void (APIENTRY *r_Vertex4f)(GLfloat, GLfloat, GLfloat, GLfloat);
static void (APIENTRY *r_Vertex4fv)(const GLfloat *);
static void (APIENTRY *r_TexCoord2f)(GLfloat, GLfloat);
static void (APIENTRY *r_TexCoord2fv)(const GLfloat *);
static void (APIENTRY *r_DrawArrays)(GLenum, GLint, GLsizei);
static void (APIENTRY *r_DrawElements)(GLenum, GLsizei, GLenum, const void *);
static void (APIENTRY *r_CallList)(GLuint);
static void (APIENTRY *r_CallLists)(GLsizei, GLenum, const void *);
static void (APIENTRY *r_Rectf)(GLfloat, GLfloat, GLfloat, GLfloat);
static void (APIENTRY *r_Recti)(GLint, GLint, GLint, GLint);
static void (APIENTRY *r_VertexPointer)(GLint, GLenum, GLsizei, const void *);
static void (APIENTRY *r_TexCoordPointer)(GLint, GLenum, GLsizei, const void *);
static const char *pr_kind_name(int k) { return k == HUD_SPRITE ? "sprite" : k == HUD_BAR_FILL ? "barra" : k == HUD_3D ? "3D" : "-"; }
static void pr_count(int f) { if (pr_kind) pr_cnt[f][pr_kind]++; }
static void pr_vert(float x, float y, float z, float w) {
    if (!pr_in || (!pr_kind && pr_frame_dump != 2) || pr_nv >= 16) return;
    pr_v[pr_nv][0] = x; pr_v[pr_nv][1] = y; pr_v[pr_nv][2] = z; pr_v[pr_nv][3] = w;
    pr_tc[pr_nv][0] = pr_cur_tc[0]; pr_tc[pr_nv][1] = pr_cur_tc[1]; pr_nv++;
}
// registra um desenho: barras sempre que a geometria muda; o resto, os 2 primeiros de cada textura a cada 10 s
static void pr_log_draw(const char *via, int mode, int nv, const float (*v)[4], const float (*tc)[2]) {
    static int nlog; static DWORD last_t[8192]; static char last_sig[8][160]; static GLuint last_bar[8];
    if (nlog >= 600) return;
    char sig[160] = "", buf[1024]; int o = 0;
    for (int i = 0; i < nv && i < 8; i++) o += snprintf(sig + o, sizeof sig - o > 0 ? sizeof sig - o : 0, "%.0f,%.0f ", v[i][0], v[i][1]);
    DWORD now = GetTickCount();
    if (pr_kind == HUD_BAR_FILL) {
        int slot = (int)(pr_tex % 8);
        if (last_bar[slot] == pr_tex && !strcmp(last_sig[slot], sig)) return;
        last_bar[slot] = pr_tex; snprintf(last_sig[slot], sizeof last_sig[slot], "%s", sig);
    } else {
        GLuint k = pr_tex < 8192 ? pr_tex : 0;
        if (last_t[k] && now - last_t[k] < 10000) return;
        last_t[k] = now;
    }
    o = snprintf(buf, sizeof buf, "hud_probe: %s tex %u (%s) modo %u proj %d nv %d:", via, pr_tex, pr_kind_name(pr_kind), mode, pr_proj, nv);
    for (int i = 0; i < nv && i < 8 && o < (int)sizeof buf - 80; i++)
        o += snprintf(buf + o, sizeof buf - o, " (%.1f %.1f %.2f %.2f | %.3f %.3f)", v[i][0], v[i][1], v[i][2], v[i][3], tc[i][0], tc[i][1]);
    logf_("%s", buf); nlog++;
}
// lê vértices de arrays do cliente (só float/short/int) para registrar DrawArrays/DrawElements
static void pr_fetch(int idx, float *v, float *tc) {
    v[0] = v[1] = v[2] = 0; v[3] = 1; tc[0] = tc[1] = 0;
    if (pr_vp.ptr) {
        int sz = pr_vp.type == GL_FLOAT ? 4 : pr_vp.type == GL_SHORT ? 2 : pr_vp.type == GL_INT ? 4 : pr_vp.type == GL_DOUBLE ? 8 : 0;
        if (sz) {
            const BYTE *b = (const BYTE *)pr_vp.ptr + (size_t)idx * (pr_vp.stride ? pr_vp.stride : sz * pr_vp.size);
            for (int c = 0; c < pr_vp.size && c < 4; c++)
                v[c] = pr_vp.type == GL_FLOAT ? ((const float *)b)[c] : pr_vp.type == GL_SHORT ? ((const short *)b)[c] : pr_vp.type == GL_INT ? (float)((const int *)b)[c] : (float)((const double *)b)[c];
        }
    }
    if (pr_tp.ptr && pr_tp.type == GL_FLOAT) {
        const BYTE *b = (const BYTE *)pr_tp.ptr + (size_t)idx * (pr_tp.stride ? pr_tp.stride : 4 * pr_tp.size);
        tc[0] = ((const float *)b)[0]; if (pr_tp.size > 1) tc[1] = ((const float *)b)[1];
    }
}
static void APIENTRY h_BindTexture(GLenum t, GLuint n) { if (t == GL_TEXTURE_2D) { pr_tex = n; pr_kind = hud_tex_kind(n); } r_BindTexture(t, n); }
static void APIENTRY h_Begin(GLenum m) { pr_count(PR_BEGIN); pr_in = 1; pr_mode = (int)m; pr_nv = 0; r_Begin(m); }
static void pr_dump_batch(void) {
    static int nl;
    if (pr_nv < 1 || fabsf(pr_v[0][2]) >= 50 || nl >= 3000) return;
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f, s0 = 1e9f, t0 = 1e9f, s1 = -1e9f, t1 = -1e9f;
    for (int i = 0; i < pr_nv; i++) {
        if (pr_v[i][0] < x0) x0 = pr_v[i][0]; if (pr_v[i][0] > x1) x1 = pr_v[i][0];
        if (pr_v[i][1] < y0) y0 = pr_v[i][1]; if (pr_v[i][1] > y1) y1 = pr_v[i][1];
        if (pr_tc[i][0] < s0) s0 = pr_tc[i][0]; if (pr_tc[i][0] > s1) s1 = pr_tc[i][0];
        if (pr_tc[i][1] < t0) t0 = pr_tc[i][1]; if (pr_tc[i][1] > t1) t1 = pr_tc[i][1];
    }
    GLuint k = pr_tex < 8192 ? pr_tex : 0;
    GLboolean tx = glIsEnabled(GL_TEXTURE_2D), bl = glIsEnabled(GL_BLEND), at = glIsEnabled(GL_ALPHA_TEST);
    logf_("quadro: tex %u %016llx %ux%u tx%d bl%d at%d cor %.2f %.2f %.2f %.2f modo %d nv %d ret %.1f,%.1f - %.1f,%.1f z %.2f tc %.3f,%.3f - %.3f,%.3f",
          pr_tex, g_tex_hash[k], g_tex_w[k], g_tex_h[k], tx, bl, at, pr_col[0], pr_col[1], pr_col[2], pr_col[3], pr_mode, pr_nv, x0, y0, x1, y1, pr_v[0][2], s0, t0, s1, t1);
    nl++;
}
static void APIENTRY h_End(void) {
    if (pr_in && pr_frame_dump == 2) pr_dump_batch();
    if (pr_in && pr_kind && pr_nv) pr_log_draw("Begin/End", pr_mode, pr_nv, (const float (*)[4])pr_v, (const float (*)[2])pr_tc);
    pr_in = 0; r_End();
}
static void (APIENTRY *r_TexCoord2d)(GLdouble, GLdouble);
static void (APIENTRY *r_TexCoord2dv)(const GLdouble *);
static void (APIENTRY *r_TexCoord2i)(GLint, GLint);
static void (APIENTRY *r_TexCoord2s)(GLshort, GLshort);
static void (APIENTRY *r_TexCoord3f)(GLfloat, GLfloat, GLfloat);
static void (APIENTRY *r_TexCoord3fv)(const GLfloat *);
static void (APIENTRY *r_TexCoord4f)(GLfloat, GLfloat, GLfloat, GLfloat);
static void (APIENTRY *r_TexCoord4fv)(const GLfloat *);
static void (APIENTRY *r_Color4ub)(GLubyte, GLubyte, GLubyte, GLubyte);
static void (APIENTRY *r_Color4ubv)(const GLubyte *);
static void (APIENTRY *r_Color4f)(GLfloat, GLfloat, GLfloat, GLfloat);
static void (APIENTRY *r_Color4fv)(const GLfloat *);
static void (APIENTRY *r_Color3f)(GLfloat, GLfloat, GLfloat);
static void (APIENTRY *r_Color3ub)(GLubyte, GLubyte, GLubyte);
static void pr_tcx(float s0, float t0) { pr_count(PR_TCX); pr_cur_tc[0] = s0; pr_cur_tc[1] = t0; }
static void pr_color(float r, float g, float b, float a) { pr_count(PR_COLOR); pr_col[0] = r; pr_col[1] = g; pr_col[2] = b; pr_col[3] = a; }
static void APIENTRY h_TexCoord2d(GLdouble a, GLdouble b) { pr_tcx((float)a, (float)b); r_TexCoord2d(a, b); }
static void APIENTRY h_TexCoord2dv(const GLdouble *p) { pr_tcx((float)p[0], (float)p[1]); r_TexCoord2dv(p); }
static void APIENTRY h_TexCoord2i(GLint a, GLint b) { pr_tcx((float)a, (float)b); r_TexCoord2i(a, b); }
static void APIENTRY h_TexCoord2s(GLshort a, GLshort b) { pr_tcx(a, b); r_TexCoord2s(a, b); }
static void APIENTRY h_TexCoord3f(GLfloat a, GLfloat b, GLfloat c) { pr_tcx(a, b); r_TexCoord3f(a, b, c); }
static void APIENTRY h_TexCoord3fv(const GLfloat *p) { pr_tcx(p[0], p[1]); r_TexCoord3fv(p); }
static void APIENTRY h_TexCoord4f(GLfloat a, GLfloat b, GLfloat c, GLfloat d) { pr_tcx(d != 0 ? a / d : a, d != 0 ? b / d : b); r_TexCoord4f(a, b, c, d); }
static void APIENTRY h_TexCoord4fv(const GLfloat *p) { pr_tcx(p[3] != 0 ? p[0] / p[3] : p[0], p[3] != 0 ? p[1] / p[3] : p[1]); r_TexCoord4fv(p); }
static void APIENTRY h_Color4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) { pr_color(r / 255.f, g / 255.f, b / 255.f, a / 255.f); r_Color4ub(r, g, b, a); }
static void APIENTRY h_Color4ubv(const GLubyte *p) { pr_color(p[0] / 255.f, p[1] / 255.f, p[2] / 255.f, p[3] / 255.f); r_Color4ubv(p); }
static void APIENTRY h_Color4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { pr_color(r, g, b, a); r_Color4f(r, g, b, a); }
static void APIENTRY h_Color4fv(const GLfloat *p) { pr_color(p[0], p[1], p[2], p[3]); r_Color4fv(p); }
static void APIENTRY h_Color3f(GLfloat r, GLfloat g, GLfloat b) { pr_color(r, g, b, 1); r_Color3f(r, g, b); }
static void APIENTRY h_Color3ub(GLubyte r, GLubyte g, GLubyte b) { pr_color(r / 255.f, g / 255.f, b / 255.f, 1); r_Color3ub(r, g, b); }
static void APIENTRY h_Vertex2f(GLfloat x, GLfloat y) { pr_count(PR_V2F); pr_vert(x, y, 0, 1); r_Vertex2f(x, y); }
static void APIENTRY h_Vertex2fv(const GLfloat *p) { pr_count(PR_V2FV); pr_vert(p[0], p[1], 0, 1); r_Vertex2fv(p); }
static void APIENTRY h_Vertex2i(GLint x, GLint y) { pr_count(PR_V2I); pr_vert((float)x, (float)y, 0, 1); r_Vertex2i(x, y); }
static void APIENTRY h_Vertex2s(GLshort x, GLshort y) { pr_count(PR_V2S); pr_vert(x, y, 0, 1); r_Vertex2s(x, y); }
static void APIENTRY h_Vertex3f(GLfloat x, GLfloat y, GLfloat z) { pr_count(PR_V3F); pr_vert(x, y, z, 1); r_Vertex3f(x, y, z); }
static void APIENTRY h_Vertex3fv(const GLfloat *p) { pr_count(PR_V3FV); pr_vert(p[0], p[1], p[2], 1); r_Vertex3fv(p); }
static void APIENTRY h_Vertex3i(GLint x, GLint y, GLint z) { pr_count(PR_V3I); pr_vert((float)x, (float)y, (float)z, 1); r_Vertex3i(x, y, z); }
static void APIENTRY h_Vertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) { pr_count(PR_V4F); pr_vert(x, y, z, w); r_Vertex4f(x, y, z, w); }
static void APIENTRY h_Vertex4fv(const GLfloat *p) { pr_count(PR_V4FV); pr_vert(p[0], p[1], p[2], p[3]); r_Vertex4fv(p); }
static void APIENTRY h_TexCoord2f(GLfloat s0, GLfloat t0) { pr_count(PR_TC2F); pr_cur_tc[0] = s0; pr_cur_tc[1] = t0; r_TexCoord2f(s0, t0); }
static void APIENTRY h_TexCoord2fv(const GLfloat *p) { pr_count(PR_TC2FV); pr_cur_tc[0] = p[0]; pr_cur_tc[1] = p[1]; r_TexCoord2fv(p); }
static void APIENTRY h_VertexPointer(GLint n, GLenum t, GLsizei st, const void *p) { pr_vp.size = n; pr_vp.type = t; pr_vp.stride = st; pr_vp.ptr = p; r_VertexPointer(n, t, st, p); }
static void APIENTRY h_TexCoordPointer(GLint n, GLenum t, GLsizei st, const void *p) { pr_tp.size = n; pr_tp.type = t; pr_tp.stride = st; pr_tp.ptr = p; r_TexCoordPointer(n, t, st, p); }
static void APIENTRY h_DrawArrays(GLenum m, GLint first, GLsizei n) {
    pr_count(PR_DRAWARRAYS);
    if (pr_kind && n > 0) {
        float v[8][4], tc[8][2]; int k = n < 8 ? n : 8;
        for (int i = 0; i < k; i++) pr_fetch(first + i, v[i], tc[i]);
        pr_log_draw("DrawArrays", (int)m, k, (const float (*)[4])v, (const float (*)[2])tc);
    }
    r_DrawArrays(m, first, n);
}
static void APIENTRY h_DrawElements(GLenum m, GLsizei n, GLenum t, const void *ix) {
    pr_count(PR_DRAWELEMENTS);
    if (pr_kind && n > 0 && ix) {
        float v[8][4], tc[8][2]; int k = n < 8 ? n : 8;
        for (int i = 0; i < k; i++) {
            unsigned id = t == GL_UNSIGNED_SHORT ? ((const unsigned short *)ix)[i] : t == GL_UNSIGNED_BYTE ? ((const unsigned char *)ix)[i] : ((const unsigned *)ix)[i];
            pr_fetch((int)id, v[i], tc[i]);
        }
        pr_log_draw("DrawElements", (int)m, k, (const float (*)[4])v, (const float (*)[2])tc);
    }
    r_DrawElements(m, n, t, ix);
}
static void APIENTRY h_CallList(GLuint l) { pr_count(PR_CALLLIST); r_CallList(l); }
static void APIENTRY h_CallLists(GLsizei n, GLenum t, const void *p) { pr_count(PR_CALLLISTS); r_CallLists(n, t, p); }
static void APIENTRY h_Rectf(GLfloat a, GLfloat b, GLfloat c, GLfloat d) {
    pr_count(PR_RECTF);
    if (pr_kind) { float v[2][4] = { { a, b, 0, 1 }, { c, d, 0, 1 } }, tc[2][2] = { { 0, 0 }, { 0, 0 } }; pr_log_draw("Rectf", 0, 2, (const float (*)[4])v, (const float (*)[2])tc); }
    r_Rectf(a, b, c, d);
}
static void APIENTRY h_Recti(GLint a, GLint b, GLint c, GLint d) { pr_count(PR_RECTI); r_Recti(a, b, c, d); }
// resumo a cada 5 s: quantas chamadas de cada função com textura do HUD ligada
static void pr_tick(void) {
    static DWORD t0; static int nl;
    if (pr_frame_dump == 2) { pr_frame_dump = 0; logf_("quadro: fim"); }
    if (pr_frame_dump == 1) { pr_frame_dump = 2; logf_("quadro: inicio (F10)"); }
    if (!cfg.hud_probe || nl >= 120) return;
    DWORD now = GetTickCount();
    if (!t0) t0 = now;
    if (now - t0 < 5000) return;
    t0 = now;
    char buf[1024]; int o = 0, any = 0;
    for (int k = 1; k < 4; k++)
        for (int f = 0; f < PR_N; f++)
            if (pr_cnt[f][k]) { any = 1; if (o < (int)sizeof buf - 40) o += snprintf(buf + o, sizeof buf - o, " %s/%s=%u", pr_names[f], pr_kind_name(k), pr_cnt[f][k]); }
    if (any) { logf_("hud_probe (5 s):%s", buf); nl++; }
    memset(pr_cnt, 0, sizeof pr_cnt);
    LONG f4 = InterlockedExchange(&g_f4_prompt_n, 0);
    if (f4) logf_("hud_probe (5 s): dica do F4 pedida %ld vezes", f4);
}

#define H(name) { #name, (void **)&r_##name, (void *)h_##name }
static struct { const char *name; void **real; void *hook; } gl_hooks[] = {
    H(Viewport), H(Scissor), H(Ortho), H(Frustum), H(MatrixMode), H(LoadMatrixf), H(LoadMatrixd), H(Clear),
    H(ReadPixels), H(CopyTexImage2D), H(CopyTexSubImage2D), H(Enable), H(Disable), H(DrawPixels),
    H(TexParameterf), H(TexParameteri), H(TexImage2D), H(TexSubImage2D), H(DrawBuffer), H(ReadBuffer),
    H(DeleteTextures),
};
static struct { const char *name; void **real; void *hook; } gl_probe_hooks[] = {
    H(BindTexture), H(Begin), H(End), H(Vertex2f), H(Vertex2fv), H(Vertex2i), H(Vertex2s), H(Vertex3f), H(Vertex3fv),
    H(Vertex3i), H(Vertex4f), H(Vertex4fv), H(TexCoord2f), H(TexCoord2fv), H(DrawArrays), H(DrawElements), H(CallList),
    H(CallLists), H(Rectf), H(Recti), H(VertexPointer), H(TexCoordPointer), H(TexCoord2d), H(TexCoord2dv), H(TexCoord2i),
    H(TexCoord2s), H(TexCoord3f), H(TexCoord3fv), H(TexCoord4f), H(TexCoord4fv), H(Color4ub), H(Color4ubv), H(Color4f),
    H(Color4fv), H(Color3f), H(Color3ub),
};
#undef H

static HMODULE WINAPI h_LoadLibraryA(LPCSTR name) {
    HMODULE m = r_LoadLibraryA(name);
    logf_("LoadLibraryA(%s) -> %p", name ? name : "(null)", (void *)m);
    if (m && name) {
        const char *b = strrchr(name, '\\'); b = b ? b + 1 : name;
        if (!_stricmp(b, "opengl32.dll")) g_gl = m;
    }
    return m;
}

static FARPROC WINAPI h_GetProcAddress(HMODULE m, LPCSTR name) {
    FARPROC p = r_GetProcAddress(m, name);
    if (!p || (DWORD)name < 0x10000) return p;
    if (name[0] == 'g' && name[1] == 'l' && name[2] != 'u') {
        for (size_t i = 0; i < sizeof gl_hooks / sizeof *gl_hooks; i++)
            if (!strcmp(name + 2, gl_hooks[i].name)) { *gl_hooks[i].real = (void *)p; return (FARPROC)gl_hooks[i].hook; }
        if (cfg.hud_probe)
            for (size_t i = 0; i < sizeof gl_probe_hooks / sizeof *gl_probe_hooks; i++)
                if (!strcmp(name + 2, gl_probe_hooks[i].name)) { *gl_probe_hooks[i].real = (void *)p; return (FARPROC)gl_probe_hooks[i].hook; }
    }
    if (name[0] == 'w' && name[1] == 'g' && name[2] == 'l') {
        if (!g_gl) g_gl = m;
        if (!strcmp(name, "wglSwapBuffers"))        { r_wglSwapBuffers = (void *)p;        return (FARPROC)h_wglSwapBuffers; }
        if (!strcmp(name, "wglMakeCurrent"))        { r_wglMakeCurrent = (void *)p;        return (FARPROC)h_wglMakeCurrent; }
        if (!strcmp(name, "wglChoosePixelFormat"))  { r_wglChoosePixelFormat = (void *)p;  return (FARPROC)h_wglChoosePixelFormat; }
        if (!strcmp(name, "wglSetPixelFormat"))     { r_wglSetPixelFormat = (void *)p;     return (FARPROC)h_wglSetPixelFormat; }
        if (!strcmp(name, "wglGetProcAddress"))     { r_wglGetProcAddress = (void *)p;     return p; }
        if (!strcmp(name, "wglCreateContext"))      { r_wglCreateContext = (void *)p;      return p; }
        if (!strcmp(name, "wglDeleteContext"))      { r_wglDeleteContext = (void *)p;      return (FARPROC)h_wglDeleteContext; }
        if (!strcmp(name, "wglDescribePixelFormat")) { r_DescribePF_wgl = (void *)p;      return (FARPROC)h_DescribePF_wgl; }
    }
    if (!strcmp(name, "SwapBuffers"))       { r_SwapBuffersGdi = (void *)p;       return (FARPROC)h_SwapBuffersGdi; }
    if (!strcmp(name, "ChoosePixelFormat")) { r_ChoosePixelFormatGdi = (void *)p; return (FARPROC)h_ChoosePixelFormatGdi; }
    if (!strcmp(name, "SetPixelFormat"))    { r_SetPixelFormatGdi = (void *)p;    return (FARPROC)h_SetPixelFormatGdi; }
    if (!strcmp(name, "DescribePixelFormat")) { r_DescribePF_gdi = (void *)p;     return (FARPROC)h_DescribePF_gdi; }
    return p;
}

// ---------------------------------------------------------------- janela / modo de vídeo
typedef HWND (WINAPI *PFN_CWEx)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef LONG (WINAPI *PFN_CDS)(DEVMODEA *, DWORD);
typedef BOOL (WINAPI *PFN_SWP)(HWND, HWND, int, int, int, int, UINT);
typedef BOOL (WINAPI *PFN_GCR)(HWND, LPRECT);
typedef int (WINAPI *PFN_GSM)(int);
typedef BOOL (WINAPI *PFN_DW)(HWND);
static PFN_CWEx r_CreateWindowExA;
static PFN_CDS r_ChangeDisplaySettingsA;
static PFN_SWP r_SetWindowPos;
static PFN_GCR r_GetClientRect;
static PFN_GSM r_GetSystemMetrics;
static PFN_DW r_DestroyWindow;
static WNDPROC g_orig_proc;

static LPARAM map_mouse(LPARAM lp) {
    if (!scaling_active() || !g_vw || !g_vh) return lp;
    int x = (short)LOWORD(lp), y = (short)HIWORD(lp);
    int gx = (int)((double)(x - g_vx) * g_gw / g_vw), gy = (int)((double)(y - g_vy) * g_gh / g_vh);
    if (gx < 0) gx = 0; if (gx >= g_gw) gx = g_gw - 1;
    if (gy < 0) gy = 0; if (gy >= g_gh) gy = g_gh - 1;
    return MAKELPARAM(gx, gy);
}
// cursor do mouse: some enquanto o controle navega pelos menus 2D e volta quando o mouse de verdade se move.
// Vai pela imagem do cursor (SetCursor e WM_SETCURSOR), não pelo contador do ShowCursor, que o jogo também mexe.
static PADCUR g_padcur;
static void padcur_apply(int act) {
    if (act == PADCUR_HIDE) SetCursor(NULL);
    else if (act == PADCUR_SHOW) SetCursor(LoadCursorA(NULL, (LPCSTR)IDC_ARROW));
    if (act != PADCUR_KEEP) { static int nl; if (nl++ < 6) logf_("controle no menu: cursor do mouse %s", act == PADCUR_HIDE ? "escondido" : "de volta"); }
}
static LRESULT CALLBACK game_proc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    if (m == WM_MOUSEMOVE && g_padcur.hidden) { POINT c; if (GetCursorPos(&c)) padcur_apply(padcur_mouse(&g_padcur, c.x, c.y)); }
    if (m == WM_SETCURSOR && g_padcur.hidden && LOWORD(lp) == HTCLIENT) { SetCursor(NULL); return TRUE; }
    switch (m) {
    case WM_ACTIVATE:
        // o jogo "suspende" (destrói o OpenGL e recria a janela) ao perder o foco; com background=1 ele segue rodando
        if (cfg.background && LOWORD(wp) == WA_INACTIVE) { static int n; if (n++ < 5) logf_("perda de foco ignorada (background=1)"); return DefWindowProcA(w, m, wp, lp); }
        break;
    case WM_ACTIVATEAPP:
        if (cfg.background && !wp) return DefWindowProcA(w, m, wp, lp);
        break;
    case WM_MOUSEMOVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK: case WM_MBUTTONDOWN: case WM_MBUTTONUP:
        lp = map_mouse(lp); break;
    case WM_SIZE:
        update_rect(); lp = MAKELPARAM(g_gw, g_gh); break;
    case WM_DISPLAYCHANGE:
        lp = MAKELPARAM(g_gw, g_gh); break;
    case WM_ERASEBKGND:
        if (!g_gl_on) { if (g_vdc) present_2d(1); else { RECT r; GetClientRect(w, &r); FillRect((HDC)wp, &r, (HBRUSH)GetStockObject(BLACK_BRUSH)); } }
        return 1;
    }
    return CallWindowProcA(g_orig_proc, w, m, wp, lp);
}

static HWND WINAPI h_CreateWindowExA(DWORD ex, LPCSTR cls, LPCSTR title, DWORD st, int x, int y, int w, int h, HWND par, HMENU menu, HINSTANCE inst, LPVOID lp) {
    int game = HIWORD(cls) && !strcmp(cls, "BBLIT Game");
    if (game && cfg.enabled && cfg.background) ex &= ~WS_EX_TOPMOST;   // sem "sempre no topo": Alt+Tab e o outro monitor funcionam
    HWND r = r_CreateWindowExA(ex, cls, title, st, x, y, w, h, par, menu, inst, lp);
    logf_("CreateWindowExA(%s, %s, estilo %08lx, %d,%d %dx%d) -> %p", HIWORD(cls) ? cls : "#atom", title ? title : "", st, x, y, w, h, (void *)r);
    if (game && r && cfg.enabled) {
        if (g_hwnd && !IsWindow(g_hwnd)) g_hwnd = NULL;
        g_hwnd = r;
        if (w > 0 && h > 0 && w < 10000) { g_gw = w; g_gh = h; }
        g_orig_proc = (WNDPROC)SetWindowLongA(r, GWL_WNDPROC, (LONG)game_proc);
        fit_window();
        paint_black();
    }
    return r;
}
static BOOL WINAPI h_DestroyWindow(HWND w) {
    if (w == g_hwnd) { logf_("DestroyWindow(janela do jogo)"); g_hwnd = NULL; g_winw = g_winh = 0; }
    return r_DestroyWindow(w);
}
static BOOL WINAPI h_SetWindowPos(HWND w, HWND after, int x, int y, int cx, int cy, UINT fl) {
    if (!is_game_wnd(w)) return r_SetWindowPos(w, after, x, y, cx, cy, fl);
    static int n; if (n++ < 20) logf_("SetWindowPos(jogo, %d,%d %dx%d, %04x)", x, y, cx, cy, fl);
    if (!(fl & SWP_NOSIZE) && cx > 0 && cy > 0) { g_gw = cx; g_gh = cy; }
    monitor_rect();
    if (cfg.background && after == HWND_TOPMOST) after = HWND_TOP;
    BOOL r = r_SetWindowPos(w, after, g_mon.left, g_mon.top, g_mon.right - g_mon.left, g_mon.bottom - g_mon.top, fl & ~(SWP_NOSIZE | SWP_NOMOVE));
    update_rect();
    return r;
}
static BOOL WINAPI h_GetClientRect(HWND w, LPRECT r) {
    if (is_game_wnd(w) && r) { SetRect(r, 0, 0, g_gw, g_gh); return TRUE; }
    return r_GetClientRect(w, r);
}
static int WINAPI h_GetSystemMetrics(int i) {
    if (g_hwnd && cfg.enabled) { if (i == SM_CXSCREEN) return g_gw; if (i == SM_CYSCREEN) return g_gh; }
    return r_GetSystemMetrics(i);
}

// O jogo exige área de trabalho em 16 bits e troca a resolução do monitor; o Windows 10/11 não tem modo de 16 bits.
// Aqui o modo de vídeo nunca é trocado: o pedido só define o tamanho "virtual" que o jogo pensa ter.
typedef BOOL (WINAPI *PFN_EDS)(LPCSTR, DWORD, DEVMODEA *);
typedef int (WINAPI *PFN_GDC)(HDC, int);
static PFN_EDS r_EnumDisplaySettingsA;
static PFN_GDC r_GetDeviceCaps;

static LONG WINAPI h_ChangeDisplaySettingsA(DEVMODEA *dm, DWORD fl) {
    if (!dm) { logf_("ChangeDisplaySettingsA(NULL): nada a restaurar"); return DISP_CHANGE_SUCCESSFUL; }
    DWORD w = (dm->dmFields & DM_PELSWIDTH) ? dm->dmPelsWidth : 0, h = (dm->dmFields & DM_PELSHEIGHT) ? dm->dmPelsHeight : 0;
    logf_("ChangeDisplaySettingsA(%lux%lu %lubpp, %08lx) -> virtual", w, h, dm->dmBitsPerPel, fl);
    if (!(fl & CDS_TEST) && w && h) {
        g_gw = w; g_gh = h;
        if (g_hwnd) { fit_window(); paint_black(); }
    }
    return DISP_CHANGE_SUCCESSFUL;
}

static BOOL WINAPI h_EnumDisplaySettingsA(LPCSTR dev, DWORD mode, DEVMODEA *dm) {
    if (mode == ENUM_CURRENT_SETTINGS || mode == ENUM_REGISTRY_SETTINGS) {
        BOOL r = r_EnumDisplaySettingsA(dev, mode, dm);
        if (r && dm) dm->dmBitsPerPel = 16;
        return r;
    }
    // modos de 32 bits do monitor apresentados como 16 bits, sem repetir resolução
    static DWORD map[1024]; static int nmap = -1;
    if (nmap < 0) {
        nmap = 0;
        DEVMODEA t; memset(&t, 0, sizeof t); t.dmSize = sizeof t;
        static DWORD seen[512][2]; int ns = 0;
        for (DWORD i = 0; nmap < 1024 && r_EnumDisplaySettingsA(dev, i, &t); i++) {
            if (t.dmBitsPerPel != 32 || t.dmPelsWidth < 640) continue;
            int dup = 0;
            for (int k = 0; k < ns; k++) if (seen[k][0] == t.dmPelsWidth && seen[k][1] == t.dmPelsHeight) dup = 1;
            if (dup || ns >= 512) continue;
            seen[ns][0] = t.dmPelsWidth; seen[ns][1] = t.dmPelsHeight; ns++;
            map[nmap++] = i;
        }
        logf_("EnumDisplaySettings: %d resolucoes", nmap);
    }
    // modo extra: a resolução 4:3 nativa (não existe como modo do monitor, mas aqui nada é trocado de verdade)
    if (g_nat_w && mode == (DWORD)nmap) {
        if (!r_EnumDisplaySettingsA(dev, ENUM_CURRENT_SETTINGS, dm)) return FALSE;
        dm->dmPelsWidth = g_nat_w; dm->dmPelsHeight = g_nat_h; dm->dmBitsPerPel = 16;
        return TRUE;
    }
    if ((int)mode >= nmap) return FALSE;
    BOOL r = r_EnumDisplaySettingsA(dev, map[mode], dm);
    if (r && dm) dm->dmBitsPerPel = 16;
    return r;
}

static int WINAPI h_GetDeviceCaps(HDC dc, int idx) {
    int r = r_GetDeviceCaps(dc, idx);
    if (idx == BITSPIXEL && r > 16) return 16;
    if (is_game_dc(dc)) { if (idx == HORZRES) return g_gw; if (idx == VERTRES) return g_gh; }
    return r;
}

// ---------------------------------------------------------------- 2D (GDI): tela virtual
// Tudo o que o jogo desenha com GDI na janela vai para uma tela virtual g_gw x g_gh; essa tela é ampliada
// (com suavização) para o retângulo 4:3 na resolução nativa. O DC real continua sendo do jogo (OpenGL precisa dele).
typedef BOOL (WINAPI *PFN_BB)(HDC, int, int, int, int, HDC, int, int, DWORD);
typedef int (WINAPI *PFN_SDIB)(HDC, int, int, int, int, int, int, int, int, const VOID *, const BITMAPINFO *, UINT, DWORD);
typedef int (WINAPI *PFN_FR)(HDC, const RECT *, HBRUSH);
typedef int (WINAPI *PFN_DT)(HDC, LPCSTR, int, LPRECT, UINT);
typedef HGDIOBJ (WINAPI *PFN_SO)(HDC, HGDIOBJ);
typedef COLORREF (WINAPI *PFN_SC)(HDC, COLORREF);
typedef int (WINAPI *PFN_SBM)(HDC, int);
typedef UINT (WINAPI *PFN_STA)(HDC, UINT);
typedef int (WINAPI *PFN_RDC)(HWND, HDC);
typedef BOOL (WINAPI *PFN_EP2)(HWND, const PAINTSTRUCT *);
typedef BOOL (WINAPI *PFN_PM)(LPMSG, HWND, UINT, UINT, UINT);
static PFN_BB r_BitBlt; static PFN_SDIB r_StretchDIBits; static PFN_FR r_FillRect; static PFN_DT r_DrawTextA;
static PFN_SO r_SelectObject; static PFN_SC r_SetBkColor, r_SetTextColor; static PFN_SBM r_SetBkMode; static PFN_STA r_SetTextAlign;
static PFN_RDC r_ReleaseDC; static PFN_EP2 r_EndPaint; static PFN_PM r_PeekMessageA;
typedef HDC (WINAPI *PFN_BP)(HWND, LPPAINTSTRUCT);
static PFN_BP r_BeginPaint;
// WM_PAINT: o jogo redesenha só o pedaço inválido (rcPaint) e conta com o recorte do Windows para não apagar o
// resto (telas de opções: fundo inteiro + só o item que mudou). Na tela virtual o mesmo recorte é aplicado.
static HDC g_paint_dc; static RECT g_paint_rc;

static HBITMAP g_vbmp, g_vold; static int g_vdw, g_vdh;
static int g_dirty; static DWORD g_last_present;

static int gdi_redirect(HDC dc) { return cfg.enabled && is_game_dc(dc); }
// Tela virtual 2D em alta resolução: o jogo desenha os menus em 640x480; aqui cada coordenada é multiplicada por
// g_k (= altura da imagem na janela / 480) e as fontes são recriadas no tamanho final. Texto nítido; as imagens
// de fundo (bitmaps 640x480) são ampliadas com suavização.
static double g_k = 1.0;
static int KX(int v) { return (int)floor(v * g_k + 0.5); }
static double want_k(void) {
    if (!cfg.hires_menu || !g_gh || !g_vh) return 1.0;
    double k = (double)g_vh / g_gh;
    return k < 1.0 ? 1.0 : k > 4.0 ? 4.0 : k;
}
static struct { HFONT src, dst; double k; } g_fc[32]; static int g_fcn;
static HFONT scaled_font(HFONT f) {
    if (g_k == 1.0 || !f) return f;
    for (int i = 0; i < g_fcn; i++) if (g_fc[i].src == f && g_fc[i].k == g_k) return g_fc[i].dst;
    LOGFONTA lf; if (!GetObjectA(f, sizeof lf, &lf)) return f;
    lf.lfHeight = (LONG)floor(lf.lfHeight * g_k + (lf.lfHeight < 0 ? -0.5 : 0.5));
    lf.lfWidth = (LONG)floor(lf.lfWidth * g_k + 0.5);
    lf.lfQuality = ANTIALIASED_QUALITY;
    HFONT n = CreateFontIndirectA(&lf);
    if (!n) return f;
    int slot = g_fcn < 32 ? g_fcn++ : 0;
    if (slot == 0 && g_fcn == 32 && g_fc[0].dst) DeleteObject(g_fc[0].dst);
    g_fc[slot].src = f; g_fc[slot].dst = n; g_fc[slot].k = g_k;
    return n;
}
static HDC vdc(void) {
    update_rect();
    double k = want_k();
    int vw = (int)floor(g_gw * k + 0.5), vh = (int)floor(g_gh * k + 0.5);
    if (g_vdc && g_vdw == vw && g_vdh == vh) return g_vdc;
    HDC scr = GetDC(NULL);
    HDC nd = CreateCompatibleDC(scr);
    HBITMAP nb = CreateCompatibleBitmap(scr, vw, vh);
    ReleaseDC(NULL, scr);
    HBITMAP no = (HBITMAP)SelectObject(nd, nb);
    RECT r = { 0, 0, vw, vh }; FillRect(nd, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    if (g_vdc) {  // preserva o conteúdo ao trocar de tamanho
        SetStretchBltMode(nd, HALFTONE); SetBrushOrgEx(nd, 0, 0, NULL);
        StretchBlt(nd, 0, 0, vw, vh, g_vdc, 0, 0, g_vdw, g_vdh, SRCCOPY);
        SelectObject(g_vdc, g_vold); DeleteObject(g_vbmp); DeleteDC(g_vdc);
    }
    g_vdc = nd; g_vbmp = nb; g_vold = no; g_vdw = vw; g_vdh = vh; g_k = k;
    SetBkMode(g_vdc, TRANSPARENT);
    logf_("tela virtual 2D %dx%d (jogo %dx%d, escala %.2f)", vw, vh, g_gw, g_gh, k);
    return g_vdc;
}
static void stretch_mode(HDC d, DWORD rop) {
    SetStretchBltMode(d, rop == SRCCOPY ? HALFTONE : COLORONCOLOR);
    if (rop == SRCCOPY) SetBrushOrgEx(d, 0, 0, NULL);
}

// ---- fundo das laterais das telas 2D (menu_fill=1): a própria tela, desfocada e escurecida
static HDC g_fdc; static HBITMAP g_fbmp, g_fold; static DWORD *g_fbits; static int g_fw, g_fh;
static HDC g_sdc; static HBITMAP g_sbmp, g_sold; static DWORD *g_sbits;
static DWORD g_fsmall[32 * 24], g_ftime; static int g_fvalid, g_fill_pending;
#define FSW 32
#define FSH 24
static HBITMAP dib32(HDC dc, int w, int h, DWORD **bits) {
    BITMAPINFO bi; memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize = sizeof bi.bmiHeader; bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    return CreateDIBSection(dc, &bi, DIB_RGB_COLORS, (void **)bits, NULL, 0);
}
// ---- laterais que continuam o fundo (menu_fill=3): os fundos das telas 2D são anéis concêntricos (levemente
// achatados) ou uma cor lisa. O mod mede isso na própria tela (centro, achatamento e a cor de cada raio, com a
// mediana para ignorar o que estiver desenhado por cima) e desenha a continuação nas laterais: 16:9 de verdade.
// Fundo que não é nenhum dos dois: volta ao desfoque (menu_fill=1).
static BYTE *fe_menu(void);
#define BG_BIN 0.5
#define BG_NB 1400
typedef struct { int kind; double cx, cy, sx, P, rmax; int nb; float prof[BG_NB][3]; DWORD solid[4]; DWORD sig[48];
                 float (*hp)[3]; int hnb; double hbin; } BGMODEL;      // hp: perfil medido na imagem HD (faixas finas)
static BGMODEL g_bgm[6]; static int g_bgm_n, g_bgm_cur = -1, g_bgm_drawn = -1, g_bgm_dw, g_bgm_dh;
static HDC g_gdc; static HBITMAP g_gbmp, g_gold; static DWORD *g_gbits; static int g_gbw, g_gbh;
static int bg_lum(DWORD v) { return ((v & 255) * 29 + (v >> 8 & 255) * 150 + (v >> 16 & 255) * 77) >> 8; }
static float bg_cost(const float *xs, const float *ys, const float *ls, int n, double cx, double cy, double sx) {
    static float sum[700]; static int cnt[700]; static short bins[9000];
    memset(sum, 0, sizeof sum); memset(cnt, 0, sizeof cnt);
    float fcx = (float)cx, fcy = (float)cy, fsx = (float)sx;
    for (int i = 0; i < n; i++) {
        float dx = (xs[i] - fcx) * fsx, dy = ys[i] - fcy; int b = (int)sqrtf(dx * dx + dy * dy);
        if (b >= 700) b = 699;
        bins[i] = (short)b; sum[b] += ls[i]; cnt[b]++;
    }
    for (int b = 0; b < 700; b++) if (cnt[b]) sum[b] /= cnt[b];
    float c = 0;
    for (int i = 0; i < n; i++) { float d = fabsf(ls[i] - sum[bins[i]]); c += d < 30 ? d : 30; }
    return c / (n ? n : 1);
}
static float bg_med(float *a, int n) {                // mediana (seleção parcial)
    int k = n / 2, lo = 0, hi = n - 1;
    while (lo < hi) {
        float piv = a[(lo + hi) / 2]; int i = lo, j = hi;
        while (i <= j) { while (a[i] < piv) i++; while (a[j] > piv) j--; if (i <= j) { float t = a[i]; a[i] = a[j]; a[j] = t; i++; j--; } }
        if (k <= j) hi = j; else if (k >= i) lo = i; else break;
    }
    return a[k];
}
static void bg_binmean(const float *xs, const float *ys, const float *ls, int n, double cx, double cy, double sx, float *mean) {
    static int cnt[700]; memset(cnt, 0, sizeof cnt); memset(mean, 0, 700 * sizeof *mean);
    float fcx = (float)cx, fcy = (float)cy, fsx = (float)sx;
    for (int i = 0; i < n; i++) {
        float dx = (xs[i] - fcx) * fsx, dy = ys[i] - fcy; int b = (int)sqrtf(dx * dx + dy * dy);
        if (b >= 700) b = 699;
        mean[b] += ls[i]; cnt[b]++;
    }
    for (int b = 0; b < 700; b++) mean[b] = cnt[b] ? mean[b] / cnt[b] : -1000.0f;
}
// quanto a luminância l foge do perfil médio no raio r (descontando a inclinação: nas bordas dos anéis a cor muda rápido)
static float bg_dev(const float *mean, float r, float l) {
    float f = r - 0.5f; int b = (int)f; if (b < 0) b = 0; if (b > 698) b = 698;
    float t = f - b; if (t < 0) t = 0; if (t > 1) t = 1;
    float a = mean[b], c = mean[b + 1];
    if (a < -500) a = c; if (c < -500) c = a;
    if (a < -500) return 0;
    return fabsf(l - (a + (c - a) * t)) - fabsf(c - a) * 0.75f;
}
// clean = imagem do fundo sem nada do menu por cima (lida do bitmap que o jogo carregou)
static int bg_fit(const DWORD *px, int w, int h, BGMODEL *m, int clean) {
    static BYTE ex[480][640];
    if (w != 640 || h != 480) return 0;
    memset(ex, 0, sizeof ex);
    BYTE *d = clean ? NULL : fe_menu();               // o que o menu desenha por cima não entra na medida
    for (int pn = 0; d && pn < 16 && !IsBadReadPtr(d, 0x20); pn++, d = *(BYTE **)(d + 0x18)) {
        BYTE *it = *(BYTE **)(d + 8);
        for (int k = 0; it && k < 96 && !IsBadReadPtr(it, 0x118); k++, it = *(BYTE **)(it + 0x110)) {
            int x0 = *(int *)(it + 0x70) - 4, y0 = *(int *)(it + 0x74) - 4, x1 = x0 + *(int *)(it + 0x78) + 8, y1 = y0 + *(int *)(it + 0x7c) + 8;
            if (*(int *)(it + 0x6c) == 4) { x0 -= 24; x1 += 24; y1 += 34; }   // imagem: o nome vai escrito embaixo
            if (x1 - x0 <= 8 || y1 - y0 <= 8) continue;
            if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 > 640) x1 = 640; if (y1 > 480) y1 = 480;
            for (int y = y0; y < y1; y++) memset(&ex[y][x0 < 640 ? x0 : 640], 1, x1 > x0 ? x1 - x0 : 0);
        }
    }
    // cor lisa nas bordas (telas de abertura)
    double ss[2][4] = { { 0 } }; int ok = 1;
    for (int side = 0; side < 2 && ok; side++) {
        double a[3] = { 0 }, a2 = 0, al = 0; int n = 0;
        for (int y = 0; y < h; y++) for (int x = side ? w - 6 : 0; x < (side ? w : 6); x++) {
            DWORD v = px[y * w + x]; int l = bg_lum(v);
            a[0] += v & 255; a[1] += v >> 8 & 255; a[2] += v >> 16 & 255; al += l; a2 += (double)l * l; n++;
        }
        double mean = al / n, var = a2 / n - mean * mean;
        if (var > 6.0) ok = 0;
        for (int c = 0; c < 3; c++) ss[side][c] = a[c] / n;
    }
    if (ok) {
        m->kind = 1;
        for (int side = 0; side < 2; side++) m->solid[side] = (DWORD)(ss[side][0] + 0.5) | (DWORD)(ss[side][1] + 0.5) << 8 | (DWORD)(ss[side][2] + 0.5) << 16;
        m->solid[2] = m->solid[0]; m->solid[3] = m->solid[1];
        return 1;
    }
    // anéis: amostras das faixas laterais
    static float xs[9000], ys[9000], ls[9000];
    int tot = 0;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) if ((x < 110 || x >= 530) && !ex[y][x]) tot++;
    if (tot < 2000) return 0;
    int step = tot / 8000 + 1, n = 0, q = 0;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) if ((x < 110 || x >= 530) && !ex[y][x]) {
        if (q++ % step) continue;
        if (n < 9000) { xs[n] = x + 0.5f; ys[n] = y + 0.5f; ls[n] = (float)bg_lum(px[y * w + x]); n++; }
    }
    double bx = 320, by = 240, bs = 1.0; float bc = 1e9f;
    for (int cy = 150; cy <= 410; cy += 16) for (int cx = 240; cx <= 400; cx += 16) {
        float c = bg_cost(xs, ys, ls, n, cx, cy, 1.0);
        if (c < bc) { bc = c; bx = cx; by = cy; }
    }
    static const double st[] = { 8, 4, 2, 1, 0.5, 0.25 }, sst[] = { 0.02, 0.02, 0.01, 0.005, 0.0025, 0.0025 };
    for (int si = 0; si < 6; si++) {
        for (int it = 0, moved = 1; moved && it < 40; it++) {
            moved = 0;
            for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) for (int dz = -1; dz <= 1; dz++) {
                if (!dx && !dy && !dz) continue;
                double cx = bx + dx * st[si], cy = by + dy * st[si], sx = bs + dz * sst[si];
                if (sx < 0.8 || sx > 1.25) continue;
                float c = bg_cost(xs, ys, ls, n, cx, cy, sx);
                if (c < bc - 1e-4f) { bc = c; bx = cx; by = cy; bs = sx; moved = 1; }
            }
        }
    }
    // segunda passada sem o que não segue os anéis (painel, setas e ícones desenhados na própria imagem)
    static float bmean[700]; int n0 = n;
    bg_binmean(xs, ys, ls, n, bx, by, bs, bmean);
    { int k = 0;
      for (int i = 0; i < n; i++) {
          float dx = (xs[i] - (float)bx) * (float)bs, dy = ys[i] - (float)by;
          if (bg_dev(bmean, sqrtf(dx * dx + dy * dy), ls[i]) <= 10.0f) { xs[k] = xs[i]; ys[k] = ys[i]; ls[k] = ls[i]; k++; }
      }
      if (k < n0 * 6 / 10 || k < 1500) { static int nr; if (nr++ < 16) logf_("fundo 2D: so %d de %d pontos seguem aneis", k, n0); return 0; }
      n = k; }
    bc = bg_cost(xs, ys, ls, n, bx, by, bs);
    for (int si = 2; si < 6; si++) {
        for (int it = 0, moved = 1; moved && it < 40; it++) {
            moved = 0;
            for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) for (int dz = -1; dz <= 1; dz++) {
                if (!dx && !dy && !dz) continue;
                double cx = bx + dx * st[si], cy = by + dy * st[si], sx = bs + dz * sst[si];
                if (sx < 0.8 || sx > 1.25) continue;
                float c = bg_cost(xs, ys, ls, n, cx, cy, sx);
                if (c < bc - 1e-4f) { bc = c; bx = cx; by = cy; bs = sx; moved = 1; }
            }
        }
    }
    bg_binmean(xs, ys, ls, n, bx, by, bs, bmean);
    static int nl; if (nl++ < 24) logf_("fundo 2D: aneis centro %.2f,%.2f achatamento %.3f desvio %.2f (%d%% dos pontos)", bx, by, bs, bc, n * 100 / n0);
    if (bc > 3.0f || bx < 200 || bx > 440 || by < 120 || by > 420) return 0;   // não é de anéis (ou o centro fugiu da tela)
    // perfil: mediana de cada cor por faixa de raio (0,5 pixel), com todos os pixels das laterais
    static float *chan[3]; static int *bin, *start, *cntb; static float *tmp;
    if (!bin) { bin = malloc(640 * 480 * 4); start = malloc((BG_NB + 1) * 4); cntb = malloc(BG_NB * 4); tmp = malloc(640 * 480 * 4); for (int c = 0; c < 3; c++) chan[c] = malloc(640 * 480 * 4); }
    if (!bin || !start || !cntb || !tmp || !chan[0] || !chan[1] || !chan[2]) return 0;
    memset(cntb, 0, BG_NB * 4);
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        int i = y * w + x; bin[i] = -1;
        if (!((x < 110 || x >= 530) && !ex[y][x])) continue;
        double dx = (x + 0.5 - bx) * bs, dy = y + 0.5 - by, rr = sqrt(dx * dx + dy * dy); int b = (int)(rr / BG_BIN);
        if (b >= BG_NB) continue;
        if (bg_dev(bmean, (float)rr, (float)bg_lum(px[i])) > 12.0f) continue;   // fora dos anéis (painel, ícones)
        bin[i] = b; cntb[b]++;
    }
    start[0] = 0; for (int b = 0; b < BG_NB; b++) start[b + 1] = start[b] + cntb[b];
    { static int fillp[BG_NB]; memcpy(fillp, start, BG_NB * 4);
      for (int i = 0; i < w * h; i++) if (bin[i] >= 0) { DWORD v = px[i]; int o = fillp[bin[i]]++; chan[0][o] = (float)(v & 255); chan[1][o] = (float)(v >> 8 & 255); chan[2][o] = (float)(v >> 16 & 255); } }
    int last = -1;
    for (int b = 0; b < BG_NB; b++) {
        if (cntb[b] < 3) { m->prof[b][0] = -1; continue; }
        for (int c = 0; c < 3; c++) { memcpy(tmp, chan[c] + start[b], cntb[b] * 4); m->prof[b][c] = bg_med(tmp, cntb[b]); }
        if (cntb[b] >= 6) last = b;
    }
    if (last < 100) return 0;
    for (int b = 0; b <= last; b++) if (m->prof[b][0] < 0) {        // buracos: interpola dos vizinhos
        int a = b - 1, z = b + 1;
        while (a >= 0 && m->prof[a][0] < 0) a--;
        while (z <= last && m->prof[z][0] < 0) z++;
        for (int c = 0; c < 3; c++) m->prof[b][c] = a < 0 ? m->prof[z][c] : z > last ? m->prof[a][c] : m->prof[a][c] + (m->prof[z][c] - m->prof[a][c]) * (b - a) / (float)(z - a);
    }
    m->nb = last + 1; m->rmax = last * BG_BIN; m->cx = bx; m->cy = by; m->sx = bs;
    // período dos anéis (para continuar além do último raio medido)
    int a0 = last - (int)(200 / BG_BIN); if (a0 < 0) a0 = 0;
    double best = -1; int bl = 0; static float corr[402];
    memset(corr, 0, sizeof corr);
    for (int lag = (int)(20 / BG_BIN); lag <= (int)(200 / BG_BIN) && lag < last - a0; lag++) {
        double sxy = 0, sxx = 0, syy = 0, mx = 0, my = 0; int k = 0;
        for (int b = a0; b + lag <= last; b++) { mx += m->prof[b][1]; my += m->prof[b + lag][1]; k++; }
        if (k < 20) break;
        mx /= k; my /= k;
        for (int b = a0; b + lag <= last; b++) { double u = m->prof[b][1] - mx, v = m->prof[b + lag][1] - my; sxy += u * v; sxx += u * u; syy += v * v; }
        double r = sxy / sqrt(sxx * syy + 1e-9);
        corr[lag] = (float)r;
        if (r > best) { best = r; bl = lag; }
    }
    {   // o melhor pode ser meio período (os anéis claro e escuro se parecem): confere perto do dobro
        int lo = bl * 2 - bl / 10, hi = bl * 2 + bl / 10, b2 = -1;
        for (int lag = lo; lag <= hi && lag <= (int)(200 / BG_BIN) && lag < last - a0; lag++) if (b2 < 0 || corr[lag] > corr[b2]) b2 = lag;
        if (b2 > 0 && corr[b2] >= best - 0.06) bl = b2;
    }
    m->P = best > 0.8 ? bl * BG_BIN : 0;
    if (nl < 24) logf_("fundo 2D: perfil ate o raio %.1f, periodo %.1f (correlacao %.3f)", m->rmax, m->P, best);
    if (m->P < 60) return 0;                          // sem repetição clara (anéis largos): não é o fundo de anéis
    m->kind = 2;
    return 1;
}
// laterais (e faixas de cima/baixo) de uma imagem W x H cuja parte 4:3 (640x480) fica em (vx,vy) com vw x vh
static void bg_render_to(const BGMODEL *m, DWORD *dst, int W, int H, int vx, int vy, int vw, int vh) {
    float kx = 640.0f / vw, ky = 480.0f / vh, fsx = (float)m->sx, fcx = (float)m->cx, fcy = (float)m->cy;
    const float (*prof)[3] = m->hp ? (const float (*)[3])m->hp : (const float (*)[3])m->prof;
    float bin = m->hp ? (float)m->hbin : (float)BG_BIN; int nb2 = (m->hp ? m->hnb : m->nb) - 2;
    float rmax = (float)m->rmax, P = (float)m->P, inv = 1.0f / bin;
    if (m->hp && (nb2 + 1) * bin < rmax) rmax = (nb2 + 1) * bin;
    for (int Y = 0; Y < H; Y++) {
        int inside_y = Y >= vy && Y < vy + vh;
        float dy = (Y + 0.5f - vy) * ky - fcy, dy2 = dy * dy;
        DWORD *row = dst + (size_t)Y * W;
        for (int X = 0; X < W; X++) {
            if (inside_y && X >= vx && X < vx + vw) { X = vx + vw - 1; continue; }
            if (m->kind == 1) { row[X] = m->solid[X < vx ? 0 : X >= vx + vw ? 1 : Y < vy ? 2 : 3]; continue; }
            float dx = ((X + 0.5f - vx) * kx - fcx) * fsx, r = sqrtf(dx * dx + dy2);
            if (r > rmax) r -= P * ceilf((r - rmax) / P);
            float f = r * inv; int i = (int)f; if (i < 0) i = 0; if (i > nb2) i = nb2;
            float t = f - i; if (t < 0) t = 0; if (t > 1) t = 1;
            const float *a = prof[i], *b = prof[i + 1];
            int c0 = (int)(a[0] + (b[0] - a[0]) * t + 0.5f), c1 = (int)(a[1] + (b[1] - a[1]) * t + 0.5f), c2 = (int)(a[2] + (b[2] - a[2]) * t + 0.5f);
            if (c0 > 255) c0 = 255; if (c1 > 255) c1 = 255; if (c2 > 255) c2 = 255;
            if (c0 < 0) c0 = 0; if (c1 < 0) c1 = 0; if (c2 < 0) c2 = 0;
            row[X] = (DWORD)c0 | (DWORD)c1 << 8 | (DWORD)c2 << 16;
        }
    }
}
static void bg_render(const BGMODEL *m) { bg_render_to(m, g_fbits, g_winw, g_winh, g_vx, g_vy, g_vw, g_vh); GdiFlush(); }
// atualiza o modelo do fundo quando as bordas da tela mudam; 1 = laterais prontas em g_fdc
static int bg_update(void) {
    DWORD sig[48];
    for (int y = 0; y < FSH; y++) { sig[y] = g_sbits[y * FSW] & 0xfcfcfc; sig[24 + y] = g_sbits[y * FSW + FSW - 1] & 0xfcfcfc; }
    int cur = -1;
    for (int i = 0; i < g_bgm_n; i++) if (!memcmp(g_bgm[i].sig, sig, sizeof sig)) { cur = i; break; }
    if (cur < 0) {
        if (!g_gdc || g_gbw != g_gw || g_gbh != g_gh) {
            if (g_gdc) { SelectObject(g_gdc, g_gold); DeleteObject(g_gbmp); DeleteDC(g_gdc); g_gdc = NULL; }
            HDC scr = GetDC(NULL); g_gdc = CreateCompatibleDC(scr); g_gbmp = dib32(scr, g_gw, g_gh, &g_gbits); ReleaseDC(NULL, scr);
            if (!g_gbmp) { DeleteDC(g_gdc); g_gdc = NULL; return 0; }
            g_gold = SelectObject(g_gdc, g_gbmp); g_gbw = g_gw; g_gbh = g_gh;
        }
        SetStretchBltMode(g_gdc, COLORONCOLOR);
        StretchBlt(g_gdc, 0, 0, g_gw, g_gh, g_vdc, 0, 0, g_vdw, g_vdh, SRCCOPY);
        GdiFlush();
        int slot = g_bgm_n < 6 ? g_bgm_n++ : (g_bgm_cur + 1) % 6;
        if (slot == g_bgm_drawn) g_bgm_drawn = -1;
        BGMODEL *m = &g_bgm[slot];
        m->kind = 0;
        DWORD t0 = GetTickCount();
        if (!bg_fit(g_gbits, g_gw, g_gh, m, 0)) m->kind = 0;
        memcpy(m->sig, sig, sizeof sig);
        static int nl; if (nl++ < 16) logf_("fundo 2D: %s (%u ms)", m->kind == 2 ? "aneis, laterais continuam o desenho" : m->kind == 1 ? "cor lisa nas bordas" : "outro, laterais desfocadas", GetTickCount() - t0);
        cur = slot;
    }
    g_bgm_cur = cur;
    if (!g_bgm[cur].kind) return 0;
    if (g_bgm_drawn != cur || g_bgm_dw != g_winw || g_bgm_dh != g_winh) { bg_render(&g_bgm[cur]); g_bgm_drawn = cur; g_bgm_dw = g_winw; g_bgm_dh = g_winh; }
    return 1;
}
static int fill_update(void) {
    if (!g_vdc || g_winw <= 0 || g_winh <= 0) return 0;
    if (!g_fdc || g_fw != g_winw || g_fh != g_winh) {
        if (g_fdc) { SelectObject(g_fdc, g_fold); DeleteObject(g_fbmp); DeleteDC(g_fdc); g_fdc = NULL; }
        HDC scr = GetDC(NULL);
        g_fdc = CreateCompatibleDC(scr); g_fbmp = dib32(scr, g_winw, g_winh, &g_fbits);
        if (!g_sdc) { g_sdc = CreateCompatibleDC(scr); g_sbmp = dib32(scr, FSW, FSH, &g_sbits); g_sold = SelectObject(g_sdc, g_sbmp); }
        ReleaseDC(NULL, scr);
        if (!g_fbmp || !g_sbmp) { if (g_fdc) DeleteDC(g_fdc); g_fdc = NULL; return 0; }
        g_fold = SelectObject(g_fdc, g_fbmp); g_fw = g_winw; g_fh = g_winh; g_fvalid = 0;
    }
    DWORD now = GetTickCount();
    if (g_fvalid && now - g_ftime < 120) { g_fill_pending = 1; return 1; }   // no máximo ~8 vezes por segundo
    g_ftime = now; g_fill_pending = 0;
    SetStretchBltMode(g_sdc, HALFTONE); SetBrushOrgEx(g_sdc, 0, 0, NULL);
    StretchBlt(g_sdc, 0, 0, FSW, FSH, g_vdc, 0, 0, g_vdw, g_vdh, SRCCOPY);
    GdiFlush();
    if (cfg.menu_fill == 3) {
        if (bg_update()) { g_fvalid = 1; memset(g_fsmall, 0, sizeof g_fsmall); return 1; }
        g_bgm_drawn = -1;                             // fundo sem modelo: desfoque
    }
    if (g_fvalid && !memcmp(g_fsmall, g_sbits, sizeof g_fsmall)) return 1;
    memcpy(g_fsmall, g_sbits, sizeof g_fsmall);
    // desfoque (3 passadas de caixa 3x3) em ponto flutuante
    static float buf[2][FSH][FSW][3];
    for (int y = 0; y < FSH; y++) for (int x = 0; x < FSW; x++) { DWORD v = g_fsmall[y * FSW + x]; buf[0][y][x][0] = v & 255; buf[0][y][x][1] = v >> 8 & 255; buf[0][y][x][2] = v >> 16 & 255; }
    int cur = 0;
    for (int pass = 0; pass < 3; pass++) {
        for (int y = 0; y < FSH; y++) for (int x = 0; x < FSW; x++) for (int c = 0; c < 3; c++) {
            float a = 0; int n = 0;
            for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
                int yy = y + dy, xx = x + dx; if (yy < 0 || xx < 0 || yy >= FSH || xx >= FSW) continue;
                a += buf[cur][yy][xx][c]; n++;
            }
            buf[!cur][y][x][c] = a / n;
        }
        cur = !cur;
    }
    // amplia (bilinear) cobrindo a janela, só nas faixas fora da imagem; escurece e faz sombra junto da imagem
    double sc = (double)g_winw / FSW > (double)g_winh / FSH ? (double)g_winw / FSW : (double)g_winh / FSH;
    double ox = (g_winw - FSW * sc) / 2, oy = (g_winh - FSH * sc) / 2;
    for (int Y = 0; Y < g_winh; Y++) {
        int inside_y = Y >= g_vy && Y < g_vy + g_vh;
        for (int X = 0; X < g_winw; X++) {
            if (inside_y && X >= g_vx && X < g_vx + g_vw) { X = g_vx + g_vw - 1; continue; }
            double u = (X - ox) / sc - 0.5, v = (Y - oy) / sc - 0.5;
            int x0 = (int)floor(u), y0 = (int)floor(v); double fx = u - x0, fy = v - y0;
            int x1 = x0 + 1, y1 = y0 + 1;
            if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 >= FSW) x1 = FSW - 1; if (y1 >= FSH) y1 = FSH - 1;
            if (x0 >= FSW) x0 = FSW - 1; if (y0 >= FSH) y0 = FSH - 1; if (x1 < 0) x1 = 0; if (y1 < 0) y1 = 0;
            // distância até a imagem central (sombra suave de ~60 px)
            int dx = X < g_vx ? g_vx - X : X >= g_vx + g_vw ? X - (g_vx + g_vw - 1) : 0;
            int dy = Y < g_vy ? g_vy - Y : Y >= g_vy + g_vh ? Y - (g_vy + g_vh - 1) : 0;
            double d = dx > dy ? dx : dy, shade = 0.42 * (0.55 + 0.45 * (d > 60 ? 1.0 : d / 60.0));
            DWORD o = 0;
            for (int c = 0; c < 3; c++) {
                double val = (buf[cur][y0][x0][c] * (1 - fx) + buf[cur][y0][x1][c] * fx) * (1 - fy) +
                             (buf[cur][y1][x0][c] * (1 - fx) + buf[cur][y1][x1][c] * fx) * fy;
                int iv = (int)(val * shade + 0.5); if (iv > 255) iv = 255;
                o |= (DWORD)iv << (8 * c);
            }
            g_fbits[(size_t)Y * g_winw + X] = o;
        }
    }
    GdiFlush();
    g_fvalid = 1;
    return 1;
}
// ---- fundos 2D prontos em 16:9 (menu_fill=3). Cada bitmap 640x480 que o jogo carrega (LoadImageA) é medido na
// hora, limpo, sem nada do menu por cima. Na primeira vez que aparece vira uma imagem do tamanho da janela: o meio é
// o próprio bitmap ampliado e as laterais continuam os anéis. Daí em diante, quando o jogo desenha o fundo, a tela
// inteira sai pronta de uma vez (sem medir nada nem esperar). O que não for um desses fundos usa o caminho acima.
// Pacote HD opcional (pasta hd ao lado do mod, gerado por tools/hd_menu): hd\<nome>.bmp é a imagem 4x e
// hd\1x\<nome>.bmp substitui a original em 1x (é assim que a tela inicial fica com o azul escuro).
typedef struct { DWORD hash; int kind; BGMODEL *m; DWORD *spx; HDC sdc; HBITMAP sbm; HGDIOBJ sold; char hd[MAX_PATH];
                 HDC wdc; HBITMAP wbmp; HGDIOBJ wold; DWORD *wbits; int ww, wh, wvx, wvy, wvw, wvh; DWORD used, wver; } BGIMG;
#define BGI_MAX 16
static BGIMG g_bgi[BGI_MAX]; static int g_bgi_n;
static struct { HBITMAP h; int e; } g_bgh[64]; static int g_bgh_n;   // bitmaps do jogo -> fundo
static int g_bg_live = -1;                            // fundo inteiro na tela agora (-1: outro desenho)
static void bg_wfree_idx(int i);
static void bg_live(int e, const char *why) {
    if (e == g_bg_live) return;
    int old = g_bg_live; g_bg_live = e;
    if (old >= 0 && g_bgi[old].kind == 1) bg_wfree_idx(old);   // aberturas: aparecem uma vez só
    static int nl; if (nl++ < 40) logf_("fundo 2D na tela: %d -> %d (%s)", old, e, why);
}
static HDC dib_dc(int w, int h, DWORD **bits, HBITMAP *bm, HGDIOBJ *old) {
    HDC scr = GetDC(NULL); HDC d = CreateCompatibleDC(scr); *bm = dib32(scr, w, h, bits); ReleaseDC(NULL, scr);
    if (!d || !*bm) { if (*bm) DeleteObject(*bm); if (d) DeleteDC(d); *bm = NULL; return NULL; }
    *old = SelectObject(d, *bm); return d;
}
static int bmp_pixels(HBITMAP hb, int w, int h, DWORD *dst) {   // bitmap (fora de qualquer DC) -> 32 bits, de cima p/ baixo
    BITMAPINFO bi; memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize = sizeof bi.bmiHeader; bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    HDC scr = GetDC(NULL); int r = GetDIBits(scr, hb, 0, h, dst, &bi, DIB_RGB_COLORS); ReleaseDC(NULL, scr);
    return r == h;
}
static void bg_map(HBITMAP hb, int e) {
    for (int i = 0; i < g_bgh_n; i++) if (g_bgh[i].h == hb) { g_bgh[i].e = e; return; }
    if (g_bgh_n < 64) { g_bgh[g_bgh_n].h = hb; g_bgh[g_bgh_n].e = e; g_bgh_n++; }
}
static int bg_entry_px(const DWORD *px, const char *hd) {   // fundo com estes pixels (cria e mede na 1ª vez)
    DWORD h = 2166136261u; for (int i = 0; i < 640 * 480; i++) h = (h ^ (px[i] & 0xffffff)) * 16777619u;
    for (int i = 0; i < g_bgi_n; i++) if (g_bgi[i].hash == h) { if (hd && !g_bgi[i].hd[0]) snprintf(g_bgi[i].hd, MAX_PATH, "%s", hd); return i; }
    if (g_bgi_n >= BGI_MAX) return -1;
    BGIMG *b = &g_bgi[g_bgi_n]; memset(b, 0, sizeof *b); b->hash = h;
    b->sdc = dib_dc(640, 480, &b->spx, &b->sbm, &b->sold);
    if (!b->sdc) return -1;
    memcpy(b->spx, px, 640 * 480 * 4); GdiFlush();
    if (hd) snprintf(b->hd, MAX_PATH, "%s", hd);
    b->m = (BGMODEL *)calloc(1, sizeof(BGMODEL));
    DWORD t0 = GetTickCount();
    if (b->m && bg_fit(px, 640, 480, b->m, 1)) b->kind = b->m->kind;
    int e = g_bgi_n++;
    logf_("fundo 2D %d: %s%s (%u ms)", e, b->kind == 2 ? "aneis" : b->kind == 1 ? "cor lisa" : "outro (laterais desfocadas)", b->hd[0] ? ", com imagem HD" : "", GetTickCount() - t0);
    return e;
}
static HDC g_pdc; static HBITMAP g_pbmp; static HGDIOBJ g_pold; static DWORD *g_pbits;
static int bg_entry_for(HDC s) {                       // fundo do bitmap selecionado em s (-1: não é 640x480)
    HBITMAP hb = (HBITMAP)GetCurrentObject(s, OBJ_BITMAP);
    if (!hb) return -1;
    for (int i = 0; i < g_bgh_n; i++) if (g_bgh[i].h == hb) return g_bgh[i].e;
    BITMAP bm; if (!GetObjectA(hb, sizeof bm, &bm) || bm.bmWidth != 640 || bm.bmHeight != 480) return -1;
    if (!g_pdc && !(g_pdc = dib_dc(640, 480, &g_pbits, &g_pbmp, &g_pold))) return -1;
    BitBlt(g_pdc, 0, 0, 640, 480, s, 0, 0, SRCCOPY); GdiFlush();
    int e = bg_entry_px(g_pbits, NULL);
    bg_map(hb, e);
    return e;
}
static void bg_forget(HDC d) {                         // o jogo desenhou num bitmap: medir de novo se for fundo
    if (!g_bgh_n || !d) return;
    HBITMAP hb = (HBITMAP)GetCurrentObject(d, OBJ_BITMAP);
    for (int i = 0; i < g_bgh_n; i++) if (g_bgh[i].h == hb) { g_bgh[i] = g_bgh[--g_bgh_n]; return; }
}
static int bg_wready(int e) {
    BGIMG *b = &g_bgi[e];
    return b->wdc && b->ww == g_winw && b->wh == g_winh && b->wvx == g_vx && b->wvy == g_vy && b->wvw == g_vw && b->wvh == g_vh;
}
static void bg_wfree(BGIMG *b) { if (b->wdc) { SelectObject(b->wdc, b->wold); DeleteObject(b->wbmp); DeleteDC(b->wdc); b->wdc = NULL; b->wbits = NULL; } }
static void bg_wfree_idx(int i) { bg_wfree(&g_bgi[i]); }
static void bg_release(void) { for (int i = 0; i < g_bgi_n; i++) bg_wfree(&g_bgi[i]); bg_live(-1, "OpenGL"); }
static int bg_walive(int except) { int n = 0; for (int i = 0; i < g_bgi_n; i++) if (i != except && g_bgi[i].wdc && g_bgi[i].kind == 2) n++; return n; }
// perfil dos anéis medido na própria imagem HD (mesmo centro e achatamento do 1x, faixas 4x mais finas): as
// laterais ficam com as bordas tão nítidas quanto o meio, sem emenda
static void bg_hdprof(BGMODEL *m, HBITMAP hb) {
    DIBSECTION ds; if (!GetObjectA(hb, sizeof ds, &ds) || !ds.dsBm.bmBits || (ds.dsBm.bmBitsPixel != 24 && ds.dsBm.bmBitsPixel != 32)) return;
    int W = ds.dsBm.bmWidth, H = ds.dsBm.bmHeight, bpp = ds.dsBm.bmBitsPixel / 8, stride = ds.dsBm.bmWidthBytes, up = ds.dsBmih.biHeight > 0;
    double f = W / 640.0; if (f < 1.5 || fabs(H / 480.0 - f) > 0.01) return;
    double bin = BG_BIN / f; int nb = (int)(m->rmax / bin) + 2;
    double *acc = (double *)calloc((size_t)nb * 4, sizeof(double)); if (!acc) return;
    const BYTE *bits = (const BYTE *)ds.dsBm.bmBits; int x0 = (int)(110 * f), x1 = (int)(530 * f);
    for (int y = 0; y < H; y++) {
        const BYTE *row = bits + (size_t)(up ? H - 1 - y : y) * stride;
        double dy = (y + 0.5) / f - m->cy, dy2 = dy * dy;
        for (int x = 0; x < W; x++) {
            if (x == x0) { x = x1 - 1; continue; }
            double dx = ((x + 0.5) / f - m->cx) * m->sx, r = sqrt(dx * dx + dy2); int b = (int)(r / bin);
            if (b >= nb) continue;
            const BYTE *p = row + x * bpp; float l = (float)((p[0] * 29 + p[1] * 150 + p[2] * 77) >> 8);
            // fora dos anéis (painel, ícones): compara com o perfil 1x perto deste raio
            float lo = 1e9f, hi = -1e9f;
            for (int k = -1; k <= 1; k++) {
                int i = (int)((r + k * 0.75) / BG_BIN); if (i < 0) i = 0; if (i > m->nb - 1) i = m->nb - 1;
                float ml = (m->prof[i][0] * 29 + m->prof[i][1] * 150 + m->prof[i][2] * 77) / 256.0f;
                if (ml < lo) lo = ml; if (ml > hi) hi = ml;
            }
            if (l < lo - 12 || l > hi + 12) continue;
            double *a = acc + (size_t)b * 4; a[0] += p[0]; a[1] += p[1]; a[2] += p[2]; a[3] += 1;
        }
    }
    float (*hp)[3] = (float (*)[3])malloc((size_t)nb * sizeof *hp); int last = -1, first = -1;
    if (!hp) { free(acc); return; }
    for (int b = 0; b < nb; b++) {
        double *a = acc + (size_t)b * 4;
        if (a[3] >= 2) { for (int c = 0; c < 3; c++) hp[b][c] = (float)(a[c] / a[3]); last = b; if (first < 0) first = b; }
        else hp[b][0] = -1;
    }
    free(acc);
    if (last < 0) { free(hp); return; }
    for (int b = 0; b <= last; b++) if (hp[b][0] < 0) {           // buracos e o começo (sem amostras): do 1x / vizinhos
        int a = b - 1, z = b + 1;
        while (a >= 0 && hp[a][0] < 0) a--;
        while (z <= last && hp[z][0] < 0) z++;
        if (a < 0) { int i = (int)(b * bin / BG_BIN); if (i > m->nb - 1) i = m->nb - 1; for (int c = 0; c < 3; c++) hp[b][c] = m->prof[i][c]; continue; }
        for (int c = 0; c < 3; c++) hp[b][c] = z > last ? hp[a][c] : hp[a][c] + (hp[z][c] - hp[a][c]) * (b - a) / (float)(z - a);
    }
    m->hp = hp; m->hnb = last + 1; m->hbin = bin;
    logf_("fundo 2D: perfil dos aneis medido na imagem HD (%d faixas de %.3f)", m->hnb, bin);
}
static int bg_wbuild(int e) {                          // imagem do tamanho da janela com o fundo e as laterais
    BGIMG *b = &g_bgi[e];
    if (!b->kind || g_winw <= 0 || g_winh <= 0 || g_vw <= 0 || g_vh <= 0) return 0;
    if (bg_wready(e)) { b->used = GetTickCount(); return 1; }
    while (b->kind == 2 && bg_walive(e) >= 3) {       // no máximo 3 de anéis prontas (cada uma tem o tamanho da janela)
        int lru = -1;
        for (int i = 0; i < g_bgi_n; i++) if (i != e && i != g_bg_live && g_bgi[i].wdc && g_bgi[i].kind == 2 && (lru < 0 || g_bgi[i].used - g_bgi[lru].used > 0x80000000u)) lru = i;
        if (lru < 0) break;
        bg_wfree(&g_bgi[lru]);
    }
    if (b->wdc && (b->ww != g_winw || b->wh != g_winh)) bg_wfree(b);
    if (!b->wdc) {
        if (!(b->wdc = dib_dc(g_winw, g_winh, &b->wbits, &b->wbmp, &b->wold))) return 0;
        b->ww = g_winw; b->wh = g_winh;
    }
    DWORD t0 = GetTickCount(); int hd = 0;
    SetStretchBltMode(b->wdc, HALFTONE); SetBrushOrgEx(b->wdc, 0, 0, NULL);
    if (b->hd[0]) {                                   // imagem HD do pacote: lida só agora (é grande)
        HBITMAP hb = (HBITMAP)LoadImageA(NULL, b->hd, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        BITMAP bm;
        if (hb && GetObjectA(hb, sizeof bm, &bm) && bm.bmWidth >= 640 && bm.bmHeight >= 480) {
            if (b->kind == 2 && !b->m->hp) bg_hdprof(b->m, hb);
            HDC t = CreateCompatibleDC(NULL); HGDIOBJ o = SelectObject(t, hb);
            StretchBlt(b->wdc, g_vx, g_vy, g_vw, g_vh, t, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
            SelectObject(t, o); DeleteDC(t); hd = 1;
        }
        if (hb) DeleteObject(hb);
    }
    if (!hd) StretchBlt(b->wdc, g_vx, g_vy, g_vw, g_vh, b->sdc, 0, 0, 640, 480, SRCCOPY);
    GdiFlush();
    bg_render_to(b->m, b->wbits, g_winw, g_winh, g_vx, g_vy, g_vw, g_vh);
    GdiFlush();
    b->wvx = g_vx; b->wvy = g_vy; b->wvw = g_vw; b->wvh = g_vh; b->used = GetTickCount(); b->wver++;
    static int nl; if (nl++ < 24) logf_("fundo 2D %d: tela %dx%d pronta%s (%u ms)", e, g_winw, g_winh, hd ? " (HD)" : "", GetTickCount() - t0);
    return 1;
}
// BitBlt do jogo vindo de um bitmap: se for um fundo conhecido, copia da imagem pronta (1 = desenhado)
static int bg_blit(HDC v, int x, int y, int w, int h, HDC s, int sx, int sy, DWORD rop) {
    if (cfg.menu_fill != 3 || g_gl_on) return 0;
    int full = x <= 0 && y <= 0 && x + w >= g_gw && y + h >= g_gh;
    if (!s || rop != SRCCOPY || x != sx || y != sy || g_gw != 640 || g_gh != 480) { if (full) bg_live(-1, s ? "copia de outra imagem" : "tela pintada"); return 0; }
    int e = bg_entry_for(s);
    if (e < 0 || !g_bgi[e].kind) {
        if (full) {
            // nas trocas de menu o jogo copia a tela inteira de um DC com um ícone pequeno selecionado: só o canto
            // muda e o resto da tela continua igual, então o fundo das laterais também continua
            BITMAP bm; HGDIOBJ hb = GetCurrentObject(s, OBJ_BITMAP); char why[64];
            if (hb && GetObjectA(hb, sizeof bm, &bm)) {
                if (bm.bmWidth < g_gw || bm.bmHeight < g_gh) return 0;
                snprintf(why, sizeof why, "imagem %dx%d %d bits", (int)bm.bmWidth, (int)bm.bmHeight, (int)bm.bmBitsPixel);
            } else snprintf(why, sizeof why, "imagem desconhecida");
            bg_live(-1, why);
        }
        return 0;
    }
    if (full) bg_live(e, "fundo");
    if (g_vdw != g_vw || g_vdh != g_vh || !bg_wbuild(e)) return 0;
    BGIMG *b = &g_bgi[e];
    r_BitBlt(v, KX(x), KX(y), KX(x + w) - KX(x), KX(y + h) - KX(y), b->wdc, b->wvx + KX(x), b->wvy + KX(y), SRCCOPY);
    return 1;
}
static void bg_fullcover(int x, int y, int w, int h, const char *why) { if (x <= 0 && y <= 0 && x + w >= g_gw && y + h >= g_gh) bg_live(-1, why); }

// ---- ícones e outras imagens dos menus com o pacote HD: o ícone ampliado do pacote é misturado ao fundo pronto
// (os ícones do jogo trazem um pedaço do fundo desenhado junto; onde o ícone é só fundo, vale a imagem pronta,
// para não aparecer o retângulo do ícone)
typedef struct { HBITMAP h; int w, hh, own1x; DWORD *base; HDC bdc; HBITMAP bbm; HGDIOBJ bold;
                 HDC hdc; HBITMAP hbm; HGDIOBJ hold; int hdw, hdh;
                 HDC cdc; HBITMAP cbm; HGDIOBJ cold; DWORD *cbits; int cw, ch, cox, coy, cbg; double ck; DWORD cver; } MIMG;
#define MIMG_MAX 160
static MIMG g_mi[MIMG_MAX]; static int g_min;
static void mimg_free(MIMG *m) {
    if (m->cdc) { SelectObject(m->cdc, m->cold); DeleteObject(m->cbm); DeleteDC(m->cdc); }
    if (m->hdc) { SelectObject(m->hdc, m->hold); DeleteObject(m->hbm); DeleteDC(m->hdc); }
    if (m->bdc) { SelectObject(m->bdc, m->bold); DeleteObject(m->bbm); DeleteDC(m->bdc); }
    memset(m, 0, sizeof *m);
}
static MIMG *mimg_find(HBITMAP h) { for (int i = 0; i < g_min; i++) if (g_mi[i].h == h) return &g_mi[i]; return NULL; }
static int g_mimg_hd_n, g_mimg_1x_n;            // contadores do pacote HD dos menus (nome próprio: g_hd_n é a lista de texturas HD)
static void mimg_add(HBITMAP r, const char *name) {
    BITMAP bm; if (!GetObjectA(r, sizeof bm, &bm) || bm.bmWidth <= 0 || bm.bmHeight <= 0 || bm.bmWidth > 4096 || bm.bmHeight > 4096) return;
    int w = bm.bmWidth, h = bm.bmHeight, isbg = w == 640 && h == 480;
    const char *bn = name; for (const char *q = name; *q; q++) if (*q == '\\' || *q == '/') bn = q + 1;
    char p1[MAX_PATH], p4[MAX_PATH];
    snprintf(p1, sizeof p1, "%s\\hd\\1x\\%s", g_dir, bn); snprintf(p4, sizeof p4, "%s\\hd\\%s", g_dir, bn);
    int has1 = cfg.menu_hd && GetFileAttributesA(p1) != INVALID_FILE_ATTRIBUTES, has4 = cfg.menu_hd && GetFileAttributesA(p4) != INVALID_FILE_ATTRIBUTES;
    if (!isbg && !has1 && !has4) return;
    if (isbg && cfg.menu_fill != 3 && !has1 && !has4) return;
    DWORD *px = (DWORD *)malloc((size_t)w * h * 4); if (!px) return;
    int own1x = 0;
    if (has1) {                                       // versão 1x do pacote no lugar da original
        HBITMAP o1 = (HBITMAP)LoadImageA(NULL, p1, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        BITMAP b1; if (o1 && GetObjectA(o1, sizeof b1, &b1) && b1.bmWidth == w && b1.bmHeight == h && bmp_pixels(o1, w, h, px)) own1x = 1;
        if (o1) DeleteObject(o1);
    }
    if (!own1x && !bmp_pixels(r, w, h, px)) { free(px); return; }
    if (isbg) {
        if (cfg.menu_fill == 3) {
            int e = bg_entry_px(px, has4 ? p4 : NULL);
            if (e >= 0) {
                bg_map(r, e);
                if (g_bgi[e].kind == 2 && g_vdc && !g_gl_on && g_vdw == g_vw && g_vdh == g_vh) bg_wbuild(e);
            }
            static int nl; if (nl++ < 32) logf_("imagem %s -> fundo %d%s", bn, e, own1x ? " (versao 1x do pacote)" : "");
        }
        free(px); return;
    }
    if (g_min >= MIMG_MAX) { free(px); return; }
    MIMG *m = &g_mi[g_min]; memset(m, 0, sizeof *m);
    m->h = r; m->w = w; m->hh = h; m->own1x = own1x; m->cbg = -2;
    if (!(m->bdc = dib_dc(w, h, &m->base, &m->bbm, &m->bold))) { free(px); return; }
    memcpy(m->base, px, (size_t)w * h * 4); free(px); GdiFlush();
    if (has4) {
        HBITMAP hb = (HBITMAP)LoadImageA(NULL, p4, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        BITMAP b4;
        if (hb && GetObjectA(hb, sizeof b4, &b4) && b4.bmWidth >= w * 2 && b4.bmWidth % w == 0 && b4.bmHeight == b4.bmWidth / w * h) {
            m->hdc = CreateCompatibleDC(NULL); m->hbm = hb; m->hold = SelectObject(m->hdc, hb); m->hdw = b4.bmWidth; m->hdh = b4.bmHeight;
        } else if (hb) DeleteObject(hb);
    }
    g_min++; if (m->hdc) g_mimg_hd_n++; if (own1x) g_mimg_1x_n++;
    static int nl; if (nl++ < 8) logf_("imagem %s %dx%d: %s%s", bn, w, h, m->hdc ? "HD do pacote" : "sem HD", own1x ? ", versao 1x do pacote" : "");
}
// desenha a imagem m com origem (ox, oy) em pixels do jogo na memória cdc, na escala da tela virtual
static int mimg_render(MIMG *m, int ox, int oy) {
    int e = (cfg.menu_fill == 3 && !g_gl_on && g_bg_live >= 0 && bg_wready(g_bg_live) && g_vdw == g_vw && g_vdh == g_vh) ? g_bg_live : -1;
    DWORD ver = e >= 0 ? g_bgi[e].wver : 0;
    int cw = KX(ox + m->w) - KX(ox), ch = KX(oy + m->hh) - KX(oy);
    if (cw <= 0 || ch <= 0) return 0;
    if (m->cdc && m->cox == ox && m->coy == oy && m->ck == g_k && m->cbg == e && m->cver == ver && m->cw == cw && m->ch == ch) return 1;
    if (m->cdc && (m->cw != cw || m->ch != ch)) { SelectObject(m->cdc, m->cold); DeleteObject(m->cbm); DeleteDC(m->cdc); m->cdc = NULL; }
    if (!m->cdc && !(m->cdc = dib_dc(cw, ch, &m->cbits, &m->cbm, &m->cold))) return 0;
    SetStretchBltMode(m->cdc, HALFTONE); SetBrushOrgEx(m->cdc, 0, 0, NULL);
    if (m->hdc) StretchBlt(m->cdc, 0, 0, cw, ch, m->hdc, 0, 0, m->hdw, m->hdh, SRCCOPY);
    else StretchBlt(m->cdc, 0, 0, cw, ch, m->bdc, 0, 0, m->w, m->hh, SRCCOPY);
    GdiFlush();
    if (e >= 0) {
        BGIMG *b = &g_bgi[e]; int w = m->w, h = m->hh;
        float *al = (float *)malloc((size_t)w * h * sizeof(float) * 2);
        if (al) {
            float *tmp = al + (size_t)w * h;
            for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
                int gx = ox + i, gy = oy + j; float a = 1;
                if (gx >= 0 && gy >= 0 && gx < 640 && gy < 480) {
                    DWORD p = m->base[j * w + i], q = b->spx[gy * 640 + gx]; int d = 0;
                    for (int c = 0; c < 24; c += 8) { int t = abs((int)(p >> c & 255) - (int)(q >> c & 255)); if (t > d) d = t; }
                    a = (d - 3) / 7.0f; a = a < 0 ? 0 : a > 1 ? 1 : a;
                }
                al[j * w + i] = a;
            }
            for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {     // alarga 1 pixel (borda do desenho)
                float mx = 0;
                for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
                    int ii = i + dx, jj = j + dy; if (ii < 0 || jj < 0 || ii >= w || jj >= h) continue;
                    if (al[jj * w + ii] > mx) mx = al[jj * w + ii];
                }
                tmp[j * w + i] = mx;
            }
            for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {     // e suaviza
                float sum = 0; int n = 0;
                for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
                    int ii = i + dx, jj = j + dy; if (ii < 0 || jj < 0 || ii >= w || jj >= h) continue;
                    sum += tmp[jj * w + ii]; n++;
                }
                al[j * w + i] = sum / n;
            }
            int bx0 = b->wvx + KX(ox), by0 = b->wvy + KX(oy);
            for (int Y = 0; Y < ch; Y++) {
                int wy = by0 + Y; if (wy < 0 || wy >= b->wh) continue;
                float v = (Y + 0.5f) * h / ch - 0.5f; int j0 = (int)floorf(v); float fy = v - j0; int j1 = j0 + 1;
                if (j0 < 0) j0 = 0; if (j1 >= h) j1 = h - 1; if (j0 >= h) j0 = h - 1;
                DWORD *row = m->cbits + (size_t)Y * cw, *wr = b->wbits + (size_t)wy * b->ww;
                for (int X = 0; X < cw; X++) {
                    int wx = bx0 + X; if (wx < 0 || wx >= b->ww) continue;
                    float u = (X + 0.5f) * w / cw - 0.5f; int i0 = (int)floorf(u); float fx = u - i0; int i1 = i0 + 1;
                    if (i0 < 0) i0 = 0; if (i1 >= w) i1 = w - 1; if (i0 >= w) i0 = w - 1;
                    float a = (al[j0 * w + i0] * (1 - fx) + al[j0 * w + i1] * fx) * (1 - fy) + (al[j1 * w + i0] * (1 - fx) + al[j1 * w + i1] * fx) * fy;
                    if (a >= 0.999f) continue;
                    DWORD p = row[X], q = wr[wx], o = 0;
                    for (int c = 0; c < 24; c += 8) o |= (DWORD)((int)((q >> c & 255) * (1 - a) + (p >> c & 255) * a + 0.5f)) << c;
                    row[X] = o;
                }
            }
            free(al); GdiFlush();
        }
    }
    m->cox = ox; m->coy = oy; m->ck = g_k; m->cbg = e; m->cver = ver; m->cw = cw; m->ch = ch;
    return 1;
}
static int mimg_blit(HDC v, int x, int y, int w, int h, HDC s, int sx, int sy, DWORD rop) {
    if (!g_min || !s || rop != SRCCOPY || g_gl_on) return 0;
    MIMG *m = mimg_find((HBITMAP)GetCurrentObject(s, OBJ_BITMAP));
    if (!m || (!m->hdc && !m->own1x)) return 0;
    if (sx < 0 || sy < 0 || w <= 0 || h <= 0 || sx + w > m->w || sy + h > m->hh) return 0;
    int ox = x - sx, oy = y - sy;
    if (!mimg_render(m, ox, oy)) return 0;
    r_BitBlt(v, KX(x), KX(y), KX(x + w) - KX(x), KX(y + h) - KX(y), m->cdc, KX(x) - KX(ox), KX(y) - KX(oy), SRCCOPY);
    return 1;
}
typedef HANDLE (WINAPI *PFN_LI)(HINSTANCE, LPCSTR, UINT, int, int, UINT);
typedef BOOL (WINAPI *PFN_DO)(HGDIOBJ);
static PFN_LI r_LoadImageA; static PFN_DO r_DeleteObject;
static HANDLE WINAPI h_LoadImageA(HINSTANCE hi, LPCSTR name, UINT type, int cx, int cy, UINT fl) {
    HANDLE r = r_LoadImageA(hi, name, type, cx, cy, fl);
    if (r && type == IMAGE_BITMAP && cfg.enabled && (ULONG_PTR)name > 0xffff) mimg_add((HBITMAP)r, name);
    return r;
}
static BOOL WINAPI h_DeleteObject(HGDIOBJ o) {
    for (int i = 0; i < g_bgh_n; i++) if (g_bgh[i].h == (HBITMAP)o) { g_bgh[i] = g_bgh[--g_bgh_n]; break; }
    for (int i = 0; i < g_min; i++) if (g_mi[i].h == (HBITMAP)o) { mimg_free(&g_mi[i]); g_mi[i] = g_mi[--g_min]; memset(&g_mi[g_min], 0, sizeof g_mi[g_min]); break; }
    return r_DeleteObject(o);
}
static void present_2d(int force) {
    if (!g_vdc || !g_hwnd || g_gl_on) return;
    DWORD now = GetTickCount();
    if (!force && now - g_last_present < 8) return;
    g_last_present = now; g_dirty = 0;
    update_rect();
    HDC dc = GetDC(g_hwnd);
    if (g_vdw == g_vw && g_vdh == g_vh) BitBlt(dc, g_vx, g_vy, g_vw, g_vh, g_vdc, 0, 0, SRCCOPY);
    else { SetStretchBltMode(dc, HALFTONE); SetBrushOrgEx(dc, 0, 0, NULL); StretchBlt(dc, g_vx, g_vy, g_vw, g_vh, g_vdc, 0, 0, g_vdw, g_vdh, SRCCOPY); }
    if (g_vx > 0 || g_vy > 0) {
        HDC fd = NULL;
        if (cfg.menu_fill == 3 && g_bg_live >= 0 && bg_wready(g_bg_live)) { fd = g_bgi[g_bg_live].wdc; g_fill_pending = 0; }   // fundo pronto
        else if ((cfg.menu_fill == 1 || cfg.menu_fill == 3) && fill_update()) fd = g_fdc;
        if (fd) {
            if (g_vx > 0) { BitBlt(dc, 0, 0, g_vx, g_winh, fd, 0, 0, SRCCOPY); BitBlt(dc, g_vx + g_vw, 0, g_winw - g_vx - g_vw, g_winh, fd, g_vx + g_vw, 0, SRCCOPY); }
            if (g_vy > 0) { BitBlt(dc, 0, 0, g_winw, g_vy, fd, 0, 0, SRCCOPY); BitBlt(dc, 0, g_vy + g_vh, g_winw, g_winh - g_vy - g_vh, fd, 0, g_vy + g_vh, SRCCOPY); }
        } else {
            HBRUSH k = (HBRUSH)GetStockObject(BLACK_BRUSH);
            if (g_vx > 0) { RECT l = { 0, 0, g_vx, g_winh }, rr = { g_vx + g_vw, 0, g_winw, g_winh }; FillRect(dc, &l, k); FillRect(dc, &rr, k); }
            if (g_vy > 0) { RECT t = { 0, 0, g_winw, g_vy }, bb = { 0, g_vy + g_vh, g_winw, g_winh }; FillRect(dc, &t, k); FillRect(dc, &bb, k); }
        }
    }
    ReleaseDC(g_hwnd, dc);
}
static void touched(void) { g_dirty = 1; present_2d(0); }

static int vclip_begin(HDC d, HDC v) {
    if (!g_paint_dc || d != g_paint_dc) return 0;
    int id = SaveDC(v);
    IntersectClipRect(v, KX(g_paint_rc.left), KX(g_paint_rc.top), KX(g_paint_rc.right), KX(g_paint_rc.bottom));
    return id;
}
static void vclip_end(HDC v, int id) { if (id) RestoreDC(v, id); }
static BOOL WINAPI h_BitBlt(HDC d, int x, int y, int w, int h, HDC s, int sx, int sy, DWORD rop) {
    int dg = gdi_redirect(d), sg = s && gdi_redirect(s);
    if (dg) {
        HDC v = vdc(); BOOL r; int cl = vclip_begin(d, v);
        if (sg) { r = r_BitBlt(v, KX(x), KX(y), KX(x + w) - KX(x), KX(y + h) - KX(y), v, KX(sx), KX(sy), rop); bg_fullcover(x, y, w, h, "copia da propria tela"); }
        else if (bg_blit(v, x, y, w, h, s, sx, sy, rop)) r = TRUE;
        else if (mimg_blit(v, x, y, w, h, s, sx, sy, rop)) r = TRUE;
        else if (g_k == 1.0 || !s) r = r_BitBlt(v, KX(x), KX(y), KX(x + w) - KX(x), KX(y + h) - KX(y), s, sx, sy, rop);
        else { stretch_mode(v, rop); r = StretchBlt(v, KX(x), KX(y), KX(x + w) - KX(x), KX(y + h) - KX(y), s, sx, sy, w, h, rop); }
        vclip_end(v, cl);
        touched(); return r;
    }
    bg_forget(d);
    if (sg) {   // o jogo lendo da tela: reduz da tela virtual
        HDC v = vdc();
        if (g_k == 1.0) return r_BitBlt(d, x, y, w, h, v, sx, sy, rop);
        stretch_mode(d, rop);
        return StretchBlt(d, x, y, w, h, v, KX(sx), KX(sy), KX(sx + w) - KX(sx), KX(sy + h) - KX(sy), rop);
    }
    return r_BitBlt(d, x, y, w, h, s, sx, sy, rop);
}
static int WINAPI h_StretchDIBits(HDC d, int x, int y, int w, int h, int sx, int sy, int sw, int sh, const VOID *bits, const BITMAPINFO *bi, UINT u, DWORD rop) {
    if (gdi_redirect(d)) {
        HDC v = vdc(); stretch_mode(v, rop); int cl = vclip_begin(d, v); bg_fullcover(x, y, w, h, "StretchDIBits");
        int r = r_StretchDIBits(v, KX(x), KX(y), KX(x + w) - KX(x), KX(y + h) - KX(y), sx, sy, sw, sh, bits, bi, u, rop);
        vclip_end(v, cl);
        touched(); return r;
    }
    return r_StretchDIBits(d, x, y, w, h, sx, sy, sw, sh, bits, bi, u, rop);
}
static int WINAPI h_FillRect(HDC d, const RECT *rc, HBRUSH b) {
    if (!gdi_redirect(d)) bg_forget(d);
    if (gdi_redirect(d) && rc) { HDC v = vdc(); int cl = vclip_begin(d, v); bg_fullcover(rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, "FillRect"); RECT r2 = { KX(rc->left), KX(rc->top), KX(rc->right), KX(rc->bottom) }; int r = r_FillRect(v, &r2, b); vclip_end(v, cl); touched(); return r; }
    return r_FillRect(d, rc, b);
}
// Nomes embaixo dos ícones dos menus: o jogo centraliza cada nome sob o ícone, na largura exata do texto. Em
// português alguns ficam mais largos que o espaço ("Carregar Jogo" entra em "Configuração"); esses são
// desenhados um pouco mais estreitos, centralizados no mesmo lugar (sempre dentro do retângulo original, que é
// o que o jogo redesenha quando o mouse passa por cima).
static double label_squeeze(HDC d, LPCSTR t, int n, const RECT *rc) {
    BYTE *m = fe_menu();
    if (!m || !t || !rc) return 1.0;
    int len = n < 0 ? (int)strlen(t) : n;
    struct { RECT r; } lab[64]; int nl = 0, me = -1;
    for (int pn = 0; m && pn < 16 && !IsBadReadPtr(m, 0x20); pn++, m = *(BYTE **)(m + 0x18)) {
        BYTE *it = *(BYTE **)(m + 8);
        for (int k = 0; it && k < 96 && nl < 64 && !IsBadReadPtr(it, 0x118); k++, it = *(BYTE **)(it + 0x110)) {
            if (*(int *)(it + 0x6c) != 4) continue;              // só itens de imagem com nome embaixo
            const char *tx = (const char *)(it + 0x2c); int tl = 0;
            while (tl < 64 && tx[tl]) tl++;
            if (!tl || tl >= 64) continue;
            SIZE sz; if (!GetTextExtentPoint32A(d, tx, tl, &sz) || sz.cx <= 0) continue;
            int x = *(int *)(it + 0x70), y = *(int *)(it + 0x74), w = *(int *)(it + 0x78), h = *(int *)(it + 0x7c);
            RECT r = { x + w / 2 - sz.cx / 2, y + h, x + w / 2 - sz.cx / 2 + sz.cx, y + h + sz.cy };
            // enquanto o nome é desenhado o jogo soma a altura do texto à do item: o próprio usa o retângulo recebido
            if (me < 0 && r.left == rc->left && r.right == rc->right && !strncmp(tx, t, len < tl ? len : tl)) { me = nl; r = *rc; }
            lab[nl++].r = r;
        }
    }
    if (me < 0) return 1.0;
    const RECT *a = &lab[me].r; double fct = 1.0;
    for (int i = 0; i < nl; i++) {
        if (i == me) continue;
        const RECT *b = &lab[i].r;
        if (a->top >= b->bottom || b->top >= a->bottom || a->left >= b->right || b->left >= a->right) continue;   // não encostam
        double ca = (a->left + a->right) / 2.0, cb = (b->left + b->right) / 2.0, half = ((a->right - a->left) + (b->right - b->left)) / 2.0;
        double need = (fabs(cb - ca) - 6.0) / half;
        if (need < fct) fct = need;
    }
    return fct < 0.7 ? 0.7 : fct;
}
static struct { HFONT src; double k, f; HFONT dst; } g_cf[16]; static int g_cfn;
static HFONT condensed_font(HFONT f, double fct) {       // fonte já ampliada, só mais estreita
    for (int i = 0; i < g_cfn; i++) if (g_cf[i].src == f && g_cf[i].k == g_k && g_cf[i].f == fct) return g_cf[i].dst;
    LOGFONTA lf; if (!GetObjectA(f, sizeof lf, &lf)) return NULL;
    HDC v = vdc(); HGDIOBJ o = r_SelectObject(v, f); TEXTMETRICA tm; GetTextMetricsA(v, &tm); r_SelectObject(v, o);
    lf.lfWidth = (LONG)floor(tm.tmAveCharWidth * fct + 0.5); if (lf.lfWidth < 1) lf.lfWidth = 1;
    HFONT nf = CreateFontIndirectA(&lf); if (!nf) return NULL;
    int slot = g_cfn < 16 ? g_cfn++ : 0;
    if (g_cfn == 16 && slot == 0 && g_cf[0].dst) DeleteObject(g_cf[0].dst);
    g_cf[slot].src = f; g_cf[slot].k = g_k; g_cf[slot].f = fct; g_cf[slot].dst = nf;
    return nf;
}
static int WINAPI h_DrawTextA(HDC d, LPCSTR t, int n, LPRECT rc, UINT f) {
    if (!(f & DT_CALCRECT) && !gdi_redirect(d)) bg_forget(d);
    if (gdi_redirect(d) && rc && !(f & DT_CALCRECT)) {
        HDC v = vdc();
        RECT r2 = { KX(rc->left), KX(rc->top), KX(rc->right), KX(rc->bottom) };
        int cl = vclip_begin(d, v);
        double sq = (f & (DT_CENTER | DT_RIGHT | DT_WORDBREAK)) ? 1.0 : label_squeeze(d, t, n, rc);
        HFONT cf = sq < 0.999 ? condensed_font((HFONT)GetCurrentObject(v, OBJ_FONT), sq) : NULL;
        if (cf) {
            static int nl; if (nl++ < 8) logf_("menu: nome '%.24s' estreitado para %.0f%% (encostava no vizinho)", t, sq * 100);
            HGDIOBJ o = r_SelectObject(v, cf); r_DrawTextA(v, t, n, &r2, f | DT_CENTER); r_SelectObject(v, o);
        } else r_DrawTextA(v, t, n, &r2, f);
        vclip_end(v, cl);
        touched();
        RECT m = *rc; return r_DrawTextA(d, t, n, &m, f | DT_CALCRECT);   // altura em unidades do jogo
    }
    return r_DrawTextA(d, t, n, rc, f);
}
// estado do DC (fonte, cores, alinhamento) é espelhado na tela virtual; a fonte vai no tamanho ampliado
static HGDIOBJ WINAPI h_SelectObject(HDC d, HGDIOBJ o) {
    HGDIOBJ r = r_SelectObject(d, o);
    if (gdi_redirect(d) && o) {
        DWORD t = GetObjectType(o);
        if (t == OBJ_FONT) r_SelectObject(vdc(), scaled_font((HFONT)o));
        else if (t == OBJ_BRUSH || t == OBJ_PEN) r_SelectObject(vdc(), o);
    }
    return r;
}
static COLORREF WINAPI h_SetBkColor(HDC d, COLORREF c) { if (gdi_redirect(d)) r_SetBkColor(vdc(), c); return r_SetBkColor(d, c); }
static COLORREF WINAPI h_SetTextColor(HDC d, COLORREF c) { if (gdi_redirect(d)) r_SetTextColor(vdc(), c); return r_SetTextColor(d, c); }
static int WINAPI h_SetBkMode(HDC d, int m) { if (gdi_redirect(d)) r_SetBkMode(vdc(), m); return r_SetBkMode(d, m); }
static UINT WINAPI h_SetTextAlign(HDC d, UINT a) { if (gdi_redirect(d)) r_SetTextAlign(vdc(), a); return r_SetTextAlign(d, a); }
static int WINAPI h_ReleaseDC(HWND w, HDC d) { if (is_game_wnd(w) && g_dirty) present_2d(1); return r_ReleaseDC(w, d); }
static HDC WINAPI h_BeginPaint(HWND w, LPPAINTSTRUCT ps) {
    HDC d = r_BeginPaint(w, ps);
    if (d && ps && is_game_wnd(w)) { g_paint_dc = d; g_paint_rc = ps->rcPaint; }
    return d;
}
static BOOL WINAPI h_EndPaint(HWND w, const PAINTSTRUCT *ps) { if (is_game_wnd(w)) { g_paint_dc = NULL; present_2d(1); } return r_EndPaint(w, ps); }
// ---------------------------------------------------------------- menus 2D do início (GDI): itens clicáveis
// Menu atual: [0x63972c] = índice; descritor em [0x639324 + 36*índice]: +0x04 nº de itens, +0x08 primeiro item,
// +0x10 item sob o mouse. Item: +0x2c texto, +0x6c tipo, +0x70/+0x74 x,y, +0x78/+0x7c largura/altura (640x480),
// +0x110 próximo.
static BYTE *fe_menu(void) {
    if (g_gl_on) return NULL;                                  // em jogo (OpenGL na janela)
    int idx = *(volatile int *)0x63972c;
    if (idx < 0 || idx > 64) return NULL;
    BYTE *d = *(BYTE **)(0x639324 + 36 * idx);
    if (!d || IsBadReadPtr(d, 0x20)) return NULL;
    return d;
}
static void fe_dump(void) {
    static int last_idx = -1; static BYTE *last_d; static DWORD last_t;
    static int ls = -99, li = -99; static void *lp = (void *)1;
    int st = *(volatile int *)0x4d0800, ii = *(volatile int *)0x63972c;
    void *pp = (ii >= 0 && ii <= 64) ? *(void **)(0x639324 + 36 * ii) : NULL;
    if (st != ls || ii != li || pp != lp) { ls = st; li = ii; lp = pp; logf_("frontend: estado %d menu %d descritor %p ocupado %d", st, ii, pp, *(volatile int *)0x639730); }
    BYTE *d = fe_menu();
    int idx = *(volatile int *)0x63972c;
    if (!d) { last_d = NULL; return; }
    if (d == last_d && idx == last_idx && GetTickCount() - last_t < 3000) return;
    if (d == last_d && idx == last_idx) { last_t = GetTickCount(); return; }
    last_d = d; last_idx = idx; last_t = GetTickCount();
    for (int pn = 0; d && pn < 8 && !IsBadReadPtr(d, 0x20); pn++, d = *(BYTE **)(d + 0x18)) {
    logf_("menu %d painel %d: descritor %p n=%d itens %p hover %p ocupado=%d", idx, pn, (void *)d, *(int *)(d + 4), *(void **)(d + 8), *(void **)(d + 0x10), *(volatile int *)0x639730);
    BYTE *it = *(BYTE **)(d + 8);
    for (int k = 0; it && k < 64 && !IsBadReadPtr(it, 0x118); k++, it = *(BYTE **)(it + 0x110)) {
        if (*(int *)(it + 0x6c) == 7) {
            char hx[1200]; int o = 0;
            for (int q = 0; q < 0x118 / 4 && o < 1150; q++) o += snprintf(hx + o, sizeof hx - o, "%s%x", q % 8 ? " " : (q ? " | " : ""), (unsigned)((DWORD *)it)[q]);
            logf_("  lista %p: %s", (void *)it, hx);
        }
        char txt[40]; int j = 0;
        for (; j < 39 && it[0x2c + j] >= 32 && it[0x2c + j] < 255; j++) txt[j] = (char)it[0x2c + j];
        txt[j] = 0;
        logf_("  item %d %p tipo %d ret %d,%d %dx%d +24=%d +88=%d +ac=%d cb=%p e8=%p '%s'", k, (void *)it, *(int *)(it + 0x6c),
              *(int *)(it + 0x70), *(int *)(it + 0x74), *(int *)(it + 0x78), *(int *)(it + 0x7c), *(int *)(it + 0x24),
              *(int *)(it + 0x88), *(int *)(it + 0xac), *(void **)(it + 0x8c), *(void **)(it + 0xe8), txt);
    }
    }
}
static void fe_pad(void);
static BOOL WINAPI h_PeekMessageA(LPMSG m, HWND w, UINT a, UINT b, UINT f) {
    if (cfg.debug) fe_dump();
    if (cfg.xinput && cfg.menu_pad) fe_pad();
    if (g_dirty) present_2d(0);
    else if (g_fill_pending && GetTickCount() - g_ftime >= 120) present_2d(1);   // atualiza o fundo das laterais
    return r_PeekMessageA(m, w, a, b, f);
}

// ---------------------------------------------------------------- controle XInput -> teclas do jogo
typedef struct { WORD wButtons; BYTE bLeftTrigger, bRightTrigger; SHORT sThumbLX, sThumbLY, sThumbRX, sThumbRY; } XPAD;
typedef struct { DWORD dwPacketNumber; XPAD Gamepad; } XSTATE;
typedef DWORD (WINAPI *PFN_XGetState)(DWORD, XSTATE *);
static PFN_XGetState x_GetState;
enum { XB_UP = 1, XB_DOWN = 2, XB_LEFT = 4, XB_RIGHT = 8, XB_START = 0x10, XB_BACK = 0x20, XB_LS = 0x40, XB_RS = 0x80,
       XB_LB = 0x100, XB_RB = 0x200, XB_A = 0x1000, XB_B = 0x2000, XB_X = 0x4000, XB_Y = 0x8000 };

// ordem das 12 ações do jogador 1 em config.pc (offset 0x4c), conferida com o readme:
enum { K_UP, K_DOWN, K_RIGHT, K_LEFT, K_FRIEND, K_CAML, K_CAMSW, K_CAMR, K_ACTION, K_EARS, K_JUMP, K_KICK, K_COUNT };
static BYTE g_vk[K_COUNT] = { VK_UP, VK_DOWN, VK_RIGHT, VK_LEFT, 'E', 'Q', 'R', 'W', 'A', 'S', 'D', 'F' };
static BYTE g_padkeys[256];   // por código virtual (GetKeyboardState)
static BYTE g_paddik[256];    // por código DirectInput (DIK), que é o que o jogo lê nas fases e menus
static BYTE g_dik[K_COUNT] = { 0xC8, 0xD0, 0xCD, 0xCB, 0x12, 0x10, 0x13, 0x11, 0x1E, 0x1F, 0x20, 0x21 };

static BYTE sc_to_vk(DWORD sc) {
    switch (sc) {
    case 0x148: return VK_UP;    case 0x150: return VK_DOWN; case 0x14b: return VK_LEFT; case 0x14d: return VK_RIGHT;
    case 0x147: return VK_HOME;  case 0x14f: return VK_END;  case 0x149: return VK_PRIOR; case 0x151: return VK_NEXT;
    case 0x152: return VK_INSERT; case 0x153: return VK_DELETE; case 0x11d: return VK_RCONTROL; case 0x138: return VK_RMENU;
    case 0x11c: return VK_RETURN; case 0x135: return VK_DIVIDE;
    case 0x48: return VK_NUMPAD8; case 0x4c: return VK_NUMPAD5; case 0x4b: return VK_NUMPAD4; case 0x4d: return VK_NUMPAD6;
    case 0x50: return VK_NUMPAD2; case 0x47: return VK_NUMPAD7; case 0x49: return VK_NUMPAD9; case 0x4f: return VK_NUMPAD1; case 0x51: return VK_NUMPAD3;
    }
    if (sc < 0x100) return (BYTE)MapVirtualKeyA(sc, 3 /* MAPVK_VSC_TO_VK_EX */);
    return 0;
}

static void load_keymap(void) {
    char path[MAX_PATH]; snprintf(path, sizeof path, "%s\\config.pc", g_dir);
    FILE *f = fopen(path, "rb");
    if (!f) { logf_("config.pc nao encontrado, usando teclas padrao"); return; }
    DWORD sc[K_COUNT];
    if (fseek(f, 0x4c, SEEK_SET) == 0 && fread(sc, 4, K_COUNT, f) == K_COUNT) {
        for (int i = 0; i < K_COUNT; i++) {
            BYTE v = sc_to_vk(sc[i]); if (v) g_vk[i] = v;
            if (sc[i] && sc[i] < 0x200) g_dik[i] = (BYTE)(sc[i] >= 0x100 ? ((sc[i] & 0x7F) | 0x80) : sc[i]);
        }
        logf_("teclas do jogador 1 (vk): cima %02x baixo %02x dir %02x esq %02x amigo %02x camE %02x camTroca %02x camD %02x acao %02x orelhas %02x pulo %02x chute %02x",
              g_vk[0], g_vk[1], g_vk[2], g_vk[3], g_vk[4], g_vk[5], g_vk[6], g_vk[7], g_vk[8], g_vk[9], g_vk[10], g_vk[11]);
    }
    fclose(f);
}

static int game_has_focus(void) {
    HWND fg = GetForegroundWindow(); DWORD pid = 0;
    if (fg) GetWindowThreadProcessId(fg, &pid);
    return pid == GetCurrentProcessId();
}

static void init_xinput(void);
static volatile int g_sim_pad;                  // debug F8: 0 = controle real, 1 = finge desconectado, 2 = finge conectado
static void poll_pad(void) {
    static int xi_init;
    if (!xi_init) { xi_init = 1; init_xinput(); }
    static DWORD last_tick, last_ok_log;
    static int have_pad = -1;
    DWORD now = GetTickCount();
    if (now - last_tick < 4) return;  // GetKeyboardState pode ser chamado várias vezes por quadro
    last_tick = now;
    memset(g_padkeys, 0, sizeof g_padkeys); memset(g_paddik, 0, sizeof g_paddik);
    if (!x_GetState || !cfg.xinput) return;
    XSTATE st; int got = -1;
    for (DWORD i = 0; i < 4 && got < 0; i++) if (x_GetState(i, &st) == 0) got = (int)i;
    if (g_sim_pad == 1) got = -1;                   // debug F8: finge que o controle foi desconectado
    int shown = g_sim_pad == 2 || got >= 0;         // debug F8: finge que há controle (só para os ícones)
    if (shown != (have_pad == 1)) { have_pad = shown; g_pad_on = have_pad; logf_(have_pad ? "controle XInput conectado (porta %d)" : "controle XInput desconectado", got); }
    (void)last_ok_log;
    if (got < 0 || !game_has_focus()) return;
    XPAD *p = &st.Gamepad;
    WORD b = p->wButtons;
    if (cfg.swap_ab) { WORD a = b & XB_A, bb = b & XB_B; b &= ~(XB_A | XB_B); if (a) b |= XB_B; if (bb) b |= XB_A; }
    int dz = 32767 * cfg.deadzone / 100;
    double lx = p->sThumbLX, ly = p->sThumbLY, mag = sqrt(lx * lx + ly * ly);
    int up = 0, down = 0, left = 0, right = 0;
    if (mag > dz) {  // 8 direções com setores de 45°
        double a = atan2(ly, lx) * 180.0 / 3.14159265358979;
        if (a > 22.5 && a < 157.5) up = 1;
        if (a < -22.5 && a > -157.5) down = 1;
        if (a > 112.5 || a < -112.5) left = 1;
        if (a < 67.5 && a > -67.5) right = 1;
    }
    if (b & XB_UP) up = 1;
    if (b & XB_DOWN) down = 1;
    if (b & XB_LEFT) left = 1;
    if (b & XB_RIGHT) right = 1;
    BYTE *k = g_padkeys, *d = g_paddik;
#define PRESS(vk, dik) do { k[(vk)] = 0x80; d[(dik)] = 0x80; } while (0)
    if (up)    { PRESS(g_vk[K_UP], g_dik[K_UP]);       PRESS(VK_UP, 0xC8); }
    if (down)  { PRESS(g_vk[K_DOWN], g_dik[K_DOWN]);   PRESS(VK_DOWN, 0xD0); }
    if (left)  { PRESS(g_vk[K_LEFT], g_dik[K_LEFT]);   PRESS(VK_LEFT, 0xCB); }
    if (right) { PRESS(g_vk[K_RIGHT], g_dik[K_RIGHT]); PRESS(VK_RIGHT, 0xCD); }
    if (b & XB_A) { PRESS(g_vk[K_JUMP], g_dik[K_JUMP]); if (cfg.a_enter) PRESS(VK_RETURN, 0x1C); }
    if (b & XB_X) PRESS(g_vk[K_KICK], g_dik[K_KICK]);
    if (b & XB_B) PRESS(g_vk[K_EARS], g_dik[K_EARS]);
    if (b & XB_Y) PRESS(g_vk[K_ACTION], g_dik[K_ACTION]);
    if (b & XB_LB) PRESS(g_vk[K_FRIEND], g_dik[K_FRIEND]);
    if (b & XB_RB) PRESS(g_vk[K_CAMSW], g_dik[K_CAMSW]);
    if (b & XB_START) PRESS(VK_ESCAPE, 0x01);          // menu / pausa
    if (b & XB_BACK) PRESS(VK_F4, 0x3E);               // "F4 menu anterior"
    if ((b & XB_B) && pad_menu_back(now, g_f4_prompt_tick)) {   // com "menu anterior" na tela, B também volta
        PRESS(VK_F4, 0x3E);
        static int nl; if (nl++ < 5) logf_("controle: B volta ao menu anterior (F4)");
    }
    int caml = 0, camr = 0;
    {   // gatilhos: LT/RT (invert_camera_triggers troca o lado)
        int lt = p->bLeftTrigger > 60, rt = p->bRightTrigger > 60;
        if (cfg.inv_trig) { int t = lt; lt = rt; rt = t; }
        caml |= lt; camr |= rt;
    }
    if (cfg.cam_stick) {   // analógico direito (invert_camera_stick troca o lado)
        int sl = p->sThumbRX < -16000, sr = p->sThumbRX > 16000;
        if (cfg.inv_stick) { int t = sl; sl = sr; sr = t; }
        caml |= sl; camr |= sr;
    }
    if (b & XB_RS) caml = camr = 1;   // visão em primeira pessoa = as duas teclas de câmera
    if (caml) PRESS(g_vk[K_CAML], g_dik[K_CAML]);
    if (camr) PRESS(g_vk[K_CAMR], g_dik[K_CAMR]);
#undef PRESS
    static WORD lastb; if (b != lastb) { lastb = b; static int nb; if (nb++ < 30) logf_("controle: botoes %04x LX %d LY %d", b, p->sThumbLX, p->sThumbLY); }
}

typedef BOOL (WINAPI *PFN_GKS)(PBYTE);
typedef SHORT (WINAPI *PFN_GAKS)(int);
static PFN_GKS r_GetKeyboardState;
static PFN_GAKS r_GetAsyncKeyState;

static BOOL WINAPI h_GetKeyboardState(PBYTE keys) {
    BOOL r = r_GetKeyboardState(keys);
    if (r && cfg.xinput) { poll_pad(); for (int i = 0; i < 256; i++) keys[i] |= g_padkeys[i]; }
    return r;
}
static SHORT WINAPI h_GetAsyncKeyState(int vk) {
    SHORT r = r_GetAsyncKeyState(vk);
    if (cfg.xinput && vk >= 0 && vk < 256) { poll_pad(); if (g_padkeys[vk]) r |= (SHORT)0x8000; }
    return r;
}

static void init_xinput(void) {
    const char *names[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
    for (int i = 0; i < 3 && !x_GetState; i++) {
        HMODULE m = LoadLibraryA(names[i]);
        if (m) { x_GetState = (PFN_XGetState)GetProcAddress(m, "XInputGetState"); if (x_GetState) logf_("XInput: %s", names[i]); }
    }
    if (!x_GetState) logf_("XInput indisponivel");
}

// ---------------------------------------------------------------- controle nos menus 2D do início
// Os menus do início (GDI) só respondem ao mouse: o jogo ignora o teclado ali. Com o controle, o direcional ou o
// analógico pula entre os itens do menu atual (o cursor vai para o centro do item e o jogo realça o item como faria
// com o mouse), A/Start clica e B clica em "Voltar". Itens: todos os painéis do menu atual (+0x18), tipos texto
// clicável (0), caixa de marcar (1), imagem (4) e as linhas visíveis da lista de jogos salvos (7); teclas a
// redefinir (5) ficam de fora, porque pedem uma tecla do teclado.
typedef struct { BYTE *it; int row, x, y, w, h; } FEITEM;
static int fe_collect(BYTE *d, FEITEM *o, int max) {
    int n = 0;
    for (int pn = 0; d && pn < 16 && !IsBadReadPtr(d, 0x20); pn++, d = *(BYTE **)(d + 0x18)) {
        BYTE *it = *(BYTE **)(d + 8);
        for (int k = 0; it && k < 96 && n < max && !IsBadReadPtr(it, 0x118); k++, it = *(BYTE **)(it + 0x110)) {
            int ty = *(int *)(it + 0x6c), x = *(int *)(it + 0x70), y = *(int *)(it + 0x74), w = *(int *)(it + 0x78), h = *(int *)(it + 0x7c);
            if (!*(int *)(it + 0xac) || w <= 0 || h <= 0 || x < 0 || y < 0 || x >= 640 || y >= 480) continue;
            if (ty == 7) {                                  // lista: uma entrada por linha visível (22 pixels)
                int cnt = *(volatile int *)0x4d226c, top = *(int *)(it + 0xec);
                if (top < 0) top = 0;
                for (int r = 0; top + r < cnt && (r + 1) * 22 <= h && n < max; r++)
                    o[n++] = (FEITEM){ it, top + r, x + 10, y + r * 22, w - 20, 22 };
            } else if (ty == 0 || ty == 1 || ty == 4) o[n++] = (FEITEM){ it, -1, x, y, w, h };
        }
    }
    return n;
}
static void fe_point(const FEITEM *f, POINT *p) {      // centro do item em coordenadas da janela
    double mx = f->x + f->w / 2.0, my = f->y + f->h / 2.0;
    p->x = g_vx + (int)(mx * g_vw / g_gw); p->y = g_vy + (int)(my * g_vh / g_gh);
}
static void fe_hover(const FEITEM *f) {
    POINT p; fe_point(f, &p);
    POINT s = p; ClientToScreen(g_hwnd, &s);
    SetCursorPos(s.x, s.y);
    padcur_apply(padcur_nav(&g_padcur, s.x, s.y));
    PostMessageA(g_hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(p.x, p.y));
}
static void fe_click(const FEITEM *f) {
    POINT p; fe_point(f, &p);
    fe_hover(f);
    PostMessageA(g_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(p.x, p.y));
    PostMessageA(g_hwnd, WM_LBUTTONUP, 0, MAKELPARAM(p.x, p.y));
}
static int fe_find(const FEITEM *v, int n, BYTE *it, int row) {
    for (int i = 0; i < n; i++) if (v[i].it == it && v[i].row == row) return i;
    return -1;
}
static int fe_under_cursor(const FEITEM *v, int n) {     // item sob o cursor do mouse, se houver
    POINT c; if (!GetCursorPos(&c) || !ScreenToClient(g_hwnd, &c) || !g_vw || !g_vh) return -1;
    double mx = (double)(c.x - g_vx) * g_gw / g_vw, my = (double)(c.y - g_vy) * g_gh / g_vh;
    for (int i = 0; i < n; i++) if (mx >= v[i].x && mx < v[i].x + v[i].w && my >= v[i].y && my < v[i].y + v[i].h) return i;
    return -1;
}
static int fe_first(const FEITEM *v, int n) {            // a 1ª linha da lista de jogos salvos, senão o mais acima
    for (int i = 0; i < n; i++) if (v[i].row >= 0) return i;
    int b = -1;
    for (int i = 0; i < n; i++) if (b < 0 || v[i].y + v[i].h / 2 < v[b].y + v[b].h / 2 - 8 || (abs(v[i].y + v[i].h / 2 - (v[b].y + v[b].h / 2)) <= 8 && v[i].x < v[b].x)) b = i;
    return b;
}
static int fe_step(const FEITEM *v, int n, int cur, int dx, int dy) {
    double cx = v[cur].x + v[cur].w / 2.0, cy = v[cur].y + v[cur].h / 2.0, best = 1e9; int b = -1;
    for (int i = 0; i < n; i++) {
        if (i == cur) continue;
        double vx = v[i].x + v[i].w / 2.0 - cx, vy = v[i].y + v[i].h / 2.0 - cy;
        double along = vx * dx + vy * dy, side = fabs(vx * dy - vy * dx);
        if (along <= 4) continue;                       // só o que está na direção pedida
        double sc = along + 2.5 * side;
        if (sc < best) { best = sc; b = i; }
    }
    return b;
}
static void fe_pad(void) {
    static DWORD last; static WORD prev; static int dir_prev, have_focus, f_row; static BYTE *f_it, *f_menu;
    static DWORD dir_next;
    DWORD now = GetTickCount();
    if (now - last < 10) return;
    last = now;
    BYTE *d = fe_menu();
    if (!d || *(volatile int *)0x639730 || !g_hwnd || !game_has_focus()) { prev = 0xffff; dir_prev = 0; padcur_apply(padcur_leave(&g_padcur)); return; }
    if (!x_GetState) { static int tried; if (tried) return; tried = 1; init_xinput(); if (!x_GetState) return; }
    XSTATE st; int got = -1;
    for (DWORD i = 0; i < 4 && got < 0; i++) if (x_GetState(i, &st) == 0) got = (int)i;
    if (got < 0) memset(&st, 0, sizeof st);
    if (cfg.debug) {                                      // debug: I/K/J/L = direcional, U = A, O = B (sem controle)
        static const struct { int vk; WORD bit; } sim[] = { { 'I', XB_UP }, { 'K', XB_DOWN }, { 'J', XB_LEFT }, { 'L', XB_RIGHT }, { 'U', XB_A }, { 'O', XB_B } };
        for (int i = 0; i < 6; i++) if (GetAsyncKeyState(sim[i].vk) & 0x8000) { st.Gamepad.wButtons |= sim[i].bit; got = 0; }
    }
    if (got < 0) { prev = 0; dir_prev = 0; return; }
    WORD b = st.Gamepad.wButtons;
    if (cfg.swap_ab) { WORD a = b & XB_A, bb = b & XB_B; b &= ~(XB_A | XB_B); if (a) b |= XB_B; if (bb) b |= XB_A; }
    int lx = st.Gamepad.sThumbLX, ly = st.Gamepad.sThumbLY, dir = 0;
    if ((b & XB_UP) || ly > 16000) dir = 1; else if ((b & XB_DOWN) || ly < -16000) dir = 2;
    else if ((b & XB_LEFT) || lx < -16000) dir = 3; else if ((b & XB_RIGHT) || lx > 16000) dir = 4;
    if (prev == 0xffff) { prev = b; dir_prev = dir; return; }   // botões já apertados ao entrar no menu: ignora
    WORD pressed = b & ~prev; prev = b;
    if (d != f_menu) { f_menu = d; have_focus = 0; }     // trocou de tela
    FEITEM v[96]; int n = fe_collect(d, v, 96);
    if (!n) return;
    int cur = have_focus ? fe_find(v, n, f_it, f_row) : -1;
    int fire = 0;
    if (dir && dir != dir_prev) { fire = 1; dir_next = now + 380; }
    else if (dir && (int)(now - dir_next) >= 0) { fire = 1; dir_next = now + 130; }
    dir_prev = dir;
    if (fire) {
        int u = fe_under_cursor(v, n);
        if (u >= 0 && u != cur && !have_focus) cur = u;   // começa de onde o mouse estava
        int nx;
        if (cur < 0) nx = u >= 0 ? u : fe_first(v, n);
        else nx = fe_step(v, n, cur, dir == 3 ? -1 : dir == 4 ? 1 : 0, dir == 1 ? -1 : dir == 2 ? 1 : 0);
        if (nx < 0) nx = cur;
        if (nx >= 0) { f_it = v[nx].it; f_row = v[nx].row; have_focus = 1; fe_hover(&v[nx]); }
        return;
    }
    if (pressed & (XB_A | XB_START)) {
        if (cur < 0) cur = fe_under_cursor(v, n);
        if (cur < 0) { int f0 = fe_first(v, n); if (f0 >= 0) { f_it = v[f0].it; f_row = v[f0].row; have_focus = 1; fe_hover(&v[f0]); } return; }
        f_it = v[cur].it; f_row = v[cur].row; have_focus = 1;
        fe_click(&v[cur]);
        if (v[cur].row >= 0)                              // escolheu um jogo salvo: o foco vai para "Carregar"
            for (int i = 0; i < n; i++) if (v[i].row < 0 && v[i].x == 539 && v[i].y == 355) { f_it = v[i].it; f_row = -1; fe_hover(&v[i]); break; }
        static int nlog; if (nlog++ < 20) { char t[24]; int j = 0; BYTE *tx = v[cur].it + 0x2c; for (; j < 23 && tx[j] >= 32; j++) t[j] = (char)tx[j]; t[j] = 0; logf_("controle no menu: clique em '%s'", t); }
        return;
    }
    if (pressed & XB_B) {                                 // "Voltar": a mãozinha em (35,375) de cada tela
        for (int i = 0; i < n; i++) if (v[i].row < 0 && v[i].x == 35 && v[i].y == 375) { have_focus = 0; fe_click(&v[i]); return; }
    }
}


// ---------------------------------------------------------------- diagnóstico
typedef UINT (WINAPI *PFN_GDT)(LPCSTR);
typedef BOOL (WINAPI *PFN_GVI)(LPCSTR, LPSTR, DWORD, LPDWORD, LPDWORD, LPDWORD, LPSTR, DWORD);
typedef HANDLE (WINAPI *PFN_CFA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef int (WINAPI *PFN_MBA)(HWND, LPCSTR, LPCSTR, UINT);
typedef HWND (WINAPI *PFN_FWA)(LPCSTR, LPCSTR);
typedef void (WINAPI *PFN_EP)(UINT);
static PFN_GDT r_GetDriveTypeA; static PFN_GVI r_GetVolumeInformationA; static PFN_CFA r_CreateFileA;
static PFN_MBA r_MessageBoxA; static PFN_FWA r_FindWindowA; static PFN_EP r_ExitProcess;
static int n_cf;
static UINT WINAPI h_GetDriveTypeA(LPCSTR p) { UINT r = r_GetDriveTypeA(p); static int n; if (cfg.debug && n++ < 40) logf_("GetDriveTypeA(%s) -> %u", p ? p : "(null)", r); return r; }
static BOOL WINAPI h_GetVolumeInformationA(LPCSTR root, LPSTR name, DWORD nl, LPDWORD sn, LPDWORD ml, LPDWORD fl, LPSTR fs, DWORD fsl) {
    BOOL r = r_GetVolumeInformationA(root, name, nl, sn, ml, fl, fs, fsl);
    static int n; if (n++ < 40) logf_("GetVolumeInformationA(%s) -> %d rotulo '%s'", root ? root : "(null)", r, (r && name) ? name : "");
    return r;
}
static HANDLE WINAPI h_CreateFileA(LPCSTR f, DWORD a, DWORD sh, LPSECURITY_ATTRIBUTES sa, DWORD c, DWORD fl, HANDLE t) {
    HANDLE h = r_CreateFileA(f, a, sh, sa, c, fl, t);
    if (cfg.debug ? n_cf++ < 300 : (h == INVALID_HANDLE_VALUE && f && (strstr(f, "speech") || strstr(f, "SPEECH")) && n_cf++ < 20))
        logf_("CreateFileA(%s) -> %s", f ? f : "(null)", h == INVALID_HANDLE_VALUE ? "FALHOU" : "ok");
    return h;
}
static int WINAPI h_MessageBoxA(HWND w, LPCSTR t, LPCSTR c, UINT u) { logf_("MessageBoxA('%s', '%s')", c ? c : "", t ? t : ""); return r_MessageBoxA(w, t, c, u); }
static HWND WINAPI h_FindWindowA(LPCSTR c, LPCSTR t) { HWND r = r_FindWindowA(c, t); logf_("FindWindowA(%s, %s) -> %p", HIWORD(c) ? c : "#", t ? t : "(null)", (void *)r); return r; }
static void WINAPI h_ExitProcess(UINT c) { logf_("ExitProcess(%u)", c); r_ExitProcess(c); }
static LONG CALLBACK veh(EXCEPTION_POINTERS *e) {
    DWORD code = e->ExceptionRecord->ExceptionCode;
    if (code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_ILLEGAL_INSTRUCTION || code == EXCEPTION_INT_DIVIDE_BY_ZERO || code == EXCEPTION_STACK_OVERFLOW || code == EXCEPTION_PRIV_INSTRUCTION) {
        static int n; if (n++ < 10) logf_("EXCECAO %08lx em %p (eip %08lx) dados %08lx %08lx", code, e->ExceptionRecord->ExceptionAddress, e->ContextRecord->Eip,
            e->ExceptionRecord->NumberParameters > 0 ? (DWORD)e->ExceptionRecord->ExceptionInformation[0] : 0,
            e->ExceptionRecord->NumberParameters > 1 ? (DWORD)e->ExceptionRecord->ExceptionInformation[1] : 0);
        if (n <= 3) {                                  // onde: módulo + deslocamento, registradores e endereços de retorno na pilha
            CONTEXT *c = e->ContextRecord;
            char mod[MAX_PATH] = "?"; HMODULE hm = NULL;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)e->ExceptionRecord->ExceptionAddress, &hm))
                GetModuleFileNameA(hm, mod, sizeof mod);
            const char *b = strrchr(mod, '\\');
            logf_("  modulo %s + %lx | eax %08lx ebx %08lx ecx %08lx edx %08lx esi %08lx edi %08lx ebp %08lx esp %08lx",
                  b ? b + 1 : mod, (DWORD)((BYTE *)e->ExceptionRecord->ExceptionAddress - (BYTE *)hm),
                  c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp);
            const DWORD *sp = (const DWORD *)c->Esp; int k = 0;
            for (int i = 0; i < 256 && k < 8; i++) {
                MEMORY_BASIC_INFORMATION mi;
                if (!VirtualQuery(sp + i, &mi, sizeof mi) || mi.State != MEM_COMMIT) break;
                HMODULE rm = NULL; DWORD v = sp[i];
                if (v < 0x10000 || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)v, &rm)) continue;
                char rn[MAX_PATH]; GetModuleFileNameA(rm, rn, sizeof rn);
                const char *rb = strrchr(rn, '\\');
                logf_("  pilha+%03x: %s + %lx", i * 4, rb ? rb + 1 : rn, v - (DWORD)rm);
                k++;
            }
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
static DWORD g_main_tid;
static DWORD WINAPI heartbeat(LPVOID p) {
    (void)p;
    HANDLE th = OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, g_main_tid);
    for (int i = 0; i < 15; i++) {
        Sleep(2000);
        DWORD eip = 0;
        if (th && SuspendThread(th) != (DWORD)-1) {
            CONTEXT c; c.ContextFlags = CONTEXT_CONTROL;
            if (GetThreadContext(th, &c)) eip = c.Eip;
            ResumeThread(th);
        }
        logf_("vivo: quadros %ld, janela %p, eip da thread principal %08lx", g_frames, (void *)g_hwnd, eip);
    }
    if (th) CloseHandle(th);
    return 0;
}


// ---------------------------------------------------------------- idioma
// O idioma vem do registro (HKCU\Software\Infogrames\...\language). Se ele não bater com as falas do CD
// (o disco PT-BR só tem SPEECH\05), o jogo fica em inglês e sem vozes. language=-1 no ini: detecta pelo CD.
typedef LONG (WINAPI *PFN_RQV)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
static PFN_RQV r_RegQueryValueExA;
static int g_lang = -1;
static int detect_language(void) {
    if (cfg.language >= 0) return cfg.language;
    for (char d = 'C'; d <= 'Z'; d++) {
        char root[4] = { d, ':', '\\', 0 };
        if (GetDriveTypeA(root) != DRIVE_CDROM) continue;
        char pat[MAX_PATH]; snprintf(pat, sizeof pat, "%c:\\Datas\\SPEECH\\*", d);
        WIN32_FIND_DATAA fd; HANDLE h = FindFirstFileA(pat, &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        int found = -1;
        do { if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0] >= '0' && fd.cFileName[0] <= '9') found = atoi(fd.cFileName); } while (found < 0 && FindNextFileA(h, &fd));
        FindClose(h);
        if (found >= 0) { logf_("idioma detectado pelas falas do CD (%c:): %02d", d, found); return found; }
    }
    // sem CD: procura falas copiadas para a pasta do jogo
    char pat[MAX_PATH]; snprintf(pat, sizeof pat, "%s\\..\\SPEECH\\*", g_dir);
    WIN32_FIND_DATAA fd; HANDLE h = FindFirstFileA(pat, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        int found = -1;
        do { if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0] >= '0' && fd.cFileName[0] <= '9') found = atoi(fd.cFileName); } while (found < 0 && FindNextFileA(h, &fd));
        FindClose(h);
        if (found >= 0) return found;
    }
    return -1;
}
static int fake_install_path(char *out, DWORD cap);
#define FAKE_KEY_ ((HKEY)(ULONG_PTR)0x7B1B2001)
static int dir_is_bin(void) { const char *b = strrchr(g_dir, '\\'); return b && !_stricmp(b + 1, "bin"); }
static LONG WINAPI h_RegQueryValueExA(HKEY k, LPCSTR name, LPDWORD res, LPDWORD type, LPBYTE data, LPDWORD cb) {
    LONG r;
    DWORD cap = cb ? *cb : 0;
    if (k != FAKE_KEY_ && name && !_stricmp(name, "installation path")) {
        r = r_RegQueryValueExA(k, name, res, type, data, cb);
        // cópia da pasta do jogo: usa a pasta de onde ele está rodando, não a da instalação original
        char want[MAX_PATH], got[MAX_PATH] = "";
        if (r == ERROR_SUCCESS && data && cb && dir_is_bin() && fake_install_path(want, sizeof want)) {
            snprintf(got, sizeof got, "%.*s", (int)(*cb < sizeof got ? *cb : sizeof got - 1), (char *)data);
            size_t n = strlen(got); while (n && got[n - 1] == '\\') got[--n] = 0;
            if (_stricmp(got, want) && strlen(want) + 1 <= cap) {
                logf_("installation path: %s -> %s (pasta atual do jogo)", got, want);
                strcpy((char *)data, want); *cb = (DWORD)strlen(want) + 1;
            } else logf_("installation path: %s", got);
        }
        return r;
    }
    if (k == FAKE_KEY_) {
        r = ERROR_FILE_NOT_FOUND;
        if (name && !_stricmp(name, "installation path") && data && cb && fake_install_path((char *)data, *cb)) {
            *cb = (DWORD)strlen((char *)data) + 1; if (type) *type = REG_SZ; r = ERROR_SUCCESS;
            logf_("installation path (sem registro) -> %s", (char *)data);
        }
        if (!name || _stricmp(name, "language")) return r;
    } else r = r_RegQueryValueExA(k, name, res, type, data, cb);
    if (name && !_stricmp(name, "language")) {
        if (g_lang < 0) g_lang = detect_language();
        DWORD orig = (r == ERROR_SUCCESS && data && cb && *cb >= 1) ? data[0] : 0xffffffff;
        if (g_lang >= 0 && data && cb && *cb >= 4) {
            *(DWORD *)data = (DWORD)g_lang; *cb = 4; if (type) *type = REG_DWORD; r = ERROR_SUCCESS;
        }
        logf_("registro language: %ld -> %d", (long)orig, g_lang);
    }
    return r;
}

// ---------------------------------------------------------------- resolução nativa 4:3
// O motor desenha em pixels da resolução configurada; com 16:9 a imagem fica esticada. native_res=1 grava no
// config.pc a maior resolução 4:3 que cabe no monitor (ex.: 1920x1440 num monitor 2560x1440).
static void *detour(DWORD va, const BYTE *expect, int n, void *hook, const char *name);
typedef void (__cdecl *PFN_LoadCfg)(void);
static PFN_LoadCfg r_LoadCfg;
static void apply_config_mem(void) {
    BYTE *c = (BYTE *)0x639f80;
    if (*(DWORD *)c != 0x14) { logf_("config.pc: versao %lu inesperada, nada alterado", *(DWORD *)c); return; }
    int *renderer = (int *)(c + 0x04);             // 2 = OpenGL (4/5 = a última tentativa falhou, 1 = Direct3D, 0 = software)
    if (cfg.force_gl && *renderer != 2) { logf_("config.pc: renderizador %d -> 2 (OpenGL)", *renderer); *renderer = 2; }
    WORD *res = (WORD *)(c + 0x10);
    if (cfg.game_res == 1 && g_nat_w > 0) {
        if (res[0] != g_nat_w || res[1] != g_nat_h) { logf_("config.pc: resolucao %ux%u -> %dx%d", res[0], res[1], g_nat_w, g_nat_h); res[0] = (WORD)g_nat_w; res[1] = (WORD)g_nat_h; }
        else logf_("config.pc: resolucao %dx%d", g_nat_w, g_nat_h);
    }
}
static void __cdecl h_LoadCfg(void) { r_LoadCfg(); apply_config_mem(); }
// O jogo ajusta matiz/saturação/brilho (menu Vídeo) em cada paleta antes de montar as texturas (0x407d30). Com
// saturação alta, cores originais diferentes podem virar a mesma, e o desenho da textura mudaria conforme o ajuste do
// jogador. Aqui elas continuam diferentes (no máximo 1 ou 2 níveis num canal, invisível): a lista HD e os ícones do
// controle reconhecem as texturas pelo desenho, com qualquer ajuste de cor.
typedef void (__cdecl *PFN_AdjPal)(BYTE *pal);
static PFN_AdjPal r_AdjPal;
static void __cdecl h_AdjPal(BYTE *pal) {
    int n = *(int *)(pal + 0x0c); DWORD *e = *(DWORD **)(pal + 0x14);
    DWORD *orig = (n > 1 && n <= 4096 && e) ? malloc((size_t)n * 4) : NULL;
    if (orig) memcpy(orig, e, (size_t)n * 4);
    r_AdjPal(pal);
    if (!orig) return;
    int fixed = 0;
    for (int i = 1; i < n; i++) {
        int same = -1;
        for (int j = 0; j < i; j++) if (!((orig[j] ^ orig[i]) & 0xffffff)) { same = j; break; }
        if (same >= 0) { e[i] = (e[i] & 0xff000000u) | (e[same] & 0xffffffu); continue; }   // mesma cor: igual
        DWORD a0 = e[i];
        for (int k = 0; k <= 48; k++) {
            DWORD c = a0;
            if (k) {                                // ±1, ±2 ... em cada canal
                int d = (k - 1) / 6 + 1, ch = ((k - 1) / 2) % 3, sg = (k - 1) % 2 ? -1 : 1;
                int v = (int)((a0 >> (8 * ch)) & 255) + sg * d;
                if (v < 0 || v > 255) continue;
                c = (a0 & ~(255u << (8 * ch))) | ((DWORD)v << (8 * ch));
            }
            int clash = 0;
            for (int j = 0; j < i; j++) if (!((e[j] ^ c) & 0xffffff)) { clash = 1; break; }
            if (!clash) { if (k) fixed++; e[i] = c; break; }
        }
    }
    free(orig);
    static int nlog; if (fixed && nlog++ < 10) logf_("paleta de %d cores: %d cores separadas apos o ajuste de cor do jogo", n, fixed);
}
static void install_pal_hook(void) {
    if (!cfg.hd_tex && !cfg.dump_tex && !(cfg.xinput && cfg.pad_prompts == 2)) return;
    static const BYTE e[] = { 0x55, 0x8b, 0xec, 0x83, 0xe4, 0xf8 };
    r_AdjPal = (PFN_AdjPal)detour(0x407d30, e, sizeof e, (void *)h_AdjPal, "ajuste de cor das paletas");
}
static void native_res(void) {
    HMONITOR m = MonitorFromPoint((POINT){0, 0}, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi; mi.cbSize = sizeof mi;
    if (!GetMonitorInfoA(m, &mi)) return;
    int w = mi.rcMonitor.right - mi.rcMonitor.left, h = mi.rcMonitor.bottom - mi.rcMonitor.top;
    if (cfg.aspect == 1) { if (w * 3 > h * 4) w = h * 4 / 3; else h = w * 3 / 4; }
    else if (cfg.aspect == 2 && w * 3 < h * 4) h = w * 3 / 4;   // monitor mais "quadrado" que 4:3 (5:4): faixas em cima e embaixo
    w &= ~7; h &= ~1;
    g_nat_w = w; g_nat_h = h;
    // o jogo carrega o config.pc em 0x415e10; depois dele a DLL ajusta resolução e renderizador na memória
    // (o jogo grava o arquivo sozinho logo em seguida). Sem o gancho, grava direto no arquivo.
    static const BYTE e[] = { 0x56, 0x6a, 0x00, 0x6a, 0x00 };
    r_LoadCfg = (PFN_LoadCfg)detour(0x415e10, e, sizeof e, (void *)h_LoadCfg, "config.pc");
    if (r_LoadCfg || cfg.game_res != 1) return;
    char path[MAX_PATH]; snprintf(path, sizeof path, "%s\\config.pc", g_dir);
    FILE *f = fopen(path, "r+b");
    if (!f) return;
    unsigned short cur[2];
    if (fseek(f, 0x10, SEEK_SET) == 0 && fread(cur, 2, 2, f) == 2 && (cur[0] != w || cur[1] != h)) {
        unsigned short nw[2] = { (unsigned short)w, (unsigned short)h };
        fseek(f, 0x10, SEEK_SET); fwrite(nw, 2, 2, f);
        logf_("config.pc (arquivo): resolucao %ux%u -> %dx%d", cur[0], cur[1], w, h);
    }
    fclose(f);
}


// ---------------------------------------------------------------- DirectInput: teclado do jogo
// O jogo lê o teclado com IDirectInputDevice::GetDeviceState(256 bytes, códigos DIK). O controle é somado ali.
typedef HRESULT (WINAPI *PFN_DI_CreateDevice)(void *self, REFGUID g, void **dev, void *outer);
typedef HRESULT (WINAPI *PFN_DID_GetDeviceState)(void *self, DWORD cb, void *data);
static PFN_DI_CreateDevice r_DI_CreateDevice;
static PFN_DID_GetDeviceState r_DID_GetDeviceState;
static void *g_kbd;
static const GUID k_GUID_SysKeyboard = { 0x6F1D2B61, 0xD5A0, 0x11CF, { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

static void patch_vtbl(void **vtbl, int idx, void *hook, void **orig) {
    if (vtbl[idx] == hook) return;
    if (!*orig) *orig = vtbl[idx];
    DWORD old; VirtualProtect(&vtbl[idx], 4, PAGE_EXECUTE_READWRITE, &old);
    vtbl[idx] = hook;
    VirtualProtect(&vtbl[idx], 4, old, &old);
}
static HRESULT WINAPI h_DID_GetDeviceState(void *self, DWORD cb, void *data) {
    HRESULT r = r_DID_GetDeviceState(self, cb, data);
    if (self == g_kbd && cb == 256 && data && cfg.xinput) {
        poll_pad();
        BYTE *k = data; int any = 0;
        for (int i = 0; i < 256; i++) if (g_paddik[i]) { any = 1; break; }
        if (any) {
            if (FAILED(r)) memset(k, 0, 256);   // teclado não adquirido: entrega só o controle
            for (int i = 0; i < 256; i++) k[i] |= g_paddik[i];
            r = 0;
        }
    }
    return r;
}
static HRESULT WINAPI h_DI_CreateDevice(void *self, REFGUID g, void **dev, void *outer) {
    HRESULT r = r_DI_CreateDevice(self, g, dev, outer);
    if (r == 0 && dev && *dev && g && IsEqualGUID(g, &k_GUID_SysKeyboard)) {
        g_kbd = *dev;
        patch_vtbl(*(void ***)*dev, 9, (void *)h_DID_GetDeviceState, (void **)&r_DID_GetDeviceState);
        static int n; if (n++ < 5) logf_("DirectInput: teclado %p (controle ligado nele)", *dev);
    }
    return r;
}
static void hook_dinput(void *di) {
    if (!di || !cfg.enabled) return;
    patch_vtbl(*(void ***)di, 3, (void *)h_DI_CreateDevice, (void **)&r_DI_CreateDevice);
}


// ---------------------------------------------------------------- widescreen (Hor+)
// O motor projeta em "pixels de PlayStation" (512x240) e escala por sx = largura/512 e sy = altura/240.
// Em 16:9 isso estica a imagem. Aqui a função de tela (0x405ef0) roda como se a tela fosse 4:3 (mesma altura),
// o que mantém todas as escalas certas, e depois os limites de tela/recorte recebem a largura real:
// a projeção passa a mostrar mais cenário dos lados em vez de esticar.
static volatile LONG g_kx;
typedef void (__cdecl *PFN_SetScreen)(int w, int h, int bw, int bh);
typedef void (__cdecl *PFN_SetClip)(int n, int f, int w, int h);
static PFN_SetScreen r_SetScreen;
static int g_ws_on;
// Modelos 3D presos à tela (ícones do HUD: cenoura, moeda de ouro, claquete; objetos de menus): 0x404d40 posiciona
// cada um em x * largura / 512. No widescreen isso os empurrava para as bordas, longe dos números 2D, que ficam no
// 4:3 central. O operando de largura dessa conta passa a ler g_hud_w (= largura 4:3 no widescreen).
static volatile DWORD g_hud_w = 640;
static void __cdecl h_SetScreen(int w, int h, int bw, int bh) {
    w &= 0xffff; h &= 0xffff;
    if (cfg.aspect == 2 && w * 3 > h * 4 + 3) {
        int w43 = (h * 4 / 3) & ~1;
        bw &= 0xffff; bh &= 0xffff;
        r_SetScreen(w43, h, bw, bh);
        int hw = w / 2;
        *(WORD *)0x639ec4 = (WORD)w;
        *(WORD *)0x639ec8 = (WORD)hw;
        *(float *)0x63c054 = (float)-hw;
        *(float *)0x63bf90 = (float)(hw - 1);
        *(int *)0x63c050 = -hw;
        *(int *)0x63bf8c = hw - 1;
        *(int *)0x639f64 = hw;
        *(WORD *)0x639ed0 = (WORD)((bw * w + w43 / 2) / w43);   // caminho 2D que usa largura/base: mesma escala sx
        ((PFN_SetClip)0x409760)(0, 0x7fff, w, h);
        g_ws_on = 1;
        g_kx = (LONG)((long long)4096 * w43 / w);
        g_hud_w = (DWORD)w43;
        logf_("widescreen: tela %dx%d com escalas de %dx%d (base %dx%d), sx %.3f sy %.3f", w, h, w43, h, bw, bh, *(float *)0x639ef0, *(float *)0x639ef4);
    } else {
        r_SetScreen(w, h, bw, bh);
        g_ws_on = 0;
        g_kx = 4096;
        g_hud_w = (DWORD)w;
        logf_("tela %dx%d (base %dx%d)", w, h, bw, bh);
    }
}
static void patch_dword(DWORD va, DWORD expect, DWORD val, const char *name);
static void install_widescreen(void) {
    static const BYTE expect[] = { 0x8b, 0x4c, 0x24, 0x04, 0x8b, 0x44, 0x24, 0x0c };
    BYTE *site = (BYTE *)0x405ef0;
    if (memcmp(site, expect, sizeof expect)) { logf_("widescreen: bytes inesperados em 0x405ef0, desligado"); return; }
    BYTE *tr = VirtualAlloc(NULL, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!tr) return;
    memcpy(tr, site, 8);
    tr[8] = 0xE9; *(DWORD *)(tr + 9) = (DWORD)(site + 8) - (DWORD)(tr + 13);
    r_SetScreen = (PFN_SetScreen)tr;
    DWORD old; VirtualProtect(site, 5, PAGE_EXECUTE_READWRITE, &old);
    site[0] = 0xE9; *(DWORD *)(site + 1) = (DWORD)h_SetScreen - (DWORD)(site + 5);
    VirtualProtect(site, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), site, 5);
    logf_("widescreen: gancho em 0x405ef0 instalado");
    patch_dword(0x404dda, 0x639ec4, (DWORD)&g_hud_w, "HUD 3D");   // mov ecx, [0x639ec4] -> mov ecx, [g_hud_w]
    if (*(DWORD *)0x404dda == (DWORD)&g_hud_w) logf_("HUD 3D: icones alinhados ao 4:3 central");
}


// ---------------------------------------------------------------- distância de visão
// 0x409840(a, b): corte de profundidade de cada parte do mundo. "Nível de Detalhes" escolhe a (baixo), a meia (médio)
// ou b (alto); b também é o limite máximo (malha simplificada e objetos fixos). Os dois são multiplicados por
// draw_distance/100, até 0x7FFF (tamanho da tabela de ordenação). O céu/fundo (0x7D00) fica como está.
typedef void (__cdecl *PFN_SetCut)(int a, int b);
static PFN_SetCut r_SetCut;
static void __cdecl h_SetCut(int a, int b) {
    {   // registra os pares (a, b) distintos que as fases usam (diagnóstico da distância de visão)
        static int seen[64][2], ns;
        int k, found = 0;
        for (k = 0; k < ns; k++) if (seen[k][0] == a && seen[k][1] == b) { found = 1; break; }
        if (!found && ns < 64) { seen[ns][0] = a; seen[ns][1] = b; ns++; logf_("corte de profundidade da fase: a=%d b=%d", a, b); }
    }
    if (cfg.draw_dist != 100 && b > 0 && b < 0x7d00 && a >= 0 && a <= 0x7fff) {
        long long k = cfg.draw_dist;
        int a2 = (int)(a * k / 100), b2 = (int)(b * k / 100);
        if (a2 > 0x7fff) a2 = 0x7fff;
        if (b2 > 0x7fff) b2 = 0x7fff;
        static int last_a = -1, last_b = -1, n;
        if ((a != last_a || b != last_b) && n < 40) { n++; last_a = a; last_b = b; logf_("distancia: corte %d/%d -> %d/%d", a, b, a2, b2); }
        a = a2; b = b2;
    }
    r_SetCut(a, b);
}

// desvio genérico: copia n bytes (instruções inteiras, sem endereços relativos) para um trampolim
static void *detour(DWORD va, const BYTE *expect, int n, void *hook, const char *name) {
    BYTE *site = (BYTE *)va;
    if (memcmp(site, expect, n)) { logf_("%s: bytes inesperados em %08lx, desligado", name, va); return NULL; }
    BYTE *tr = VirtualAlloc(NULL, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!tr) return NULL;
    memcpy(tr, site, n);
    tr[n] = 0xE9; *(DWORD *)(tr + n + 1) = (DWORD)(site + n) - (DWORD)(tr + n + 5);
    DWORD old; VirtualProtect(site, 5, PAGE_EXECUTE_READWRITE, &old);
    site[0] = 0xE9; *(DWORD *)(site + 1) = (DWORD)hook - (DWORD)(site + 5);
    VirtualProtect(site, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), site, 5);
    logf_("%s: gancho em %08lx", name, va);
    return tr;
}
static void patch_dword(DWORD va, DWORD expect, DWORD val, const char *name) {
    DWORD *p = (DWORD *)va;
    if (*p != expect) { logf_("%s: valor inesperado em %08lx", name, va); return; }
    DWORD old; VirtualProtect(p, 4, PAGE_EXECUTE_READWRITE, &old);
    *p = val;
    VirtualProtect(p, 4, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 4);
}
static void install_draw_distance(void) {
    // buffers do motor 4x maiores: mais cenário visível = mais polígonos por quadro
    patch_dword(0x459f9e, 0xf4240, 0x3d0900, "buffer de primitivas (tamanho)");
    patch_dword(0x459fa9, 0xf4240, 0x3d0900, "buffer de primitivas (fim)");
    patch_dword(0x424f90, 0x111700, 0x445c00, "buffer de vertices GL");
    if (cfg.draw_dist == 100) return;
    static const BYTE e[] = { 0x8b, 0x4c, 0x24, 0x04, 0xa1, 0x74, 0x08, 0x4d, 0x00 };
    r_SetCut = (PFN_SetCut)detour(0x409840, e, sizeof e, (void *)h_SetCut, "distancia de visao");
    logf_("distancia de visao: %d%%", cfg.draw_dist);
}


// ---------------------------------------------------------------- instalação sem registro (pasta copiada)
// O jogo lê HKCU\Software\Infogrames\Bugs Bunny & Taz - Time Busters\"installation path" e muda a pasta de
// trabalho para <caminho>\bin; sem a chave ele fecha. Se ela não existir, a DLL responde com a pasta dela.
#define FAKE_KEY ((HKEY)(ULONG_PTR)0x7B1B2001)
typedef LONG (WINAPI *PFN_ROK)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
typedef LONG (WINAPI *PFN_RCK)(HKEY);
static PFN_ROK r_RegOpenKeyExA;
static PFN_RCK r_RegCloseKey;
static LONG WINAPI h_RegOpenKeyExA(HKEY k, LPCSTR sub, DWORD o, REGSAM sam, PHKEY out) {
    LONG r = r_RegOpenKeyExA(k, sub, o, sam, out);
    if (r != ERROR_SUCCESS && k == HKEY_CURRENT_USER && sub && out && !_stricmp(sub, "Software\\Infogrames\\Bugs Bunny & Taz - Time Busters")) {
        logf_("registro do jogo ausente: usando a pasta da DLL como instalacao");
        *out = FAKE_KEY; r = ERROR_SUCCESS;
    }
    return r;
}
static LONG WINAPI h_RegCloseKey(HKEY k) { return k == FAKE_KEY ? ERROR_SUCCESS : r_RegCloseKey(k); }
static int fake_install_path(char *out, DWORD cap) {
    char p[MAX_PATH]; snprintf(p, sizeof p, "%s", g_dir);
    char *s = strrchr(p, '\\'); if (s) *s = 0;                 // pasta acima de bin
    if (strlen(p) + 1 > cap) return 0;
    strcpy(out, p);
    return 1;
}

// ---------------------------------------------------------------- objetos: alcance e frustum
// Lista de visão 0x45eee0: objeto com +0x6e & 2 é desligado (nem atualiza, nem desenha) quando a distância até a
// câmera passa do alcance word[+0x76] — é o "brotar" de itens e personagens. O alcance é multiplicado por
// object_distance/100. No widescreen, o teste de frustum (coeficientes do 4:3) recebe x * 4:3/largura, para os
// objetos das laterais não sumirem.
static volatile LONG g_kr = 512;     // alcance x g_kr / 256
static volatile LONG g_klod = 1024;  // distância da câmera / (g_klod / 256) antes de escolher o LOD do modelo
static volatile LONG g_kx = 4096;    // x do frustum x g_kx / 4096 (declarado antes, valor inicial aqui)
static BYTE *stub_mem(int n) { return VirtualAlloc(NULL, n, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE); }
static int put_jmp(BYTE *at, void *to) { at[0] = 0xE9; *(DWORD *)(at + 1) = (DWORD)to - (DWORD)(at + 5); return 5; }
static int put_jcc(BYTE *at, BYTE cc, void *to) { at[0] = 0x0F; at[1] = cc; *(DWORD *)(at + 2) = (DWORD)to - (DWORD)(at + 6); return 6; }
static void write_site(DWORD va, const BYTE *expect, int n, void *stub, const char *name) {
    BYTE *site = (BYTE *)va;
    DWORD old; VirtualProtect(site, n, PAGE_EXECUTE_READWRITE, &old);
    put_jmp(site, stub);
    for (int i = 5; i < n; i++) site[i] = 0x90;
    VirtualProtect(site, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), site, n);
    (void)expect;
    logf_("%s: gancho em %08lx", name, va);
}
// Alcance de cada objeto: o jogo compara a distância com o alcance do objeto (word +0x76) e desliga quem está fora.
// O mod multiplica esse alcance. Diagnóstico (debug=1): registra os objetos que só ficam ligados por causa do mod e
// F6 escolhe um tipo deles para ficar com o alcance original (para achar o que esconde o Piu-Piu).
static struct { BYTE cat; WORD id; } g_rtypes[64]; static int g_rtypen, g_rexcl = -1, g_rlogic = 1;
static int rtype_index(BYTE cat, WORD id, int add) {
    for (int i = 0; i < g_rtypen; i++) if (g_rtypes[i].cat == cat && g_rtypes[i].id == id) return i;
    if (add && g_rtypen < 64) { g_rtypes[g_rtypen].cat = cat; g_rtypes[g_rtypen].id = id; return g_rtypen++; }
    return -1;
}
static DWORD __cdecl range_decide(const BYTE *o, DWORD dist, DWORD range) {
    BYTE cat = o[0x31];
    // objetos de lógica (categoria 0x10: sem modelo, gatilhos e pontos de passagem) não ganham nada visível com o
    // alcance maior e, ligados de longe, mexem no jogo (o Piu-Piu ia parar num ponto distante): alcance original
    if (cat == 0x10 && g_rlogic) return range;
    DWORD eff = (DWORD)(((unsigned long long)range * (DWORD)g_kr) >> 8);
    if (!cfg.debug || g_kr == 256) return eff;
    WORD id = *(const WORD *)(o + 0x14c);
    int band = dist > range && dist <= eff;            // ligado só por causa do mod
    int ti = rtype_index(cat, id, band);
    if (band) {
        static const BYTE *seen[400]; static int ns, nl;
        int k; for (k = 0; k < ns; k++) if (seen[k] == o) break;
        if (k == ns && ns < 400 && nl < 300) {
            seen[ns++] = o; nl++;
            logf_("alcance: obj %p tipo %d (cat %02x id %04x) flags6e %04x f8 %08lx f24 %08lx w36 %04x p9c %08lx p118 %08lx alcance %lu dist %lu",
                  (const void *)o, ti, cat, id, *(const WORD *)(o + 0x6e), *(const DWORD *)(o + 8), *(const DWORD *)(o + 0x24),
                  *(const WORD *)(o + 0x36), *(const DWORD *)(o + 0x9c), *(const DWORD *)(o + 0x118), range, dist);
        }
    }
    if (g_rexcl >= 0 && ti == g_rexcl) return range;   // F7: esse tipo fica com o alcance original
    return eff;
}
static void install_object_hooks(void) {
    g_kr = cfg.actor_dist * 256 / 100;
    // alcance: 0x45f36b  mov ecx,[esi] / cmp ecx,eax / jbe 0x45f381   (fora: 0x45f371)
    static const BYTE e1[] = { 0x8b, 0x0e, 0x3b, 0xc8, 0x76, 0x10 };
    if (!memcmp((void *)0x45f36b, e1, sizeof e1)) {
        BYTE *st = stub_mem(64), *p = st;
        *p++ = 0x51; *p++ = 0x52;                                         // push ecx / push edx
        *p++ = 0x50;                                                      // push eax (alcance do objeto)
        *p++ = 0xff; *p++ = 0x36;                                         // push dword [esi] (distância)
        *p++ = 0x53;                                                      // push ebx (objeto)
        *p++ = 0xe8; *(DWORD *)p = (DWORD)range_decide - (DWORD)(p + 4); p += 4;   // call range_decide -> eax
        *p++ = 0x83; *p++ = 0xc4; *p++ = 0x0c;                            // add esp, 12
        *p++ = 0x5a; *p++ = 0x59;                                         // pop edx / pop ecx
        *p++ = 0x8b; *p++ = 0x0e;                                         // mov ecx, [esi]
        *p++ = 0x3b; *p++ = 0xc8;                                         // cmp ecx, eax
        p += put_jcc(p, 0x86, (void *)0x45f381);                         // jbe dentro
        p += put_jmp(p, (void *)0x45f371);                               // fora
        write_site(0x45f36b, e1, sizeof e1, st, "alcance dos objetos");
        logf_("alcance dos objetos: %d%%", cfg.actor_dist);
    } else logf_("alcance dos objetos: bytes inesperados, desligado");

    // frustum, bloco 1: 0x45f6db  mov si,[esi] / test si,si   -> volta em 0x45f6e1
    static const BYTE f1[] = { 0x66, 0x8b, 0x36, 0x66, 0x85, 0xf6 };
    // bloco 2: 0x45f8ea  mov si,[esi] / add edx,ecx            -> volta em 0x45f8ef
    static const BYTE f2[] = { 0x66, 0x8b, 0x36, 0x03, 0xd1 };
    // bloco 3: 0x45fa7f  mov ecx,[edi] / add esp,0x30          -> volta em 0x45fa84
    static const BYTE f3[] = { 0x8b, 0x0f, 0x83, 0xc4, 0x30 };
    if (!memcmp((void *)0x45f6db, f1, sizeof f1) && !memcmp((void *)0x45f8ea, f2, sizeof f2) && !memcmp((void *)0x45fa7f, f3, sizeof f3)) {
        BYTE *st, *p;
        st = stub_mem(64); p = st;
        *p++ = 0x0f; *p++ = 0xbf; *p++ = 0x36;                            // movsx esi, word [esi]
        *p++ = 0x0f; *p++ = 0xaf; *p++ = 0x35; *(DWORD *)p = (DWORD)&g_kx; p += 4;   // imul esi, [g_kx]
        *p++ = 0xc1; *p++ = 0xfe; *p++ = 0x0c;                            // sar esi, 12
        *p++ = 0x66; *p++ = 0x85; *p++ = 0xf6;                            // test si, si
        p += put_jmp(p, (void *)0x45f6e1);
        write_site(0x45f6db, f1, sizeof f1, st, "frustum 1");
        st = stub_mem(64); p = st;
        *p++ = 0x0f; *p++ = 0xbf; *p++ = 0x36;                            // movsx esi, word [esi]
        *p++ = 0x0f; *p++ = 0xaf; *p++ = 0x35; *(DWORD *)p = (DWORD)&g_kx; p += 4;   // imul esi, [g_kx]
        *p++ = 0xc1; *p++ = 0xfe; *p++ = 0x0c;                            // sar esi, 12
        *p++ = 0x03; *p++ = 0xd1;                                         // add edx, ecx
        p += put_jmp(p, (void *)0x45f8ef);
        write_site(0x45f8ea, f2, sizeof f2, st, "frustum 2");
        st = stub_mem(64); p = st;
        *p++ = 0x8b; *p++ = 0x0f;                                         // mov ecx, [edi]
        *p++ = 0x0f; *p++ = 0xaf; *p++ = 0x0d; *(DWORD *)p = (DWORD)&g_kx; p += 4;   // imul ecx, [g_kx]
        *p++ = 0xc1; *p++ = 0xf9; *p++ = 0x0c;                            // sar ecx, 12
        *p++ = 0x83; *p++ = 0xc4; *p++ = 0x30;                            // add esp, 0x30
        p += put_jmp(p, (void *)0x45fa84);
        write_site(0x45fa7f, f3, sizeof f3, st, "frustum 3");
    } else logf_("frustum dos objetos: bytes inesperados, desligado");

    // LOD dos modelos: 0x45f3f2 compara a distância (em [esi]) com byte[+0x121]*256 e byte[+0x122]*256 para trocar
    // para os modelos simplificados. A distância é dividida por lod_distance/100 logo antes (histerese incluída).
    g_klod = cfg.lod_dist * 256 / 100;
    static const BYTE l1[] = { 0x33, 0xc0, 0x8a, 0xa3, 0x21, 0x01, 0x00, 0x00 };   // xor eax,eax / mov ah,[ebx+0x121]
    if (cfg.lod_dist == 100) logf_("LOD dos modelos: original");
    else if (!memcmp((void *)0x45f3f2, l1, sizeof l1)) {
        BYTE *st = stub_mem(64), *p = st;
        *p++ = 0x51; *p++ = 0x52;                                          // push ecx / push edx
        *p++ = 0x8b; *p++ = 0x06;                                          // mov eax, [esi]
        *p++ = 0xb9; *(DWORD *)p = 0x100; p += 4;                          // mov ecx, 256
        *p++ = 0xf7; *p++ = 0xe1;                                          // mul ecx
        *p++ = 0xf7; *p++ = 0x35; *(DWORD *)p = (DWORD)&g_klod; p += 4;   // div dword [g_klod]
        *p++ = 0x89; *p++ = 0x06;                                          // mov [esi], eax
        *p++ = 0x5a; *p++ = 0x59;                                          // pop edx / pop ecx
        memcpy(p, l1, sizeof l1); p += sizeof l1;                         // instruções originais
        p += put_jmp(p, (void *)0x45f3fa);
        write_site(0x45f3f2, l1, sizeof l1, st, "LOD dos modelos");
        logf_("LOD dos modelos: %d%%", cfg.lod_dist);
    } else logf_("LOD dos modelos: bytes inesperados, desligado");
}


// ---------------------------------------------------------------- dicas de botão com controle conectado
// As mensagens do jogo mostram a tecla configurada entre aspas ("'d' para pular"), pelo op 0x14 do texto
// (0x467667 chama 0x40fbe0 = GetKeyNameText). Com o controle conectado, a chamada passa por aqui e devolve o botão
// do Xbox: um byte de ícone da fonte (button_prompts=2, ícones redesenhados) ou o nome ("A", "LB"...).
static volatile BYTE g_kn_icon;  // a última chamada devolveu um ícone (o texto sai sem aspas)
static int btn_for_scancode(int sc) {
    const DWORD *keys = (const DWORD *)(0x639f80 + 0x4c);     // teclas atuais do jogador 1 (config.pc na memória)
    int a = cfg.swap_ab ? BTN_B : BTN_A, b = cfg.swap_ab ? BTN_A : BTN_B;
    if (sc == (int)keys[K_JUMP]) return a;
    if (sc == (int)keys[K_KICK]) return BTN_X;
    if (sc == (int)keys[K_EARS]) return b;
    if (sc == (int)keys[K_ACTION]) return BTN_Y;
    if (sc == (int)keys[K_FRIEND]) return BTN_LB;
    if (sc == (int)keys[K_CAMSW]) return BTN_RB;
    if (sc == (int)keys[K_CAML]) return cfg.inv_trig ? BTN_RT : BTN_LT;
    if (sc == (int)keys[K_CAMR]) return cfg.inv_trig ? BTN_LT : BTN_RT;
    if (sc == 0x1c && cfg.a_enter) return a;
    if (sc == 0x01) return BTN_START;
    if (sc == 0x3e) return b;                               // "menu anterior": B volta, como no Xbox (o View também)
    return -1;
}
typedef int (__cdecl *PFN_KeyName)(int sc, char *buf);
static int __cdecl kn_text(int sc, char *buf) {
    if (sc == 0x3e) { DWORD t = GetTickCount(); g_f4_prompt_tick = t ? t : 1; InterlockedIncrement(&g_f4_prompt_n); }
    if (cfg.xinput && cfg.pad_prompts && g_pad_on && buf) {
        int b = btn_for_scancode(sc);
        if (b >= 0) {
            if (cfg.pad_prompts == 2 && pad_icon_char(b)) { buf[0] = (char)pad_icon_char(b); buf[1] = 0; g_kn_icon = 1; return 1; }
            strcpy(buf, k_btn_name[b]); g_kn_icon = 0;
            return (int)strlen(buf);
        }
    }
    g_kn_icon = 0;
    return ((PFN_KeyName)0x40fbe0)(sc, buf);
}
// hud_probe: registra o texto cru de cada mensagem que pede uma tecla (op 0x14 em 0x4673c6), uma vez por mensagem
static void __cdecl op14_log(DWORD id, const BYTE *txt, DWORD pos) {
    static DWORD seen[48]; static int nseen, nl;
    if (nl >= 60 || !txt || IsBadReadPtr(txt, 96)) return;
    DWORD h = 2166136261u ^ (id & 0xff);
    for (int i = 0; i < 24; i++) h = (h ^ txt[i]) * 16777619u;
    for (int i = 0; i < nseen; i++) if (seen[i] == h) return;
    if (nseen < 48) seen[nseen++] = h;
    char hx[96 * 3 + 1], as[97]; int o = 0;
    for (int i = 0; i < 96; i++) { o += snprintf(hx + o, sizeof hx - o, "%02x ", txt[i]); as[i] = txt[i] >= 32 && txt[i] < 127 ? (char)txt[i] : '.'; }
    as[96] = 0;
    const BYTE *st = *(BYTE **)0x639f70;
    logf_("op14: acao %02lx pos %lu esquema %d estado %lx | %s| %s", id & 0xff, pos & 0xffff, st ? st[0x95] : -1, st ? *(DWORD *)st : 0, as, hx);
    nl++;
}
// o que a op 0x14 deixou em 0x4d5fe0 (via 0 = nome da tecla com aspas/ícone, 1 = vazio com ícone do Enter)
static void __cdecl op14_out(DWORD via) {
    static DWORD seen[32]; static int nseen, nl;
    const BYTE *o = (const BYTE *)0x4d5fe0;
    DWORD h = 2166136261u ^ via;
    for (int i = 0; i < 16; i++) h = (h ^ o[i]) * 16777619u;
    if (nl >= 40) return;
    for (int i = 0; i < nseen; i++) if (seen[i] == h) return;
    if (nseen < 32) seen[nseen++] = h;
    char hx[16 * 3 + 1]; int n = 0;
    for (int i = 0; i < 16; i++) n += snprintf(hx + n, sizeof hx - n, "%02x ", o[i]);
    logf_("op14: resultado via %lu tecla %02lx pad %d | %s", via, *(DWORD *)0x4d5f20, (int)g_pad_on, hx);
    nl++;
}
// hud_probe: cada texto novo que passa pela função de texto do jogo (0x466ce0), com os 4 argumentos e os bytes crus
static void __cdecl txt_log(const DWORD *sp) {
    static DWORD seen[512]; static int nseen, nl;
    if (nl >= 400) return;
    const BYTE *t = (const BYTE *)sp[2];
    if (!t || IsBadReadPtr(t, 160)) return;
    DWORD h = 2166136261u;
    for (int i = 0; i < 48; i++) h = (h ^ t[i]) * 16777619u;
    for (int i = 0; i < nseen; i++) if (seen[i] == h) return;
    if (nseen < 512) seen[nseen++] = h;
    char hx[160 * 3 + 1], as[161]; int o = 0;
    for (int i = 0; i < 160; i++) { o += snprintf(hx + o, sizeof hx - o, "%02x ", t[i]); as[i] = t[i] >= 32 && t[i] < 127 ? (char)t[i] : '.'; }
    as[160] = 0;
    logf_("texto: a1 %08lx a3 %08lx a4 %08lx | %s| %s", sp[1], sp[3], sp[4], as, hx);
    nl++;
}
// ---- menus "nome ... valor" centrados (Configuração > Som etc.): os x dos textos do jogo são trocados na memória,
// antes de a função de texto lê-los, pelo original + o deslocamento medido no quadro anterior (menu_center.c)
static struct { BYTE *p; short orig, written; } g_mc_x[96]; static int g_mc_xn;   // x original de cada trecho visto
static struct { BYTE *p; MC_RUN r; } g_mc_cur[64]; static int g_mc_curn;           // trechos do quadro atual
static BYTE *g_mc_used[64]; static int g_mc_usedn, g_mc_shift;                     // decisão do quadro anterior
static int mc_orig(BYTE *p) {                         // x original guardado (ou o atual, se o texto é novo/mudou)
    short cur = (short)(p[0] << 8 | p[1]);
    for (int i = 0; i < g_mc_xn; i++) if (g_mc_x[i].p == p) {
        if (cur != g_mc_x[i].written) { g_mc_x[i].orig = cur; g_mc_x[i].written = cur; }
        return i;
    }
    int i = g_mc_xn < 96 ? g_mc_xn++ : (int)(GetTickCount() % 96);
    g_mc_x[i].p = p; g_mc_x[i].orig = cur; g_mc_x[i].written = cur;
    return i;
}
static void mc_text(BYTE *s) {
    int len = 0;                                       // tamanho até o 0x18, com cada op conferido
    for (int o = 0; o < 4096; ) {
        if (IsBadReadPtr(s + o, 3)) return;
        int code = s[o], l = s[o + 1] << 8 | s[o + 2];
        if (code == 0x18) { len = o + 3; break; }
        if (l < 3) return;
        o += l;
    }
    if (!len || IsBadReadPtr(s, len)) return;
    float sc = *(volatile float *)0x639efc; if (!(sc > 0.25f && sc < 4)) sc = 1;
    MC_RUN r[24]; int n = mc_scan(s, len, (const BYTE *)0x4cf370, sc, r, 24);
    for (int k = 0; k < n; k++) {
        if (r[k].align != MC_LEFT && r[k].align != MC_RIGHT) continue;
        BYTE *p = s + r[k].xoff; int i = mc_orig(p);
        r[k].x = g_mc_x[i].orig;
        if (g_mc_curn < 64) { g_mc_cur[g_mc_curn].p = p; g_mc_cur[g_mc_curn].r = r[k]; g_mc_curn++; }
        int used = 0;
        for (int j = 0; j < g_mc_usedn; j++) if (g_mc_used[j] == p) { used = 1; break; }
        short want = (short)(used ? g_mc_x[i].orig + g_mc_shift : g_mc_x[i].orig);
        if (want != g_mc_x[i].written && !IsBadWritePtr(p, 2)) { p[0] = (BYTE)((unsigned short)want >> 8); p[1] = (BYTE)want; g_mc_x[i].written = want; }
    }
}
static void mc_frame_end(void) {
    if (!cfg.center_menus) return;
    MC_RUN r[64] = { { 0 } }; unsigned char use[64]; int n = g_mc_curn;
    for (int i = 0; i < n; i++) r[i] = g_mc_cur[i].r;
    int sh = mc_shift(r, n, use), un = 0;
    for (int i = 0; i < n; i++) if (use[i] && un < 64) g_mc_used[un++] = g_mc_cur[i].p;
    if (sh != g_mc_shift || un != g_mc_usedn) {
        static int nl; if (un && nl++ < 10) logf_("menu nome/valor centralizado: %d trechos, x %+d", un, sh);
    }
    g_mc_usedn = un; g_mc_shift = sh; g_mc_curn = 0;
}
static void __cdecl txt_hook(const DWORD *sp) {
    if (cfg.hud_probe) txt_log(sp);
    if (cfg.center_menus && sp[2]) mc_text((BYTE *)sp[2]);
}
static void install_text_hook(void) {
    if (!cfg.center_menus && !cfg.hud_probe) return;
    static const BYTE e0[] = { 0x81, 0xec, 0xcc, 0x00, 0x00, 0x00 };   // 0x466ce0: sub esp, 0xcc (função de texto)
    if (memcmp((void *)0x466ce0, e0, sizeof e0)) { logf_("texto: bytes inesperados em 0x466ce0, menus sem centralizar"); return; }
    BYTE *s0 = stub_mem(64), *q = s0;
    *q++ = 0x60;                                                          // pushad
    *q++ = 0x8d; *q++ = 0x44; *q++ = 0x24; *q++ = 0x20;                   // lea eax, [esp+0x20] (esp original)
    *q++ = 0x50;                                                          // push eax
    *q++ = 0xe8; *(DWORD *)q = (DWORD)txt_hook - (DWORD)(q + 4); q += 4;  // call txt_hook
    *q++ = 0x83; *q++ = 0xc4; *q++ = 0x04; *q++ = 0x61;                   // add esp, 4 / popad
    memcpy(q, e0, sizeof e0); q += sizeof e0;
    put_jmp(q, (void *)0x466ce6);
    write_site(0x466ce0, e0, sizeof e0, s0, "texto do jogo");
}
static void install_op14_probe(void) {
    static const BYTE e[] = { 0x8b, 0xfa, 0x83, 0xff, 0x14 };   // mov edi, edx / cmp edi, 0x14
    if (memcmp((void *)0x4673c6, e, sizeof e)) { logf_("op14: bytes inesperados"); return; }
    BYTE *st = stub_mem(64), *p = st;
    *p++ = 0x60;                                                          // pushad
    *p++ = 0x56; *p++ = 0x53; *p++ = 0x52;                                // push esi / push ebx / push edx
    *p++ = 0xe8; *(DWORD *)p = (DWORD)op14_log - (DWORD)(p + 4); p += 4;  // call op14_log
    *p++ = 0x83; *p++ = 0xc4; *p++ = 0x0c;                                // add esp, 12
    *p++ = 0x61;                                                          // popad
    memcpy(p, e, sizeof e); p += sizeof e;                                // instruções originais (a flag do cmp segue)
    p += put_jmp(p, (void *)0x4673cb);
    write_site(0x4673c6, e, sizeof e, st, "op14 (texto das dicas)");
    static const BYTE e2[] = { 0xe9, 0x8e, 0x02, 0x00, 0x00 };          // 0x46769c: jmp 0x46792f (fim do nome da tecla)
    static const BYTE e3[] = { 0xc6, 0x44, 0x24, 0x18, 0x01 };          // 0x4676a1: mov byte [esp+0x18], 1 (ícone do Enter)
    if (!memcmp((void *)0x46769c, e2, 5) && !memcmp((void *)0x4676a1, e3, 5)) {
        BYTE *s2 = stub_mem(64), *q = s2;
        *q++ = 0x60; *q++ = 0x6a; *q++ = 0x00;                                    // pushad / push 0
        *q++ = 0xe8; *(DWORD *)q = (DWORD)op14_out - (DWORD)(q + 4); q += 4;      // call op14_out
        *q++ = 0x83; *q++ = 0xc4; *q++ = 0x04; *q++ = 0x61;                       // add esp, 4 / popad
        put_jmp(q, (void *)0x46792f);
        write_site(0x46769c, e2, 5, s2, "op14 (nome da tecla)");
        BYTE *s3 = stub_mem(64); q = s3;
        *q++ = 0x60; *q++ = 0x6a; *q++ = 0x01;                                    // pushad / push 1
        *q++ = 0xe8; *(DWORD *)q = (DWORD)op14_out - (DWORD)(q + 4); q += 4;      // call op14_out
        *q++ = 0x83; *q++ = 0xc4; *q++ = 0x04; *q++ = 0x61;                       // add esp, 4 / popad
        memcpy(q, e3, 5); q += 5;
        put_jmp(q, (void *)0x4676a6);
        write_site(0x4676a1, e3, 5, s3, "op14 (icone do Enter)");
    } else logf_("op14: bytes inesperados no fim");
}
static void install_pad_prompts(void) {
    if (!cfg.xinput || !cfg.pad_prompts) return;
    BYTE *call = (BYTE *)0x467667;
    static const BYTE e1[] = { 0xe8, 0x74, 0x85, 0xfa, 0xff };      // call 0x40fbe0
    static const BYTE e2[] = { 0x68, 0xe1, 0x5f, 0x4d, 0x00 };      // push 0x4d5fe1 (antes de minúsculas e aspas)
    if (memcmp(call, e1, 5) || memcmp((void *)0x46766c, e2, 5)) { logf_("dicas de botao: bytes inesperados, desligado"); return; }
    DWORD old; VirtualProtect(call, 10, PAGE_EXECUTE_READWRITE, &old);
    *(DWORD *)(call + 1) = (DWORD)kn_text - (DWORD)(call + 5);
    // ícone: sem aspas nem minúsculas
    BYTE *st = stub_mem(64), *p = st;
    *p++ = 0x80; *p++ = 0x3d; *(DWORD *)p = (DWORD)&g_kn_icon; p += 4; *p++ = 0x00;   // cmp byte [g_kn_icon], 0
    BYTE *jb = p; *p++ = 0x74; *p++ = 0;                                      // je normal
    *p++ = 0xa0; *(DWORD *)p = 0x4d5fe1; p += 4;                               // mov al, [0x4d5fe1]
    *p++ = 0xa2; *(DWORD *)p = 0x4d5fe0; p += 4;                               // mov [0x4d5fe0], al
    *p++ = 0xc6; *p++ = 0x05; *(DWORD *)p = 0x4d5fe1; p += 4; *p++ = 0;       // mov byte [0x4d5fe1], 0
    *p++ = 0x83; *p++ = 0xc4; *p++ = 0x08;                                    // add esp, 8 (args do 0x40fbe0)
    p += put_jmp(p, (void *)0x46769c);
    jb[1] = (BYTE)(p - (jb + 2));
    *p++ = 0x68; *(DWORD *)p = 0x4d5fe1; p += 4;                               // push 0x4d5fe1 (original)
    p += put_jmp(p, (void *)0x467671);
    put_jmp((BYTE *)0x46766c, st);
    VirtualProtect(call, 10, old, &old);
    FlushInstructionCache(GetCurrentProcess(), call, 10);
    if (cfg.pad_prompts == 2) {                         // o "START" do PS1 (0x0b) vira o ícone redondo do View: avanço de ícone
        BYTE *adv = (BYTE *)0x4cf370 + pad_icon_char(BTN_BACK);
        if (*adv == 50) { DWORD o2; VirtualProtect(adv, 1, PAGE_READWRITE, &o2); *adv = (BYTE)pad_icon_advance(BTN_BACK); VirtualProtect(adv, 1, o2, &o2); }
        else logf_("dicas de botao: avanco do BACK inesperado (%d), mantido", *adv);
    }
    logf_("dicas de botao: com controle conectado, as teclas viram botoes do Xbox (%s)", cfg.pad_prompts == 2 ? "icones" : "nomes");
}

// debug=1: F9 passa por alcance/LOD do mod e originais (4 combinações); F6 escolhe um tipo de objeto para ficar com
// o alcance original; F11 liga/desliga os efeitos de imagem
static void debug_keys(void) {
    static int prev6, prev7;
    int d6 = (GetAsyncKeyState(VK_F6) & 0x8000) != 0;
    if (d6 && !prev6) { g_rlogic = !g_rlogic; logf_("F6: objetos de logica (cat 10) com o alcance %s", g_rlogic ? "ORIGINAL (padrao)" : "do mod (como antes)"); }
    prev6 = d6;
    int d7 = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
    if (d7 && !prev7) {
        g_rexcl = g_rexcl + 1 >= g_rtypen ? -1 : g_rexcl + 1;
        if (g_rexcl < 0) logf_("F7: todos os tipos com o alcance do mod");
        else logf_("F7: tipo %d (cat %02x id %04x) com o alcance ORIGINAL", g_rexcl, g_rtypes[g_rexcl].cat, g_rtypes[g_rexcl].id);
    }
    prev7 = d7;
    static int prev8;
    int d8 = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    if (d8 && !prev8) { g_sim_pad = (g_sim_pad + 1) % 3; logf_("F8: controle %s", g_sim_pad == 1 ? "DESCONECTADO (simulado)" : g_sim_pad == 2 ? "CONECTADO (simulado)" : "real"); }
    prev8 = d8;
    static int prev11;
    int d11 = (GetAsyncKeyState(VK_F11) & 0x8000) != 0;
    if (d11 && !prev11) { g_fx_bypass = !g_fx_bypass; logf_("F11: efeitos de imagem %s", g_fx_bypass ? "DESLIGADOS" : "ligados"); }
    static int prev10;
    int d10 = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
    if (d10 && !prev10 && cfg.hud_probe && !pr_frame_dump) pr_frame_dump = 1;
    prev10 = d10;
    prev11 = d11;
    static int prev, st;                    // F9 em 4 passos: alcance e LOD separados, para achar qual dos dois causa algo
    int down = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
    if (down && !prev) {
        st = (st + 1) % 4;
        int orig_r = st == 1 || st == 3, orig_l = st == 2 || st == 3;
        g_kr = orig_r ? 256 : cfg.actor_dist * 256 / 100;
        g_klod = orig_l ? 256 : cfg.lod_dist * 256 / 100;
        logf_("F9 (%d): alcance dos objetos %s, LOD %s", st, orig_r ? "ORIGINAL" : "do mod", orig_l ? "ORIGINAL" : "do mod");
    }
    prev = down;
}

// ---------------------------------------------------------------- proxy dinput
static HMODULE g_real_dinput;
static FARPROC real_fn(const char *n) {
    if (!g_real_dinput) {
        char p[MAX_PATH]; GetSystemDirectoryA(p, MAX_PATH); strcat(p, "\\dinput.dll");
        g_real_dinput = LoadLibraryA(p);
        logf_("dinput real: %s -> %p", p, (void *)g_real_dinput);
    }
    return g_real_dinput ? GetProcAddress(g_real_dinput, n) : NULL;
}
HRESULT WINAPI DirectInputCreateA(HINSTANCE h, DWORD v, void **o, void *u) {
    HRESULT (WINAPI *f)(HINSTANCE, DWORD, void **, void *) = (void *)real_fn("DirectInputCreateA");
    HRESULT r = f ? f(h, v, o, u) : E_FAIL;
    logf_("DirectInputCreateA(versao %lx) -> %08lx", v, r);
    if (r == 0 && o && *o) hook_dinput(*o);
    return r;
}
HRESULT WINAPI DirectInputCreateW(HINSTANCE h, DWORD v, void **o, void *u) {
    HRESULT (WINAPI *f)(HINSTANCE, DWORD, void **, void *) = (void *)real_fn("DirectInputCreateW");
    return f ? f(h, v, o, u) : E_FAIL;
}
HRESULT WINAPI DirectInputCreateEx(HINSTANCE h, DWORD v, REFIID r, void **o, void *u) {
    HRESULT (WINAPI *f)(HINSTANCE, DWORD, REFIID, void **, void *) = (void *)real_fn("DirectInputCreateEx");
    return f ? f(h, v, r, o, u) : E_FAIL;
}
HRESULT WINAPI DllCanUnloadNow(void) { return S_FALSE; }
HRESULT WINAPI DllGetClassObject(REFCLSID c, REFIID i, void **o) {
    HRESULT (WINAPI *f)(REFCLSID, REFIID, void **) = (void *)real_fn("DllGetClassObject");
    return f ? f(c, i, o) : E_FAIL;
}
HRESULT WINAPI DllRegisterServer(void) { return E_FAIL; }
HRESULT WINAPI DllUnregisterServer(void) { return E_FAIL; }

// ---------------------------------------------------------------- init
static void init(HMODULE self) {
    GetModuleFileNameA(self, g_dir, MAX_PATH);
    char *s = strrchr(g_dir, '\\'); if (s) *s = 0;
    InitializeCriticalSection(&g_lock);
    load_config();
    char lp[MAX_PATH]; snprintf(lp, sizeof lp, "%s\\bblit2fix.log", g_dir);
    g_log = fopen(lp, "w");
    logf_("BBLIT2 Fix %s", FIX_VERSION);
    if (!cfg.enabled) { logf_("desativado no ini (enabled=0)"); return; }
    host_ok = check_host();
    if (!host_ok) return;

    // DPI: sem isto o Windows com escala >100%% entrega uma janela em resolução menor
    HMODULE u32 = GetModuleHandleA("user32.dll");
    BOOL (WINAPI *dpi)(void) = u32 ? (void *)GetProcAddress(u32, "SetProcessDPIAware") : NULL;
    if (dpi) dpi();

    HMODULE exe = GetModuleHandleA(NULL);
    r_GetProcAddress = hook_iat(exe, "KERNEL32.dll", "GetProcAddress", h_GetProcAddress);
    r_LoadLibraryA   = hook_iat(exe, "KERNEL32.dll", "LoadLibraryA", h_LoadLibraryA);
    r_CreateWindowExA = hook_iat(exe, "USER32.dll", "CreateWindowExA", h_CreateWindowExA);
    r_SetWindowPos = hook_iat(exe, "USER32.dll", "SetWindowPos", h_SetWindowPos);
    r_GetClientRect = hook_iat(exe, "USER32.dll", "GetClientRect", h_GetClientRect);
    r_GetSystemMetrics = hook_iat(exe, "USER32.dll", "GetSystemMetrics", h_GetSystemMetrics);
    r_DestroyWindow = hook_iat(exe, "USER32.dll", "DestroyWindow", h_DestroyWindow);
    r_FillRect = hook_iat(exe, "USER32.dll", "FillRect", h_FillRect);
    r_DrawTextA = hook_iat(exe, "USER32.dll", "DrawTextA", h_DrawTextA);
    r_ReleaseDC = hook_iat(exe, "USER32.dll", "ReleaseDC", h_ReleaseDC);
    r_EndPaint = hook_iat(exe, "USER32.dll", "EndPaint", h_EndPaint);
    r_BeginPaint = hook_iat(exe, "USER32.dll", "BeginPaint", h_BeginPaint);
    r_PeekMessageA = hook_iat(exe, "USER32.dll", "PeekMessageA", h_PeekMessageA);
    r_BitBlt = hook_iat(exe, "GDI32.dll", "BitBlt", h_BitBlt);
    r_StretchDIBits = hook_iat(exe, "GDI32.dll", "StretchDIBits", h_StretchDIBits);
    r_LoadImageA = hook_iat(exe, "USER32.dll", "LoadImageA", h_LoadImageA);
    r_DeleteObject = hook_iat(exe, "GDI32.dll", "DeleteObject", h_DeleteObject);
    if (!r_DeleteObject) r_DeleteObject = DeleteObject;
    r_SelectObject = hook_iat(exe, "GDI32.dll", "SelectObject", h_SelectObject);
    r_SetBkColor = hook_iat(exe, "GDI32.dll", "SetBkColor", h_SetBkColor);
    r_SetTextColor = hook_iat(exe, "GDI32.dll", "SetTextColor", h_SetTextColor);
    r_SetBkMode = hook_iat(exe, "GDI32.dll", "SetBkMode", h_SetBkMode);
    r_SetTextAlign = hook_iat(exe, "GDI32.dll", "SetTextAlign", h_SetTextAlign);
    r_ChangeDisplaySettingsA = hook_iat(exe, "USER32.dll", "ChangeDisplaySettingsA", h_ChangeDisplaySettingsA);
    r_GetKeyboardState = hook_iat(exe, "USER32.dll", "GetKeyboardState", h_GetKeyboardState);
    r_GetAsyncKeyState = hook_iat(exe, "USER32.dll", "GetAsyncKeyState", h_GetAsyncKeyState);
    logf_("ganchos: GPA %p LLA %p CWEx %p CDS %p GKS %p GAKS %p", (void *)r_GetProcAddress, (void *)r_LoadLibraryA,
          (void *)r_CreateWindowExA, (void *)r_ChangeDisplaySettingsA, (void *)r_GetKeyboardState, (void *)r_GetAsyncKeyState);
    r_EnumDisplaySettingsA = hook_iat(exe, "USER32.dll", "EnumDisplaySettingsA", h_EnumDisplaySettingsA);
    r_GetDeviceCaps = hook_iat(exe, "GDI32.dll", "GetDeviceCaps", h_GetDeviceCaps);
    if (!r_EnumDisplaySettingsA) r_EnumDisplaySettingsA = EnumDisplaySettingsA;
    r_GetDriveTypeA = hook_iat(exe, "KERNEL32.dll", "GetDriveTypeA", h_GetDriveTypeA);
    r_GetVolumeInformationA = hook_iat(exe, "KERNEL32.dll", "GetVolumeInformationA", h_GetVolumeInformationA);
    r_CreateFileA = hook_iat(exe, "KERNEL32.dll", "CreateFileA", h_CreateFileA);
    r_ExitProcess = hook_iat(exe, "KERNEL32.dll", "ExitProcess", h_ExitProcess);
    r_MessageBoxA = hook_iat(exe, "USER32.dll", "MessageBoxA", h_MessageBoxA);
    r_FindWindowA = hook_iat(exe, "USER32.dll", "FindWindowA", h_FindWindowA);
    AddVectoredExceptionHandler(1, veh);
    g_main_tid = GetCurrentThreadId();
    if (cfg.debug) CloseHandle(CreateThread(NULL, 0, heartbeat, NULL, 0, NULL));
    if (!r_GetProcAddress) r_GetProcAddress = GetProcAddress;
    if (!r_LoadLibraryA) r_LoadLibraryA = LoadLibraryA;

    if (cfg.unlock_joy)  // jb 0x40f1a0 ("Joystick are not supported in WinNT") -> nops
        patch_bytes(0x40ef48, "\x0f\x82\x52\x02\x00\x00", 6, "\x90\x90\x90\x90\x90\x90");

    r_RegQueryValueExA = hook_iat(exe, "ADVAPI32.dll", "RegQueryValueExA", h_RegQueryValueExA);
    r_RegOpenKeyExA = hook_iat(exe, "ADVAPI32.dll", "RegOpenKeyExA", h_RegOpenKeyExA);
    r_RegCloseKey = hook_iat(exe, "ADVAPI32.dll", "RegCloseKey", h_RegCloseKey);
    if (!r_RegCloseKey) r_RegCloseKey = RegCloseKey;
    // o jogo abre "..\bin\config.pc" etc. relativo à pasta de trabalho: garante que é a pasta bin
    if (SetCurrentDirectoryA(g_dir)) logf_("pasta de trabalho: %s", g_dir);
    native_res();
    install_widescreen();
    install_draw_distance();
    install_object_hooks();
    hd_load_list();
    install_pal_hook();
    install_pad_prompts();
    install_text_hook();
    if (cfg.hud_probe) install_op14_probe();
    load_keymap();
    logf_("config: aspect=%d msaa=%d vsync=%d xinput=%d unlock_dinput_joystick=%d", cfg.aspect, cfg.msaa, cfg.vsync, cfg.xinput, cfg.unlock_joy);
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r) {
    (void)r;
    if (reason == DLL_PROCESS_ATTACH) { DisableThreadLibraryCalls(h); init(h); }
    else if (reason == DLL_PROCESS_DETACH && g_log) { logf_("fim (%ld quadros)", g_frames); fclose(g_log); g_log = NULL; }
    return TRUE;
}
