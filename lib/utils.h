#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_types.h>
#include <gr_io_signature.h>
#include <itpp/comm/bch.h>
#include <iostream>
#include <sstream>
#include <vector>

using boost::shared_ptr;
using itpp::bvec;
namespace gr {
    namespace mixalot {
        shared_ptr<itpp::bvec> get_vec(const std::string binstr);
        void make_numeric_message(const std::string message, std::vector<gr_uint32> &msgwords);
        void make_alpha_message(const std::string message, std::vector<gr_uint32> &msgwords);
        gr_uint32 encodeword(gr_uint32 dw);
        void gr_uint32o_bvec(gr_uint32 d, bvec &bv, int nbits = 32);
        gr_uint32 bvec_to_uint32(const bvec &bv);
        unsigned char even_parity(gr_uint32 x);
    }
}

