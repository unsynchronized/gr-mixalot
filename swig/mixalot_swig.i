/* -*- c++ -*- */

#define MIXALOT_API

%include "gnuradio.i"           // the common stuff

//load generated python docstrings
%include "mixalot_swig_doc.i"

%{
#include "mixalot/pocencode.h"
#include "mixalot/flexencode.h"
#include "mixalot/gscencode.h"
%}

%include "mixalot/pocencode.h"
%include "mixalot/flexencode.h"
%include "mixalot/gscencode.h"
GR_SWIG_BLOCK_MAGIC2(mixalot, pocencode);
GR_SWIG_BLOCK_MAGIC2(mixalot, flexencode);
GR_SWIG_BLOCK_MAGIC2(mixalot, gscencode);
