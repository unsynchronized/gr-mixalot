#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_io_signature.h>
#include <itpp/comm/bch.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "utils.h"

using namespace itpp;
using std::string;

namespace gr {
    namespace mixalot {
        unsigned char
        even_parity(gr_uint32 x) {
            x ^= x >> 16;
            x ^= x >> 8;
            x ^= x >> 4;
            x &= 0xf;
            return (0x6996 >> x) & 1;
        }

        shared_ptr<itpp::bvec> 
        get_vec(const string binstr) {
            std::stringstream ss;
            for(string::const_iterator it = binstr.begin(); it != binstr.end(); ++it) {
                const char c = *it;
                switch(c) {
                    case '0':
                        ss << "0 ";
                        break;
                    case '1':
                        ss << "1 ";
                        break;
                    case ' ':
                        break;
                    default:
                        std::cerr << "invalid value: " << (int)c << std::endl;
                        throw std::invalid_argument("invalid character in bit vector string: expected 1, 0, or space");
                }
            }
            shared_ptr<itpp::bvec> bp(new itpp::bvec(ss.str()));
            return bp;
        }

        // Given a 21-bit dataword (stored in the upper 21 bits of dw), generate a parity-protected
        // BCH(31,21) codeword.  This is systematic encoding -- the data is left as a contiguous stream
        // of bits, shifted to the upper 21 bits of the result; these are followed by the 11-bit BCH ECC,
        // and finally the parity bit is the LSB.
        gr_uint32 
        encodeword(gr_uint32 dw) {
            static BCH bch(31,21,2,ivec("3 5 5 1"), true);
            bvec b(21);
            gr_uint32o_bvec(dw, b, 21);
            assert((dw >> 11) == bvec_to_uint32(b));
            b = bch.encode(b);
            b.set_size(32, true);
            gr_uint32 codeword = (bvec_to_uint32(b));
            codeword |= (even_parity(codeword) & 1);
            return codeword;
        }
        void 
        gr_uint32o_bvec(gr_uint32 d, bvec &bv, int nbits) {
            assert(nbits >= 0 && nbits < 33);
            bv.zeros();
            for(int i = 0; i < nbits; i++) {
                bv(i) = ((d & 0x80000000) == 0x80000000) ? 1 : 0;
                d = (d << 1);
            }
        }



        gr_uint32 
        bvec_to_uint32(const bvec &bv) {
            gr_uint32 u = 0;
            for(unsigned int i = 0; i < (bv.length() > 32 ? 32 : bv.length()); i++) {
                u <<= 1;
                u |= bv(i);
            }
            return u;
        }



        #define MSG_BASE 0x33333333
        void 
        make_numeric_message(const std::string message, std::vector<gr_uint32> &msgwords) {
            int curpos = 0;
            gr_uint32 msg = MSG_BASE;
            for(int i = 0; i < message.length(); i++) {
                char d = message[i];
                gr_uint32 mbit = 0;
                switch(d) {
                    case ']':
                    case ')':
                        mbit = 7; 
                        break;
                    case 'S':
                    case 'E':
                        mbit = 5;
                        break;
                    case 'U':
                        mbit = 0xd;
                        break;
                    case '-':
                        mbit = 0xb;
                        break;
                    case '[':
                    case '(':
                        mbit = 0xf;
                        break;
                    case ' ':
                        mbit = 3;
                        break;
                    case '0':
                        mbit = 0;
                        break;
                    case '1':
                        mbit = 8;
                        break;
                    case '2': 
                        mbit = 4;
                        break;
                    case '3':
                        mbit = 0xc;
                        break;
                    case '4':
                        mbit = 2;
                        break;
                    case '5':
                        mbit = 0xa;
                        break;
                    case '6':
                        mbit = 6;
                        break;
                    case '7':
                        mbit = 0xe;
                        break;
                    case '8':
                        mbit = 1;
                        break;
                    case '9':
                        mbit = 9;
                        break;
                    default:
                        throw std::invalid_argument("non-digit character included in numeric message");
                        break;
                }
                msg <<= 4;
                msg |= mbit;
                curpos++;
                if(curpos == 5) {
                    const gr_uint32 msgtemp = (msg << 11) | 0x80000000;
                    const gr_uint32 msgword = encodeword(msgtemp);
                    assert((msgword & 0xFFFFF800) == msgtemp);      // sanity
                    msgwords.push_back(msgword);
                    curpos = 0;
                    msg = MSG_BASE;
                }
            }
            if(curpos > 0) {
                for(; curpos < 5; curpos++) {
                    msg <<= 4;
                    msg |= 0x3;
                }
                const gr_uint32 msgtemp = (msg << 11) | 0x80000000;
                const gr_uint32 msgword = encodeword(msgtemp);
                assert((msgword & 0xFFFFF800) == msgtemp);      // sanity
                msgwords.push_back(msgword);
            }
        }
        void 
        make_alpha_message(const std::string message, std::vector<gr_uint32> &msgwords) {
            std::vector<bool> bitvec;
            for(int i = 0; i < message.length(); i++) {
                unsigned char c = (unsigned char)message[i];
                for(int j = 0; j < 7; j++) {
                    bitvec.push_back(c & 1);
                    c >>= 1;
                }
            }
            bitvec.push_back(3);     // EOT -- not required, but helps
            unsigned int bc = 0;
            gr_uint32 msg = 0;
            for(std::vector<bool>::iterator bit = bitvec.begin(); bit != bitvec.end(); bit++) {
                msg <<= 1;
                msg |= *bit;
                bc++;
                if(bc == 20) {
                    const gr_uint32 msgtemp = (msg << 11) | 0x80000000;
                    const gr_uint32 msgword = encodeword(msgtemp);
                    assert((msgword & 0xFFFFF800) == msgtemp);      // sanity
                    msgwords.push_back(msgword);
                    bc = 0;
                    msg = 0;
                }
            }
            if(bc > 0) {
                msg <<= (20-bc);
                const gr_uint32 msgtemp = (msg << 11) | 0x80000000;
                const gr_uint32 msgword = encodeword(msgtemp);
                assert((msgword & 0xFFFFF800) == msgtemp);      // sanity
                msgwords.push_back(msgword);
            }
        }
    }
}


