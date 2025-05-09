#pragma once

#include <stddef.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>

#define __cdbg_count_helper_2($1, ...) ((typeof($1)[]){ $1, __VA_ARGS__ })
#define __cdbg_count_helper_1(...) \
  ((uint64_t)(sizeof(__VA_ARGS__)/sizeof(*(__VA_ARGS__))))
#define __cdbg_count(...) \
  (0 __VA_OPT__( + __cdbg_count_helper_1(__cdbg_count_helper_2(__VA_ARGS__))))

#define __cdbg_stringify_helper($Value) #$Value
#define __cdbg_stringify_widen_helper($Value) L##$Value
#define __cdbg_stringify($Value) __cdbg_stringify_helper($Value)
#define __cdbg_stringify_widen($Value) __cdbg_stringify_widen_helper($Value)

#define __cdbg_assert($Expression, $Abort, $ArgCount, ...)                     \
  ((void)({                                                                    \
    do {                                                                       \
      if(!($Expression))                                                       \
      {                                                                        \
        cdbg_assert(                                                           \
          (__cdbg_stringify_widen(__FILE__)),                                  \
          (__func__),                                                          \
          (__LINE__),                                                          \
          (__cdbg_stringify_widen(__cdbg_stringify($Expression))),             \
          ($Abort),                                                            \
          ($ArgCount),                                                         \
          ##__VA_ARGS__                                                        \
        );                                                                     \
      }                                                                        \
    }                                                                          \
    while(0);                                                                  \
  }))

#define assert($Expression, ...)                                               \
  __cdbg_assert(($Expression), (true), (false), (__cdbg_count(__VA_ARGS__)), ##__VA_ARGS__)

#define assert_soft($Expression, ...)                                          \
  __cdbg_assert(($Expression), (false), (false), (__cdbg_count(__VA_ARGS__)), ##__VA_ARGS__)

#define goto_assert($Label, $Expression, ...)                                  \
  ((void)({                                                                    \
    do {                                                                       \
      if(!($Expression))                                                       \
      {                                                                        \
        cdbg_assert(                                                           \
          (__cdbg_stringify_widen(__FILE__)),                                  \
          (__func__),                                                          \
          (__LINE__),                                                          \
          (__cdbg_stringify_widen(__cdbg_stringify($Expression))),             \
          (false),                                                             \
          (__cdbg_count(__VA_ARGS__)),                                         \
          ##__VA_ARGS__                                                        \
        );                                                                     \
        goto *(&&$Label);                                                      \
      }                                                                        \
    }                                                                          \
    while(0);                                                                  \
  }))

void
cdbg_assert(
  const wchar_t *a_file,
  const char *a_function,
  uint64_t a_line,
  const wchar_t *a_expression,
  bool a_abort,
  uint64_t a_argc,
  ...
);

void
cdbg_abort();

typedef struct cdbg_dump_lookaround_s
{
  uint8_t m_dummy : 1;
  uint32_t m_lookbehind;
  uint32_t m_lookahead;
} cdbg_dump_lookaround_t;

#define dump($Value, $ValueSize, ...)                                          \
  ((void)({                                                                    \
    wchar_t l_address[48];                                                     \
    ((void)(swprintf(l_address, sizeof(l_address), L"%p", $Value)));           \
    ((void)(cdbg_dump(                                                         \
      (__cdbg_stringify_widen(__FILE__)),                                      \
      (__func__),                                                              \
      (__LINE__),                                                              \
      (l_address),                                                             \
      (cdbg_dump_lookaround_t){0, ##__VA_ARGS__},                              \
      (__cdbg_stringify_widen(__cdbg_stringify($Value))),                      \
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
  char *a_value
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
