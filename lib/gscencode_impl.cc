/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef GR_OLD
#include <gr_io_signature.h>
#else
#include <gnuradio/io_signature.h>
#endif
#include "gscencode_impl.h"
#include <iostream>
#include <sstream>
#include <vector>
#include "utils.h"
#include "golay.h"

using namespace itpp;
using std::vector;
using std::string;
using std::shared_ptr;


namespace gr {
    namespace mixalot {

        // Reverses the bits in a byte and then shifts.  Useful for converting 
        static inline uint8_t 
        convchar(uint8_t b) {
            b = ((b & 0xf0) >> 4) | ((b & 0x0f) << 4);
            b = ((b & 0xcc) >> 2) | ((b & 0x33) << 2);
            b = ((b & 0xaa) >> 1) | ((b & 0x55) << 1);
            return (b >> 1);
        }
        gscencode::sptr
        gscencode::make(int type, unsigned int capcode, std::string message, unsigned long symrate) {
            return gnuradio::get_initial_sptr (new gscencode_impl(type, capcode, message, symrate));
        }

        // XXX things to try:
        // - positive frequency shift is 1, negative is 0
        // - preamble polarity; plain is single, inverted is batch
        // - LSB first
        // - position and order of the parity bits in the data block
        // - starting polarity of the comma -- "must be the same as the first bit of preamble"
        // - information before parity

        // Rep. 900-2, Annex I, Table V
        static const unsigned int preamble_values[] = {
            2030, 1628, 3198, 647, 191, 3315, 1949, 2540, 1560, 2335
        };
        /**
         * Queue up n alternating bits of comma, with the starting polarity specified.
         */
        void
        gscencode_impl::queue_comma(unsigned int nbits, bool startingpolarity) {
            bool pol = startingpolarity;
            for(int i = 0; i < nbits; i++) {
                queuebit(pol);
                pol = !pol;
            }
        }


        
        /**
         * Queue up the GSC preamble. There are ten different combinations of
         * these, and therefore num must be [0,9].
         */
        void
        gscencode_impl::queue_preamble(unsigned int num) {
            if(num > 9) {
                throw std::runtime_error("Invalid preamble value");
            }
            const uint32_t data = preamble_values[num];
            unsigned long golay = calcgolay(data);
            bvec info(12), parity(11);
            uint32_to_bvec_rev(data, info, 12);
            uint32_to_bvec_rev(golay, parity, 11);
            queue_comma(28, (data & 1));
            for(int i = 0; i < 18; i++) {
                queue_dup(info);
                queue_dup(parity);
            }
        }
        static const uint32_t word1s[50] = {
            721, 2731, 2952, 1387, 1578,
            1708, 2650, 1747, 2580, 1376,
            2692, 696, 1667, 3800, 3552,
            3424, 1384, 3595, 876, 3124,
            2285, 2608, 899, 3684, 3129,
            2124, 1287, 2616, 1647, 3216,
            375, 1232, 2824, 1840, 408,
            3127, 3387, 882, 3468, 3267,
            1575, 3463, 3152, 2572, 1252,
            2592, 1552, 835, 1440, 160
        };
        void
        gscencode_impl::queue_startcode() {
            unsigned long golay = calcgolay(713);
            bvec info1(12), parity1(11), info2(12), parity2(11);
            uint32_to_bvec_rev(713, info1, 12);
            uint32_to_bvec_rev(golay, parity1, 11);
            invert_bvec(info1, info2);
            invert_bvec(parity1, parity2);
            queue_comma(28, 1);     // this is fixed because (713 & 1) == 1

            queue_dup(info1);
            queue_dup(parity1);
            queuebit(1);            // this is fixed because (~(713) & 1) == 0 (and this comma bit must be opposite)
            queue_dup(info2);
            queue_dup(parity2);
        }
        void
        gscencode_impl::queue_address(unsigned int word1, unsigned int word2) {
            bool compword1 = false, compword2 = false;
            assert(word1 < 100);
            if(word1 > 49) {
                word1 -= 50;
                compword1 = true;
            }
            uint32_t data1 = word1s[word1], data2 = word2;
            bvec info1(12), parity1(11), info2(12), parity2(11);
            uint32_t golay1 = calcgolay(data1), golay2 = calcgolay(data2);
            uint32_to_bvec_rev(data1, info1, 12);
            uint32_to_bvec_rev(golay1, parity1, 11);
            compword1 = !compword1;
            if(compword1) {
                invert_bvec(info1, info1);
                invert_bvec(parity1, parity1);
            }
            uint32_to_bvec_rev(data2, info2, 12);
            uint32_to_bvec_rev(golay2, parity2, 11);
            if(compword2) {
                invert_bvec(info2, info2);
                invert_bvec(parity2, parity2);
            }
            queue_comma(28, info1(0));
            queue_dup(info1);
            queue_dup(parity1);
            if(info2(0)) {
                queuebit(0);
            } else {
                queuebit(1);
            }
            queue_dup(info2);
            queue_dup(parity2);
        }

