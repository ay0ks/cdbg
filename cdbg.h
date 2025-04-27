#pragma once

#include <stdio.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>

#define assert($Expression, ...)                                                               \
  ((void)((!!($Expression))                                                                    \
          || (cdbg_assert(__FILE__, __func__, __LINE__, #$Expression, true, ##__VA_ARGS__), 0) \
  ))

#define assert_soft($Expression, ...)                                                           \
  ((void)((!!($Expression))                                                                     \
          || (cdbg_assert(__FILE__, __func__, __LINE__, #$Expression, false, ##__VA_ARGS__), 0) \
  ))

void
cdbg_assert(
  const char *a_file,
  const char *a_function,
  uint64_t a_line,
  const char *a_expression,
  bool a_abort,
  ...
);

void
cdbg_abort();

typedef struct cdbg_dump_lookaround_s
{
  uint8_t m_dummy;
  uint64_t m_lookbehind;
  uint64_t m_lookahead;
} cdbg_dump_lookaround_t;

#define dump($Value, $ValueSize, ...)                                                    \
  ((void)({                                                                              \
    char l_address[48];                                                                  \
    snprintf(l_address, sizeof(l_address), "%p", $Value);                                \
    ((void)(cdbg_dump(                                                                   \
      __FILE__,                                                                          \
      __func__,                                                                          \
      __LINE__,                                                                          \
      l_address,                                                                         \
      (cdbg_dump_lookaround_t){0, ##__VA_ARGS__},                                        \
      #$Value,                                                                           \
      ($ValueSize),                                                                      \
      (char *)($Value)                                                                   \
    )));                                                                                 \
  }))

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
  const char *a_file,
  const char *a_function,
  uint64_t a_line
);

void
cdbg_breakpoint_break(
  cdbg_breakpoint_t *a_breakpoint,
  const char *a_file,
  const char *a_function,
  uint64_t a_line
);

void
cdbg_breakpoint_clear(cdbg_breakpoint_t *a_breakpoint);

#define breakpoint_set($Breakpoint)                                                      \
  (cdbg_breakpoint_set($Breakpoint, __FILE__, __func__, __LINE__))

#define breakpoint_trigger($Breakpoint)                                                  \
  (cdbg_breakpoint_break($Breakpoint, __FILE__, __func__, __LINE__))

#define breakpoint_clear($Breakpoint) (cdbg_breakpoint_clear($Breakpoint))
