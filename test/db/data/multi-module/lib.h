#ifndef TEST_DB_DATA_MULTIMODULE_LIB_H
#define TEST_DB_DATA_MULTIMODULE_LIB_H

#define DOUBLY_SO(X) ((X)*2)

namespace test
{

using argument_type = int;

int function(int arg);

int other_function(argument_type input);

} // namespace test

#endif // TEST_DB_DATA_MULTIMODULE_LIB_H
