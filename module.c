#include "py/dynruntime.h"
#include "libmad/mad.h"

static mp_obj_t hello(mp_obj_t in) {
  const char *world = mp_obj_str_get_str(in);
  mp_printf(&mp_plat_print, "Hello %s", world);
  return MP_OBJ_NULL;
}
static MP_DEFINE_CONST_FUN_OBJ_1(hello_obj, hello);

mp_obj_t global_callback = MP_OBJ_NULL;
static mp_obj_t set_cb(mp_obj_t callback) {
  global_callback = callback;
  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(set_cb_obj, set_cb);

static mp_obj_t call_cb(mp_obj_t data) {
  if (global_callback == MP_OBJ_NULL) {
    mp_raise_ValueError("callback not set");
  }
  mp_obj_t args[1];
  args[0] = data;
  mp_obj_t result = mp_call_function_n_kw(global_callback, 1, 0, args);
  return result;
}
static MP_DEFINE_CONST_FUN_OBJ_1(call_cb_obj, call_cb);

struct mp_type_libmad_t {
    mp_obj_base_t base;
    // add any fields you need here
    mp_obj_t py_input_cb;
    mp_obj_t py_output_cb;
    mp_obj_t py_error_cb;
};

static enum mad_flow input_cb(void *data, struct mad_stream *stream) {
  struct mp_type_libmad_t *self = (struct mp_type_libmad_t *)data;
  mp_obj_t args[1];
  args[0] = MP_OBJ_FROM_PTR(stream);

  mp_obj_t result = mp_call_function_n_kw(self->py_input_cb, 1, 0, args);
  int flow = mp_obj_get_int(result);
  return (enum mad_flow)flow;
}

static enum mad_flow output_cb(void *data, struct mad_header const *header, struct mad_pcm *pcm)
{
  struct mp_type_libmad_t *self = (struct mp_type_libmad_t *)data;
  mp_obj_t args[2];
  args[0] = MP_OBJ_FROM_PTR(header);
  args[1] = MP_OBJ_FROM_PTR(pcm);
  mp_obj_t result = mp_call_function_n_kw(self->py_output_cb, 2, 0, args);
  int flow = mp_obj_get_int(result);
  return (enum mad_flow)flow;
}

static enum mad_flow error_cb(void *data, struct mad_stream *stream, struct mad_frame *frame)
{
  struct mp_type_libmad_t *self = (struct mp_type_libmad_t *)data;
  mp_obj_t args[2];
  args[0] = MP_OBJ_FROM_PTR(stream);
  args[1] = MP_OBJ_FROM_PTR(frame);
  mp_obj_t result = mp_call_function_n_kw(self->py_error_cb, 2, 0, args);
  int flow = mp_obj_get_int(result);
  return (enum mad_flow)flow;
}

struct buffer {
  unsigned char const *start;
  unsigned long length;
};

static mp_obj_t mpy_mad_decoder_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  // The allowed_args array must have its qstr's populated at runtime.
  enum { ARG_input, ARG_output, ARG_error, ARG_header, ARG_filter, ARG_message };
  mp_arg_t allowed_args[] = {
      { MP_QSTR_, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
      { MP_QSTR_, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
      { MP_QSTR_, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
      { MP_QSTR_, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
      { MP_QSTR_, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
      { MP_QSTR_, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
  };
  allowed_args[0].qst = MP_QSTR_input;
  allowed_args[1].qst = MP_QSTR_output;
  allowed_args[2].qst = MP_QSTR_error;
  allowed_args[3].qst = MP_QSTR_header;
  allowed_args[4].qst = MP_QSTR_filter;
  allowed_args[5].qst = MP_QSTR_message;

  mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
  mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

  // BTREEINFO openinfo = {0};
  // openinfo.flags = args[ARG_flags].u_int;
  // openinfo.cachesize = args[ARG_cachesize].u_int;
  // openinfo.psize = args[ARG_pagesize].u_int;
  // openinfo.minkeypage = args[ARG_minkeypage].u_int;
  // DB *db = __bt_open(MP_OBJ_TO_PTR(pos_args[0]), &btree_stream_fvtable, &openinfo, 0);
  // if (db == NULL) {
  //     mp_raise_OSError(native_errno);
  // }

  // return MP_OBJ_FROM_PTR(btree_new(db, pos_args[0]));

  struct buffer buffer4k;
  struct mad_decoder decoder;

  (void)input_cb;
  (void)output_cb;
  (void)error_cb;

  mad_decoder_init(&decoder, &buffer4k,
		   &input_cb, 0 /* header */, 0 /* filter */, &output_cb,
		   &error_cb, 0 /* message */);


  return mp_obj_new_int(45);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mad_decoder_init_obj, 2, mpy_mad_decoder_init);

mp_obj_t mpy_init(mp_obj_fun_bc_t *self, size_t n_args, size_t n_kw, mp_obj_t *args) {
  MP_DYNRUNTIME_INIT_ENTRY

  // add function calls here
  mp_store_global(MP_QSTR_hello, MP_OBJ_FROM_PTR(&hello_obj));
  mp_store_global(MP_QSTR_set_cb, MP_OBJ_FROM_PTR(&set_cb_obj));
  mp_store_global(MP_QSTR_call_cb, MP_OBJ_FROM_PTR(&call_cb_obj));
  mp_store_global(MP_QSTR_mad_decoder_init, MP_OBJ_FROM_PTR(&mad_decoder_init_obj));

  MP_DYNRUNTIME_INIT_EXIT
}
