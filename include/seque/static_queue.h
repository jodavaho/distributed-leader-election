/**
 * 
 *   Copyright (c) 2022 California Institute of Technology (“Caltech”). 
 *   U.S.  Government sponsorship acknowledged.
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *    * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    *  Neither the name of Caltech nor its operating division, the Jet
 *    Propulsion Laboratory, nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file static_queue.h
 * @brief a static-sized single-ended queue for use in GhsState
 */
#ifndef STATIC_QUEUE_H
#define STATIC_QUEUE_H

#include "le/errno.h"

/**
 * This namespace presents a Single Ended QUEue implementation that is part of the I/O with le::ghs::GhsState objects
 */
namespace seque{

  /**
   *
   * @brief a static-sized single-ended queue for use in GhsState
   *
   * The StaticQueue class is a statically-sized, single-ended queue based on a cirlce buffer
   * that is used to present and queue messags for the GhsState class
   *
   * @param T a typename of the object to store
   * @param N the number of elements to allocat storage for
   */
  template<typename T, unsigned int N>
    class StaticQueue
    {
      public:



        /**
         * Constructs with static-sizing a circle-buf-backed queue
         */
        StaticQueue();
        ~StaticQueue();

        /**
         * Returns true if size()==N, and no more elements can be push()'d
         */
        bool is_full() const;

        /**
         * Returns true if size()==0, and no more elements can be pop()'d or checked via front()
         */
        bool is_empty() const;

        /**
         * Returns the current number of elements in the queue
         */
        unsigned int size() const;

        /**
         * Sets the given reference to be identical to the front of the queue, but
         * does not alter the elements of the queue in any way
         *
         * In any error condition, out_item is not changed.
         *
         */
        le::Errno front(T &out_item) const;

        /** 
         *
         * Removes the front of the queue, reducing size by 1. No memory is
         * recovered, but the element is irretreivable after this operation.
         */
        le::Errno pop();

        /**
         *
         * This convenience function is identical to:
         * ```
         * T item;
         * front(T);
         * pop();
         * return T;
         * ```
         *
         * It will return an error if either front() or pop() would. 
         * In any error condition, out_item is not changed.
         *
         */
        le::Errno pop(T &out_item);

        /**
         *
         * emplaces an element at the back of the queue. no memory is allocated.
         *
         * Fails if size()==N (the static templated size)
         *
         */
        le::Errno push(const T item);

        /**
         *
         * returns (symantically) the element at front()+idx, such that if idx==0, front() is returned. If idx==N, the back is returned.
         *
         * Returns ERR_BAD_IDX if idx>N 
         * Returns ERR_NO_SUCH_ELEMENT if idx>size()
         *
         * In any error condition, out_item is not changed.
         */
        le::Errno at(const unsigned int idx, T &out_item ) const;

        le::Errno clear();



      private:
        T circle_buf[N];
        unsigned int idx_front;
        unsigned int idx_back;
        unsigned int count;

    };


#include "seque/static_queue_impl.hpp"

}

#endif
