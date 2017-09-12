#ifndef TAMMX_ERRORS_H_
#define TAMMX_ERRORS_H_

#include <cassert>

namespace tammx {

#define NOT_IMPLEMENTED() do {                        \
  std::cout<<"ERROR: "                                \
           <<"file:"<<__FILE__                        \
           <<"function:"<<__func__                    \
           <<" line:"<<__LINE__                       \
           <<". This is not implemented\n";           \
  } while(0)

#define UNREACHABLE() do {                            \
    std::cout<<"ERROR: "                              \
             <<"file:"<<__FILE__                      \
             <<"function:"<<__func__                  \
             <<" line:"<<__LINE__                     \
             <<". This line should be unreachable\n"; \
  } while(0)

#define EXPECTS(cond) assert(cond)

} //namespace tammx

#endif // TAMMX_ERRORS_H_

