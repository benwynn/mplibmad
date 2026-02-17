/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: decoder.c,v 1.22 2004/01/23 09:41:32 rob Exp $
 */
#include "libmad/mad.h"
#include "decoder.h"


static
enum mad_flow input_cb(mp_obj_libmad_decoder_t *decoder, unsigned char *buf, int room) {
  //mp_printf(&mp_plat_print, "In input_cb(%p)\n", decoder);

  mp_obj_t tmpbuf = mp_obj_new_bytearray_by_ref(room, buf);

  mp_obj_t args[3];
  args[0] = decoder;
  args[1] = decoder->cb_data;
  args[2] = tmpbuf;

  mp_obj_t result = mp_call_function_n_kw(decoder->py_input_cb, 3, 0, args);
  int flow = mp_obj_get_int(result);
  
  return (enum mad_flow)flow;
}

// Source - https://stackoverflow.com/a/43255382
// Posted by Jeroen
// Retrieved 2026-02-15, License - CC BY-SA 3.0

static enum mad_flow get_input(mp_obj_libmad_decoder_t *decoder) {

  int retval; /* Return value from read(). */
  int len; /* Length of the new buffer. */
  int eof; /* Whether this is the last buffer that we can provide. */

  // move any remaining data to the begining of the stream, tell me how many bytes i can fit in the buffer.
  int keep = mad_stream_advance_frame(&decoder->stream);
  int room = MP3_BUF_SIZE - keep;

  /* Append new data to the buffer. */
  int bytesread = input_cb(decoder, decoder->stream.buffer + keep, room);

  mp_printf(&mp_plat_print, "mad_decoder_run: input %d\n", bytesread);

  if (bytesread < 0) {
    /* Read error. */
    return MAD_FLOW_STOP;
  } else if (bytesread == 0) {
    mp_printf(&mp_plat_print, "mad_decoder_run: EOF %d\n", keep);

    /* End of file. Append MAD_BUFFER_GUARD zero bytes to make sure that the
    last frame is properly decoded. */
    if (keep + MAD_BUFFER_GUARD <= MP3_BUF_SIZE) {
      /* Append all guard bytes and stop decoding after this buffer. */
      memset(decoder->mp3buf + keep, 0, MAD_BUFFER_GUARD);
      len = keep + MAD_BUFFER_GUARD;
      mad_stream_buffer(&decoder->stream, decoder->mp3buf, len);
      mp_printf(&mp_plat_print, "mad_decoder_run: returning MAD_FLOW_STOP\n");
      return MAD_FLOW_STOP;
    } else {
      /* The guard bytes don't all fit in our buffer, so we need to continue
      decoding and write all of the guard bytes in the next call to input(). */
      memset(decoder->mp3buf + keep, 0, MP3_BUF_SIZE - keep);
      len = MP3_BUF_SIZE;
      mad_stream_buffer(&decoder->stream, decoder->mp3buf, len);
      return MAD_FLOW_CONTINUE;
    }
  }
  
  /* New buffer length is amount of bytes that we kept from the previous
  buffer plus the bytes that we read just now. */
  len = keep + bytesread;

  /* Pass the new buffer information to libmad. */
  mad_stream_buffer(&decoder->stream, decoder->mp3buf, len);

  return MAD_FLOW_CONTINUE;
}
// End Attribution

static
enum mad_flow output_cb(mp_obj_libmad_decoder_t *decoder) {
  //mp_printf(&mp_plat_print, "In output_cb...");

  mp_obj_t args[2];
  args[0] = decoder;
  args[1] = decoder->cb_data;
  mp_obj_t result = mp_call_function_n_kw(decoder->py_output_cb, 2, 0, args);
  int flow = mp_obj_get_int(result);

  //mp_printf(&mp_plat_print, " returning flow=%d\n", flow);
  return (enum mad_flow)flow;
}

static
enum mad_flow error_default(int *bad_last_frame, struct mad_stream *stream,
			    struct mad_frame *frame)
{
  switch (stream->error) {
  case MAD_ERROR_BADCRC:
    if (*bad_last_frame) {
      mp_printf(&mp_plat_print, "bad CRC, muting frame\n");
      mad_frame_mute(frame);
    } else {
      *bad_last_frame = 1;
    }

    return MAD_FLOW_IGNORE;

  default:
    return MAD_FLOW_CONTINUE;
  }
}

static
enum mad_flow error_cb(mp_obj_libmad_decoder_t *decoder, int *bad_last_frame, struct mad_stream *stream, struct mad_frame *frame)
{
  //mp_printf(&mp_plat_print, "error_cb: stream->error = %s\n", mad_stream_errorstr(stream));

  if (decoder->py_error_cb == MP_OBJ_NULL) {
    // no error callback defined, use default
    return error_default(bad_last_frame, stream, frame);
  }

