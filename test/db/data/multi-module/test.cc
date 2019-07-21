#include <iostream>
#include <string>

#include "lib.h"

int main(int argc, char* argv[])
{
   if (argc < 3)
   {
      return -1;
   }

   const int count = std::stoi(argv[1]);
   int arg = std::stoi(argv[2]);

   for (int ii = 0; ii < count; ++ii)
   {
      arg = test::function(arg);
   }

   std::cout << arg << std::endl;

   return 0;
}