        void
        gscencode_impl::queue_message() {
            unsigned int finallen = d_message.length();
            if((finallen % 8) != 0) {
                finallen += (8-(finallen % 8));
            }
            unsigned char chars[finallen];

            if(d_msgtype == Alpha) {
                memset(chars, 0x3e, finallen);

                // Based off of Table VII, Rep. 900-2, Annex I
                for(int i = 0; i < d_message.length(); i++) {
                    unsigned char c = (unsigned char)toupper(d_message[i]);
                    if(c == 0x0a || c == 0x0d) {
                        c = 0x3c;
                    } else if(c == 0x7b) {
                        c = 0x3b;
                    } else if(c == 0x7e) {
                        c = 0x3d;
                    } else if(c == 0x5c) {
                        c = 0x20;
                    } else if(c < 0x20 || c > 0x5d) {
                        c = 0x20;
                    } else {
                        c = c - 0x20;
                    }
                    chars[i] = c;
                }
            } else if(d_msgtype == Numeric) {
                std::cout << "WARNING: GSC Numeric mode is untested!" << std::endl;
                memset(chars, 0xa, finallen);
                for(int i = 0; i < d_message.length(); i++) {
                    unsigned char c = (unsigned char)d_message[i];
                    unsigned char v = 0;
                    switch(c) {
                        case '0':
                            v = 0;
                            break;
                        case '1':
                            v = 1;
                            break;
                        case '2':
                            v = 2;
                            break;
                        case '3':
                            v = 3;
                            break;
                        case '4':
                            v = 4;
                            break;
                        case '5':
                            v = 5;
                            break;
                        case '6':
                            v = 6;
                            break;
                        case '7':
                            v = 7;
                            break;
                        case '8':
                            v = 8;
                            break;
                        case '9':
                            v = 9;
                            break;
                        case 'U':
                            v = 11;
                            break;
                        case ' ':
                            v = 12;
                            break;
                        case '-':
                            v = 13;
                            break;
                        case '=':
                            v = 14;
                            break;
                        case 'E':
                            v = 15;
                            break;
                        default:
                            throw std::invalid_argument("non-digit character included in numeric message");
                            break;
                    }
                    chars[i] = v;
                }
            } else {
                throw std::runtime_error("Invalid message type specified.");
            }
            assert((finallen % 8) == 0);

            for(int i = 0; i < finallen; i += 8) {
                bool continuebit;
                if((i+8) == finallen) {
                    continuebit = false;
                } else {
                    continuebit = true;
                }
                queue_data_block(&chars[i], continuebit);
            }

        }
        void
        gscencode_impl::queue_data_block(unsigned char *blockmsg, bool continuebit) {
            //unsigned char blockmsg[8] = { 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x3e };
            unsigned long infowords[8] = {
                ((unsigned long)blockmsg[0] | ((unsigned long)blockmsg[1] << 6)) & 0x7f,
                (((unsigned long)blockmsg[1] >> 1) | ((unsigned long)blockmsg[2] << 5)) & 0x7f,
                (((unsigned long)blockmsg[2] >> 2) | ((unsigned long)blockmsg[3] << 4)) & 0x7f,
                (((unsigned long)blockmsg[3] >> 3) | ((unsigned long)blockmsg[4] << 3)) & 0x7f,
                (((unsigned long)blockmsg[4] >> 4) | ((unsigned long)blockmsg[5] << 2)) & 0x7f,
                (((unsigned long)blockmsg[5] >> 5) | ((unsigned long)blockmsg[6] << 1)) & 0x7f,
                ((unsigned long)blockmsg[7] | (continuebit << 6)) & 0x7f,
                0,
            };
            unsigned long checkval = 0;
            for(unsigned int i = 0; i < 7; i++) {
                checkval += infowords[i];
            }
            infowords[7] = (checkval & 0x7f);
            printf("infowords: %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx\n", infowords[0], infowords[1], infowords[2], infowords[3], infowords[4], infowords[5], infowords[6], infowords[7]);
            BCH bch(15, 7, 2, bin2oct("1 1 1 0 1 0 0 0 1"), true);
            vector<shared_ptr<bvec> > bvecs;
            for(unsigned int i = 0; i < 8; i++) {
                bvec bv(7);
                uint32_to_bvec_rev(infowords[i], bv, 7);
                bvec p = bch.encode(bv);
                shared_ptr<bvec> fullbv(new bvec(p));
                std::cout << "    bv " << i << ": " << bv << std::endl;
                std::cout << "     p " << i << ": " << p << std::endl;
                std::cout << "fullbv " << i << ": " << *fullbv << std::endl;
                std::cout << std::endl;
                bvecs.push_back(fullbv);
            }

            queuebit(((*bvecs[0])[0]) == 0 ? 1 : 0);       // comma bit
            for(int bit = 0; bit < 15; bit++) {
                for(unsigned int word = 0; word < 8; word++) {
                    queuebit(((*bvecs[word])[bit] == 0) ? 0 : 1);
                    printf("%d ", ((*bvecs[word])[bit] == 0) ? 0 : 1);
                }
                printf("\n");
            }
        }
        void 
        gscencode_impl::calc_pagerid(const unsigned int code, unsigned int &word1, unsigned int &word2, unsigned int &preamble_idx) {
            unsigned int c = code;
            if(c > 999999) {
                throw std::runtime_error("invalid code -- too large!");
            }
            unsigned int i = (c / 100000);
            c -= (i * 100000);
            unsigned int g1 = (c / 10000);
            c -= (g1 * 10000);
            unsigned int g0 = (c / 1000);
            c -= (g0 * 1000);
            unsigned int a2 = (c / 100);
            c -= (a2 * 100);
            unsigned int a1 = (c / 10);
            c -= (a1 * 10);
            unsigned int a0 = c;

            preamble_idx = (i + g0) % 10;
            unsigned int ap = (code % 1000) * 2;
            const unsigned int ap3 = ap / 1000;
            ap -= (ap3 * 1000);
            const unsigned int ap2 = ap / 100;
            ap -= (ap2 * 100);
            const unsigned int ap1 = ap / 10;
            ap -= (ap1 * 10);
            const unsigned int ap0 = ap;

            const unsigned int b1b0 = (ap1 * 10 + ap0) / 2;
            const unsigned int b3b2 = (ap3 * 10 + ap2);
            const unsigned int g1g0 = (g1 * 10 + g0);
            if(g1g0 >= 50) {
                word1 = g1g0 - 50;
                word2 = (b3b2 * 100 + b1b0) + 50;
            } else {
                word1 = g1g0;
                word2 = (b3b2 * 100 + b1b0);
            }
            const unsigned int a2a1a0 = (a2 * 100 + a1 * 10 + a0);
            const unsigned int illegal_low[16] = { 000, 025, 051, 103, 206, 340, 363, 412, 445, 530, 642, 726, 782, 810, 825, 877 };
            const unsigned int illegal_high[7] = { 000, 292, 425, 584, 631, 841, 851 };
            if(g1g0 < 50) {
                for(unsigned int n = 0; n < 16; n++) {
                    if(a2a1a0 == illegal_low[n]) {
                        throw new std::runtime_error("invalid value for a2a1a0");
                    }
                }
            } else {
                for(unsigned int n = 0; n < 7; n++) {
                    if(a2a1a0 == illegal_high[n]) {
                        throw new std::runtime_error("invalid value for a2a1a0");
                    }
                }
            }
            printf("word1: %u  word2: %u  index: %u\n", word1, word2, preamble_idx);
        }


