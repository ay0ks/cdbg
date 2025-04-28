#pragma once

#include <stdio.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>

#define __cdbg_stringify($Value) #$Value
#define __cdbg_stringify_widen($Value) L##$Value

#define cdbg_stringify($Value) __cdbg_stringify($Value)
#define cdbg_stringify_widen($Value) __cdbg_stringify_widen($Value)

#if defined(_WIN32) || defined(_WIN64)
#  define cdbg_fprintf fwprintf_s
#  define cdbg_sprintf _snwprintf
#  define cdbg_printf wprintf_s
#else
#  define cdbg_fprintf fwprintf
#  define cdbg_sprintf swprintf
#  define cdbg_printf wprintf
#endif

#define assert($Expression, ...)                                                                                                                                    \
  ((void)((!!($Expression))                                                                                                                                         \
          || (cdbg_assert((cdbg_stringify_widen(__FILE__)), (__func__), (__LINE__), (cdbg_stringify_widen(cdbg_stringify($Expression))), true, ##__VA_ARGS__), (0)) \
  ))

#define assert_soft($Expression, ...)                                                                                                                                \
  ((void)((!!($Expression))                                                                                                                                          \
          || (cdbg_assert((cdbg_stringify_widen(__FILE__)), (__func__), (__LINE__), (cdbg_stringify_widen(cdbg_stringify($Expression))), false, ##__VA_ARGS__), (0)) \
  ))

void
cdbg_assert(
  const wchar_t *a_file,
  const char *a_function,
  uint64_t a_line,
  const wchar_t *a_expression,
  bool a_abort,
  ...
);

void
cdbg_abort();

typedef struct cdbg_dump_lookaround_s
{
  uint8_t m_dummy : 1;
  uint64_t m_lookbehind;
  uint64_t m_lookahead;
} cdbg_dump_lookaround_t;

#define dump($Value, $ValueSize, ...)                                          \
  ((void)({                                                                    \
    wchar_t l_address[48];                                                     \
    ((void)(cdbg_sprintf(l_address, sizeof(l_address), L"%p", $Value)));       \
    ((void)(cdbg_dump(                                                         \
      (cdbg_stringify_widen(__FILE__)),                                        \
      (__func__),                                                              \
      (__LINE__),                                                              \
      (l_address),                                                             \
      (cdbg_dump_lookaround_t){0, ##__VA_ARGS__},                              \
      (cdbg_stringify_widen(cdbg_stringify($Value))),                          \
      ($ValueSize),                                                            \
      ((char *)($Value))                                                       \
    )));                                                                       \
  }))

void
cdbg_dump(
  const wchar_t *a_file,
  const char *a_function,
  uint64_t a_line,
  const wchar_t *a_address,
  cdbg_dump_lookaround_t a_lookaround,
  const wchar_t *a_value_repr,
  uint64_t a_size,
  const char *a_value
);

typedef struct cdbg_breakpoint_s
{
  bool m_armed;
  struct
  {
    const wchar_t *m_file;
    const wchar_t *m_function;
    uint64_t m_line;
  } m_set_site;
  struct
  {
    jmp_buf m_buffer;
    const wchar_t *m_file;
    const wchar_t *m_function;
    uint64_t m_line;
  } m_jump_site;
} cdbg_breakpoint_t;

void
cdbg_breakpoint_set(
  cdbg_breakpoint_t *a_breakpoint,
  const wchar_t *a_file,
  const char *a_function,
  uint64_t a_line
);

void
cdbg_breakpoint_break(
  cdbg_breakpoint_t *a_breakpoint,
  const wchar_t *a_file,
  const char *a_function,
  uint64_t a_line
);

void
cdbg_breakpoint_clear(cdbg_breakpoint_t *a_breakpoint);

#define breakpoint_set($Breakpoint)                                            \
  (cdbg_breakpoint_set(($Breakpoint), (cdbg_stringify_wide(__FILE__)), (__func__), (__LINE__)))

#define breakpoint_trigger($Breakpoint)                                        \
  (cdbg_breakpoint_break(($Breakpoint), (cdbg_stringify_wide(__FILE__)), (__func__), (__LINE__)))

#define breakpoint_clear($Breakpoint) (cdbg_breakpoint_clear(($Breakpoint)))
