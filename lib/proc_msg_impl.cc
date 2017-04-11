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
#include "proc_msg_impl.h"


namespace gr {
  namespace edacs {

    proc_msg::sptr
    proc_msg::make(uint16_t talkgroup, std::vector<float> freq_list, float center_freq, bool find_lcns, bool analog, bool digital)
    {
      return gnuradio::get_initial_sptr
        (new proc_msg_impl(talkgroup, freq_list, center_freq, find_lcns, analog, digital));
    }

    /* The private constructor */
    proc_msg_impl::proc_msg_impl(uint16_t talkgroup, std::vector<float> freq_list, float center_freq, bool find_lcns, bool analog, bool digital)
      : gr::block("proc_msg",
              gr::io_signature::make(1, 1, sizeof(unsigned char)),
              gr::io_signature::make(1, 1, sizeof(unsigned char))),
      d_freq_list(freq_list),
      d_center_freq(center_freq),
      d_find_lcns(find_lcns),
      d_analog(analog),
      d_digital(digital)
    {
      set_output_multiple(OUTPUT_LEN);

      message_port_register_in(pmt::mp("eot_status_in"));
      set_msg_handler(pmt::mp("eot_status_in"),
                      boost::bind(&proc_msg_impl::change_eot_status, this, _1));

      message_port_register_in(pmt::mp("chan_status_in"));
      set_msg_handler(pmt::mp("chan_status_in"),
                      boost::bind(&proc_msg_impl::change_chan_status, this, _1));

      message_port_register_out(pmt::mp("eot_status_out"));
      message_port_register_out(pmt::mp("chan_status_out"));
      message_port_register_out(pmt::mp("ctrl_freq"));
      message_port_register_out(pmt::mp("voice_freq"));

      listening = false;
      in_msg = false;
      pkt_index = 0;
      scanning = true;
      lf_lcn = false;
      chan_index = 0;
      ctrl_chan = 0;
      n_chans = d_freq_list.size();
      chan_indices = new int[n_chans];
      temp_freqs = new float[n_chans];
      std::fill(chan_indices, chan_indices + n_chans, -1);
      delay = DELAY;
      bit_count = 0;

      clear_msg(msg1);
      clear_msg(msg2);
      clear_msg(msg_target);

      target_agency = (AGENCY_MASK & talkgroup) >> 8;
      target_fleet = (FLEET_MASK & talkgroup) >> 4;
      target_subfleet = (SUBFLEET_MASK & talkgroup);

      if (scanning) {
        printf("STARTING SCAN FOR CONTROL CHANNEL...\n");
      }
    }

    /* Our virtual destructor */
    proc_msg_impl::~proc_msg_impl()
    {
      delete[] chan_indices;
      delete[] temp_freqs;
    }

    void
    proc_msg_impl::change_eot_status(pmt::pmt_t msg)
    {
      listening = to_bool(msg);
    }

    void
    proc_msg_impl::change_chan_status(pmt::pmt_t msg)
    {
      if(d_find_lcns) {
        lf_lcn = true;
        int unknown_count = 0;
        int i;
        /* Check if all channel number have been found */
        for (i = 0; i < n_chans; i++) {
          chan_indices[i] = to_long(pmt::vector_ref(msg, i));
          if (chan_indices[i] == -1)
            unknown_count++;
        }
        /* All channel numbers have been found, so print out the proper 
         * frequency order and sort d_freq_list */
        if (unknown_count <= 1 && !TEST_CHAN_FINDER) {
          for (i = 0; i < n_chans; i++) {
            if (chan_indices[i] == -1)
              temp_freqs[i] = d_freq_list[ctrl_chan-1];
            else
              temp_freqs[i] = d_freq_list[chan_indices[i]];
          }
          printf("Frequency Order:\n");
          for (i = 0; i < n_chans-1; i++) {
            d_freq_list[i] = temp_freqs[i];
            printf("%.4f, ", d_freq_list[i]);
          }
          d_freq_list[n_chans-1] = temp_freqs[n_chans-1];
          printf("%.4f\n", d_freq_list[n_chans-1]);
          d_find_lcns = false;
          lf_lcn = false;
        }
      }
    }

    int
    proc_msg_impl::get_chan()
    {
      return msg_target.lcn;
    }

    void
    proc_msg_impl::clear_msg(msg &m)
    {
      m.cmd = 0;
      m.lcn = 0;
      m.status = 0;
      m.afs = 0;
    }

