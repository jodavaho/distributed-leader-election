/**
 *
 * @file agent.h
 * @brief provides le::ghs::agent_t defintion
 *
 */
#ifndef GHS_AGENT
#define GHS_AGENT

namespace le{
  namespace ghs{

    /// @typedef le::ghs::agent_t 
    /// The ghs implementation supports arbitrary integers, as long as they are
    /// greater than or equal to zero. Use is_valid() to ensure an agent_t will
    //not generate
    /// problems for GhsState
    typedef int agent_t;

    /** 
     * This means not set 
     */
    const agent_t NO_AGENT=-1;

    /**
     * Just in case you want an official alias for a>=0.
     *
     * @return true if a>=0
     * @return false if a==NO_AGENT or is otherwise <0
     */
    bool is_valid(const agent_t a);

  }
}
#endif
