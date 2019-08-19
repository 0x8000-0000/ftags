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

#include <spdlog/spdlog.h>

#include <sstream>

int main(int argc, char* argv[])
{
   if (argc < 2)
   {
      spdlog::error("Compilation database argument missing");
      return -1;
   }

   ftags::ProjectDb projectDb{/* name = */ "test", /* rootDirectory = */ argv[1]};

   ftags::parseProject(argv[1], projectDb);

   {
      std::ostringstream writer;
      projectDb.dumpStats(writer);
      spdlog::info(writer.str());
   }

   return 0;
}
