/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "flexencode_impl.h"
#include <iostream>
#include <sstream>
#include <vector>
#include "utils.h"

using namespace itpp;
using std::string;
using boost::shared_ptr;


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

        flexencode::sptr
        flexencode::make() {
            return gnuradio::get_initial_sptr (new flexencode_impl());
        }
        std::string
        u32tostring(unsigned int x) {
            std::stringstream ss;
            for(unsigned int i = 0; i < 32; i++) {
            if((x & 0x80000000) == 0x80000000) {
                    ss << '1';
                } else {
                    ss << '0';
                }
                x <<= 1;
            }
            return ss.str();
        }

        /**
         * Add a 4-bit ("x"-style) checksum to the lower 4 bits of dw.
         * See section 3.8.1 in the flex spec.
         */
        void
        add_flex_checksum(uint32_t &dw) {
            uint32_t cksum = 
                ((dw >> 4) & 0xf)
                + ((dw >> 8) & 0xf)
                + ((dw >> 12) & 0xf)
                + ((dw >> 16) & 0xf)
                + ((dw >> 20) & 1);
            cksum = ~cksum;
            dw |= (cksum & 0xf);
        }


        /**
         * Make an encoded FIW with the given parameters.
         *
         * The resulting 32-bit word includes checksum and parity, and is reversed. So, the
         * MSB of the return value here is actually bit 1 (LSB of x, aka x0 in section
         * 3.8.3), and the LSB of the return value here is the parity bit.
         */
        uint32_t
        make_fiw(uint32_t cycle, uint32_t frame, uint32_t roaming, uint32_t repeat, uint32_t t) {
            uint32_t dw = 0;
            dw |= (cycle & 0xf) << 4;
            dw |= (frame & 0x7f) << 8;
            dw |= (roaming & 1) << 15;
            dw |= (repeat & 1) << 16;
            dw |= (t & 0xf) << 17;
            
            std::cout << "XXX for dw: " << std::hex << dw << std::endl;
            std::cout << "XXX: " << u32tostring(dw) << std::endl;
            std::cout << "XXX rev dw: " << std::hex << reverse_bits32(dw) << std::endl;
            std::cout << "XXX: " << u32tostring(reverse_bits32(dw)) << std::endl;

            add_flex_checksum(dw);

            std::cout << "XXX for dw: " << std::hex << dw << std::endl;
            std::cout << "XXX: " << u32tostring(dw) << std::endl;
            std::cout << "XXX rev dw: " << std::hex << reverse_bits32(dw) << std::endl;
            std::cout << "XXX: " << u32tostring(reverse_bits32(dw)) << std::endl;

            uint32_t encoded = encodeword(reverse_bits32(dw));
            std::cout << "XXX for encoded: " << std::hex << encoded << std::endl;
            std::cout << "XXX: " << u32tostring(encoded) << std::endl;
            std::cout << "XXX rev encoded: " << std::hex << reverse_bits32(encoded) << std::endl;
            std::cout << "XXX: " << u32tostring(reverse_bits32(encoded)) << std::endl;

            return encoded;
        }

        void
        flexencode_impl::queue_flex_batch() {
            static const shared_ptr<bvec> bit_sync_1 = get_vec("10101010101010101010101010101010");
            static const shared_ptr<bvec> a1 =         get_vec("01111000111100110101100100111001");
            static const shared_ptr<bvec> b          = get_vec("0101010101010101");
            static const shared_ptr<bvec> a1_inv =     get_vec("10000111000011001010011011000110");
            static const shared_ptr<bvec> cblock =     get_vec("1010111011011000010001010001001001111011");

            for(unsigned int frame = 0; frame < 10; frame++) {
                uint32_t fiw = make_fiw(0, frame, 0, 0, 0x0);
                queue(bit_sync_1);
                queue(a1);
                queue(b);
                queue(a1_inv);
                queue(fiw);
            }
        }


        // XXX: REMOVE
