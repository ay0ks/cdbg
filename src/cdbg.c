#include <cdbg.h>

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
#  include <debugapi.h>
#  include <io.h>
#elif defined(__linux__)
#  include <unistd.h>
#endif

#define __cdbg_minmax_helper($Op, $1, $2)                                      \
  ((((typeof($1))($1))$Op((typeof($2))($2))) ? ((typeof($1))($1)) : ((typeof($2))($2)))
#define __cdbg_min($1, $2) __cdbg_minmax_helper(<, $1, $2)
#define __cdbg_max($1, $2) __cdbg_minmax_helper(>, $1, $2)

#define __cdbg_clamp($1, $2, $3)                                               \
  ((((typeof($2))($2)) < ((typeof($1))($1)))                                   \
    ? ((typeof($1))($1))                                                       \
    : (((typeof($2))($2)) > ((typeof($3))($3))                                 \
      ? ((typeof($3))($3)) : ((typeof($2))($2))))

bool g_locale_set = false;

void
cdbg_assert(
  const wchar_t *a_file,
  const char *a_function,
  uint64_t a_line,
  const wchar_t *a_expression,
  bool a_abort,
  uint64_t a_argc,
  ...
)
{
  if(!g_locale_set) { setlocale(LC_ALL, ""); }
  uint64_t l_function_length = strlen(a_function);
  wchar_t l_function[l_function_length + 1];
  mbstate_t l_state;
  mbsrtowcs(l_function, &a_function, l_function_length, &l_state);
  l_function[l_function_length] = L'\0';
  va_list l_args;
  va_start(l_args, a_abort);
  wchar_t *l_message = nullptr, *l_message_2 = nullptr;
  if(a_argc > 0) { l_message = va_arg(l_args, wchar_t *); }
  if(a_argc > 1) {
    va_list l_args_2;
    va_copy(l_args_2, l_args);
    uint64_t l_message_size = vswprintf(nullptr, 0, l_message, l_args_2);
    va_end(l_args_2);
    l_message_2 = malloc((l_message_size + 1) * sizeof(wchar_t));
    assert(l_message_2 != NULL);
    vswprintf(l_message_2, l_message_size, l_message, l_args);
  }
  else { l_message_2 = l_message; }
  va_end(l_args);
  fwprintf(
    stderr, L"%ls:%ls:%llu: assertion failed\n  expression: %ls", a_file, l_function, a_line, a_expression
  );
  if(l_message != nullptr) { fwprintf(stderr, L"\n  reason: %ls", l_message_2); }
  fwprintf(stderr, L"\n");
  if(a_abort) { cdbg_abort(); }
}

void
cdbg_abort()
{
#if defined(_WIN32) || defined(_WIN64)
  DebugBreak();
#elif defined(__linux__)
  raise(SIGTRAP);
#endif
  raise(SIGABRT);
}

