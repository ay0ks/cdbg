#pragma once

#include <stddef.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>

#define __clog_stringify_helper($Value) #$Value
#define __clog_stringify($Value) __clog_stringify_helper($Value)

#define __clog_assert($Expression, $Abort, ...)                                                                        \
  ((void)({                                                                                                            \
    do {                                                                                                               \
      if(!($Expression))                                                                                               \
      {                                                                                                                \
        clog_assert(                                                                                                   \
          (__FILE__),                                                                                                  \
          (__func__),                                                                                                  \
          (__LINE__),                                                                                                  \
          (__clog_stringify($Expression)),                                                                             \
          ($Abort),                                                                                                    \
          __VA_OPT__(__VA_ARGS__, ) nullptr                                                                            \
        );                                                                                                             \
      }                                                                                                                \
    }                                                                                                                  \
    while(0);                                                                                                          \
  }))

#define assert($Expression, ...) __clog_assert(($Expression), (true), ##__VA_ARGS__)

#define assert_soft($Expression, ...) __clog_assert(($Expression), (false), ##__VA_ARGS__)

#define goto_assert($Label, $Expression, ...)                                                                          \
  ((void)({                                                                                                            \
    do {                                                                                                               \
      if(!($Expression))                                                                                               \
      {                                                                                                                \
        clog_assert(                                                                                                   \
          (__FILE__),                                                                                                  \
          (__func__),                                                                                                  \
          (__LINE__),                                                                                                  \
          (__clog_stringify($Expression)),                                                                             \
          (false),                                                                                                     \
          __VA_OPT__(__VA_ARGS__, ) nullptr                                                                            \
        );                                                                                                             \
        goto *(&&$Label);                                                                                              \
      }                                                                                                                \
    }                                                                                                                  \
    while(0);                                                                                                          \
  }))

void
clog_assert(
  const char *a_file,
  const char *a_function,
  uint64_t a_line,
  const char *a_expression,
  bool a_abort,
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

#define dump($Value, $ValueSize, ...)                                                                                  \
  ((void)({                                                                                                            \
    wchar_t l_address[48];                                                                                             \
    ((void)(swprintf(l_address, sizeof(l_address), L"%p", $Value)));                                                   \
    ((void)(clog_dump(                                                                                                 \
      (__FILE__),                                                                                                      \
      (__func__),                                                                                                      \
      (__LINE__),                                                                                                      \
      (l_address),                                                                                                     \
      (clog_dump_lookaround_t){0, ##__VA_ARGS__},                                                                      \
      (__clog_stringify($Value)),                                                                                      \
      ($ValueSize),                                                                                                    \
      ((char *)($Value))                                                                                               \
    )));                                                                                                               \
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

typedef enum clog_logger_level_e: int32_t
{
  k_TRACE = -2,
  k_DEBUG = -1,
  k_INFORMATION = 0,
  k_WARNING = 1,
  k_ERROR = 2,
  k_FATAL = 3
} clog_logger_level_t;

#define CLOG_LOGGER_ID_CAPACITY 256
#define CLOG_LOGGER_ID_SIZE 32

typedef struct clog_logger_s
{
  char m_id[CLOG_LOGGER_ID_CAPACITY];
  clog_logger_level_t m_level;
} clog_logger_t;

void
clog_logger_new(
  clog_logger_t **a_logger_out,
  const char a_id[CLOG_LOGGER_ID_SIZE],
  clog_logger_level_t a_level
);

void
clog_logger_log(
  clog_logger_t *a_logger,
  clog_logger_level_t a_level,
  const char *a_format,
  ...
);

void
clog_logger_log_extended(
  clog_logger_t *a_logger,
  clog_logger_level_t a_level,
  const char *a_format,
  va_list a_args
);

void
clog_logger_free(clog_logger_t **a_logger);
