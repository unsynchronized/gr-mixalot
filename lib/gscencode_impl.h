/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_MIXALOT_GSCENCODE_IMPL_H
#define INCLUDED_MIXALOT_GSCENCODE_IMPL_H

#include <gnuradio/mixalot/gscencode.h>
#include <queue>
#include <itpp/comm/bch.h>

using namespace itpp;
using std::string;
using std::shared_ptr;


namespace gr {
  namespace mixalot {
      
    class gscencode_impl : public gscencode
    {
    private:
        std::queue<bool> d_bitqueue;   // Queue of symbols to be sent out.
        int d_msgtype;                // message type
        unsigned int d_capcode;             // capcode (pager ID)
        unsigned long d_symrate;            // output symbol rate (must be evenly divisible by the baud rate)
        std::string d_message;              // message to send

        inline void queuebit(bool bit);
        inline unsigned long queuesize() { return d_bitqueue.size(); }
        void calc_pagerid(const unsigned int code, unsigned int &word1, unsigned int &word2, unsigned int &preamble_idx);
        void queue_comma(unsigned int nbits, bool startingpolarity);
        void queue_preamble(unsigned int num);
        void queue_startcode();
        void queue_dup(bvec &bv);
        void queue_dup_rev(bvec &bv);
        void queue_address(unsigned int word1, unsigned int word2);
        void queue_message();
        void queue_data_block(unsigned char *blockmsg, bool continuebit);

    public:
      gscencode_impl(int msgtype, unsigned int capcode, std::string message, unsigned long symrate);
      ~gscencode_impl();

      // Where all the action really happens
        void queue_batch();
        void queue(shared_ptr<bvec> bvptr);
        void queue(uint32_t val);
        int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };


  } // namespace mixalot
} // namespace gr

#endif /* INCLUDED_MIXALOT_GSCENCODE_IMPL_H */