void
cdbg_dump(
  const wchar_t *a_file,
  const char *a_function,
  uint64_t a_line,
  const wchar_t *a_address,
  cdbg_dump_lookaround_t a_lookaround,
  const wchar_t *a_value_repr,
  uint64_t a_size,
  char *a_value
)
{
  if(!g_locale_set) { setlocale(LC_ALL, ""); }
  uint64_t l_function_length = strlen(a_function);
  wchar_t l_function[l_function_length + 1];
  wmemset(l_function, L'\0', l_function_length + 1);
  mbstate_t l_state;
  memset(&l_state, 0, sizeof(l_state));
  mbsrtowcs(l_function, &a_function, l_function_length, &l_state);
  l_function[l_function_length] = L'\0';
  uint64_t l_size_total = a_size + a_lookaround.m_lookbehind + a_lookaround.m_lookahead;
  char *l_value = a_value - a_lookaround.m_lookbehind;
  bool l_tty = (bool)isatty(fileno(stderr));
  fwprintf(
    stderr,
    L"%ls:%ls:%llu: dump of %ls at %ls (%llu <-%llu %llu-> %llu)\n",
    a_file,
    l_function,
    a_line,
    a_value_repr,
    a_address,
    a_size,
    a_lookaround.m_lookbehind,
    a_lookaround.m_lookahead,
    l_size_total
  );
  fwprintf(
    stderr, L"  offset  hex                                              ascii\n"
  );
  for(uint64_t l_u = 0; l_u < l_size_total; l_u += 16)
  {
    fwprintf(stderr, L"  ");
    if(a_lookaround.m_lookbehind > 0 && l_u < a_lookaround.m_lookbehind)
    {
      fwprintf(
        stderr,
        L"%06x",
        __cdbg_clamp(0, a_lookaround.m_lookbehind - l_u, a_lookaround.m_lookbehind)
      );
    }
    else if(a_lookaround.m_lookbehind > 0
            && a_lookaround.m_lookahead > 0
            && l_u > a_size + a_lookaround.m_lookbehind
            && l_u <= a_size + a_lookaround.m_lookbehind + a_lookaround.m_lookahead)
    {
      fwprintf(
        stderr,
        L"%06x",
        __cdbg_clamp(0, l_u - a_lookaround.m_lookbehind, a_size + a_lookaround.m_lookahead)
      );
    }
    else
    {
      fwprintf(
        stderr,
        L"%06x",
        __cdbg_clamp(0, l_u - a_lookaround.m_lookbehind, a_size)
      );
    }
    fwprintf(stderr, L"  ");
    for(uint8_t l_i = 0; l_i < 16; l_i += 1)
    {
      uint64_t l_k = l_u + l_i;
      if(l_k >= a_size + a_lookaround.m_lookbehind + a_lookaround.m_lookahead)
      {
        fwprintf(stderr, l_tty ? L"\x1b[7m??\x1b[27m " : L"?? ");
      }
      else
      {
        uint8_t l_byte = *(l_value + l_k);
        if(l_tty)
        {
          fwprintf(
            stderr,
            (l_k >= a_lookaround.m_lookbehind && l_k < a_size + a_lookaround.m_lookbehind)
              ? L"\x1b[1m"
              : L"\x1b[2m"
          );
        }
        fwprintf(stderr, L"%02X ", l_byte);
        if(l_tty) { fwprintf(stderr, L"\x1b[0m"); }
      }
    }
    fwprintf(stderr, L" ");
    for(uint8_t l_i = 0; l_i < 16; l_i += 1)
    {
      uint64_t l_k = l_u + l_i;
      if(l_k >= a_size + a_lookaround.m_lookbehind + a_lookaround.m_lookahead)
      {
        fwprintf(stderr, l_tty ? L"\x1b[7m?\x1b[27m" : L"?");
      }
      else
      {
        uint8_t l_byte = *(l_value + l_k);
        if(l_tty)
        {
          fwprintf(
            stderr,
            (l_k >= a_lookaround.m_lookbehind && l_k < a_size + a_lookaround.m_lookbehind)
              ? L"\x1b[1m"
              : L"\x1b[2m"
          );
        }
        fwprintf(stderr, L"%c", isprint(l_byte) ? l_byte : '.');
        if(l_tty) { fwprintf(stderr, L"\x1b[0m"); }
      }
    }
    fwprintf(stderr, L"\n");
  }
}

void
cdbg_breakpoint_set(
  cdbg_breakpoint_t *a_breakpoint,
  const wchar_t *a_file,
  const char *a_function,
  uint64_t a_line
)
{
  assert(a_breakpoint != nullptr);
  if(setjmp(a_breakpoint->m_jump_site.m_buffer) == 0)
  {
    a_breakpoint->m_armed = true;
    uint64_t l_function_length = strlen(a_function);
    wchar_t l_function[l_function_length + 1];
    mbstate_t l_state;
    mbsrtowcs(l_function, &a_function, l_function_length, &l_state);
    l_function[l_function_length] = L'\0';
    a_breakpoint->m_set_site.m_file = a_file;
    a_breakpoint->m_set_site.m_function = l_function;
    a_breakpoint->m_set_site.m_line = a_line;
  }
  else
  {
    fwprintf(
      stderr,
      L"%ls:%ls:%llu: breakpoint %ls:%ls:%llu triggered\n",
      a_breakpoint->m_set_site.m_file,
      a_breakpoint->m_set_site.m_function,
      a_breakpoint->m_set_site.m_line,
      a_breakpoint->m_jump_site.m_file,
      a_breakpoint->m_jump_site.m_function,
      a_breakpoint->m_jump_site.m_line
    );
    cdbg_abort();
  }
}

void
cdg_breakpoint_break(
  cdbg_breakpoint_t *a_breakpoint,
  const wchar_t *a_file,
  const char *a_function,
  uint64_t a_line
)
{
  assert(a_breakpoint != nullptr);
  if(!g_locale_set) { setlocale(LC_ALL, ""); }
  if(a_breakpoint->m_armed)
  {
    a_breakpoint->m_armed = false;
    uint64_t l_function_length = strlen(a_function);
    wchar_t l_function[l_function_length + 1];
    mbstate_t l_state;
    mbsrtowcs(l_function, &a_function, l_function_length, &l_state);
    l_function[l_function_length] = L'\0';
    a_breakpoint->m_jump_site.m_file = a_file;
    a_breakpoint->m_jump_site.m_function = l_function;
    a_breakpoint->m_jump_site.m_line = a_line;
    longjmp(a_breakpoint->m_jump_site.m_buffer, 1);
  }
}

void
cdbg_breakpoint_clear(
  cdbg_breakpoint_t *a_breakpoint
)
{
  assert(a_breakpoint != nullptr);
  a_breakpoint->m_armed = false;
  memset(
    &a_breakpoint->m_jump_site.m_buffer, 0, sizeof(a_breakpoint->m_jump_site.m_buffer)
  );
  memset(&a_breakpoint->m_set_site, 0, sizeof(a_breakpoint->m_set_site));
  memset(&a_breakpoint->m_jump_site, 0, sizeof(a_breakpoint->m_jump_site));
}
