#include <cdbg.h>

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>
#include <ctype.h>

#if defined(_WIN32) || defined(_WIN64)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <debugapi.h>
#elif defined(__linux__)
#endif

#define __cdbg_minmax_helper($Op, $1, $2)                                      \
  ((((typeof($1))($1))$Op((typeof($2))($2))) ? ((typeof($1))($1)) : ((typeof($2))($2)))
#define __cdbg_min($1, $2) __cdbg_minmax_helper(<, $1, $2)
#define __cdbg_max($1, $2) __cdbg_minmax_helper(>, $1, $2)

#define __cdbg_clamp($1, $2, $3)                                               \
  ({                                                                           \
    typeof($1) l_min = ($1);                                                   \
    typeof($2) l_value = ($2);                                                 \
    typeof($3) l_max = ($3);                                                   \
    (l_value < l_min) ? l_min : (l_value > l_max ? l_max : l_value);           \
  })

bool g_locale_set = false;

void
cdbg_assert(
  const wchar_t *a_file,
  const char *a_function,
  uint64_t a_line,
  const wchar_t *a_expression,
  bool a_abort,
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
  const wchar_t *l_message = va_arg(l_args, wchar_t *);
  va_end(l_args);
  fwprintf(
    stderr, L"%ls:%ls:%llu: assertion failed\n  expression: %ls", a_file, l_function, a_line, a_expression
  );
  if(l_message != NULL) { fwprintf(stderr, L"\n  reason: %ls", l_message); }
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
    if(a_lookaround.m_lookbehind > 0 && l_u <= a_lookaround.m_lookbehind)
    {
      fwprintf(
        stderr,
        L"%06x",
        __cdbg_clamp(0, a_lookaround.m_lookbehind - l_u, a_lookaround.m_lookbehind)
      );
    }
    else if(a_lookaround.m_lookbehind > 0
            && l_u > a_lookaround.m_lookbehind
            && l_u <= a_size + a_lookaround.m_lookbehind)
    {
      fwprintf(
        stderr,
        L"%06x",
        __cdbg_clamp(0, l_u - a_lookaround.m_lookahead, a_lookaround.m_lookahead)
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
        __cdbg_clamp(0, l_u - a_lookaround.m_lookahead, a_size + a_lookaround.m_lookahead)
      );
    }
    fwprintf(stderr, L"  ");
    for(uint8_t l_i = 0; l_i < 16; l_i += 1)
    {
      uint64_t l_k = l_u + l_i;
      uint8_t l_byte = *(l_value + l_k);
      fwprintf(stderr, L"%02X ", l_byte);
    }
    fwprintf(stderr, L" ");
    for(uint8_t l_i = 0; l_i < 16; l_i += 1)
    {
      uint64_t l_k = l_u + l_i;
      uint8_t l_byte = *(l_value + l_k);
      fwprintf(stderr, L"%c", isprint(l_byte) ? l_byte : '.');
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
  assert(a_breakpoint != NULL);
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
  if(!g_locale_set) { setlocale(LC_ALL, ""); }
  assert(a_breakpoint != NULL);
  if(a_breakpoint->m_armed)
  {
    a_breakpoint->m_armed = false;
    uint64_t l_function_length = strlen(a_function);
    wchar_t l_function[l_function_length + 1];
    mbstate_t l_state;
    mbsrtowcs(l_function, &a_function, l_function_length, &l_state);
    l_function[l_function_length] = L'\0';
    a_breakpoint->m_set_site.m_file = a_file;
    a_breakpoint->m_set_site.m_function = l_function;
    a_breakpoint->m_jump_site.m_line = a_line;
    longjmp(a_breakpoint->m_jump_site.m_buffer, 1);
  }
}

void
cdbg_breakpoint_clear(
  cdbg_breakpoint_t *a_breakpoint
)
{
  assert(a_breakpoint != NULL);
  a_breakpoint->m_armed = false;
  memset(
    &a_breakpoint->m_jump_site.m_buffer, 0, sizeof(a_breakpoint->m_jump_site.m_buffer)
  );
  memset(&a_breakpoint->m_set_site, 0, sizeof(a_breakpoint->m_set_site));
  memset(&a_breakpoint->m_jump_site, 0, sizeof(a_breakpoint->m_jump_site));
}
