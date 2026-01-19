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

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
# endif

# ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
# endif

# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif

# include <stdlib.h>

# ifdef HAVE_ERRNO_H
#  include <errno.h>
# endif

# include "stream.h"
# include "frame.h"
# include "synth.h"
# include "decoder.h"

/*
 * NAME:	decoder->init()
 * DESCRIPTION:	initialize a decoder object with callback routines
 */
void mad_decoder_init(struct mad_decoder *decoder, void *data,
    enum mad_flow (*input_func)  (void *, struct mad_stream *),
		enum mad_flow (*header_func) (void *, struct mad_header const *),
		enum mad_flow (*filter_func) (void *, struct mad_stream const *, struct mad_frame *),
		enum mad_flow (*output_func) (void *, struct mad_header const *, struct mad_pcm *),
		enum mad_flow (*error_func)  (void *, struct mad_stream *, struct mad_frame *),
		enum mad_flow (*message_func)(void *, void *, unsigned int *))
{
  decoder->options      = 0;
  decoder->cb_data      = data;

  decoder->input_func   = input_func;
  decoder->header_func  = header_func;
  decoder->filter_func  = filter_func;
  decoder->output_func  = output_func;
  decoder->error_func   = error_func;
  decoder->message_func = message_func;
}

static
enum mad_flow error_default(void *data, struct mad_stream *stream,
			    struct mad_frame *frame)
{
  int *bad_last_frame = data;

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

/*
 * NAME:	decoder->run()
 * DESCRIPTION:	run the decoder
 */
int mad_decoder_run(struct mad_decoder *decoder)
{
  enum mad_flow (*error_func)(void *, struct mad_stream *, struct mad_frame *);
  void *error_data;
  int bad_last_frame = 0;
  struct mad_stream *stream;
  struct mad_frame *frame;
  struct mad_synth *synth;
  int result = 0;

  if (decoder->input_func == 0)
    return 0;

  if (decoder->error_func) {
    error_func = decoder->error_func;
    error_data = decoder->cb_data;
  }
  else {
    error_func = error_default;
    error_data = &bad_last_frame;
  }

  stream = &decoder->stream;
  frame  = &decoder->frame;
  synth  = &decoder->synth;

  mad_stream_init(stream);
  mad_frame_init(frame);
  mad_synth_init(synth);

  mad_stream_options(stream, decoder->options);

  do {
    switch (decoder->input_func(decoder->cb_data, stream)) {
    case MAD_FLOW_STOP:
      goto done;
    case MAD_FLOW_BREAK:
      goto fail;
    case MAD_FLOW_IGNORE:
      continue;
    case MAD_FLOW_CONTINUE:
      break;
    }

    while (1) {
      if (decoder->header_func) {
        if (mad_header_decode(&frame->header, stream) == -1) {
          if (!MAD_RECOVERABLE(stream->error))
            break;

          switch (error_func(error_data, stream, frame)) {
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

      if (mad_frame_decode(frame, stream) == -1) {
        if (!MAD_RECOVERABLE(stream->error))
          break;

        switch (error_func(error_data, stream, frame)) {
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

      mad_synth_frame(synth, frame);

      if (decoder->output_func) {
        switch (decoder->output_func(decoder->cb_data,
                  &frame->header, &synth->pcm)) {
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


