
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

#define ID_INPUT  101
#define ID_CALC   102
#define ID_CLEAR  103
#define ID_RESULT 104

HWND hInput, hResult;
HFONT hFont;

/* ---------- Array-based statistical functions ---------- */

double a_sum(const double a[], int n)
{
    double s = 0;
    for (int i = 0; i < n; i++)
        s += a[i];
    return s;
}

double a_mean(const double a[], int n)
{
    return n > 0 ? a_sum(a, n) / n : NAN;
}

double a_min(const double a[], int n)
{
    if (n == 0) return NAN;

    double m = a[0];
    for (int i = 1; i < n; i++)
        if (a[i] < m) m = a[i];

    return m;
}

double a_max(const double a[], int n)
{
    if (n == 0) return NAN;

    double m = a[0];
    for (int i = 1; i < n; i++)
        if (a[i] > m) m = a[i];

    return m;
}

int compare_double(const void *x, const void *y)
{
    double a = *(const double *)x;
    double b = *(const double *)y;
    return (a > b) - (a < b);
}

double a_median(const double a[], int n)
{
    if (n == 0) return NAN;

    double *t = malloc(n * sizeof(double));
    if (!t) return NAN;

    memcpy(t, a, n * sizeof(double));
    qsort(t, n, sizeof(double), compare_double);

    double result;

    if (n % 2)
        result = t[n / 2];
    else
        result = (t[n / 2 - 1] + t[n / 2]) / 2.0;

    free(t);
    return result;
}

double a_mode(const double a[], int n, int *frequency)
{
    if (n == 0) {
        *frequency = 0;
        return NAN;
    }

    double *t = malloc(n * sizeof(double));
    if (!t) {
        *frequency = 0;
        return NAN;
    }

    memcpy(t, a, n * sizeof(double));
    qsort(t, n, sizeof(double), compare_double);

    double mode = t[0];
    int best = 1, count = 1;

    for (int i = 1; i < n; i++) {
        if (t[i] == t[i - 1])
            count++;
        else
            count = 1;

        if (count > best) {
            best = count;
            mode = t[i];
        }
    }

    free(t);
    *frequency = best;
    return mode;
}

double a_variance(const double a[], int n, int sample)
{
    int divisor = sample ? n - 1 : n;
    if (divisor <= 0) return NAN;

    double m = a_mean(a, n);
    double total = 0;

    for (int i = 0; i < n; i++) {
        double d = a[i] - m;
        total += d * d;
    }

    return total / divisor;
}

/* ---------- Variadic API: original project concept ---------- */

double sum(int n, ...)
{
    va_list ap;
    va_start(ap, n);

    double total = 0;
    for (int i = 0; i < n; i++)
        total += va_arg(ap, double);

    va_end(ap);
    return total;
}

double mean(int n, ...)
{
    va_list ap;
    va_start(ap, n);

    double total = 0;
    for (int i = 0; i < n; i++)
        total += va_arg(ap, double);

    va_end(ap);
    return n > 0 ? total / n : NAN;
}

double min_of(int n, ...)
{
    va_list ap;
    va_start(ap, n);

    if (n <= 0) {
        va_end(ap);
        return NAN;
    }

    double m = va_arg(ap, double);

    for (int i = 1; i < n; i++) {
        double v = va_arg(ap, double);
        if (v < m) m = v;
    }

    va_end(ap);
    return m;
}

double max_of(int n, ...)
{
    va_list ap;
    va_start(ap, n);

    if (n <= 0) {
        va_end(ap);
        return NAN;
    }

    double m = va_arg(ap, double);

    for (int i = 1; i < n; i++) {
        double v = va_arg(ap, double);
        if (v > m) m = v;
    }

    va_end(ap);
    return m;
}

/* ---------- GUI helper functions ---------- */

HWND add_control(
    HWND parent, const char *type, const char *text,
    DWORD style, int x, int y, int w, int h, int id)
{
    HWND control = CreateWindowA(
        type, text, WS_CHILD | WS_VISIBLE | style,
        x, y, w, h, parent, (HMENU)(INT_PTR)id,
        GetModuleHandle(NULL), NULL);

    SendMessageA(control, WM_SETFONT,
                 (WPARAM)hFont, TRUE);

    return control;
}

