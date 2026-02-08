#include "module.h"

#define DEBUG (0)

// Start libmad decoder bindings

// This is the type variable for libmad.Decoder object
// instantiated in global context so it's accessible everywhere
mp_obj_full_type_t mp_type_libmad_decoder;

// Implementation of libmad.Decoder

// Slot: make_new
static mp_obj_t mp_make_new_decoder(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args_in) {
  mp_printf(&mp_plat_print, "mp_make_new_decoder(type, n_args=%d, n_kw=%d)\n", n_args, n_kw);

  enum { ARG_cb_data, ARG_input, ARG_header, ARG_filter, ARG_output, ARG_error };
  mp_arg_t allowed_args[] = {
      { MP_QSTR_ /* cb_data   */, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none } },
      { MP_QSTR_ /* input     */, MP_ARG_KW_ONLY | MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_ /* header    */, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_ /* filter    */, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_ /* output    */, MP_ARG_KW_ONLY | MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = MP_OBJ_NULL} },
      { MP_QSTR_ /* error     */, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
  };
  // must load QSTRs at runtime since we are using dynruntime
  allowed_args[ARG_cb_data].qst = MP_QSTR_cb_data;
  allowed_args[ARG_input].qst = MP_QSTR_input;
  allowed_args[ARG_header].qst = MP_QSTR_header;
  allowed_args[ARG_filter].qst = MP_QSTR_filter;
  allowed_args[ARG_output].qst = MP_QSTR_output;
  allowed_args[ARG_error].qst = MP_QSTR_error;

  // check arguments
  mp_arg_check_num(n_args, n_kw, 0, 6, true);

  mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
  mp_arg_parse_all_kw_array(n_args, n_kw, args_in, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);

  // create the object
  mp_obj_libmad_decoder_t *self = mp_obj_malloc(mp_obj_libmad_decoder_t, type);

  // Store python callbacks
  self->cb_data      = vals[ARG_cb_data].u_obj;
  self->py_input_cb  = vals[ARG_input].u_obj;
  self->py_header_cb = vals[ARG_header].u_obj;
  self->py_filter_cb = vals[ARG_filter].u_obj;
  self->py_output_cb = vals[ARG_output].u_obj;
  self->py_error_cb  = vals[ARG_error].u_obj;

  // hand 'self' object to C callbacks, so they can translate to Python callbacks
  self->options = 0;

  self->running = false;

  return MP_OBJ_FROM_PTR(self);
}

// Slot: print
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
  int result = 33; // debug return
  mp_obj_libmad_decoder_t *self = MP_OBJ_TO_PTR(self_in);
  self->running = true;
  mp_printf(&mp_plat_print, "\n\nCalling mad_decoder_run(%p)...\n", self);
  result = mad_decoder_run(self);
  mp_printf(&mp_plat_print, "mad_decoder_run returned %d\n", result);
  self->running = false;
  return mp_obj_new_int(result);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_libmad_decoder_run_obj, mp_libmad_decoder_run);

