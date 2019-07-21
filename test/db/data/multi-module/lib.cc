#include "lib.h"

int test::function(int arg)
{
   if (arg % 2)
   {
      return 3 * arg + 1;
   }
   else
   {
      return arg / 2;
   }
}
