/**
 *   Copyright (c) 2021 California Institute of Technology (“Caltech”). 
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
 * @file static_queue_impl.hpp
 * @brief Static Queue Implementation
 */



template <typename T, std::size_t N>
StaticQueue<T,N>::StaticQueue():
  idx_front(0), idx_back(0), count(0)
{
}

template <typename T, std::size_t N>
StaticQueue<T,N>::~StaticQueue()
{
}

template <typename T, std::size_t N>
bool StaticQueue<T,N>::is_full() const{
  return size()==N;
}

template <typename T, std::size_t N>
bool StaticQueue<T,N>::is_empty() const{
  return size()==0;;
}

template <typename T, std::size_t N>
size_t StaticQueue<T,N>::size() const{
  return count;
}

template <typename T, std::size_t N>
Retcode StaticQueue<T,N>::push(const T item){
  if (is_full())
  {
    return ERR_QUEUE_FULL;
  }

  circle_buf[idx_back]=item;

  count ++;

  //ideally, idx_back=(idx_back+1)%N
  //but we can't assume N = 2^n in general
  idx_back ++;
  if (idx_back >= N)
  {
    idx_back = 0;
  } 

  return OK;
}

template <typename T, std::size_t N>
Retcode StaticQueue<T,N>::front(T &out_item) const {
  return at(0,out_item);
}

template <typename T, std::size_t N>
Retcode StaticQueue<T,N>::pop(){
  if (is_empty())
  {
    return ERR_QUEUE_EMPTY;
  }

  count --;

  //ideally, idx_back=(idx_back+1)%N
  //but we can't assume N = 2^n in general
  idx_front ++;
  if (idx_front >= N)
  {
    idx_front = 0;
  }
  return OK;
}

template <typename T, std::size_t N>
Retcode StaticQueue<T,N>::pop(T &out_item){
  auto r = front(out_item);
  if (!Q_OK(r)){
    return r;
  }
  r = pop();
  return r;
}


template <typename T, std::size_t N>
Retcode StaticQueue<T,N>::at(const size_t idx, T &out_item) const
{

  if (size()==0){
    return ERR_QUEUE_EMPTY;
  }

  if (idx>=N){
    return ERR_BAD_IDX;
  }

  if (idx>=size()){
    return ERR_NO_SUCH_ELEMENT;
  }

  size_t actual_idx = idx+idx_front;
  if (actual_idx > N){
    actual_idx -= N;
  }

  out_item = circle_buf[actual_idx];
  return OK;

}


template <typename T, std::size_t N>
Retcode StaticQueue<T,N>::clear()
{
  idx_front=0;
  idx_back =0;
  count    =0;
  return OK;
}
