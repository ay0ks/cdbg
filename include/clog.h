#pragma once

#include <stddef.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>

#define __clog_count_helper_2($1, ...) ((typeof($1)[]){$1, __VA_ARGS__})
#define __clog_count_helper_1(...)                                             \
  ((uint64_t)(sizeof(__VA_ARGS__) / sizeof(*(__VA_ARGS__))))
#define __clog_count(...)                                                      \
  (0 __VA_OPT__(+__clog_count_helper_1(__clog_count_helper_2(__VA_ARGS__))))

#define __clog_stringify_helper($Value) #$Value
#define __clog_stringify($Value) __clog_stringify_helper($Value)

#define __clog_assert($Expression, $Abort, $ArgCount, ...)                                                          \
  ((void)({                                                                                                         \
    do {                                                                                                            \
      if(!($Expression))                                                                                            \
      {                                                                                                             \
        clog_assert(                                                                                                \
          (__FILE__), (__func__), (__LINE__), (__clog_stringify($Expression)), ($Abort), ($ArgCount), ##__VA_ARGS__ \
        );                                                                                                          \
      }                                                                                                             \
    }                                                                                                               \
    while(0);                                                                                                       \
  }))

#define assert($Expression, ...)                                               \
  __clog_assert(($Expression), (true), (__clog_count(__VA_ARGS__)), ##__VA_ARGS__)

#define assert_soft($Expression, ...)                                          \
  __clog_assert(($Expression), (false), (__clog_count(__VA_ARGS__)), ##__VA_ARGS__)

#define goto_assert($Label, $Expression, ...)                                  \
  ((void)({                                                                    \
    do {                                                                       \
      if(!($Expression))                                                       \
      {                                                                        \
        clog_assert(                                                           \
          (__FILE__),                                                          \
          (__func__),                                                          \
          (__LINE__),                                                          \
          (__clog_stringify($Expression)),                                     \
          (false),                                                             \
          (__clog_count(__VA_ARGS__)),                                         \
          ##__VA_ARGS__                                                        \
        );                                                                     \
        goto *(&&$Label);                                                      \
      }                                                                        \
    }                                                                          \
    while(0);                                                                  \
  }))

void
clog_assert(
  const char *a_file,
  const char *a_function,
  uint64_t a_line,
  const char *a_expression,
  bool a_abort,
  uint64_t a_argc,
  ...
);

void
clog_abort();

typedef struct clog_dump_lookaround_s
{
  uint8_t m_dummy : 1;
  uint32_t m_lookbehind;
  uint32_t m_lookahead;
} clog_dump_lookaround_t;

#define dump($Value, $ValueSize, ...)                                          \
  ((void)({                                                                    \
    wchar_t l_address[48];                                                     \
    ((void)(swprintf(l_address, sizeof(l_address), L"%p", $Value)));           \
    ((void)(clog_dump(                                                         \
      (__FILE__),                                                              \
      (__func__),                                                              \
      (__LINE__),                                                              \
      (l_address),                                                             \
      (clog_dump_lookaround_t){0, ##__VA_ARGS__},                              \
      (__clog_stringify($Value)),                                              \
      ($ValueSize),                                                            \
      ((char *)($Value))                                                       \
    )));                                                                       \
  }))

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
);
