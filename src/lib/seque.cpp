#include <cstdlib>//size_t
#include "seque/static_queue.h"

using seque::Retcode;

namespace seque{
  bool Q_OK(const Retcode &r)
  {
    return r==OK;
  }

  const char* strerr(const Retcode &r)
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
