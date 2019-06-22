#include <project.h>

#include <filesystem>
#include <iostream>
#include <vector>

int main(int argc, char* argv[])
{
   if (argc < 2)
   {
      std::cout << "Required file name argument missing" << std::endl;
      return 1;
   }

   std::vector<const char*> arguments;
   arguments.push_back("-Wall");
   arguments.push_back("-Wextra");

   for (int ii = 2; ii < argc; ii ++)
   {
      arguments.push_back(argv[ii]);
   }

   ftags::StringTable     symbolTable;
   ftags::StringTable     fileNameTable;
   ftags::TranslationUnit translationUnit = ftags::TranslationUnit::parse(argv[1], arguments, symbolTable, fileNameTable);

   ftags::ProjectDb tagsDb;
   tagsDb.addTranslationUnit(argv[1], translationUnit);

   tagsDb.dumpRecords(std::cout);

   std::cout << std::endl;

   return 0;
}
