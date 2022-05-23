#ifndef GHS_ASSERT
#define GHS_ASSERT

//replace me
#include <cassert>
#include <stdexcept>
#define ghs_assert(x) assert(x)

#ifndef NDEBUG
#define ghs_fatal(x) throw std::runtime_error(x)
#else
#define ghs_fatal(x) if(false){}

#endif //NDEBUG
#endif //GHS_ASSERT