    void
    proc_msg_impl::cpy_msg(msg &source, msg &dest)
    {
      dest.cmd = source.cmd;
      dest.lcn = source.lcn;
      dest.status = source.status;
      dest.afs = source.afs;
    }

    uint64_t
    proc_msg_impl::check_pkt()
    {
      uint64_t best_msg = 0;
      uint64_t copy1, copy2, copy3;
      int msg_len = PKT_LEN / 6;

      for (int i = msg_len; i >= 0; --i) {
        copy1 = pkt[i];
        copy2 = ((~pkt[i+msg_len]) & 0x01);
        copy3 = pkt[i+(msg_len*2)];

        if (copy1 == copy2) {
          copy1 <<= msg_len - 1 - i;
          best_msg |= copy1;
        }
        else if (copy1 == copy3) {
          copy1 <<= msg_len - 1 - i;
          best_msg |= copy1;
        }
        else if (copy2 == copy3) {
          copy2 <<= msg_len - 1 - i;
          best_msg |= copy2;
        }
      }
      return best_msg;
    }

    void
    proc_msg_impl::split_msg(msg &m, uint64_t best_msg)
    {
      /* Get command (cmd) */
      m.cmd =    (0x000000FF00000000 & best_msg) >> 32;
      /* Get logical channel number (lcn) */
      m.lcn =    (0x00000000F8000000 & best_msg) >> 27;
      /* Get status bits */
      m.status = (0x0000000007800000 & best_msg) >> 23;
      /* Get talkgroup (afs) */
      m.afs =    (0x00000000007FF000 & best_msg) >> 12;
    }

    void
    proc_msg_impl::filter_msg(msg &m)
    {
      uint8_t a = (0x0700 & m.afs) >> 8; /* Agency */
      uint8_t f = (0x00F0 & m.afs) >> 4; /* Fleet */
      uint8_t s = (0x000F & m.afs);      /* Subfleet */

      uint8_t bit1 = (0x0008 & m.status) >> 3;
      uint8_t bit2 = (0x0004 & m.status) >> 2;
      uint8_t bit3 = (0x0002 & m.status) >> 1;
      uint8_t bit4 = (0x0001 & m.status);

      /* Verify the command is valid, 
       * verify the channel is within EDACS bounds, 
       * ignore if the previous LCN is equal to the current LCN 
       * (preventing rapid swapping between a muted and unmuted state near the EOT), 
       * and ignore if we are already listening to a transmission */
      if (((m.cmd == ANALOG_CMD && d_analog) || (m.cmd == DIGITAL_CMD && d_digital)) &&
          m.lcn >= 1 && m.lcn <= MAX_CHANS && 
          m.lcn != msg_target.lcn && 
          !listening) {

        /* If we are looking for lcn's in order to match them with frequencies,
           send the current lcn to the Find Channel Number block */
        if (d_find_lcns) {
          if (lf_lcn && chan_indices[m.lcn-1] == -1) {
            pmt::pmt_t msg = pmt::make_vector(3, pmt::from_long(0));
            pmt::vector_set(msg, 0, pmt::from_long(m.lcn));
            pmt::vector_set(msg, 1, pmt::from_long(ctrl_chan));
            pmt::vector_set(msg, 2, pmt::from_long(ctrl_status));
            message_port_pub(pmt::mp("chan_status_out"), msg);
            lf_lcn = false;
            ctrl_status = false;
          }
          return;
        }

        /* Assigns a channel by copying the received message into
         * message_target. This only occurs if the proper conditions
         * which indicate AFS matches/broadcasts are met. */
        if (a == target_agency || a == 0 || target_agency == 0) {
          if (f == target_fleet || f == 0 || target_fleet == 0) {
            if (s == target_subfleet || s == 0 || target_subfleet == 0) {
              cpy_msg(m, msg_target);

              pmt::pmt_t msg;
              if (msg_target.cmd == DIGITAL_CMD) {
                msg = pmt::from_bool(true);
                printf("Digital voice channel assignment: %2d, Talkgroup: %4d (Agency %2d, Fleet %2d, Subfleet %2d)\n", 
                       msg_target.lcn, msg_target.afs, a, f, s);
              }
              else {
                msg = pmt::from_bool(false);
                printf("Analog voice channel assignment: %2d, Talkgroup: %4d (Agency %2d, Fleet %2d, Subfleet %2d)\n", 
                       msg_target.lcn, msg_target.afs, a, f, s);
              }
              //printf("Status bits: %d %d %d %d\n", bit1, bit2, bit3, bit4);
              message_port_pub(pmt::mp("eot_status_out"), msg);

              msg = pmt::from_double((d_center_freq - d_freq_list[msg_target.lcn-1]) * pow(10, 6));
              message_port_pub(pmt::mp("voice_freq"), msg);

              listening = true;
            }
          }
        }
      } 
    }


