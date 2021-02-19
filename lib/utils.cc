#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
        even_parity(uint32_t x) {
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
        uint32_t
        reverse_bits32(uint32_t x) {
            x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
            x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
            x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
            x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
            return((x >> 16) | (x << 16));
        }

        // Given a 21-bit dataword (stored in the upper 21 bits of dw), generate a parity-protected
        // BCH(31,21) codeword.  This is systematic encoding -- the data is left as a contiguous stream
        // of bits, shifted to the upper 21 bits of the result; these are followed by the 11-bit BCH ECC,
        // and finally the parity bit is the LSB.
        uint32_t 
        encodeword(uint32_t dw) {
            static BCH bch(31,21,2,ivec("3 5 5 1"), true);
            bvec b(21);
            uint32_to_bvec(dw, b, 21);
            assert((dw >> 11) == bvec_to_uint32(b));
            b = bch.encode(b);
            b.set_size(32, true);
            uint32_t codeword = (bvec_to_uint32(b));
            codeword |= (even_parity(codeword) & 1);
            return codeword;
        }
        void 
        uint32_to_bvec(uint32_t d, bvec &bv, int nbits) {
            assert(nbits >= 0 && nbits < 33);
            bv.zeros();
            for(int i = 0; i < nbits; i++) {
                bv(i) = ((d & 0x80000000) == 0x80000000) ? 1 : 0;
                d = (d << 1);
            }
        }



        uint32_t 
        bvec_to_uint32(const bvec &bv) {
            uint32_t u = 0;
            for(unsigned int i = 0; i < (bv.length() > 32 ? 32 : bv.length()); i++) {
                u <<= 1;
                u = u | (bv(i) ? 1 : 0);
            }
            return u;
        }



        #define MSG_BASE 0x33333333
        void 
        make_numeric_message(const std::string message, std::vector<uint32_t> &msgwords) {
            int curpos = 0;
            uint32_t msg = MSG_BASE;
            for(int i = 0; i < message.length(); i++) {
                char d = message[i];
                uint32_t mbit = 0;
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
                    const uint32_t msgtemp = (msg << 11) | 0x80000000;
                    const uint32_t msgword = encodeword(msgtemp);
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
                const uint32_t msgtemp = (msg << 11) | 0x80000000;
                const uint32_t msgword = encodeword(msgtemp);
                assert((msgword & 0xFFFFF800) == msgtemp);      // sanity
                msgwords.push_back(msgword);
            }
        }
        void 
        make_alpha_message(const std::string message, std::vector<uint32_t> &msgwords) {
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
            uint32_t msg = 0;
            for(std::vector<bool>::iterator bit = bitvec.begin(); bit != bitvec.end(); bit++) {
                msg <<= 1;
                msg |= *bit;
                bc++;
                if(bc == 20) {
                    const uint32_t msgtemp = (msg << 11) | 0x80000000;
                    const uint32_t msgword = encodeword(msgtemp);
                    assert((msgword & 0xFFFFF800) == msgtemp);      // sanity
                    msgwords.push_back(msgword);
                    bc = 0;
                    msg = 0;
                }
            }
            if(bc > 0) {
                msg <<= (20-bc);
                const uint32_t msgtemp = (msg << 11) | 0x80000000;
                const uint32_t msgword = encodeword(msgtemp);
                assert((msgword & 0xFFFFF800) == msgtemp);      // sanity
                msgwords.push_back(msgword);
            }
        }
        /**
         * Decode from %02x-style hex string (41414141) to ASCII (AAAA).
         * Lossy; this drops nulls and ignores invalid chars.
         * No unicode here since we're in the 1990s
         */
        std::string 
        hex_decode(std::string const &message) {
            string outmsg = "";
            size_t sz = message.length();
            sz >>= 1;
            sz <<= 1; // clear last bit
            for(size_t i = 0; i < sz; i += 2) {
                char outc = 0;
                char msb = message[i];
                if(msb >= '0' && msb <= '9') {
                    outc |= ((msb-'0') & 0xf);
                } else if(msb >= 'A' && msb <= 'F') {
                    outc |= ((10+(msb-'A')) & 0xf);
                } else if(msb >= 'a' && msb <= 'f') {
                    outc |= ((10+(msb-'a')) & 0xf);
                } else {
                    continue;
                }
                outc <<= 4;
                char lsb = message[i+1];
                if(lsb >= '0' && lsb <= '9') {
                    outc |= ((lsb-'0') & 0xf);
                } else if(lsb >= 'A' && lsb <= 'F') {
                    outc |= ((10+(lsb-'A')) & 0xf);
                } else if(lsb >= 'a' && lsb <= 'f') {
                    outc |= ((10+(lsb-'a')) & 0xf);
                } else {
                    continue;
                }
                outmsg = outmsg + outc;
            }

            return outmsg;
        }

        void
        uint32_to_bvec_rev(uint32_t d, bvec &bv, int nbits) {
            assert(nbits >= 0 && nbits < 33);
            bv.zeros();
            for(int i = 0; i < nbits; i++) {
                bv(i) = (d & 1);
                d = (d >> 1);
            }
        }

        /**
         * Invert bvin, replacing bvout with an inverted version.
         */
        void
        invert_bvec(const bvec &bvin, bvec &bvout) {
            for(unsigned int i = 0; i < bvin.size(); i++) {
                bvout(i) = bvin(i) == 0 ? 1 : 0;
            }
        }
    }
}


