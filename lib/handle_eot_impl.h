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

#ifndef INCLUDED_EDACS_HANDLE_EOT_IMPL_H
#define INCLUDED_EDACS_HANDLE_EOT_IMPL_H

#include <edacs/handle_eot.h>
#include <gnuradio/fft/goertzel.h>
#include <gnuradio/filter/single_pole_iir.h>

/* Minimum number of input/output samples */
#define N (1024/4)
/* Number of work cycles to delay after muting is turned off
 * before muting is permitted again.
 * For example if N = 1024, samp_rate = 48000, and DELAY = 1, 
 * the wait is ~0.0213s (N * DELAY / samp_rate). */
#define DELAY 1
 /* Due to the interaction between the Selector block and the
  * Wav File Sink block we cannot switch between analog and digital
  * while outputting to a file, as it disconnects the block on the 
  * switch. However, we can output a single type -- the type that 
  * is first initialised. So set INIT_SEL_INDEX to 0 for analog or 
  * 1 for digital if the Wav File Sink is needed and only one of 
  * analog/digital are used. 
  * NOTE: APPARENTLY THE WAV FILE SINK DOESNT WORK AT ALL WHEN USING
  * THE TRUNKING RADIO HIER BLOCK */
#define INIT_SEL_INDEX 1

namespace gr {
  namespace edacs {

    class handle_eot_impl : public handle_eot
    {
     private:
      filter::single_pole_iir<double,double,double> iir;
      double pwr;
      fft::goertzel *g;
      bool digital;
      bool mute;
      int delay;
      int sel_index;
      int d_samp_rate;
      int d_tone_freq;
      float d_tone_threshold;
      float d_noise_threshold;

     public:
      handle_eot_impl(int samp_rate, int tone_freq, float tone_threshold, float noise_threshold);
      ~handle_eot_impl();

      /* Getters & setters */
      void set_tone_threshold(float tone_threshold);
      void set_noise_threshold(float noise_threshold);
      int get_sel_index();

      /* Called by the message handler when the Process Message block notifies
       * us that a transmission has started (and we should stop muting). */
      void change_status(pmt::pmt_t msg);

      /* Signal processing happens here. Note that this is a sync block so
       * noutput_items always equals the number of input items. */
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace edacs
} // namespace gr

#endif /* INCLUDED_EDACS_HANDLE_EOT_IMPL_H */