#define POCSAG_SYNCWORD 0x7CD215D8
#define POCSAG_IDLEWORD 0x7A89C197
        void 
        flexencode_impl::queue_batch() {
            std::vector<uint32_t> msgwords;
            uint32_t functionbits = 0;
            switch(d_msgtype) {
                case Numeric:
                    make_numeric_message(d_message, msgwords);
                    functionbits = 0;
                    break;
                case Alpha:
                    make_alpha_message(d_message, msgwords);
                    functionbits = 3;
                    break;
                default:
                    throw std::runtime_error("Invalid message type specified.");
            }
            msgwords.push_back(POCSAG_IDLEWORD);

            static const shared_ptr<bvec> preamble = get_vec("101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010");
            const uint32_t addrtemp = (d_capcode >> 3) << 13 | ((functionbits & 3) << 11);
            const uint32_t addrword = encodeword(addrtemp);
            const uint32_t frameoffset = d_capcode & 7;

            assert((addrword & 0xFFFFF800) == addrtemp);

            queue(preamble);
            queue(POCSAG_SYNCWORD);
            
            for(int i = 0; i < frameoffset; i++) {
                queue(POCSAG_IDLEWORD);
                queue(POCSAG_IDLEWORD);
            }
            queue(addrword);
            std::vector<uint32_t>::iterator it = msgwords.begin();

            for(int i = (frameoffset * 2)+1; i < 16; i++) {
                if(it != msgwords.end()) {
                    queue(*it);
                    it++;
                } else {
                    queue(POCSAG_IDLEWORD);
                }
            }
            while(it != msgwords.end()) {
                queue(POCSAG_SYNCWORD);
                for(int i = 0; i < 16; i++) {
                    if(it != msgwords.end()) {
                        queue(*it);
                        it++;
                    } else {
                        queue(POCSAG_IDLEWORD);
                    }
                }
            }
        }

        void 
        flexencode_impl::queue(shared_ptr<bvec> bvptr) {
            for(unsigned int i = 0; i < bvptr->size(); i++) {
                queuebit((*bvptr)[i]);
            }
        }
        void 
        flexencode_impl::queue(uint32_t val) {
            for(int i = 0; i < 32; i++) {
                queuebit(((val & 0x80000000) == 0x80000000) ? 1 : 0);
                val = val << 1;
            }
        }



        flexencode_impl::flexencode_impl()
          : d_baudrate(1600), d_capcode(425321), d_msgtype(Alpha), d_message("hello"), d_symrate(6400), 
          sync_block("flexencode",
                  io_signature::make(0, 0, 0),
                  io_signature::make(1, 1, sizeof (unsigned char)))
        {
            if(d_symrate % d_baudrate != 0) {
                std::cerr << "Output symbol rate must be evenly divisible by baud rate!" << std::endl;
                throw std::runtime_error("Output symbol rate is not evenly divisible by baud rate");
            }
            queue_flex_batch();
        }

        // Insert bits into the queue.  Here is also where we repeat a single bit
        // so that we're emitting d_symrate symbols per second.
        inline void 
        flexencode_impl::queuebit(bool bit) {
            const unsigned int interp = d_symrate / d_baudrate;
            for(unsigned int i = 0; i < interp; i++) {
                d_bitqueue.push(bit);
            }
        }

        flexencode_impl::~flexencode_impl()
        {
        }

        // Move data from our internal queue (d_bitqueue) out to gnuradio.  Here 
        // we also convert our data from bits (0 and 1) to symbols (1 and -1).  
        //
        // These symbols are then used by the FM block to generate signals that are
        // +/- the max deviation.  (For POCSAG, that deviation is 4500 Hz.)  All of
        // that is taken care of outside this block; we just emit -1 and 1.

        int
        flexencode_impl::work(int noutput_items,
                  gr_vector_const_void_star &input_items,
                  gr_vector_void_star &output_items) {
            const float *in = (const float *) input_items[0];
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
                        out[i] = 1;
                        break;
                    case 1:
                        out[i] = -1;
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

