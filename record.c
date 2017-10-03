#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

/* 4 bytes/sample
 * 8000 samples/sec
 * 32000 bytes/sec
 * 32000/128
 */
int main (int argc, char *argv[])
{
  int i;
  int err;
  unsigned int sample_rate = 48000;
  short * buf;
  snd_pcm_t *capture_handle;
  snd_pcm_hw_params_t *hw_params;
  /*  int bufsize = 2 * (16/8) * 44100 * 1;  */
  int bufsize = 128;

  if((buf = calloc(bufsize, 2 * (sizeof (short)))) == 0) {
    fprintf (stderr, "cannot allocate buffer (size %d)",
             bufsize);
    exit (1);
  }

  if ((err = snd_pcm_open (&capture_handle, argv[1], SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    fprintf (stderr, "cannot open audio device %s (%s)\n",
             argv[1],
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);

  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
    fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);
  if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (err));
    exit (1);
  }
    fprintf(stderr, "line %d\n", __LINE__);

  if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);
  if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
    fprintf (stderr, "cannot set sample format (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);
  if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &sample_rate , 0)) < 0) {
    fprintf (stderr, "cannot set sample rate (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d rate %d\n", __LINE__, sample_rate);
  if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 2)) < 0) {
    fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);
  if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);
  snd_pcm_hw_params_free (hw_params);

  if ((err = snd_pcm_prepare (capture_handle)) < 0) {
    fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);
  for (i = 0; i < 2000; ++i) {
    if ((err = snd_pcm_readi (capture_handle, buf, bufsize)) != bufsize) {
      fprintf (stderr, "read from audio interface failed (%s)\n",
               snd_strerror (err));
      exit (1);
    }
    (void) write(1, buf, bufsize * 2 * (sizeof (short)));
  }

  fprintf(stderr, "line %d\n", __LINE__);
  free(buf);
  snd_pcm_close (capture_handle);
  exit (0);
}
