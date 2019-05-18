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
#include "utils.h"

using namespace itpp;
using std::string;
using std::vector;
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
            
            add_flex_checksum(dw);

            uint32_t encoded = encodeword(reverse_bits32(dw));

            return encoded;
        }

        /**
         * Make an encoded BIW 1 with the given parameters.
         *
         * Returns a reversed 32-bit word (LSB of ret val is the parity bit).
         *
         * NOTE: The second parameter (blockinfo) is the actual value of a, not the number
         * of words (which is a+1)
         */
        uint32_t
        make_biw1(uint32_t priority, uint32_t blockinfo, uint32_t vectorstart, uint32_t carryon, uint32_t collapse) {
            uint32_t dw = 0;
            dw |= (priority & 0xf) << 4;
            dw |= (blockinfo & 0x3) << 8;
            dw |= (vectorstart & 0x3f) << 10;
            dw |= (carryon & 0x3) << 16;
            dw |= (collapse & 0x7) << 18;

            add_flex_checksum(dw);
            uint32_t encoded = encodeword(reverse_bits32(dw));
            std::cout << "XXX BIW1 for encoded: " << std::hex << encoded << std::endl;
            std::cout << "XXX: " << u32tostring(encoded) << std::endl;
            std::cout << "XXX BIW1 rev encoded: " << std::hex << reverse_bits32(encoded) << std::endl;
            std::cout << "XXX: " << u32tostring(reverse_bits32(encoded)) << std::endl;
            return encoded;
        }

        /**
         * Make a short-address word.
         *
         * Returns a reversed 32-bit word (LSB of ret val is the parity bit).
         */
        uint32_t
        make_short_address(uint32_t address) {
            assert(address >= 32769 && address <= 1966080);
            uint32_t dw = 0;
            dw |= (address & 0x1FFFFF);
            uint32_t encoded = encodeword(reverse_bits32(dw));
            return encoded;
        }

        /**
         * Make a numeric vector word.
         *
         * NOTE: nwords is not the total number of words in the message; it's the value
         * to be written into the word (the total number is nwords+1)
         *
         * Returns a reversed 32-bit word (LSB of ret val is the parity bit).
         */
        uint32_t
        make_numeric_vector(uint32_t vector_type, uint32_t message_start, uint32_t nwords, uint32_t cksum) {
            uint32_t dw = 0;
            dw |= (vector_type & 0x7) << 4;
            dw |= (message_start & 0x7f) << 7;
            dw |= (nwords & 0x7) << 14;
            dw |= (cksum & 0xf) << 17;
            add_flex_checksum(dw);
            uint32_t encoded = encodeword(reverse_bits32(dw));
            return encoded;
        }

        /**
         * Make an alphanumeric vector word.
         *
         * NOTE: Unlike with numeric vector words, the nwords parameter is actually the
         * total number of message words.
         *
         * Returns a reversed 32-bit word (LSB of ret val is the parity bit).
         */
        uint32_t
        make_alphanumeric_vector(uint32_t message_start, uint32_t nwords) {
            uint32_t dw = 0;
            dw |= (0x5) << 4;
            dw |= (message_start & 0x7f) << 7;
            dw |= (nwords & 0x7f) << 14;
            add_flex_checksum(dw);
            uint32_t encoded = encodeword(reverse_bits32(dw));
            return encoded;
        }

        void
        interleave(uint32_t *words, uint8_t *interleaved) {
            unsigned int il = 0;
            for(unsigned int i = 0; i < 32; i++) {
                for(unsigned int j = 0; j < 8; j++) {
                    interleaved[il++] = ((words[j] & 0x80000000) == 0x80000000) ? 1 : 0;
                    words[j] = words[j] << 1;
                }
            }
            assert(il == 256);
        }

        void
        flexencode_impl::queue_flex_batch() {
            static const shared_ptr<bvec> bit_sync_1 = get_vec("10101010101010101010101010101010");
            static const shared_ptr<bvec> bs =         get_vec("1010101010101010");
            static const shared_ptr<bvec> bs_inv =     get_vec("0101010101010101");
            static const shared_ptr<bvec> a1 =         get_vec("01111000111100110101100100111001");
            static const shared_ptr<bvec> a1_inv =     get_vec("10000111000011001010011011000110");
            static const shared_ptr<bvec> ar =         get_vec("11001011001000000101100100111001");
            static const shared_ptr<bvec> ar_inv =     get_vec("00110100110111111010011011000110");
            static const shared_ptr<bvec> b          = get_vec("0101010101010101");
            static const shared_ptr<bvec> cblock =     get_vec("1010111011011000010001010001001001111011");
            uint32_t idle_word = 0;
            encodeword(reverse_bits32(idle_word));
            uint32_t idle_word2 = 0x1FFFFF;
            encodeword(reverse_bits32(idle_word2));

            for(unsigned int i = 0; i < 25; i++) {
                queue(bs);
                queue(ar);
                queue(bs_inv);
                queue(ar_inv);
            }

            for(unsigned int frame = 0; frame < 1; frame++) {
                uint32_t fiw = make_fiw(0, frame, 0, 0, 0x0);
                queue(bit_sync_1);
                queue(a1);
                queue(b);
                queue(a1_inv);
                printf("XXX FIW %x\n", fiw);
                queue(fiw);
                queue(cblock);
                printf("XXX before blocks sz %lu\n", d_bitqueue.size());

                vector<uint32_t> addrwords;
                vector<uint32_t> vecwords;
                vector<uint32_t> msgwords;

                addrwords.push_back(make_short_address(1337331 + 32768));

                vector<uint32_t> allwords;
                allwords.push_back(make_biw1(0, 0, 1+addrwords.size(), 0, 0));
                allwords.insert(allwords.end(), addrwords.begin(), addrwords.end());

                uint32_t new_msg_checksum;
                //make_standard_numeric_msg(1, allwords.size()+1, "11111111112222222222333333333301234567890", vecwords, msgwords, new_msg_checksum);
                make_alphanumeric_msg(1, allwords.size()+1, "\nGRAND CENTRAL\nHACK THE PLANET", vecwords, msgwords);
                allwords.insert(allwords.end(), vecwords.begin(), vecwords.end());
                allwords.insert(allwords.end(), msgwords.begin(), msgwords.end());
                while(allwords.size() < 88) {
                    if((allwords.size() % 2) == 0) {
                        allwords.push_back(idle_word);
                    } else {
                        allwords.push_back(idle_word2);
                    }
                }
                auto blockiter = allwords.begin();
                for(unsigned int block = 0; block < 11; block++) {
                    assert(blockiter != allwords.end());
                    uint32_t blockwords[8];
                    uint8_t interleaved[256];
                    for(int i = 0; i < 8; i++) {
                        blockwords[i] = *blockiter;
                        blockiter++;
                    }
                    interleave(blockwords, interleaved);
                    queue(interleaved, 256);
                }


// XXX REMOVE
//                for(unsigned int block = 0; block < 11; block++) {
//                    uint32_t blockwords[8];
//                    uint8_t interleaved[256];
//
//                    uint32_t biw1 = make_biw1(0, 0, 2, 0, 0);
//                    uint32_t addr1 = make_short_address(1337331 + 32768);
//                    
//                    blockwords[0] = biw1;
//                    blockwords[1] = addr1;
//
//
//                    uint32_t msgx = (0x6 << 2) | (0x9 << 6) | (0xc << 10) | (0xc << 14);
//                    uint32_t binsum = (msgx & 0xff)
//                        + ((msgx >> 8) & 0xff)
//                        + ((msgx >> 16) & 0x1f);
//                    binsum &= 0xff;
//                    uint32_t tempsum = (binsum & 0x3f) + ((binsum >> 6) & 0x3);
//                    uint32_t msg_checksum = (~(tempsum) & 0x3f);
//
//                    msgx |= ((msg_checksum >> 4) & 0x3);
//                    uint32_t encoded_msg = encodeword(reverse_bits32(msgx));
//                    std::cout << "XXX encoded_msg: " << u32tostring(encoded_msg) << std::endl;
//
//
//                    
//                    // XXX: TRY nwords 0!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//                    blockwords[2] = make_numeric_vector(3, 3, 0, (msg_checksum & 0xf));
//                    blockwords[3] = encoded_msg;
//                    blockwords[4] = idle_word;
//                    blockwords[5] = idle_word2;
//                    blockwords[6] = idle_word;
//                    blockwords[7] = idle_word2;
//                    interleave(blockwords, interleaved);
//                    queue(interleaved, 256);
//                    printf("XXX after block %u sz %lu\n", block, d_bitqueue.size());
//                }
            }
        }

        void
        flexencode_impl::make_alphanumeric_msg(unsigned int num_address_words, unsigned int message_start, const string msg, vector<uint32_t> &vecwords, vector<uint32_t> &msgwords) {
            assert(num_address_words == 1);     // XXX no long address yet
            const int len = msg.length();
            if(len < 1 || len > 252) {
                std::cerr << "warning: invalid alphanumeric message len: " << len << std::endl;
                return;
            }
            uint32_t msgbuf[85];
            for(int i = 0; i < 85; i++) {
                msgbuf[i] = 0;
            }

            // First, fill in the actual message characters.  
            uint32_t wordidx = 1;
            uint32_t bitidx = 7;
            for(int i = 0; i < len; i++) {
                msgbuf[wordidx] |= ((msg[i] & 0x7f) << bitidx);
                bitidx += 7;
                if(bitidx == 21) {
                    bitidx = 0;
                    wordidx++;
                }
            }
            if(bitidx == 7) {
                msgbuf[wordidx] |= (((0x03) << 7) | ((0x03) << 14));
                wordidx++;
            } else if(bitidx == 14) {
                msgbuf[wordidx] |= ((0x03) << 14);
                wordidx++;
            }
            // Then, calculate the signature (S) over the message.
            // XXX: should we include 0x03 padding in this calculation?
            uint32_t sig = 0;
            for(int i = 1; i < wordidx; i++) {
                sig += (msgbuf[wordidx] & 0x7f);
                sig += ((msgbuf[wordidx] >> 7) & 0x7f);
                sig += ((msgbuf[wordidx] >> 14) & 0x7f);
            }
            sig = ~sig;
            msgbuf[1] |= (sig & 0x7f);

            msgbuf[0] |= (0x3 << 11);       // F = 0b11, 3.8.8.3

            // Now, we calculate the fragment checksum K.
            uint32_t binsum = 0;
            for(uint32_t i = 0; i < wordidx; i++) {
                uint32_t mw = msgbuf[i];
                uint32_t wordsum = (mw & 0xff) + ((mw >> 8) & 0xff) + ((mw >> 16) & 0x1f);
                binsum += wordsum;
            }
            uint32_t msg_checksum = (~(binsum) & 0x3ff);
            msgbuf[0] |= (msg_checksum & 0x3ff);

            uint32_t vecword = make_alphanumeric_vector(message_start, wordidx);
            vecwords.push_back(vecword);
            for(uint32_t i = 0; i < wordidx; i++) {
                msgwords.push_back(encodeword(reverse_bits32(msgbuf[i])));
            }
        }

        void
        flexencode_impl::make_standard_numeric_msg(unsigned int num_address_words, unsigned int message_start, const string msg, vector<uint32_t> &vecwords, vector<uint32_t> &msgwords, uint32_t &checksum) {
            assert(num_address_words == 1);     // XXX no long address yet
            const int len = msg.length();
            if(len < 1 || len > 41) {
                std::cerr << "warning: invalid numeric message len: " << len << std::endl;
                return;
            }
            uint32_t msgbuf[8];
            for(int i = 0; i < 8; i++) {
                msgbuf[i] = 0;
            }
            uint32_t curbit = 2;
            for(int i = 0; i < len; i++) {
                char c = msg[i];
                uint32_t val = 0;
                switch(c) {
                    case '0':
                        val = 0;
                        break;
                    case '1':
                        val = 1;
                        break;
                    case '2':
                        val = 2;
                        break;
                    case '3':
                        val = 3;
                        break;
                    case '4':
                        val = 4;
                        break;
                    case '5':
                        val = 5;
                        break;
                    case '6':
                        val = 6;
                        break;
                    case '7':
                        val = 7;
                        break;
                    case '8':
                        val = 8;
                        break;
                    case '9':
                        val = 9;
                        break;
                    case 'S':
                    case 'A':
                        val = 0xa;
                        break;
                    case 'U':
                    case 'B':
                        val = 0xb;
                        break;
                    case ' ':
                        val = 0xc;
                        break;
                    case '-':
                    case 'C':
                        val = 0xd;
                        break;
                    case ']':
                    case ')':
                    case 'D':
                        val = 0xe;
                        break;
                    case '[':
                    case '(':
                    case 'E':
                        val = 0xf;
                        break;
                    default:
                        std::cerr << "warning: invalid character in message: " << c << std::endl;
                        return;
                }
                const int wordidx = curbit / 21;
                const int bitidx = curbit % 21;
                if((21 - bitidx) >= 4) {
                    msgbuf[wordidx] |= (val << bitidx);
                } else {
                    const int firstpart = 21-bitidx;
                    const uint32_t firstmask = ((uint32_t)~(0)) >> (32-firstpart);
                    msgbuf[wordidx] |= ((val & firstmask) << bitidx);
                    msgbuf[wordidx+1] |= (val >> firstpart);
                }
                curbit += 4;
            }

            // Now, count the number of words.  If curbit is at an even word boundary
            // (e.g. at the 3rd word or the 7th word -- see 3.8.8.1), then we don't need
            // to do any filling.  Otherwise, fill all remaining 4-char blocks with 0xc
            // and all remaining spaces with zeroes (we set these all to 0 initially so
            // this is done for us)

            uint32_t nwords = 0;
            if((curbit % 21) == 0) {
                nwords = curbit / 21;
            } else {
                const uint32_t fillidx = (curbit / 21);
                const uint32_t nspaces = (21-(curbit % 21)) / 4;
                for(uint32_t i = 0; i < nspaces; i++) {
                    const int bitidx = curbit % 21;
                    assert((21-bitidx) >= 4);
                    msgbuf[fillidx] |= (0xc << bitidx);
                    curbit += 4;
                }
                nwords = fillidx+1;
            }
            assert(message_start+nwords <= 88);

            // Now, calculate the checksum according to 3.8.8.1.
            uint32_t binsum = 0;
            for(uint32_t i = 0; i < nwords; i++) {
                uint32_t mw = msgbuf[i];
                uint32_t wordsum = (mw & 0xff) + ((mw >> 8) & 0xff) + ((mw >> 16) & 0x1f);
                binsum += wordsum;
            }
            binsum &= 0xff;
            uint32_t tempsum = (binsum & 0x3f) + ((binsum >> 6) & 0x3);
            uint32_t msg_checksum = (~(tempsum) & 0x3f);

            // Now that we've calculated the checksum, we can fill in the first two bits
            // (which are the two most-significant bits in the checksum value)
            msgbuf[0] |= ((msg_checksum >> 4) & 0x3);

            uint32_t vecword = make_numeric_vector(3, message_start, nwords-1, (msg_checksum & 0xf));

            checksum = msg_checksum;
            vecwords.push_back(vecword);
            for(uint32_t i = 0; i < nwords; i++) {
                msgwords.push_back(encodeword(reverse_bits32(msgbuf[i])));
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
        flexencode_impl::queue(uint8_t *arr, size_t sz) {
            for(size_t i = 0; i < sz; i++) {
                queuebit(arr[i] == 0 ? 0 : 1);
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
          : d_baudrate(1600), d_capcode(425321), d_msgtype(Alpha), d_message("hello"), d_symrate(64000), 
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
