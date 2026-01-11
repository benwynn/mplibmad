#include <py/dynruntime.h>
#include "libmad/mad.h"

// This is the instance data for a libmad.Decoder object
typedef struct {
  // every type starts with a base...
  mp_obj_base_t base;

  // add libmad's decoder struct
  struct mad_decoder decoder;

  // then add any additional fields you need
  mp_obj_t py_input_cb;
  mp_obj_t py_output_cb;
  mp_obj_t py_error_cb;

} mp_obj_libmad_decoder_t;
