#include <project.h>

#include <filesystem>
#include <iostream>
#include <vector>

int main()
{
   auto path = std::filesystem::current_path();

   auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";

   std::vector<const char*> arguments;
   arguments.push_back("-Wall");
   arguments.push_back("-Wextra");
   arguments.push_back("-stdlib=libstdc++");
   arguments.push_back("--gcc-toolchain=/usr");

   ftags::StringTable     symbolTable;
   ftags::StringTable     fileNameTable;
   ftags::TranslationUnit helloCpp = ftags::TranslationUnit::parse(helloPath, arguments, symbolTable, fileNameTable);

   ftags::ProjectDb tagsDb;
   tagsDb.addTranslationUnit(helloPath, helloCpp);

   tagsDb.dumpRecords(std::cout);

   std::cout << std::endl;

   return 0;
}
