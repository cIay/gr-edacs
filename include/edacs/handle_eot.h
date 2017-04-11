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


#ifndef INCLUDED_EDACS_HANDLE_EOT_H
#define INCLUDED_EDACS_HANDLE_EOT_H

#include <edacs/api.h>
#include <gnuradio/sync_block.h>
#include <gnuradio/block.h>

namespace gr {
  namespace edacs {

    /*!
     * \ingroup edacs
     *
     */
    class EDACS_API handle_eot : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<handle_eot> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of edacs::handle_eot.
       *
       * To avoid accidental use of raw pointers, edacs::handle_eot's
       * constructor is in a private implementation
       * class. edacs::handle_eot::make is the public interface for
       * creating new instances.
       */
      static sptr make(int samp_rate, int tone_freq, float tone_threshold, float noise_threshold);
      
      virtual void set_tone_threshold(float tone_threshold) = 0;
      virtual void set_noise_threshold(float noise_threshold) = 0;
      virtual int get_sel_index() = 0;
    };

  } // namespace edacs
} // namespace gr

#endif /* INCLUDED_EDACS_HANDLE_EOT_H */

