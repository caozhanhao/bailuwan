#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char* fmt, ...)
{
    // FIXME
#define BUF_SZ 1024
    char buf[BUF_SZ];
    va_list ap;
    va_start(ap, fmt);
    int needed = vsnprintf(buf, BUF_SZ, fmt, ap);
    va_end(ap);

    if (needed < 0) return needed;

    int to_write = 0;
    if (needed >= BUF_SZ)
        to_write = BUF_SZ - 1;
    else
        to_write = needed;

    for (int i = 0; i < to_write; i++)
        putch(buf[i]);

#undef BUF_SZ
    return to_write;
}

int vsprintf(char* out, const char* fmt, va_list ap)
{
    return vsnprintf(out, (size_t)SIZE_MAX, fmt, ap);
}

// ISO C11:
// http://port70.net/~nsz/c/c11/n1570.html#7.21.6.6
int sprintf(char* out, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(out, (size_t)SIZE_MAX, fmt, ap);
    va_end(ap);
    return r;
}

int snprintf(char* out, size_t n, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(out, n, fmt, ap);
    va_end(ap);
    return r;
}

int vsnprintf(char* out, size_t n, const char* fmt, va_list ap)
{
    size_t pos = 0; // written
    size_t needed = 0;
    size_t cap = (n > 0) ? (n - 1) : 0;

    const char* f = fmt;

#define WRITE_CHAR(c) do { \
        if (pos < cap) out[pos++] = (c); \
        needed++; \
    } while (0)

    while (*f)
    {
        if (*f != '%')
        {
            WRITE_CHAR(*f++);
            continue;
        }

        f++;
        if (*f == '\0')
            break;

        // %% -> %
        if (*f == '%')
        {
            WRITE_CHAR('%');
            f++;
            continue;
        }

        if (*f == 's')
        {
            const char* s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            while (*s)
                WRITE_CHAR(*s++);
            f++;
        }
        else if (*f == 'd')
        {
            int v = va_arg(ap, int);
            long long val = (long long)v;
            unsigned long long u;
            if (val < 0)
            {
                WRITE_CHAR('-');
                u = (unsigned long long)(-val);
            }
            else
            {
                u = (unsigned long long)val;
            }

            char buf[32];
            int bi = 0;
            if (u == 0)
            {
                buf[bi++] = '0';
            }
            else
            {
                while (u > 0 && bi < (int)sizeof(buf))
                {
                    buf[bi++] = (char)('0' + (u % 10));
                    u /= 10;
                }
            }
            while (bi > 0)
                WRITE_CHAR(buf[--bi]);

            f++;
        }
        else
        {
            WRITE_CHAR('%');
            WRITE_CHAR(*f ? *f : '\0');
            if (*f) f++;
        }
    }

    if (n > 0)
        out[pos] = '\0';

    return (int)needed;
#undef WRITE_CHAR
}

#endif
