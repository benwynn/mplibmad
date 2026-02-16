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

// Source - https://stackoverflow.com/a/43255382
// Posted by Jeroen
// Retrieved 2026-02-15, License - CC BY-SA 3.0

#define MP3_BUF_SIZE 4096
#define MP3_FRAME_SIZE 2881

static enum mad_flow input(mp_obj_libmad_decoder_t *decoder) {

  struct mad_stream *stream = &decoder->stream;

  static char mp3_buf[MP3_BUF_SIZE]; /* MP3 data buffer. */

  int retval; /* Return value from read(). */
  int len; /* Length of the new buffer. */
  int eof; /* Whether this is the last buffer that we can provide. */

  /* Figure out how much data we need to move from the end of the previous
  buffer into the start of the new buffer. */
  int keep; /* Number of bytes to keep from the previous buffer */
  if (stream->error != MAD_ERROR_BUFLEN) {
    /* All data has been consumed, or this is the first call. */
    keep = 0;
  } else if (stream->next_frame != NULL) {
    /* The previous buffer was consumed partially. Move the unconsumed portion
    into the new buffer. */
    keep = stream->bufend - stream->next_frame;
  } else if ((stream->bufend - stream->buffer) < MP3_BUF_SIZE) {
    /* No data has been consumed at all, but our read buffer isn't full yet,
    so let's just read more data first. */
    keep = stream->bufend - stream->buffer;
  } else {
    /* No data has been consumed at all, and our read buffer is already full.
    Shift the buffer to make room for more data, in such a way that any
    possible frame position in the file is completely in the buffer at least
    once. */
    keep = MP3_BUF_SIZE - MP3_FRAME_SIZE;
  }

  /* Shift the end of the previous buffer to the start of the new buffer if we
  want to keep any bytes. */
  if (keep) {
    memmove(mp3_buf, stream->bufend - keep, keep);
  }

  /* Append new data to the buffer. */
  retval = read(in_fd, mp3_buf + keep, MP3_BUF_SIZE - keep);
  if (retval < 0) {
    /* Read error. */
    perror("failed to read from input");
    return MAD_FLOW_STOP;
  } else if (retval == 0) {
    /* End of file. Append MAD_BUFFER_GUARD zero bytes to make sure that the
    last frame is properly decoded. */
    if (keep + MAD_BUFFER_GUARD <= MP3_BUF_SIZE) {
      /* Append all guard bytes and stop decoding after this buffer. */
      memset(mp3_buf + keep, 0, MAD_BUFFER_GUARD);
      len = keep + MAD_BUFFER_GUARD;
      eof = 1;
    } else {
      /* The guard bytes don't all fit in our buffer, so we need to continue
      decoding and write all fo teh guard bytes in the next call to input(). */
      memset(mp3_buf + keep, 0, MP3_BUF_SIZE - keep);
      len = MP3_BUF_SIZE;
      eof = 0;
    }
  } else {
    /* New buffer length is amount of bytes that we kept from the previous
    buffer plus the bytes that we read just now. */
    len = keep + retval;
    eof = 0;
  }

  /* Pass the new buffer information to libmad. */
  mad_stream_buffer(stream, mp3_buf, len);
  return eof ? MAD_FLOW_STOP : MAD_FLOW_CONTINUE;
}


static
enum mad_flow input_cb(mp_obj_libmad_decoder_t *decoder) {
  //mp_printf(&mp_plat_print, "In input_cb(%p)\n", decoder);

  mp_obj_t args[2];
  args[0] = decoder;
  args[1] = decoder->cb_data;

  mp_obj_t result = mp_call_function_n_kw(decoder->py_input_cb, 2, 0, args);
  int flow = mp_obj_get_int(result);
  
  return (enum mad_flow)flow;
}

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
    if (*bad_last_frame)
      mad_frame_mute(frame);
    else
      *bad_last_frame = 1;

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

  mad_stream_init(stream);
  mad_frame_init(frame);
  mad_synth_init(synth);

  mad_stream_options(stream, decoder->options);
  
  do {
    switch (input_cb(decoder)) {
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

    //mp_printf(&mp_plat_print, "mad_decoder_run: decoding loop...\n");
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
  mad_synth_finish(synth);
  mad_frame_finish(frame);
  mad_stream_finish(stream);

  return result;
}


