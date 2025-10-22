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

#define WRITE_CHAR(c) do { \
        if (pos < cap) out[pos++] = (c); \
        needed++; \
    } while (0)

static void print_signed_integer(char* out, long long val, int zero_pad, int width, size_t* pos_ptr, size_t* cap_ptr,
                                 size_t* needed_ptr)
{
    size_t pos = *pos_ptr;
    size_t cap = *cap_ptr;
    size_t needed = *needed_ptr;

    int negative = 0;
    unsigned long long u;

    if (val < 0)
    {
        negative = 1;
        u = (unsigned long long)(-val);
    }
    else
    {
        u = (unsigned long long)val;
    }

    // produce digits into buf in reverse
    char buf[128];
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
    int digits_len = bi; // number of digit characters

    if (negative)
    {
        if (zero_pad && width > 0)
        {
            // sign first
            WRITE_CHAR('-');
            // zeros padding AFTER sign
            int pad = width - 1 - digits_len;
            while (pad-- > 0)
                WRITE_CHAR('0');
            // write digits
            while (bi > 0)
                WRITE_CHAR(buf[--bi]);
        }
        else
        {
            // space padding BEFORE sign
            int pad = width - 1 - digits_len;
            while (pad-- > 0)
                WRITE_CHAR(' ');
            // sign
            WRITE_CHAR('-');
            // digits
            while (bi > 0)
                WRITE_CHAR(buf[--bi]);
        }
    }
    else // positive
    {
        int pad = width - digits_len;
        while (pad-- > 0)
            WRITE_CHAR(zero_pad ? '0' : ' ');
        while (bi > 0)
            WRITE_CHAR(buf[--bi]);
    }

    *pos_ptr = pos;
    *cap_ptr = cap;
    *needed_ptr = needed;
}

int vsnprintf(char* out, size_t n, const char* fmt, va_list ap)
{
    size_t pos = 0; // written
    size_t needed = 0;
    size_t cap = (n > 0) ? (n - 1) : 0;

    const char* f = fmt;


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

        // optional leading '0' flag, then width digits
        int zero_pad = 0;
        int width = 0;

        // flag: '0' only meaningful when directly before width
        if (*f == '0')
        {
            zero_pad = 1;
            f++;
        }

        // parse width (digits)
        while (*f >= '0' && *f <= '9')
        {
            width = width * 10 + (*f - '0');
            f++;
        }

        if (*f == '\0')
            break;

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
            print_signed_integer(out, v, zero_pad, width, &pos, &cap, &needed);
            f++;
        }
        else if (*f == 'l' && *(f + 1) == 'd')
        {
            long v = va_arg(ap, long);
            print_signed_integer(out, v, zero_pad, width, &pos, &cap, &needed);
            f += 2;
        }
        else if (*f == 'l' && *(f + 1) == 'l' && *(f + 2) == 'd')
        {
            long long v = va_arg(ap, long long);
            print_signed_integer(out, v, zero_pad, width, &pos, &cap, &needed);
            f += 3;
        }
        else
        {
            // unsupported specifier: literally %<char>
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
