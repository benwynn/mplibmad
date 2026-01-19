/*
 * These are generic C level callbacks used by the libmad decoder.
 * They are placeholders that translate the C callback calls into Python,
 * which should return a mad_flow constant available in the module.
 */
#include "libmad/mad.h"
#include "module.h"

enum mad_flow input_cb(void *data, struct mad_stream *stream) {
  mp_obj_libmad_decoder_t *self = (mp_obj_libmad_decoder_t *)data;
  mp_printf(&mp_plat_print, "In input_cb...");

  mp_obj_t args[1];
  args[0] = MP_OBJ_FROM_PTR(stream);

  mp_obj_t result = mp_call_function_n_kw(self->py_input_cb, 1, 0, args);
  int flow = mp_obj_get_int(result);
  
  mp_printf(&mp_plat_print, " returning flow=%d\n", flow);
  return (enum mad_flow)flow;
}

enum mad_flow output_cb(void *data, struct mad_header const *header, struct mad_pcm *pcm)
{
  mp_obj_libmad_decoder_t *self = (mp_obj_libmad_decoder_t *)data;
  mp_printf(&mp_plat_print, "In output_cb...");

  mp_obj_t args[2];
  args[0] = MP_OBJ_FROM_PTR(header);
  args[1] = MP_OBJ_FROM_PTR(pcm);
  mp_obj_t result = mp_call_function_n_kw(self->py_output_cb, 2, 0, args);
  int flow = mp_obj_get_int(result);

  mp_printf(&mp_plat_print, " returning flow=%d\n", flow);
  return (enum mad_flow)flow;
}

enum mad_flow error_cb(void *data, struct mad_stream *stream, struct mad_frame *frame)
{
  mp_obj_libmad_decoder_t *self = (mp_obj_libmad_decoder_t *)data;
  mp_printf(&mp_plat_print, "In error_cb...");

  mp_obj_t args[2];
  args[0] = MP_OBJ_FROM_PTR(stream);
  args[1] = MP_OBJ_FROM_PTR(frame);
  mp_obj_t result = mp_call_function_n_kw(self->py_error_cb, 2, 0, args);
  int flow = mp_obj_get_int(result);

  mp_printf(&mp_plat_print, " returning flow=%d\n", flow);
  return (enum mad_flow)flow;
}