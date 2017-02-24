#include <string>
#include <iostream>
#include "Error.h"

namespace tamm{

 void Error(const std::string error_msg) {
     std::cerr << "Error: " << error_msg << std::endl;
     exit(1);
    }

 void Error(const int line, const int position, const std::string error_msg) {
     std::cerr << "Error at Line: " << line << "Column: " << position << ": " << error_msg << std::endl;
     exit(1);
    }

}
