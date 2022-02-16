#ifndef seque_hpp
#define seque_hpp

/** 
 *
 * Using enums allows compile time checks you are handling all possible
 * return codes
 *
 */
enum QueueRetcode{
  OK=0,
  ERR_QUEUE_FULL,
  ERR_QUEUE_EMPTY,
  ERR_BAD_IDX,
  ERR_NO_SUCH_ELEMENT
};

/**
 *
 * Convenience (laziness) function
 *
 */
bool Q_OK(const QueueRetcode &r);

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
    QueueRetcode front(T &out_item) const;

    /** 
     *
     * Removes the front of the queue, reducing size by 1. No memory is
     * recovered, but the element is irretreivable after this operation.
     */
    QueueRetcode pop();

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
    QueueRetcode pop(T &out_item);

    /**
     *
     * emplaces an element at the back of the queue. no memory is allocated.
     *
     * Fails if size()==N (the static templated size)
     *
     */
    QueueRetcode push(const T item);

    /**
     *
     * returns (symantically) the element at front()+idx, such that if idx==0, front() is returned. If idx==N, the back is returned.
     *
     * Returns ERR_BAD_IDX if idx>N 
     * Returns ERR_NO_SUCH_ELEMENT if idx>size()
     *
     * In any error condition, out_item is not changed.
     */
    QueueRetcode at(const size_t idx, T &out_item ) const;


    /**
     * A possible C++17 implementation of operator[]
     */
    //std::optional<T> operator[](size_t idx){ 
    //T ret; 
    //if (Q_OK(at(idx,ret))){
    //return ret;
    //}
    //return {}
    //}


  private:
    T circle_buf[N];
    size_t idx_front;
    size_t idx_back;
    size_t count;

};


#include "seque_impl.hpp"

#endif
