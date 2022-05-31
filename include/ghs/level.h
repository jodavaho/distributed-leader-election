/**
 * @file level.h
 * @brief defintion for le::ghs::level_t 
 *
 */
#ifndef GHS_LEVEL
#define GHS_LEVEL

namespace le{
  namespace ghs{

    /**
     * @brief A "level" which is an internal item for GhsState to track how many times the MST has merged with another
     */
    typedef int level_t;

    /**
     * @brief All levels start at 0
     */
    const level_t LEVEL_START=0;
  }
}
#endif 
