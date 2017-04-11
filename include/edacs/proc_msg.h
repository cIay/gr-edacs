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


#ifndef INCLUDED_EDACS_PROC_MSG_H
#define INCLUDED_EDACS_PROC_MSG_H

#include <edacs/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace edacs {

    /*!
     * \ingroup edacs
     *
     */
    class EDACS_API proc_msg : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<proc_msg> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of edacs::proc_msg.
       *
       * To avoid accidental use of raw pointers, edacs::proc_msg's
       * constructor is in a private implementation
       * class. edacs::proc_msg::make is the public interface for
       * creating new instances.
       */
      static sptr make(uint16_t talkgroup, std::vector<float> freq_list, float center_freq, bool find_lcns, bool analog, bool digital);

      virtual int get_chan() = 0;
    };

  } // namespace edacs
} // namespace gr

#endif /* INCLUDED_EDACS_PROC_MSG_H */

