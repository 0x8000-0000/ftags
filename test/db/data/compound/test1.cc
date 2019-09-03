#include <cstdio>
#include <cstdlib>

namespace
{

bool isEven(int value)
{
   return (value % 2) == 0;
}

/*
 * https://en.wikipedia.org/wiki/Collatz_conjecture
 */
int countCollatzSteps(int arg)
{
   int steps = 0;

   while (arg != 1)
   {
      if (isEven(arg))
      {
         arg = arg / 2;
      }
      else
      {
         arg = 3 * arg + 1;
      }
   }

   return steps;
}

} // namespace

int main(int argc, char* argv[])
{
   if (argc < 2)
   {
      return -1;
   }

   const int param = atoi(argv[1]);

   const int result = countCollatzSteps(param);

   printf("%d\n", result);

   return 0;
}
