#include "libmad/mad.h"
enum mad_flow input_cb(void *data, struct mad_stream *stream);
enum mad_flow output_cb(void *data, struct mad_header const *header, struct mad_pcm *pcm);
enum mad_flow error_cb(void *data, struct mad_stream *stream, struct mad_frame *frame);