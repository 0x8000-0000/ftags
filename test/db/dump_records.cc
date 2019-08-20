/*
   Copyright 2019 Florin Iucha

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <project.h>

#include <clara.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

void dumpTranslationUnit(const ftags::ProjectDb::TranslationUnit& translationUnit, const std::string& fileName)
{
   std::ofstream out(fileName);

   std::vector<const ftags::Record*> records = translationUnit.getRecords(true);
   for (const ftags::Record* record : records)
   {
      out << record->location.line << ':' << record->location.column << "  " << record->attributes.getRecordFlavor()
          << ' ' << record->attributes.getRecordType() << " >> " << record->symbolNameKey << std::endl;
   }
}

bool        showHelp        = false;
bool        indexEverything = false;
bool        dumpToConsole   = false;
std::string dumpFileName;
std::string projectName = "test";
std::string inputFileName;

auto cli = clara::Help(showHelp) | clara::Opt(projectName, "project")["-p"]["--project"]("Project name") |
           clara::Opt(indexEverything)["-e"]["--everything"]("Index all reachable sources and headers") |
           clara::Opt(dumpFileName, "file")["-f"]["--file"]("Dump to file") |
           clara::Opt(dumpToConsole)["-c"]["--console"]("Dump to console") |
           clara::Arg(inputFileName, "file")("Input file");

int main(int argc, char* argv[])
{
   auto result = cli.parse(clara::Args(argc, argv));
   if (!result)
   {
      std::cerr << "Failed to parse command line options: " << result.errorMessage();
      exit(-1);
   }

   if (showHelp)
   {
      std::cout << cli << std::endl;
      exit(0);
   }

   const std::vector<const char*> arguments = {
      "-Wall",
      "-Wextra",
      "-stdlib=libstdc++",
      "--gcc-toolchain=/usr",
   };

   const auto path = std::filesystem::current_path();

   ftags::ProjectDb                         tagsDb{/* name = */ projectName, /* rootDirectory = */ path.string()};
   const ftags::ProjectDb::TranslationUnit& translationUnit =
      tagsDb.parseOneFile(inputFileName, arguments, indexEverything);

   if (!dumpFileName.empty())
   {
      dumpTranslationUnit(translationUnit, dumpFileName);
   }

   if (dumpToConsole)
   {
      tagsDb.dumpRecords(std::cout);
   }

   std::cout << std::endl;

   return 0;
}
