#include <ftags.pb.h>

#include <iostream>

int main()
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   ftags::Greeting greeting;

   std::cout << "Greeting size: " << sizeof(greeting) << std::endl;

   return 0;
}
