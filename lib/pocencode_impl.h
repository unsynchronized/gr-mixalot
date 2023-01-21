/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_MIXALOT_POCENCODE_IMPL_H
#define INCLUDED_MIXALOT_POCENCODE_IMPL_H

#include <gnuradio/mixalot/pocencode.h>
#include <queue>
#include <itpp/comm/bch.h>

using namespace itpp;
using std::string;
using std::shared_ptr;


namespace gr {
  namespace mixalot {

    class pocencode_impl : public pocencode
    {
    private:
        std::queue<bool> d_bitqueue;   // Queue of symbols to be sent out.
        int d_msgtype;                // message type
        unsigned int d_baudrate;            // baud rate to transmit at -- should be 512, 1200, or 2400 (although others will work!)
        unsigned int d_capcode;             // capcode (pager ID)
        unsigned long d_symrate;            // output symbol rate (must be evenly divisible by the baud rate)
        std::string d_message;              // message to send

        inline void queuebit(bool bit);

    public:
      pocencode_impl(int msgtype, unsigned int baudrate, unsigned int capcode, std::string message, unsigned long symrate);
      ~pocencode_impl();

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

#endif /* INCLUDED_MIXALOT_POCENCODE_IMPL_H */
