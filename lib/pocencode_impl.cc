/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "pocencode_impl.h"
#include <iostream>
#include <sstream>
#include <vector>
#include "utils.h"

using namespace itpp;
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
        pocencode::sptr
        pocencode::make(int type, unsigned int baudrate, unsigned int capcode, std::string message, unsigned long symrate) {
            return gnuradio::get_initial_sptr (new pocencode_impl(type, baudrate, capcode, message, symrate));
        }


#define POCSAG_SYNCWORD 0x7CD215D8
        //#define POCSAG_IDLEWORD 0x7AC9C197
#define POCSAG_IDLEWORD 0x7A89C197
        void
        pocencode_impl::queue_batch() {
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
        pocencode_impl::queue(shared_ptr<bvec> bvptr) {
            for(unsigned int i = 0; i < bvptr->size(); i++) {
                queuebit((*bvptr)[i]);
            }
        }
        void
        pocencode_impl::queue(uint32_t val) {
            for(int i = 0; i < 32; i++) {
                queuebit(((val & 0x80000000) == 0x80000000) ? 1 : 0);
                val = val << 1;
            }
        }



        pocencode_impl::pocencode_impl(int msgtype, unsigned int baudrate, unsigned int capcode, std::string message, unsigned long symrate)
          : d_baudrate(baudrate), d_capcode(capcode), d_msgtype(msgtype), d_message(message), d_symrate(symrate),
          sync_block("pocencode",
                  io_signature::make(0, 0, 0),
                  io_signature::make(1, 1, sizeof (unsigned char)))
        {
            if(d_symrate % d_baudrate != 0) {
                std::cerr << "Output symbol rate must be evenly divisible by baud rate!" << std::endl;
                throw std::runtime_error("Output symbol rate is not evenly divisible by baud rate");
            }
            queue_batch();
        }

        // Insert bits into the queue.  Here is also where we repeat a single bit
        // so that we're emitting d_symrate symbols per second.
        inline void
        pocencode_impl::queuebit(bool bit) {
            const unsigned int interp = d_symrate / d_baudrate;
            for(unsigned int i = 0; i < interp; i++) {
                d_bitqueue.push(bit);
            }
        }

        pocencode_impl::~pocencode_impl()
        {
        }

        // Move data from our internal queue (d_bitqueue) out to gnuradio.  Here
        // we also convert our data from bits (0 and 1) to symbols (1 and -1).
        //
        // These symbols are then used by the FM block to generate signals that are
        // +/- the max deviation.  (For POCSAG, that deviation is 4500 Hz.)  All of
        // that is taken care of outside this block; we just emit -1 and 1.

        int
        pocencode_impl::work(int noutput_items,
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
