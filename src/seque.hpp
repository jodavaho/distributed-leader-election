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
  ERR_QUEUE_EMPTY
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

    StaticQueue();
    ~StaticQueue();

    bool is_full() const;
    bool is_empty() const;
    size_t size() const;
    QueueRetcode front(T &out_item) const;

    QueueRetcode pop();
    QueueRetcode pop(T &out_item);
    QueueRetcode push(const T item);

  private:
    T circle_buf[N];
    size_t idx_front;
    size_t idx_back;
    size_t count;

};


#include "seque_impl.hpp"

#endif