// Stream methods:
// stream_buffer
static mp_obj_t stream_buffer(mp_obj_t self_in, mp_obj_t data_in, mp_obj_t len_in) {
  mp_obj_libmad_decoder_t *self = MP_OBJ_TO_PTR(self_in);
  int len = mp_obj_get_int(len_in);

  #if 0
    mp_printf(&mp_plat_print, "In stream_buffer(data=");
    mp_obj_print_helper(&mp_plat_print, data_in, PRINT_REPR);
    mp_printf(&mp_plat_print, ", length=");
    mp_obj_print_helper(&mp_plat_print, len_in, PRINT_REPR);
    mp_printf(&mp_plat_print, "\n");
  #endif

  mp_buffer_info_t bufinfo;
  if (mp_get_buffer(data_in, &bufinfo, MP_BUFFER_RW)) {
    // bufinfo.len may be larger, but only use 'len' bytes of it
    mad_stream_buffer(&self->stream, bufinfo.buf, len);
  } else {
    mp_raise_TypeError("expected a writable buffer");
  }

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(stream_buffer_obj, stream_buffer);

// stream->pcm
static mp_obj_t get_pcm(mp_obj_t self_in) {
  mp_obj_libmad_decoder_t *self = MP_OBJ_TO_PTR(self_in);

  if (self->running == false) {
    return mp_const_none;
  }

  struct mad_pcm *pcm = &self->synth.pcm;

  // return a dictionary with some PCM fields
  mp_obj_t dict = mp_obj_new_dict(5);
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_channels), mp_obj_new_int(pcm->channels));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_samplerate), mp_obj_new_int(pcm->samplerate));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_length), mp_obj_new_int(pcm->length));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_bits_per_sample), mp_obj_new_int(pcm->bits_per_sample));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_samples_per_channel), mp_obj_new_int(pcm->samples_per_channel));

  return dict;
}
static MP_DEFINE_CONST_FUN_OBJ_1(get_pcm_obj, get_pcm);

// Frame methods:
// frame->header 
static mp_obj_t get_frame_header(mp_obj_t self_in) {
  mp_obj_libmad_decoder_t *self = MP_OBJ_TO_PTR(self_in);

  if (self->running == false) {
    return mp_const_none;
  }

  struct mad_header *header = &self->frame.header;

  // return a dictionary with some header fields
  mp_obj_t dict = mp_obj_new_dict(5);
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_layer), mp_obj_new_int(header->layer));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_mode), mp_obj_new_int(header->mode));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_mode_extension), mp_obj_new_int(header->mode_extension));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_emphasis), mp_obj_new_int(header->emphasis));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_bitrate), mp_obj_new_int(header->bitrate));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_samplerate), mp_obj_new_int(header->samplerate));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_crc_check), mp_obj_new_int(header->crc_check));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_crc_target), mp_obj_new_int(header->crc_target));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_flags), mp_obj_new_int(header->flags));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_private_bits), mp_obj_new_int(header->private_bits));

  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_duration_seconds), mp_obj_new_int(mad_timer_count(header->duration, MAD_UNITS_SECONDS)));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_duration_milliseconds), mp_obj_new_int(mad_timer_count(header->duration, MAD_UNITS_MILLISECONDS)));
  
  return dict;
}
static MP_DEFINE_CONST_FUN_OBJ_1(get_frame_header_obj, get_frame_header);


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
  mod_locals_dict_table[1] = (mp_map_elem_t){ MP_OBJ_NEW_QSTR(MP_QSTR_stream_buffer), MP_OBJ_FROM_PTR(&stream_buffer_obj) };
  mod_locals_dict_table[2] = (mp_map_elem_t){ MP_OBJ_NEW_QSTR(MP_QSTR_get_frame_header), MP_OBJ_FROM_PTR(&get_frame_header_obj) };
  MP_OBJ_TYPE_SET_SLOT(&mp_type_libmad_decoder, locals_dict, &mod_locals_dict, 2);

  // Make the Decoder type available on the module
  mp_store_global(MP_QSTR_Decoder, MP_OBJ_FROM_PTR(&mp_type_libmad_decoder));

  // Store some constants on the module
  mp_store_global(MP_QSTR_MAD_FLOW_CONTINUE, mp_obj_new_int(MAD_FLOW_CONTINUE));
  mp_store_global(MP_QSTR_MAD_FLOW_STOP, mp_obj_new_int(MAD_FLOW_STOP));
  mp_store_global(MP_QSTR_MAD_FLOW_BREAK, mp_obj_new_int(MAD_FLOW_BREAK));
  mp_store_global(MP_QSTR_MAD_FLOW_IGNORE, mp_obj_new_int(MAD_FLOW_IGNORE));

  // add module-level function calls here
  //mp_store_global(MP_QSTR_hello, MP_OBJ_FROM_PTR(&hello_obj));

  MP_DYNRUNTIME_INIT_EXIT
}
