/* -*- c++ -*- */

#define EDACS_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "edacs_swig_doc.i"

%{
#include "edacs/proc_msg.h"
#include "edacs/handle_eot.h"
#include "edacs/find_chan_nums.h"
%}


%include "edacs/proc_msg.h"
GR_SWIG_BLOCK_MAGIC2(edacs, proc_msg);
%include "edacs/handle_eot.h"
GR_SWIG_BLOCK_MAGIC2(edacs, handle_eot);
%include "edacs/find_chan_nums.h"
GR_SWIG_BLOCK_MAGIC2(edacs, find_chan_nums);
