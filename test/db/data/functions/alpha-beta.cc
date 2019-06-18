int alpha(int param);

int beta(int param);

int alpha(int param)
{
   return beta(param) + 2;
}

int beta(int param)
{
   if ((param % 2) == 0)
   {
      return param / 2;
   }
   else
   {
      return 3 * param + 1;
   }
}
