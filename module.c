#include "module.h"
#include "callbacks.h"
#include "libmad/mad.h"

// Sample functions to test functionality
static mp_obj_t hello(mp_obj_t in) {
  const char *world = mp_obj_str_get_str(in);
  mp_printf(&mp_plat_print, "Hello %s\n", world);
  return mp_const_none;
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
// End sample functions

// Start libmad decoder bindings

// This is the type variable for libmad.Decoder object
// instantiated in global context so it's accessible everywhere
mp_obj_full_type_t mp_type_libmad_decoder;

// Implementation of libmad.Decoder

// Constructor
static mp_obj_t mp_make_new_decoder(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args_in) {
  mp_printf(&mp_plat_print, "mp_make_new_decoder(type, n_args=%d, n_kw=%d)\n", n_args, n_kw);

  enum { ARG_input, ARG_output, ARG_error, ARG_header, ARG_filter, ARG_message };
  mp_arg_t allowed_args[] = {
      { MP_QSTR_, MP_ARG_KW_ONLY | MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = mp_const_none} },
      { MP_QSTR_, MP_ARG_KW_ONLY | MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = mp_const_none} },
      { MP_QSTR_, MP_ARG_KW_ONLY | MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = mp_const_none} },
      { MP_QSTR_, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
      { MP_QSTR_, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
      { MP_QSTR_, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
  };
  // must load QSTRs at runtime since we are using dynruntime
  allowed_args[ARG_input].qst = MP_QSTR_input;
  allowed_args[ARG_output].qst = MP_QSTR_output;
  allowed_args[ARG_error].qst = MP_QSTR_error;
  allowed_args[ARG_header].qst = MP_QSTR_header;
  allowed_args[ARG_filter].qst = MP_QSTR_filter;
  allowed_args[ARG_message].qst = MP_QSTR_message;

  // check arguments
  mp_arg_check_num(n_args, n_kw, 0, 6, true);

  mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
  mp_arg_parse_all_kw_array(n_args, n_kw, args_in, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);

  // create the object
  mp_obj_libmad_decoder_t *self = mp_obj_malloc(mp_obj_libmad_decoder_t, type);

  // Store python callbacks in self
  self->py_input_cb = vals[ARG_input].u_obj;
  self->py_output_cb = vals[ARG_output].u_obj;
  self->py_error_cb = vals[ARG_error].u_obj;

    // hand 'self' object to C callbacks, so they can translate to Python callbacks
  mad_decoder_init(&self->decoder, &self,
		   &input_cb, 0 /* header */, 0 /* filter */, &output_cb,
		   &error_cb, 0 /* message */);

  return MP_OBJ_FROM_PTR(self);
}

// Print
static void mp_libmad_decoder_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  (void)kind;  // unused parameter
  mp_obj_libmad_decoder_t *self = MP_OBJ_TO_PTR(self_in);
  mp_printf(print, "<libmad.Decoder() %p>: callbacks = (input: ", self);
  mp_obj_print_helper(print, self->py_input_cb, PRINT_REPR);
  mp_printf(print, ", output: ");
  mp_obj_print_helper(print, self->py_output_cb, PRINT_REPR);
  mp_printf(print, ", error: ");
  mp_obj_print_helper(print, self->py_error_cb, PRINT_REPR);
  mp_printf(print, ")");
}

// mad_decoder_run method
static mp_obj_t mp_libmad_decoder_run(mp_obj_t self_in) {
  mp_obj_libmad_decoder_t *self = MP_OBJ_TO_PTR(self_in);
  mp_printf(&mp_plat_print, "In mp_libmad_decoder_run...\n");
  int result = mad_decoder_run(&self->decoder);
  mp_printf(&mp_plat_print, "mad_decoder_run returned %d\n", result);
  return mp_obj_new_int(result);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_libmad_decoder_run_obj, mp_libmad_decoder_run);

// define a local dictionary table
mp_map_elem_t mod_locals_dict_table[8];
static MP_DEFINE_CONST_DICT(mod_locals_dict, mod_locals_dict_table);
// End Implementation of libmad.Decoder

// Initalize the module
mp_obj_t mpy_init(mp_obj_fun_bc_t *self, size_t n_args, size_t n_kw, mp_obj_t *args) {
  MP_DYNRUNTIME_INIT_ENTRY

  // Initalize the Decoder type
  mp_type_libmad_decoder.base.type = &mp_type_type;
  mp_type_libmad_decoder.flags = MP_TYPE_FLAG_NONE;
  mp_type_libmad_decoder.name = MP_QSTR_Decoder;

  // Set Slots (the 12 default methods an object normally has)
  MP_OBJ_TYPE_SET_SLOT(&mp_type_libmad_decoder, make_new, mp_make_new_decoder, 0);
  MP_OBJ_TYPE_SET_SLOT(&mp_type_libmad_decoder, print, mp_libmad_decoder_print, 1);

  // Add local functions to the type
  mod_locals_dict_table[0] = (mp_map_elem_t){ MP_OBJ_NEW_QSTR(MP_QSTR_run), MP_OBJ_FROM_PTR(&mp_libmad_decoder_run_obj) };
  MP_OBJ_TYPE_SET_SLOT(&mp_type_libmad_decoder, locals_dict, &mod_locals_dict, 2);

  // Make the Decoder type available on the module
  mp_store_global(MP_QSTR_Decoder, MP_OBJ_FROM_PTR(&mp_type_libmad_decoder));

  // Store some constants on the module
  mp_store_global(MP_QSTR_MAD_FLOW_CONTINUE, mp_obj_new_int(MAD_FLOW_CONTINUE));
  mp_store_global(MP_QSTR_MAD_FLOW_STOP, mp_obj_new_int(MAD_FLOW_STOP));
  mp_store_global(MP_QSTR_MAD_FLOW_BREAK, mp_obj_new_int(MAD_FLOW_BREAK));
  mp_store_global(MP_QSTR_MAD_FLOW_IGNORE, mp_obj_new_int(MAD_FLOW_IGNORE));

  // add module-level function calls here
  mp_store_global(MP_QSTR_hello, MP_OBJ_FROM_PTR(&hello_obj));
  mp_store_global(MP_QSTR_set_cb, MP_OBJ_FROM_PTR(&set_cb_obj));
  mp_store_global(MP_QSTR_call_cb, MP_OBJ_FROM_PTR(&call_cb_obj));

  MP_DYNRUNTIME_INIT_EXIT
}
