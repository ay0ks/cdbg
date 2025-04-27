#include "cdbg.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>

#if defined(_WIN32) || defined(_WIN64)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <debugapi.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#  define cdbg_fprintf fwprintf_s
#else
#  define cdbg_fprintf fwprintf
#endif

bool g_locale_set = false;

void
cdbg_assert(
  const char *const a_file,
  const char *const a_function,
  uint64_t a_line,
  const char *const a_expression,
  bool a_abort,
  ...
)
{
  if(!g_locale_set) { setlocale(LC_ALL, ""); }
  va_list l_args;
  va_start(l_args, a_abort);
  uint64_t l_file_length = strlen(a_file);
  wchar_t l_file[l_file_length + 1];
  mbstowcs(l_file, a_file, l_file_length);
  l_file[l_file_length] = L'\0';
  uint64_t l_function_length = strlen(a_function);
  wchar_t l_function[l_function_length + 1];
  mbstowcs(l_function, a_function, l_function_length);
  l_function[l_function_length] = L'\0';
  uint64_t l_expression_length = strlen(a_expression);
  wchar_t l_expression[l_expression_length + 1];
  mbstowcs(l_expression, a_expression, l_expression_length);
  l_expression[l_expression_length] = L'\0';
  const wchar_t *const l_message = va_arg(l_args, wchar_t *);
  cdbg_fprintf(
    stderr, L"%s:%llu Assertion failed in %s", l_file, a_line, l_function, l_expression
  );
  if(l_message != NULL) { cdbg_fprintf(stderr, L" (%s)", l_message); }
  cdbg_fprintf(stderr, L"\n");
  va_end(l_args);
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
  const char *a_file,
  const char *a_function,
  uint64_t a_line,
  const char *a_address,
  cdbg_dump_lookaround_t a_lookaround,
  const char *a_value_repr,
  uint64_t a_size,
  const char *a_value
)
{
  if(!g_locale_set) { setlocale(LC_ALL, ""); }
  uint64_t l_file_length = strlen(a_file);
  wchar_t l_file[l_file_length + 1];
  mbstowcs(l_file, a_file, l_file_length);
  l_file[l_file_length] = L'\0';
  uint64_t l_function_length = strlen(a_function);
  wchar_t l_function[l_function_length + 1];
  mbstowcs(l_function, a_function, l_function_length);
  l_function[l_function_length] = L'\0';
  uint64_t l_address_length = strlen(a_address);
  wchar_t l_address[l_address_length + 1];
  mbstowcs(l_address, a_address, l_address_length);
  uint64_t l_address_2 = wcstoll(l_address, NULL, 16);
  l_address[l_address_length] = L'\0';
  uint64_t l_value_repr_length = strlen(a_value_repr);
  wchar_t l_value_repr[l_value_repr_length + 1];
  mbstowcs(l_value_repr, a_value_repr, l_value_repr_length);
  l_value_repr[l_value_repr_length] = L'\0';
  wprintf_s(
    L"%s:%llu %s: %s (at %s)\n", l_file, a_line, l_function, l_value_repr, l_address
  );
  wprintf_s(L"  offset   hex                                              ascii\n");
  a_value -= a_lookaround.m_lookbehind;
  uint64_t l_size = a_size + a_lookaround.m_lookbehind + a_lookaround.m_lookahead;
  for(uint64_t l_i = 0, l_j = 0; l_i <= l_size; l_i += 16)
  {
    wprintf_s(L"\x1b[38;5;7m");
    if(a_lookaround.m_lookbehind > 0 || a_lookaround.m_lookahead > 0)
    {
      if(l_j <= a_lookaround.m_lookbehind || l_j >= a_size + a_lookaround.m_lookahead)
      {
        wprintf_s(L"\x1b[2m\x1b[38;5;8m");
      }
    }
    wprintf_s(L"  %08llu ", l_address_2 + l_i);
    for(uint64_t l_k = 0; l_k < 16; ++l_k)
    {
      if(l_i + l_k >= l_size) { wprintf_s(L"\x1b[7m??\x1b[27m "); }
      else
      {
        uint8_t l_byte = *(a_value + l_i + l_k);
        wprintf_s(L"%02X ", l_byte);
      }
      l_j += 1;
    }
    wprintf_s(L" ");
    for(int64_t l_k = 0; l_k < 16; ++l_k)
    {
      if(l_i + l_k >= l_size) { wprintf_s(L"\x1b[7m?\x1b[27m"); }
      else
      {
        wchar_t l_byte = *(a_value + l_i + l_k);
        if(l_byte < 0x20 || l_byte > 0x7E) { wprintf_s(L"\x1b[7m.\x1b[27m"); }
        else { wprintf_s(L"%c", l_byte); }
      }
    }
    wprintf_s(L"\x1b[0m\n");
  }
}

void
cdbg_breakpoint_set(
  cdbg_breakpoint_t *a_breakpoint,
  const char *const a_file,
  const char *const a_function,
  uint64_t a_line
)
{
  assert(a_breakpoint != NULL);
  if(setjmp(a_breakpoint->m_jump_site.m_buffer) == 0)
  {
    a_breakpoint->m_armed = true;
    uint64_t l_file_length = strlen(a_file);
    wchar_t l_file[l_file_length + 1];
    mbstowcs(l_file, a_file, l_file_length);
    l_file[l_file_length] = L'\0';
    uint64_t l_function_length = strlen(a_function);
    wchar_t l_function[l_function_length + 1];
    mbstowcs(l_function, a_function, l_function_length);
    l_function[l_function_length] = L'\0';
    a_breakpoint->m_set_site.m_file = l_file;
    a_breakpoint->m_set_site.m_function = l_function;
    a_breakpoint->m_set_site.m_line = a_line;
  }
  else
  {
    cdbg_fprintf(
      stderr,
      L"Breakpoint set in %s:%s at %llu triggered in %s:%s at %llu\n",
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
  const char *const a_file,
  const char *const a_function,
  uint64_t a_line
)
{
  assert(a_breakpoint != NULL);
  if(a_breakpoint->m_armed)
  {
    a_breakpoint->m_armed = false;
    uint64_t l_file_length = strlen(a_file);
    wchar_t l_file[l_file_length + 1];
    mbstowcs(l_file, a_file, l_file_length);
    l_file[l_file_length] = L'\0';
    uint64_t l_function_length = strlen(a_function);
    wchar_t l_function[l_function_length + 1];
    mbstowcs(l_function, a_function, l_function_length);
    l_function[l_function_length] = L'\0';
    a_breakpoint->m_set_site.m_file = l_file;
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