    void
    proc_msg_impl::forecast (int noutput_items, 
                             gr_vector_int &ninput_items_required)
    {
      /* The number of input items required to produce noutput_items */
      ninput_items_required[0] = (noutput_items / OUTPUT_LEN - 1) * PKT_LEN + 1;
    }

    int
    proc_msg_impl::general_work (int noutput_items,
                                 gr_vector_int &ninput_items,
                                 gr_vector_const_void_star &input_items,
                                 gr_vector_void_star &output_items)
    {
      const unsigned char *in = (const unsigned char *) input_items[0];
      unsigned char *out = (unsigned char *) output_items[0];

      int produced = 0;
      int input_to_proc = (noutput_items / OUTPUT_LEN - 1) * PKT_LEN + 1;

      /* Do <+signal processing+> */
      for (int i = 0; i < input_to_proc; i++) {
        if (in_msg) {
          /* Message one */
          if (pkt_index < PKT_LEN / 2) {
            pkt[pkt_index] = in[i];
            pkt_index++;
          } 
          /* Message one complete */
          else if (pkt_index == PKT_LEN / 2) {
            split_msg(msg1, check_pkt());
            filter_msg(msg1);
            pkt[pkt_index-(PKT_LEN/2)] = in[i];
            pkt_index++;
          }
          /* Message two */
          else if (pkt_index < PKT_LEN) {
            pkt[pkt_index-(PKT_LEN/2)] = in[i];
            pkt_index++;
          }
          /* Message two complete */
          else if (pkt_index == PKT_LEN) {
            split_msg(msg2, check_pkt());
            filter_msg(msg2);
            in_msg = false;
            pkt_index = 0;

            produced += sprintf((char*) &out[produced], 
                                "Command: 0x%02x Channel: %2d Talkgroup: %4d\n",
                                msg1.cmd, msg1.lcn, msg1.afs);
            produced += sprintf((char*) &out[produced], 
                                "Command: 0x%02x Channel: %2d Talkgroup: %4d\n",
                                msg2.cmd, msg2.lcn, msg2.afs);
          }
        }
        /* Packet found -- bit 0x02 was set by Correlate Access Code block */
        else if (in[i] & 0x02) {
          in_msg = true;
          clear_msg(msg1);
          clear_msg(msg2);
          pkt[pkt_index] = in[i] & ~0x02;
          pkt_index++;
          bit_count = 0;
          /* Found the control channel... stop scanning */
          if (scanning) {
            printf("CONTROL CHANNEL FOUND\n");
            ctrl_chan = chan_index + 1;
            scanning = false;
            if (d_find_lcns) {
              lf_lcn = true;
              ctrl_status = true;
            }
            delay = 0;
          }
        }
        /* If one second has gone by without getting a packet, assume
         * the control channel has changed and do another scan */
        else if (!scanning) {
          if (bit_count > BIT_RATE) {
            bit_count = 0;
            scanning = true;
            delay = DELAY;
          }
          else {
            bit_count++;
          }
        }
      }

      /* Can't detect the control channel... try the next frequency */
      if (scanning && delay == 0) {
        if (chan_index == MAX_CHANS-1)
          chan_index = 0;
        else
          chan_index++;
        printf("TRYING NEXT CHANNEL -- %d\n", chan_index+1);
        pmt::pmt_t msg = pmt::from_double((d_center_freq - d_freq_list[chan_index]) * pow(10, 6));
        message_port_pub(pmt::mp("ctrl_freq"), msg);
        delay = DELAY;
      }
      else if (scanning && delay > 0) {
        delay--;
      }

      /* Tell runtime system how many input items we consumed on
       * each input stream. */
      consume_each (input_to_proc);

      /* Tell runtime system how many output items we produced. */
      return produced;
    }

  } /* namespace edacs */
} /* namespace gr */

