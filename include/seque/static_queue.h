/**
 * @file static_queue.h
 * @brief a static-sized single-ended queue for use in GhsState
 */
#ifndef STATIC_QUEUE_H
#define STATIC_QUEUE_H

#include <cstdlib>

/**
 * This namespace presents a Single Ended QUEue implementation that is part of the I/O with le::ghs::GhsState objects
 */
namespace seque{

  /// 
  /// An enum class for StaticQueue error codes
  ///
  enum Retcode{
    OK=0,///< OK
    ERR_QUEUE_FULL,///< Operation failed, the queue is full
    ERR_QUEUE_EMPTY,///< Operation failed, the queue is empty
    ERR_BAD_IDX,///< Operation failed and is not possible to succeed: that idx is beyond the static size of the queue
    ERR_NO_SUCH_ELEMENT,///< Operation failed, there are less elements than the given index in the queue
  };

  /// Convenience (laziness) function
  ///
  bool Q_OK(const seque::Retcode &r);

  /** 
   * Return a human-readable string corresponding to the error code 
   * @param r a seque::Retcode
   * @return a human-readable string, useful for logging
   */
  const char* strerr(const seque::Retcode &r);


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
  template<typename T, std::size_t N>
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
        size_t size() const;

        /**
         * Sets the given reference to be identical to the front of the queue, but
         * does not alter the elements of the queue in any way
         *
         * In any error condition, out_item is not changed.
         *
         */
        Retcode front(T &out_item) const;

        /** 
         *
         * Removes the front of the queue, reducing size by 1. No memory is
         * recovered, but the element is irretreivable after this operation.
         */
        Retcode pop();

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
        Retcode pop(T &out_item);

        /**
         *
         * emplaces an element at the back of the queue. no memory is allocated.
         *
         * Fails if size()==N (the static templated size)
         *
         */
        Retcode push(const T item);

        /**
         *
         * returns (symantically) the element at front()+idx, such that if idx==0, front() is returned. If idx==N, the back is returned.
         *
         * Returns ERR_BAD_IDX if idx>N 
         * Returns ERR_NO_SUCH_ELEMENT if idx>size()
         *
         * In any error condition, out_item is not changed.
         */
        Retcode at(const size_t idx, T &out_item ) const;

        Retcode clear();



      private:
        T circle_buf[N];
        size_t idx_front;
        size_t idx_back;
        size_t count;

    };


#include "seque/static_queue_impl.hpp"

}

#endif