        void
        gscencode_impl::queue_batch() {
            unsigned int code = d_capcode;
            std::cout << "XXX capcode: " << code << std::endl;
            unsigned int word1, word2, preamble;
            calc_pagerid(code, word1, word2, preamble);
            std::cout << "XXX queue_batch:    start: sz " << queuesize() << std::endl;
            queue_preamble(preamble);
            std::cout << "XXX queue_batch: preamble: sz " << queuesize() << std::endl;
            queue_startcode();
            std::cout << "XXX queue_batch:    start: sz " << queuesize() << std::endl;
            queue_address(word1, word2);
            std::cout << "XXX queue_batch:  address: sz " << queuesize() << std::endl;
            queue_message();
            std::cout << "XXX queue_batch:  message: sz " << queuesize() << std::endl;
            queue_comma(121 * 8, 1);
            std::cout << "XXX queue_batch:    TOTAL: sz " << queuesize() << std::endl;
        }
        void
        gscencode_impl::queue_dup_rev(bvec &bvec) {     // XXX: this is ugly, get rid of it
            for(unsigned int i = 0; i < bvec.size(); i++) {
                queuebit(bvec[i] == 0 ? 1 : 0);
                queuebit(bvec[i] == 0 ? 1 : 0);
            }
        }
        void
        gscencode_impl::queue_dup(bvec &bvec) {
            for(unsigned int i = 0; i < bvec.size(); i++) {
                queuebit(bvec[i]);
                queuebit(bvec[i]);
            }
        }

