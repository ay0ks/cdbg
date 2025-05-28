#include <clog.h>

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>
#include <ctype.h>

#if defined(_WIN32) || defined(_WIN64)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <io.h>
#elif defined(__linux__)
#  include <unistd.h>
#endif

#define __clog_minmax_helper($Op, $1, $2)                                                                              \
  ((((typeof($1))($1))$Op((typeof($2))($2))) ? ((typeof($1))($1)) : ((typeof($2))($2)))
#define __clog_min($1, $2) __clog_minmax_helper(<, $1, $2)
#define __clog_max($1, $2) __clog_minmax_helper(>, $1, $2)

#define __clog_clamp($1, $2, $3)                                                                                       \
  ((((typeof($2))($2)) < ((typeof($1))($1)))                                                                           \
     ? ((typeof($1))($1))                                                                                              \
     : (((typeof($2))($2)) > ((typeof($3))($3)) ? ((typeof($3))($3)) : ((typeof($2))($2))))

bool g_locale_set = false;

void
clog_assert(
  const char *a_file,
  const char *a_function,
  uint64_t a_line,
  const char *a_expression,
  bool a_abort,
  ...
)
{
  if(!g_locale_set)
  {
    setlocale(LC_ALL, "");
  }
  va_list l_args;
  va_start(l_args, a_abort);
  char *l_message = va_arg(l_args, char *);
  char *l_message_2 = l_message;
  if(l_message != NULL)
  {
    va_list l_args_2;
    va_copy(l_args_2, l_args);
    uint64_t l_message_size = vsnprintf(nullptr, 0, l_message, l_args_2) + 1;
    va_end(l_args_2);
    l_message_2 = malloc(l_message_size);
    assert(l_message_2 != NULL);
    vsnprintf(l_message_2, l_message_size, l_message, l_args);
  }
  va_end(l_args);
  fprintf(stderr, "%s:%s:%lu: assertion failed\n  expression: %s", a_file, a_function, a_line, a_expression);
  if(l_message != NULL)
  {
    fprintf(stderr, "\n  reason: %s", l_message_2);
    if(l_message_2 != l_message)
    {
      free(l_message_2);
    }
  }
  fprintf(stderr, "\n");
  if(a_abort)
  {
    clog_abort();
  }
}

void
clog_abort()
{
#if defined(_WIN32) || defined(_WIN64)
  DebugBreak();
#elif defined(__linux__)
  raise(SIGTRAP);
#endif
  raise(SIGABRT);
}

void
clog_dump(
  const char *a_file,
  const char *a_function,
  uint64_t a_line,
  const char *a_address,
  clog_dump_lookaround_t a_lookaround,
  const char *a_value_repr,
  uint64_t a_size,
  char *a_value
)
{
  if(!g_locale_set)
  {
    setlocale(LC_ALL, "");
  }
  uint64_t l_size_total = a_size + a_lookaround.m_lookbehind + a_lookaround.m_lookahead;
  char *l_value = a_value - a_lookaround.m_lookbehind;
  bool l_tty = (bool)isatty(fileno(stderr));
  fprintf(
    stderr,
    "%s:%s:%lu: dump of %s at %s (%lu <-%u %u-> %lu)\n",
    a_file,
    a_function,
    a_line,
    a_value_repr,
    a_address,
    a_size,
    a_lookaround.m_lookbehind,
    a_lookaround.m_lookahead,
    l_size_total
  );
  fprintf(stderr, "  offset  hex                                              ascii\n");
  for(uint64_t l_u = 0; l_u < l_size_total; l_u += 16)
  {
    fprintf(stderr, "  ");
    if(a_lookaround.m_lookbehind > 0 && l_u < a_lookaround.m_lookbehind)
    {
      fprintf(stderr, "%06lx", __clog_clamp(0, a_lookaround.m_lookbehind - l_u, a_lookaround.m_lookbehind));
    }
    else if(a_lookaround.m_lookbehind > 0
            && a_lookaround.m_lookahead > 0
            && l_u > a_size + a_lookaround.m_lookbehind
            && l_u <= a_size + a_lookaround.m_lookbehind + a_lookaround.m_lookahead)
    {
      fprintf(stderr, "%06lx", __clog_clamp(0, l_u - a_lookaround.m_lookbehind, a_size + a_lookaround.m_lookahead));
    }
    else
    {
      fprintf(stderr, "%06lx", __clog_clamp(0, l_u - a_lookaround.m_lookbehind, a_size));
    }
    fprintf(stderr, "  ");
    for(uint8_t l_i = 0; l_i < 16; l_i += 1)
    {
      uint64_t l_k = l_u + l_i;
      if(l_k >= a_size + a_lookaround.m_lookbehind + a_lookaround.m_lookahead)
      {
        fprintf(stderr, l_tty ? "\x1b[7m??\x1b[27m " : "?? ");
      }
      else
      {
        uint8_t l_byte = *(l_value + l_k);
        if(l_tty)
        {
          fprintf(
            stderr,
            (l_k >= a_lookaround.m_lookbehind && l_k < a_size + a_lookaround.m_lookbehind) ? "\x1b[1m" : "\x1b[2m"
          );
        }
        fprintf(stderr, "%02X ", l_byte);
        if(l_tty)
        {
          fprintf(stderr, "\x1b[0m");
        }
      }
    }
    fprintf(stderr, " ");
    for(uint8_t l_i = 0; l_i < 16; l_i += 1)
    {
      uint64_t l_k = l_u + l_i;
      if(l_k >= a_size + a_lookaround.m_lookbehind + a_lookaround.m_lookahead)
      {
        fprintf(stderr, l_tty ? "\x1b[7m?\x1b[27m" : "?");
      }
      else
      {
        uint8_t l_byte = *(l_value + l_k);
        if(l_tty)
        {
          fprintf(
            stderr,
            (l_k >= a_lookaround.m_lookbehind && l_k < a_size + a_lookaround.m_lookbehind) ? "\x1b[1m" : "\x1b[2m"
          );
        }
        fprintf(stderr, "%c", isprint(l_byte) ? l_byte : '.');
        if(l_tty)
        {
          fprintf(stderr, "\x1b[0m");
        }
      }
    }
    fprintf(stderr, "\n");
  }
}