void show_results(HWND parent)
{
    char input[4096];
    GetWindowTextA(hInput, input, sizeof(input));

    double values[512];
    int n = 0;
    char *p = input;

    while (*p) {
        while (*p == ' ' || *p == '\t' ||
               *p == ',' || *p == '\n' || *p == '\r')
            p++;

        if (!*p) break;

        if (n >= 512) {
            MessageBoxA(parent,
                "Please enter no more than 512 numbers.",
                "Input limit", MB_OK | MB_ICONWARNING);
            return;
        }

        char *end;
        double v = strtod(p, &end);

        if (end == p || !isfinite(v)) {
            MessageBoxA(parent,
                "Invalid input. Enter numbers separated by spaces or commas.",
                "Input error", MB_OK | MB_ICONERROR);
            return;
        }

        values[n++] = v;
        p = end;

        if (*p && *p != ' ' && *p != '\t' &&
            *p != ',' && *p != '\n' && *p != '\r') {
            MessageBoxA(parent,
                "Invalid input. Please enter only numbers.",
                "Input error", MB_OK | MB_ICONERROR);
            return;
        }
    }

    if (n == 0) {
        MessageBoxA(parent, "Please enter some numbers.",
                    "No input", MB_OK | MB_ICONWARNING);
        return;
    }

    int frequency;
    double mode = a_mode(values, n, &frequency);
    double mn = a_min(values, n);
    double mx = a_max(values, n);
    double variance = a_variance(values, n, 0);
    double sample_var = a_variance(values, n, 1);

    char output[5000];

    snprintf(output, sizeof(output),
        "       STATISTICAL RESULTS\n"
        "================================\r\n"
        "Number of values : %d\r\n"
        "Sum              : %.4f\r\n"
        "Mean             : %.4f\r\n"
        "Median           : %.4f\r\n"
        "Mode             : %.4f (appears %d time(s))\r\n"
        "Minimum          : %.4f\r\n"
        "Maximum          : %.4f\r\n"
        "Range            : %.4f\r\n"
        "Population Var.  : %.4f\r\n"
        "Sample Variance  : %.4f\r\n"
        "Population SD    : %.4f\r\n"
        "Sample SD        : %.4f\r\n",
        n, a_sum(values, n), a_mean(values, n),
        a_median(values, n), mode, frequency,
        mn, mx, mx - mn, variance, sample_var,
        sqrt(variance), sqrt(sample_var));

    SetWindowTextA(hResult, output);
}

/* ---------- Windows event handler ---------- */

LRESULT CALLBACK WindowProc(
    HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        hFont = CreateFontA(
            18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        add_control(hwnd, "STATIC",
            "DYNAMIC STATISTICAL CALCULATOR",
            SS_CENTER, 20, 15, 640, 35, 0);

        add_control(hwnd, "STATIC",
            "Enter numbers separated by commas or spaces:",
            0, 25, 65, 600, 25, 0);

        hInput = add_control(hwnd, "EDIT",
            "12, 15, 18, 20, 25, 30",
            WS_BORDER | ES_AUTOHSCROLL,
            25, 95, 630, 35, ID_INPUT);

        add_control(hwnd, "BUTTON", "Calculate",
            BS_DEFPUSHBUTTON, 150, 145, 150, 40, ID_CALC);

        add_control(hwnd, "BUTTON", "Clear",
            0, 365, 145, 150, 40, ID_CLEAR);

        add_control(hwnd, "STATIC", "Results",
            0, 25, 205, 600, 25, 0);

        hResult = add_control(hwnd, "EDIT", "",
            WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
            ES_READONLY | WS_VSCROLL,
            25, 235, 630, 300, ID_RESULT);

        return 0;

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_CALC:
            show_results(hwnd);
            return 0;

        case ID_CLEAR:
            SetWindowTextA(hInput, "");
            SetWindowTextA(hResult, "");
            SetFocus(hInput);
            return 0;
        }
        break;

    case WM_DESTROY:
        if (hFont) DeleteObject(hFont);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

/* ---------- Main Windows application ---------- */

int WINAPI WinMain(
    HINSTANCE instance, HINSTANCE previous,
    LPSTR command, int show)
{
    (void)previous;
    (void)command;

    const char CLASS_NAME[] = "StatCalcWindow";

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Could not register window.",
                    "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowA(
        CLASS_NAME, "StatCalc - Statistical Calculator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 710, 610,
        NULL, NULL, instance, NULL);

    if (!hwnd) return 1;

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
