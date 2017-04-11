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

#ifndef INCLUDED_EDACS_FIND_CHAN_NUMS_IMPL_H
#define INCLUDED_EDACS_FIND_CHAN_NUMS_IMPL_H

#include <edacs/find_chan_nums.h>
#include <gnuradio/fft/fft.h>

/* Number of output bins from the FFT. Should be a power of two.
 * Note that if this value is too small, the resolution may not
 * be good enough to differentiate between channels */
#define FFT_SIZE (1024*2)
/* Starting amount of 'counts' given to each channel */
#define STARTING_WEIGHT 100
/* Amount to increment by if the channel power is over the specified threshold */
#define INC_AMOUNT 1
/* Amount to decrement by if the channel power is not over the specifed threshold */
#define DEC_AMOUNT 20
/* Set to 1 to test if the channel number finder works 
 * (if the frequencies were entered in the proper order,
 * and this order was successfully recovered then the 
 * 'Test Passed' message will be printed) */
#define TEST_CHAN_FINDER 0
/* Set to 1 for FFT output (only the bins coinciding with a channel) */
#define VERBOSE_CHAN_PWR 0
/* Set to 1 for a channel count table printout every scan */
#define VERBOSE_COUNT_TABLE 0

namespace gr {
  namespace edacs {

    class find_chan_nums_impl : public find_chan_nums
    {
     private:
      /* If a message is received from controlling block, do a scan to
       * look for what channels are active */
      void scan_start(pmt::pmt_t msg);
      /* Sets the channel number in its appropriate frequency index */
      void mark_found_chans();
      /* Prints out the channel count table */
      void print_table();
      /* Tests if all channel numbers have been determined
       * returns true if they have, false if they haven't */
      bool found_all_chans();

      fft::fft_real_rev *fft_c;
      bool scanning;
      int target_chan;
      int n_chans;
      int *bin_indices;
      int *chan_counts;
      float start_freq;
      float bandwidth;
      float bin_freq;
      int ctrl_chan;
      pmt::pmt_t msg;

      std::vector<float> d_freq_list;
      float d_center_freq;
      float d_samp_rate;
      float d_threshold;

      time_t time_start;
      time_t time_stop;
      struct tm *time_info;
      double seconds;

     public:
      find_chan_nums_impl(std::vector<float> freq_list, float center_freq, float samp_rate, float threshold);
      ~find_chan_nums_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace edacs
} // namespace gr

#endif /* INCLUDED_EDACS_FIND_CHAN_NUMS_IMPL_H */

