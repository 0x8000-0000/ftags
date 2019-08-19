#include <project.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

void dumpTranslationUnit(const ftags::ProjectDb::TranslationUnit& translationUnit, const std::string& fileName)
{
#if 0
   std::ofstream out(fileName);

   std::vector<const ftags::Record*> records = translationUnit.getRecords(true);
   for (const ftags::Record* record : records)
   {
      out << record->startLine << ':' << record->startColumn << "  " << record->attributes.getRecordFlavor() << ' '
          << record->attributes.getRecordType() << " >> " << record->symbolNameKey << std::endl;
   }
#else
   (void)translationUnit;
   (void)fileName;
#endif
}

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
   ftags::ProjectDb::TranslationUnit translationUnit =
      ftags::ProjectDb::TranslationUnit::parse(argv[1], arguments, symbolTable, fileNameTable);

   ftags::ProjectDb              tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};
   const ftags::ProjectDb::TranslationUnit& mergedTranslationUnit =
      tagsDb.addTranslationUnit(argv[1], translationUnit, symbolTable, fileNameTable);

   dumpTranslationUnit(translationUnit, std::string(argv[1]) + ".dump.orig");
   dumpTranslationUnit(mergedTranslationUnit, std::string(argv[1]) + ".dump.merged");

   tagsDb.dumpRecords(std::cout);

   std::cout << std::endl;

   return 0;
}