        void 
        gscencode_impl::queue(shared_ptr<bvec> bvptr) {
            for(unsigned int i = 0; i < bvptr->size(); i++) {
                queuebit((*bvptr)[i]);
            }
        }
        void 
        gscencode_impl::queue(uint32_t val) {
            for(int i = 0; i < 32; i++) {
                queuebit(((val & 0x80000000) == 0x80000000) ? 1 : 0);
                val = val << 1;
            }
        }



        gscencode_impl::gscencode_impl(int msgtype, unsigned int capcode, std::string message, unsigned long symrate)
          : d_capcode(capcode), d_msgtype(msgtype), d_message(message), d_symrate(symrate), 
#ifdef GR_OLD
          gr_sync_block("gscencode",
                  gr_make_io_signature(0, 0, 0),
                  gr_make_io_signature(1, 1, sizeof (unsigned char)))
#else 
          sync_block("gscencode",
                  io_signature::make(0, 0, 0),
                  io_signature::make(1, 1, sizeof (unsigned char)))
#endif
        {
            if(d_symrate % 600 != 0) {
                std::cerr << "Output symbol rate must be evenly divisible by fastest baud rate (600)!" << std::endl;
                throw std::runtime_error("Output symbol rate is not evenly divisible by baud rate (600)");
            }
            queue_batch();
        }

        // Insert bits into the queue.  Here is also where we repeat a single bit
        // so that we're emitting d_symrate symbols per second.
        inline void 
        gscencode_impl::queuebit(bool bit) {
            const unsigned int interp = d_symrate / 600;
            for(unsigned int i = 0; i < interp; i++) {
                d_bitqueue.push(bit);
            }
        }

        gscencode_impl::~gscencode_impl()
        {
        }

        // Move data from our internal queue (d_bitqueue) out to gnuradio.  Here 
        // we also convert our data from bits (0 and 1) to symbols (1 and -1).  
        //
        // These symbols are then used by the FM block to generate signals that are
        // +/- the max deviation.  (For POCSAG, that deviation is 4500 Hz.)  All of
        // that is taken care of outside this block; we just emit -1 and 1.

        int
        gscencode_impl::work(int noutput_items,
                  gr_vector_const_void_star &input_items,
                  gr_vector_void_star &output_items) {
            //const float *in = (const float *) input_items[0];
            unsigned char *out = (unsigned char *) output_items[0];

            if(d_bitqueue.empty()) {
                return -1;
            }
            const int toxfer = noutput_items < d_bitqueue.size() ? noutput_items : d_bitqueue.size();
            assert(toxfer >= 0);
            for(int i = 0; i < toxfer ; i++) {
                const bool bbit = d_bitqueue.front();
                switch((int)bbit) {
                    case 0:
                        out[i] = -1;
                        break;
                    case 1:
                        out[i] = 1;
                        break;
                    default:
                        std::cout << "invalid value in bitqueue" << std::endl;
                        abort();
                        break;
                }
                d_bitqueue.pop();
            }
            return toxfer;

        }
    } /* namespace mixalot */
} /* namespace gr */

