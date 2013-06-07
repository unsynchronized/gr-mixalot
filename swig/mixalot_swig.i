/* -*- c++ -*- */

#define MIXALOT_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "mixalot_swig_doc.i"

%{
#include "mixalot/pocencode.h"
%}


%include "mixalot/pocencode.h"
GR_SWIG_BLOCK_MAGIC2(mixalot, pocencode);

