#ifndef GHS_ASSERT
#define GHS_ASSERT


#ifndef NDEBUG

#include <stdexcept>
#define fatal(x) throw std::runtime_error(x)
#define assert(x) if ( !(x) ) { fatal("x failed!"); }

#else
#define fatal(x) if(false){}
#define assert(x) if(false){}

#endif //NDEBUG
#endif //GHS_ASSERT
