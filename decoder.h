/*
 * Rewritten extensively for MicroPython, original:
 *
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 */

#ifndef LIBMAD_DECODER_H
#define LIBMAD_DECODER_H

#include <py/dynruntime.h>
#include "libmad/mad.h"

#define MP3_BUF_SIZE 4096
#define MP3_FRAME_SIZE 2881

enum mad_flow {
  MAD_FLOW_CONTINUE = 0x0000,	/* continue normally */
  MAD_FLOW_STOP     = 0x0010,	/* stop decoding normally */
  MAD_FLOW_BREAK    = 0x0011,	/* stop decoding and signal an error */
  MAD_FLOW_IGNORE   = 0x0020	/* ignore the current frame */
};

#define mad_decoder_options(decoder, opts)  \
    ((void) ((decoder)->options = (opts)))

// This is the instance data for a libmad.Decoder object
typedef struct {
  // every type starts with a base...
  mp_obj_base_t base;

  // add libmad's decoder struct (stream options?)
  int options;

  bool running;
  
  struct mad_stream stream;
  struct mad_frame frame;
  struct mad_synth synth;

  // internal mp3 buffer will be used by the stream object
  unsigned char mp3buf[MP3_BUF_SIZE];
  int16_t samples[2][1152];

  // add addional data for MicroPython callbacks
  mp_obj_t cb_data;
  mp_obj_t py_input_cb; // enum mad_flow input(data, stream)
  mp_obj_t py_header_cb; // enum mad_flow header(data, header)
  mp_obj_t py_filter_cb; // enum mad_flow filter(data, stream, frame)
  mp_obj_t py_output_cb; // enum mad_flow ouptut(data, header, pcm)
  mp_obj_t py_error_cb;  // enum mad_flow error(data, stream, frame)

  mp_obj_t data_left;
  mp_obj_t data_right;
  
} mp_obj_libmad_decoder_t;

int mad_decoder_run(mp_obj_libmad_decoder_t *);

#endif