  mp_obj_t args[2];
  args[0] = decoder;
  args[1] = decoder->cb_data;
  
  mp_obj_t result = mp_call_function_n_kw(decoder->py_error_cb, 2, 0, args);
  return mp_obj_get_int(result);
}

/*
 * NAME:	decoder->run()
 * DESCRIPTION:	run the decoder
 */
int mad_decoder_run(mp_obj_libmad_decoder_t *decoder)
{
  int bad_last_frame = 0;
  struct mad_stream *stream;
  struct mad_frame *frame;
  struct mad_synth *synth;
  int result = 0;

  if (decoder->py_input_cb == MP_OBJ_NULL ||
      decoder->py_output_cb == MP_OBJ_NULL) {
      mp_printf(&mp_plat_print, "mad_decoder_run: missing required callback(s)\n");
      return 0;
    }

  stream = &decoder->stream;
  frame  = &decoder->frame;
  synth  = &decoder->synth;

  mad_stream_init(stream, decoder->mp3buf);
  mad_frame_init(frame);
  mad_synth_init(synth);

  // give the stream our buffer, but tell it it doesn't have any data right now.
  mad_stream_buffer(&decoder->stream, decoder->mp3buf, 0);

  decoder->stream.options = decoder->options;
  do {
    mp_printf(&mp_plat_print, "mad_decoder_run: one\n");
    switch (get_input(decoder)) {
      case MAD_FLOW_STOP:
        goto done;
      case MAD_FLOW_BREAK:
        goto fail;
      case MAD_FLOW_IGNORE:
        continue;
      case MAD_FLOW_CONTINUE:
        break;
      default:
        goto fail;
    }

    mp_printf(&mp_plat_print, "mad_decoder_run: decoding loop...\n");
    while (1) {
#if 0      
      if (decoder->header_func) {
        if (mad_header_decode(&frame->header, stream) == -1) {
          if (!MAD_RECOVERABLE(stream->error))
            break;

          switch (error_cb(decoder, &bad_last_frame, stream, frame)) {
          case MAD_FLOW_STOP:
            goto done;
          case MAD_FLOW_BREAK:
            goto fail;
          case MAD_FLOW_IGNORE:
          case MAD_FLOW_CONTINUE:
          default:
            continue;
          }
        }

        switch (decoder->header_func(decoder->cb_data, &frame->header)) {
        case MAD_FLOW_STOP:
          goto done;
        case MAD_FLOW_BREAK:
          goto fail;
        case MAD_FLOW_IGNORE:
          continue;
        case MAD_FLOW_CONTINUE:
          break;
        }
      }
#endif
      //mp_printf(&mp_plat_print, "mad_decoder_run: decoding frame\n");
      int decode_result = mad_frame_decode(frame, stream);
      if (decode_result <= -1) {
        //mp_printf(&mp_plat_print, "mad_decoder_run: decode failed... mad_frame_decode = %d\n", decode_result);
        mp_printf(&mp_plat_print, "mad_decoder_run: stream->error = %s\n", mad_stream_errorstr(stream));
        if (!MAD_RECOVERABLE(stream->error))
          break;

        switch (error_cb(decoder, &bad_last_frame, stream, frame)) {
        case MAD_FLOW_STOP:
          goto done;
        case MAD_FLOW_BREAK:
          goto fail;
        case MAD_FLOW_IGNORE:
          break;
        case MAD_FLOW_CONTINUE:
        default:
          continue;
        }
      }
      else
        bad_last_frame = 0;

#if 0
      if (decoder->filter_func) {
        switch (decoder->filter_func(decoder->cb_data, stream, frame)) {
        case MAD_FLOW_STOP:
          goto done;
        case MAD_FLOW_BREAK:
          goto fail;
        case MAD_FLOW_IGNORE:
          continue;
        case MAD_FLOW_CONTINUE:
          break;
        }
      }
#endif
      //mp_printf(&mp_plat_print, "mad_decoder_run: synth frame\n");
      mad_synth_frame(synth, frame);

      if (decoder->py_output_cb != MP_OBJ_NULL) {
        //mp_printf(&mp_plat_print, "mad_decoder_run: output callback\n");
        switch (output_cb(decoder)) {
        case MAD_FLOW_STOP:
          goto done;
        case MAD_FLOW_BREAK:
          goto fail;
        case MAD_FLOW_IGNORE:
        case MAD_FLOW_CONTINUE:
          break;
        }
      }
    }
  }
  while (stream->error == MAD_ERROR_BUFLEN);

 fail:
  result = -1;

 done:
  mp_printf(&mp_plat_print, "mad_decoder_run: done\n");
  mad_synth_finish(synth);
  mad_frame_finish(frame);
  mad_stream_finish(stream);

  return result;
}


