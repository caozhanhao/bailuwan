#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  char *p = out;
  const char *f = fmt;
  va_list ap;
  va_start(ap, fmt);

  while (*f) {
    if (*f != '%') {
      *p++ = *f++;
      continue;
    }

    f++; // eat '%'
    if (*f == '\0')
      break;

    // handle %%
    if (*f == '%') {
      *p++ = '%';
      f++;
      continue;
    }

    if (*f == 's') {
      const char *s = va_arg(ap, const char *);
      if (!s)
        s = "(null)";
      while (*s)
        *p++ = *s++;
      f++;
    } else if (*f == 'd') {
      int v = va_arg(ap, int);
      long long val = v; // promote to signed long long for safe negation
      unsigned long long u;
      int neg = 0;
      if (val < 0) {
        neg = 1;
        u = (unsigned long long)(-val);
      } else {
        u = (unsigned long long)val;
      }

      // convert u to digits in temp buffer (reverse)
      char buf[32];
      int bi = 0;
      if (u == 0) {
        buf[bi++] = '0';
      } else {
        while (u > 0) {
          buf[bi++] = '0' + (int)(u % 10);
          u /= 10;
        }
      }
      if (neg) *p++ = '-';
      // write digits in correct order
      while (bi > 0) {
        *p++ = buf[--bi];
      }
      f++;
    } else {
      // unsupported format: treat '%' + char as literal (like many simple implementations)
      *p++ = '%';
      *p++ = *f++;
    }
  }

  *p = '\0';
  va_end(ap);
  return (int)(p - out);
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
