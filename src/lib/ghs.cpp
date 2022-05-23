#include "ghs/ghs.hpp"
#include <cassert>

bool GhsOK(const GhsError &e){ return e>=0; }

void GhsAssert(const GhsError&e){ assert(GhsOK(e)); }

