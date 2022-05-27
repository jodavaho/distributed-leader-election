#include <cstdlib>//size_t
#include "seque/seque.hpp"

bool Q_OK(const QueueRetcode &r)
{
  return r==OK;
}

namespace seque{
  const char* strerr(const QueueRetcode &r)
  {
    switch (r)
    {
      case OK:
        {
          return "Queue OK";
        }
      case ERR_QUEUE_FULL:
        {
          return "Queue FULL";
        }
      case ERR_QUEUE_EMPTY:
        {
          return "Queue EMPTY";
        }
      case ERR_BAD_IDX:
        {
          return "Queue IDX >= underlying buffer size";
        }
      case ERR_NO_SUCH_ELEMENT:
        {
          return "Queue IDX >= size()";
        }
      default:
        {
          return "Unknown/Unimplemented error code";
        }
    }

  }
}
