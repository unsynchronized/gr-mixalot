/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_MIXALOT_FLEXENCODE_IMPL_H
#define INCLUDED_MIXALOT_FLEXENCODE_IMPL_H

#include <mixalot/flexencode.h>
#include <queue>
#include <itpp/comm/bch.h>

using namespace itpp;
using std::string;
using boost::shared_ptr;


namespace gr {
  namespace mixalot {
      
    class flexencode_impl : public flexencode
    {
    private:
        std::queue<bool> d_bitqueue;   // Queue of symbols to be sent out.
        msgtype_t d_msgtype;                // message type
        unsigned int d_baudrate;            // baud rate to transmit at -- should be 512, 1200, or 2400 (although others will work!)
        unsigned int d_capcode;             // capcode (pager ID)
        unsigned long d_symrate;            // output symbol rate (must be evenly divisible by the baud rate)
        std::string d_message;              // message to send

        inline void queuebit(bool bit);

    public:
      flexencode_impl();
      ~flexencode_impl();

      // Where all the action really happens
        void queue_batch();
        void queue_flex_batch();
        void queue(shared_ptr<bvec> bvptr);
        void queue(uint32_t val);
        int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };


  } // namespace mixalot
} // namespace gr

#endif /* INCLUDED_MIXALOT_FLEXENCODE_IMPL_H */

