/* Glue functionality for standard C code trying to run on MicroPython
 * Standard C code expects libc to be linked in, and we're not doing that
 * instead, we're providing linkable addresses that forward to MicroPython's
 * exported function table 'mp_fun_table' 
 */

#include <unistd.h>
#include <py/dynruntime.h>
#include "libmad/config.h"

/*
void *malloc(size_t n) {
  void *ptr = m_malloc(n);
  return ptr;
}

// yes yes, the memory's already cleared by m_malloc
void *calloc(size_t n, size_t m) {
  void *ptr = m_malloc(n * m);
  return ptr;
}

void free(void *ptr) {
  m_free(ptr);
}
*/

// mp_obj_print_helper isn't provided by the dynruntime table, so we have to implement it here
void mp_obj_print_helper(const mp_print_t *print, mp_obj_t o_in, mp_print_kind_t kind) {
  mp_cstack_check();
  if (o_in == MP_OBJ_NULL) {
    mp_printf(print, "(nil)");
    return;
  }
  const mp_obj_type_t *type = mp_obj_get_type(o_in);
  if (MP_OBJ_TYPE_HAS_SLOT(type, print)) {
    MP_OBJ_TYPE_GET_SLOT(type, print)(print, o_in, kind);
  } else {
    mp_printf(print, "<%q>", (qstr)type->name);
  }
  
}

#ifndef NDEBUG
void panic(char *);
# ifdef __GNUC__
void __assert_fail(const char *__assertion, const char *__file, unsigned int __line, const char *__function)
# else
void __assert_func(const char *file, int line, const char *func, const char *failedexpr)
# endif
{
  mp_fun_table.raise_msg(
    mp_fun_table.load_global(MP_QSTR_AssertionError),
    "Assertion Failed");
  panic("Assertion Failed");
}
#endif

void *memcpy(void *dst, const void *src, size_t n) {
  return mp_fun_table.memmove_(dst, src, n);
}

void *memmove(void *dest, const void *src, size_t n) {
  return mp_fun_table.memmove_(dest, src, n);
}

void *memset(void *s, int c, size_t n) {
  return mp_fun_table.memset_(s, c, n);
}

int sprintf(char *str, const char *format, ...) {
  // shouldn't use sprintf anyways, but it's annoying not to have a snprintf() to work with
  return 0;
}
