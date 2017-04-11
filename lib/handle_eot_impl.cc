/* -*- c++ -*- */
/* 
 * Copyright 2017 Clayton Caron.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "handle_eot_impl.h"

namespace gr {
  namespace edacs {

    handle_eot::sptr
    handle_eot::make(int samp_rate, int tone_freq, float tone_threshold, float noise_threshold)
    {
      return gnuradio::get_initial_sptr
        (new handle_eot_impl(samp_rate, tone_freq, tone_threshold, noise_threshold));
    }

    /*
     * The private constructor
     */
    handle_eot_impl::handle_eot_impl(int samp_rate, int tone_freq, float tone_threshold, float noise_threshold)
      : gr::sync_block("handle_eot",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float))),
      d_samp_rate(samp_rate), 
      d_tone_freq(tone_freq), 
      d_tone_threshold(tone_threshold), 
      d_noise_threshold(noise_threshold)
    {
      set_output_multiple(N);
      set_max_noutput_items(N);

      message_port_register_in(pmt::mp("status_in"));
      set_msg_handler(pmt::mp("status_in"),
                      boost::bind(&handle_eot_impl::change_status, this, _1));

      message_port_register_out(pmt::mp("status_out"));

      g = new fft::goertzel(d_samp_rate, N, d_tone_freq);

      mute = true;
      delay = 0;
      sel_index = INIT_SEL_INDEX;
    }

    /*
     * Our virtual destructor.
     */
    handle_eot_impl::~handle_eot_impl()
    {
      delete g;
    }

    void
    handle_eot_impl::set_tone_threshold(float tone_threshold)
    {
      d_tone_threshold = tone_threshold;
    }

    void
    handle_eot_impl::set_noise_threshold(float noise_threshold)
    {
      d_noise_threshold = noise_threshold;
    }

    int
    handle_eot_impl::get_sel_index()
    {
      return sel_index;
    }

    void
    handle_eot_impl::change_status(pmt::pmt_t msg)
    {
      digital = to_bool(msg);
      mute = false;
      delay = DELAY;
    }

    int
    handle_eot_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];

      /* If we are not muted, listen for the end of transmission
       * tone. This is done using the Goertzel algorithm which is 
       * essentially an FFT for computing a single spectral bin.
       * For EDACS this EOT tone is at 4800 Hz, so if the amplitude
       * at this frequency is found to be greater than our defined 
       * threshold, we mute.
       * For more information regarding this see:
       * http://www.embedded.com/design/real-world-applications/4401754/
         Single-tone-detection-with-the-Goertzel-algorithm */ 
      if (!mute && delay == 0) {
        float level = abs(g->batch((float*)in));
        if (level > d_tone_threshold) {
          pmt::pmt_t msg = pmt::from_bool(false);
          message_port_pub(pmt::mp("status_out"), msg);
          mute = true;
          printf("END OF TRANSMISSION\n");
        }
      } 

      /* In case we are sent to a frequency with no voice or we somehow
       * miss the EOT tone sequence, try to detect static and correct by 
       * muting. Problems still arise if we end up at a control channel
       * as we will be stuck there, requiring a restart. */
      if (!mute && delay == 0) {
        /* Magic used in pwr_squelch_ff_impl.cc */
        pwr = iir.filter(in[0]*in[0]);
        if(pwr > d_noise_threshold) {
          printf("NOISY CHANNEL, MUTING...\n");
          pmt::pmt_t msg = pmt::from_bool(false);
          message_port_pub(pmt::mp("status_out"), msg);
          mute = true;
        }
        /* Update the index representing digital/analog. It can then
           be accessed via a function probe for the selector block */
        if (digital)
          sel_index = 1;
        else
          sel_index = 0;
      }

      if(delay > 0)
        delay--;

      if (mute)
        memset(out, 0, sizeof(float) * noutput_items);
      else
        memcpy(out, in, sizeof(float) * noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace edacs */
} /* namespace gr */

