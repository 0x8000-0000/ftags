#include <cstdlib>

#include "lib.h"

int main(int argc, char* argv[])
{
   if (argc < 3)
   {
      return -1;
   }

   const int count = atoi(argv[1]);
   int       arg   = atoi(argv[2]);

   for (int ii = 0; ii < count; ++ii)
   {
      arg = test::function(arg);
   }

   return DOUBLY_SO(arg);
}
