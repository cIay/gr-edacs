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
#include <time.h>
#include "find_chan_nums_impl.h"

namespace gr {
  namespace edacs {

    find_chan_nums::sptr
    find_chan_nums::make(std::vector<float> freq_list, float center_freq, float samp_rate, float threshold)
    {
      return gnuradio::get_initial_sptr
        (new find_chan_nums_impl(freq_list, center_freq, samp_rate, threshold));
    }

    /*
     * The private constructor
     */
    find_chan_nums_impl::find_chan_nums_impl(std::vector<float> freq_list, float center_freq, float samp_rate, float threshold)
      : gr::sync_block("find_chan_nums",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0)),
      d_freq_list(freq_list),
      d_center_freq(center_freq),
      d_samp_rate(samp_rate),
      d_threshold(threshold)
    {
      set_output_multiple(FFT_SIZE/2);
      set_max_noutput_items(FFT_SIZE/2);

      fft_c = new fft::fft_real_rev(FFT_SIZE, 1);

      message_port_register_in(pmt::mp("status_in"));
      set_msg_handler(pmt::mp("status_in"),
                      boost::bind(&find_chan_nums_impl::scan_start, this, _1));

      message_port_register_out(pmt::mp("status_out"));

      d_center_freq *= pow(10, 6);
      n_chans = d_freq_list.size();
      bandwidth = d_samp_rate / 2.0;
      start_freq = d_center_freq - bandwidth;
      bin_freq = d_samp_rate / FFT_SIZE;

      bin_indices = new int[n_chans];
      chan_counts = new int[n_chans*n_chans];

      msg = pmt::make_vector(n_chans, pmt::from_long(-1));

      /* Get values needed to index correctly into the bin buffer */ 
      for (int i = 0; i < n_chans; i++)
        bin_indices[i] = ((d_freq_list[i]*pow(10, 6)) - start_freq) / d_samp_rate * FFT_SIZE;

      scanning = false;
    }

    /*
     * Our virtual destructor.
     */
    find_chan_nums_impl::~find_chan_nums_impl()
    {
      delete fft_c;
      delete[] bin_indices;
      delete[] chan_counts;
    }

    void
    find_chan_nums_impl::scan_start(pmt::pmt_t msg_in)
    {
      target_chan = pmt::to_long(pmt::vector_ref(msg_in, 0));
      ctrl_chan = pmt::to_long(pmt::vector_ref(msg_in, 1));

      /* If the control channel has changed, reset the counts */
      if (pmt::to_long(pmt::vector_ref(msg_in, 2)))
        std::fill(chan_counts, chan_counts + n_chans*n_chans, STARTING_WEIGHT);

      scanning = true;
    }

    void
    find_chan_nums_impl::mark_found_chans()
    {
      int chan_index;
      int positive_values = 0;

      for (int i = 0; i < n_chans; i++) {
        for (int j = 0; j < n_chans; j++) {
          if (j == ctrl_chan-1)
            continue;
          if (chan_counts[i*n_chans+j] > 0) {
            chan_index = j;
            positive_values++;
          }
        }

        if (positive_values == 1)
          pmt::vector_set(msg, i, pmt::from_long(chan_index));
        else
          pmt::vector_set(msg, i, pmt::from_long(-1));

        positive_values = 0;
      }
    }

    bool
    find_chan_nums_impl::found_all_chans()
    {
      int unknown_count = 0;
      for (int i = 0; i < n_chans; i++) {
        if (pmt::to_long(pmt::vector_ref(msg, i)) == -1)
          unknown_count++;
      }
      return (unknown_count <= 1);
    }

    void
    find_chan_nums_impl::print_table()
    {
      for(int i = 0; i < n_chans; i++) {
        for(int j = 0; j < n_chans; j++)
          printf("%7d ", chan_counts[i*n_chans+j]);
        if (pmt::to_long(pmt::vector_ref(msg, i)) != -1)
          printf("  *");
        printf("\n");
      }
      printf("\n");
    }

    int
    find_chan_nums_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      float binbuf[FFT_SIZE];
      float binbuf_ordered[FFT_SIZE];

      static int count = 1;

      if (scanning) {
        if (count == 1) {
          time(&time_start);
          time_info = localtime(&time_start);
        }
        /* Compute the Fourier transform of the input */
        memcpy(fft_c->get_inbuf(), in, sizeof(gr_complex)*FFT_SIZE/2);
        fft_c->execute();
        memcpy(binbuf, fft_c->get_outbuf(), sizeof(float)*FFT_SIZE);

        /* Put the spectral bins in the right order */
        for (int i = 0; i < FFT_SIZE; i++) {
          if (i < FFT_SIZE/2)
            binbuf_ordered[FFT_SIZE/2-i-1] = abs(binbuf[i]);
          else
            binbuf_ordered[FFT_SIZE-abs(FFT_SIZE/2-i)-1] = abs(binbuf[i]);
        }

        /* Iterate through the potential freqencies, marking where a signal is found by adding 1
         * to the apropriate index in chan_counts */
        for(int i = 0; i < n_chans; i++) {
          int i1, i2, i3;
          i1 = bin_indices[i];
          if (bin_indices[i] != 0) i2 = bin_indices[i]-1; else i2 = bin_indices[i];
          if (bin_indices[i] != FFT_SIZE-1) i3 = bin_indices[i]+1; else i3 = bin_indices[i];

          /* Test the bin associated with the current test frequency along with the two adjacent bins */
          if (binbuf_ordered[i1] > d_threshold || binbuf_ordered[i2] > d_threshold || binbuf_ordered[i3] > d_threshold)
            chan_counts[(target_chan-1)*n_chans+i] += INC_AMOUNT;
          else
            chan_counts[(target_chan-1)*n_chans+i] -= DEC_AMOUNT;

          if (VERBOSE_CHAN_PWR)
            printf("%.4f ", binbuf_ordered[i1]);
        }
        if (VERBOSE_CHAN_PWR)
          printf("\n\n");

        mark_found_chans();

        if (VERBOSE_COUNT_TABLE)
          print_table();

        if (found_all_chans()) {
          print_table();
          time(&time_stop);
          seconds = difftime(time_stop, time_start);
          printf("ALL CHANNEL NUMBERS FOUND\n");
          printf("Number of scans: %d\n", count);
          printf("Start time: %s", asctime(time_info));
          printf("Time elapsed: %.f seconds\n", seconds);

          if (TEST_CHAN_FINDER) {
            bool test = true;
            for (int i = 0; i < n_chans; i++) {
              if (pmt::to_long(pmt::vector_ref(msg, i)) != i && i != ctrl_chan-1) {
                test = false;
                break;
              }
            }
            if (test)
              printf("Test passed.\n");
            else
              printf("Test failed.\n");
            printf("\n\n");
            /* Reset everything for another test round */
            std::fill(chan_counts, chan_counts + n_chans*n_chans, STARTING_WEIGHT);
            pmt::vector_fill(msg, pmt::from_long(-1));
            count = 0;
          }
        }

        count++;  

        scanning = false;
        message_port_pub(pmt::mp("status_out"), msg);
      } 

      return noutput_items;
    }

  } /* namespace edacs */
} /* namespace gr */

