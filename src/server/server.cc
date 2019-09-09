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
#include <serialization_iostream.h>
#include <serialization_legacy.h>

#include <ftags.pb.h>

#include <zmq_logger_sink.h>

#include <zmq.hpp>

#include <clara.hpp>

#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>

#include <cstdlib>
#include <ctime>

namespace
{
std::string getTimeStamp()
{
   auto now       = std::chrono::system_clock::now();
   auto in_time_t = std::chrono::system_clock::to_time_t(now);

   std::stringstream ss;
   ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
   return ss.str();
}

void reportUnknownProject(zmq::socket_t&                                 socket,
                          const std::string&                             projectName,
                          const std::map<std::string, ftags::ProjectDb>& projects)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_UNKNOWN_PROJECT);
   status.set_projectname(projectName);

   *status.add_remarks() = "Known projects:";

   for (const auto& iter : projects)
   {
      *status.add_remarks() = fmt::format("{} in {}", iter.second.getName(), iter.second.getRoot());
   }

   const std::size_t headerSize = status.ByteSizeLong();
   zmq::message_t    reply(headerSize);
   status.SerializeToArray(reply.data(), static_cast<int>(headerSize));
   socket.send(reply);
}

void dispatchFind(zmq::socket_t&                socket,
                  const ftags::ProjectDb*       projectDb,
                  ftags::Command_QueryType      queryType,
                  ftags::Command_QueryQualifier queryQualifier,
                  const std::string&            symbolName)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());

   spdlog::info("Received {} {} query for '{}' in project {}",
                ftags::Command_QueryType_Name(queryType),
                ftags::Command::QueryQualifier_Name(queryQualifier),
                symbolName,
                projectDb->getName());
   const std::vector<const ftags::Record*> queryResultsVector = projectDb->findSymbol(symbolName);
   spdlog::info("Found {} occurrences for '{}'", queryResultsVector.size(), symbolName);

   std::string serializedStatus;

   if (queryResultsVector.empty())
   {
      status.set_type(ftags::Status_Type::Status_Type_QUERY_NO_RESULTS);
   }
   else
   {
      status.set_type(ftags::Status_Type::Status_Type_QUERY_RESULTS);
   }

   const std::size_t headerSize = status.ByteSizeLong();
   zmq::message_t    reply(headerSize);
   status.SerializeToArray(reply.data(), static_cast<int>(headerSize));
   socket.send(reply, ZMQ_SNDMORE);

   const ftags::CursorSet queryResultsCursor = projectDb->inflateRecords(queryResultsVector);

   const std::size_t           payloadSize = queryResultsCursor.computeSerializedSize();
   zmq::message_t              resultsMessage(payloadSize);
   ftags::util::BufferInsertor insertor(static_cast<std::byte*>(resultsMessage.data()), payloadSize);
   queryResultsCursor.serialize(insertor.getInsertor());
   socket.send(resultsMessage);
}

void dispatchQueryIdentify(zmq::socket_t&          socket,
                           const ftags::ProjectDb* projectDb,
                           const std::string&      fileName,
                           unsigned                lineNumber,
                           unsigned                columnNumber)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());

   spdlog::info("Received identify {}:{}:{} in project {}", fileName, lineNumber, columnNumber, projectDb->getName());
   const std::vector<const ftags::Record*> queryResultsVector =
      projectDb->identifySymbol(fileName, lineNumber, columnNumber);
   spdlog::info("Found {} records for {}:{}:{}", queryResultsVector.size(), fileName, lineNumber, columnNumber);

   std::string serializedStatus;

   if (queryResultsVector.empty())
   {
      status.set_type(ftags::Status_Type::Status_Type_QUERY_NO_RESULTS);
   }
   else
   {
      status.set_type(ftags::Status_Type::Status_Type_QUERY_RESULTS);
   }

   const std::size_t headerSize = status.ByteSizeLong();
   zmq::message_t    reply(headerSize);
   status.SerializeToArray(reply.data(), static_cast<int>(headerSize));
   socket.send(reply, ZMQ_SNDMORE);

   const ftags::CursorSet queryResultsCursor = projectDb->inflateRecords(queryResultsVector);

   const std::size_t           payloadSize = queryResultsCursor.computeSerializedSize();
   zmq::message_t              resultsMessage(payloadSize);
   ftags::util::BufferInsertor insertor(static_cast<std::byte*>(resultsMessage.data()), payloadSize);
   queryResultsCursor.serialize(insertor.getInsertor());
   socket.send(resultsMessage);
}

void dispatchDumpTranslationUnit(zmq::socket_t&          socket,
                                 const ftags::ProjectDb* projectDb,
                                 const std::string&      fileName)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());

   spdlog::info("Received dump request for {}", fileName);
   const std::vector<const ftags::Record*> queryResultsVector = projectDb->dumpTranslationUnit(fileName);

   std::string serializedStatus;

   if (queryResultsVector.empty())
   {
      status.set_type(ftags::Status_Type::Status_Type_QUERY_NO_RESULTS);
   }
   else
   {
      status.set_type(ftags::Status_Type::Status_Type_QUERY_RESULTS);
   }

   const std::size_t headerSize = status.ByteSizeLong();
   zmq::message_t    reply(headerSize);
   status.SerializeToArray(reply.data(), static_cast<int>(headerSize));
   socket.send(reply, ZMQ_SNDMORE);

   const ftags::CursorSet queryResultsCursor = projectDb->inflateRecords(queryResultsVector);

   const std::size_t           payloadSize = queryResultsCursor.computeSerializedSize();
   zmq::message_t              resultsMessage(payloadSize);
   ftags::util::BufferInsertor insertor(static_cast<std::byte*>(resultsMessage.data()), payloadSize);
   queryResultsCursor.serialize(insertor.getInsertor());
   socket.send(resultsMessage);
}

void dispatchUpdateTranslationUnit(zmq::socket_t& socket, ftags::ProjectDb* projectDb, const std::string& fileName)
{
   zmq::message_t payload;
   socket.recv(&payload);

   spdlog::info("Received {:n} bytes of serialized data for project {}", payload.size(), projectDb->getName());

   ftags::util::BufferExtractor extractor(static_cast<std::byte*>(payload.data()), payload.size());

   ftags::ProjectDb updatedTranslationUnit = ftags::ProjectDb::deserialize(extractor.getExtractor());

   spdlog::info("Data contains {:n} records for {:n} translation units",
                updatedTranslationUnit.getRecordCount(),
                updatedTranslationUnit.getTranslationUnitCount());
   spdlog::info("Data contains {:n} symbols extracted from {:n} files",
                updatedTranslationUnit.getSymbolCount(),
                updatedTranslationUnit.getFilesCount());

   projectDb->assertValid();

   projectDb->updateFrom(fileName, updatedTranslationUnit);

   projectDb->assertValid();

   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_TRANSLATION_UNIT_UPDATED);

   const std::size_t replySize = status.ByteSizeLong();
   zmq::message_t    reply(replySize);
   status.SerializeToArray(reply.data(), static_cast<int>(replySize));

   socket.send(reply);
   spdlog::info("Acknowledged translation unit {}", fileName);
}

void dispatchQueryStatistics(zmq::socket_t&          socket,
                             const ftags::ProjectDb* projectDb,
                             const std::string&      statisticsGroup)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_STATISTICS_REMARKS);

   std::vector<std::string> statisticsRemarks = projectDb->getStatisticsRemarks(statisticsGroup);

   for (const auto& remark : statisticsRemarks)
   {
      *status.add_remarks() = remark;
   }

   const std::size_t replySize = status.ByteSizeLong();
   zmq::message_t    reply(replySize);
   status.SerializeToArray(reply.data(), static_cast<int>(replySize));

   socket.send(reply);
}

void dispatchDataAnalysis(zmq::socket_t& socket, const ftags::ProjectDb* projectDb, const std::string& analysisType)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_STATISTICS_REMARKS);

   std::vector<std::string> statisticsRemarks = projectDb->analyzeData(analysisType);

   for (const auto& remark : statisticsRemarks)
   {
      *status.add_remarks() = remark;
   }

   const std::size_t replySize = status.ByteSizeLong();
   zmq::message_t    reply(replySize);
   status.SerializeToArray(reply.data(), static_cast<int>(replySize));

   socket.send(reply);
}

std::filesystem::path getFtagsCachePath()
{
   const char* xdgCacheHomeDirName = std::getenv("XDG_CACHE_HOME");

   std::filesystem::path xdgCachePath;

   if (xdgCacheHomeDirName == nullptr)
   {
      const char* homeDirName = std::getenv("HOME");
      if (homeDirName == nullptr)
      {
         throw(std::runtime_error("HOME environment variable is not defined"));
      }

      const std::filesystem::path homePath = std::filesystem::path{homeDirName};

      if (!std::filesystem::exists(homePath))
      {
         throw(std::runtime_error(
            fmt::format("HOME environment variable points to an invalid directory {}", homeDirName)));
      }

      xdgCachePath = homePath / ".config";
   }
   else
   {
      xdgCachePath = std::filesystem::path{xdgCacheHomeDirName};
   }

   if (!std::filesystem::exists(xdgCachePath))
   {
      std::error_code ec;

      const bool createdDir = std::filesystem::create_directory(xdgCachePath, ec);
      if (createdDir)
      {
         spdlog::warn("Created missing cache dir {}", xdgCachePath.string());
      }
      else
      {
         const std::string errorMessage{
            fmt::format("Failed to create missing cache directory {}: {}", xdgCachePath.string(), ec.message())};
         spdlog::error(errorMessage);
         throw(std::runtime_error(errorMessage));
      }
   }

   const std::filesystem::path ftagsCachePath = xdgCachePath / "ftags" / "project";

   return ftagsCachePath;
}

std::filesystem::path getProjectSaveLocation(const std::string& projectDirName)
{
   const std::filesystem::path ftagsCachePath = getFtagsCachePath();

   if (!std::filesystem::exists(ftagsCachePath))
   {
      std::error_code ec;

      const bool createdDir = std::filesystem::create_directory(ftagsCachePath, ec);
      if (createdDir)
      {
         spdlog::warn("Created missing ftags cache dir {}", ftagsCachePath.string());
      }
      else
      {
         const std::string errorMessage{fmt::format(
            "Failed to create missing ftags cache directory {}: {}", ftagsCachePath.string(), ec.message())};
         spdlog::error(errorMessage);
         throw(std::runtime_error(errorMessage));
      }
   }

   const std::filesystem::path projectSourcePath{projectDirName};

   const std::filesystem::path projectSaveLocation = ftagsCachePath / projectSourcePath.relative_path();

   return projectSaveLocation;
}

std::vector<std::filesystem::path> getSavedProjects()
{
   std::vector<std::filesystem::path> retval;

   const std::filesystem::path ftagsCachePath = getFtagsCachePath();

   if (!std::filesystem::exists(ftagsCachePath))
   {
      return retval;
   }

   for (auto& entry : std::filesystem::recursive_directory_iterator(ftagsCachePath))
   {
      if (entry.is_regular_file())
      {
         spdlog::debug("Examining {}", entry.path().string());
         if (entry.path().filename().string() == "project.data")
         {
            auto projectPath = std::filesystem::relative(entry.path().parent_path(), ftagsCachePath);
            spdlog::info("Found saved project {}", projectPath.string());
            retval.push_back(entry.path().parent_path());
         }
      }
   }

   spdlog::info("Found {} saved projects", retval.size());

   return retval;
}

void dispatchSaveDatabase(zmq::socket_t&          socket,
                          const ftags::ProjectDb* projectDb,
                          const std::string&      projectName,
                          const std::string&      projectDirectory)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_STATISTICS_REMARKS);

   try
   {
      const std::filesystem::path saveLocation{getProjectSaveLocation(projectDirectory)};

      if (!std::filesystem::exists(saveLocation))
      {
         std::error_code ec;

         const bool createdDir = std::filesystem::create_directories(saveLocation, ec);
         if (createdDir)
         {
            spdlog::warn("Created missing project save location directory {}", saveLocation.string());
         }
         else
         {
            const std::string errorMessage{fmt::format(
               "Failed to create missing project save directory {}: {}", saveLocation.string(), ec.message())};
            spdlog::error(errorMessage);
            throw(std::runtime_error(errorMessage));
         }
      }
      else
      {
         spdlog::info("Found existing save location directory {}", saveLocation.string());
      }

      const auto startSavingTimestamp = std::chrono::steady_clock::now();

      const std::size_t serializedSize{projectDb->computeSerializedSize()};

      const std::filesystem::path saveFile{saveLocation / "project.data"};

      ftags::util::OfstreamSerializationWriter writer{saveFile.string(), serializedSize};

      ftags::util::TypedInsertor insertor{writer};

      projectDb->serialize(insertor);

      const auto endSavingTimestamp = std::chrono::steady_clock::now();

      *status.add_remarks() =
         fmt::format("Saved {} to {} ({:n} bytes)", projectName, saveFile.string(), serializedSize);

      *status.add_remarks() = fmt::format(
         "Save duration: {:n} seconds",
         std::chrono::duration_cast<std::chrono::seconds>(endSavingTimestamp - startSavingTimestamp).count());
   }
   catch (std::exception& ex)
   {
      *status.add_remarks() = fmt::format("Caught exception during save project: {}", ex.what());
   }

   const std::size_t replySize = status.ByteSizeLong();
   zmq::message_t    reply(replySize);
   status.SerializeToArray(reply.data(), static_cast<int>(replySize));

   socket.send(reply);
}

ftags::ProjectDb* dispatchLoadDatabase(zmq::socket_t&                           socket,
                                       const std::string&                       projectName,
                                       const std::string&                       projectDirectory,
                                       std::map<std::string, ftags::ProjectDb>& projects)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_STATISTICS_REMARKS);

   ftags::ProjectDb* retval = nullptr;

   try
   {
      const std::filesystem::path saveLocation{getProjectSaveLocation(projectDirectory)};

      if (!std::filesystem::exists(saveLocation))
      {
         *status.add_remarks() = fmt::format("There is no project saved in {}", saveLocation.string());
      }
      else
      {
         const std::filesystem::path saveFile{saveLocation / "project.data"};

         spdlog::info("Preparing to load data from {}", saveFile.string());

         ftags::util::IfstreamSerializationReader reader{saveFile.string()};

         ftags::util::TypedExtractor extractor{reader};

         const auto startLoadingTimestamp = std::chrono::steady_clock::now();

         ftags::ProjectDb pdb = ftags::ProjectDb::deserialize(extractor);

         const auto endLoadingTimestamp = std::chrono::steady_clock::now();

         auto iter = projects.emplace(projectName, std::move(pdb));

         retval = &iter.first->second;

         spdlog::info("Loaded project from {}", saveFile.string());

         *status.add_remarks() = fmt::format("Loaded {} from disk", projectName);
         *status.add_remarks() = fmt::format(
            "Load duration: {:n} seconds",
            std::chrono::duration_cast<std::chrono::seconds>(endLoadingTimestamp - startLoadingTimestamp).count());
      }
   }
   catch (std::exception& ex)
   {
      *status.add_remarks() = fmt::format("Caught exception during save project: {}", ex.what());
   }

   const std::size_t replySize = status.ByteSizeLong();
   zmq::message_t    reply(replySize);
   status.SerializeToArray(reply.data(), static_cast<int>(replySize));

   socket.send(reply);

   return retval;
}

void dispatchPing(zmq::socket_t& socket)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_IDLE);

   const std::size_t replySize = status.ByteSizeLong();
   zmq::message_t    reply(replySize);
   status.SerializeToArray(reply.data(), static_cast<int>(replySize));

   socket.send(reply);
}

void dispatchUnknownCommand(zmq::socket_t& socket)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_UNKNOWN);

   const std::size_t replySize = status.ByteSizeLong();
   zmq::message_t    reply(replySize);
   status.SerializeToArray(reply.data(), static_cast<int>(replySize));

   socket.send(reply);
}

void dispatchShutdown(zmq::socket_t& socket)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_SHUTTING_DOWN);

   const std::size_t replySize = status.ByteSizeLong();
   zmq::message_t    reply(replySize);
   status.SerializeToArray(reply.data(), static_cast<int>(replySize));

   socket.send(reply);
}

bool showHelp         = false;
bool autoloadProjects = false;

auto cli = clara::Help(showHelp) | clara::Opt(autoloadProjects)["-a"]["--autoload"]("Autoload projects"); // NOLINT

} // namespace

int main(int argc, char* argv[])
{
   try
   {
      GOOGLE_PROTOBUF_VERIFY_VERSION;

      auto result = cli.parse(clara::Args(argc, argv));
      if (!result)
      {
         spdlog::error("Failed to parse command line options: {}", result.errorMessage());
         exit(-1);
      }

      if (showHelp)
      {
         std::cout << cli << std::endl;
         exit(0);
      }

      //  Prepare our context and socket
      zmq::context_t context(1);

      ftags::ZmqCentralLogger centralLogger{context, std::string{"server"}};

      spdlog::info("Started");

      std::map<std::string, ftags::ProjectDb>  projects;
      std::map<std::string, ftags::ProjectDb*> projectsByPath;

      if (autoloadProjects)
      {
         const auto savedProjects = getSavedProjects();

         const auto ftagsCachePath = getFtagsCachePath();

         const std::filesystem::path rootPath{"/"};

         const auto startLoadingTimestamp = std::chrono::steady_clock::now();
         for (const auto& savedProjectData : savedProjects)
         {
            const std::filesystem::path fullProjectPath = ftagsCachePath / savedProjectData / "project.data";

            ftags::util::IfstreamSerializationReader reader{fullProjectPath.string()};

            ftags::util::TypedExtractor extractor{reader};

            ftags::ProjectDb pdb = ftags::ProjectDb::deserialize(extractor);

            spdlog::info(
               "Loaded project {} with root {} from {}", pdb.getName(), pdb.getRoot(), savedProjectData.string());

            const std::string projectRoot = pdb.getRoot();

            auto iter = projects.emplace(pdb.getName(), std::move(pdb));

            projectsByPath.emplace(projectRoot, &iter.first->second);
         }
         const auto endLoadingTimestamp = std::chrono::steady_clock::now();

         spdlog::info(
            "Load duration: {:n} seconds",
            std::chrono::duration_cast<std::chrono::seconds>(endLoadingTimestamp - startLoadingTimestamp).count());
      }

      const char*       xdgRuntimeDir  = std::getenv("XDG_RUNTIME_DIR");
      const std::string socketLocation = fmt::format("ipc://{}/ftags_server", xdgRuntimeDir);

      zmq::socket_t socket(context, ZMQ_REP);
      socket.bind(socketLocation);

      bool shuttingDown = false;

      while (!shuttingDown)
      {
         zmq::message_t request;
         ftags::Command command{};

         //  Wait for next request from client
         socket.recv(&request);
         command.ParseFromArray(request.data(), static_cast<int>(request.size()));
         spdlog::info("Received request from {}: {}", command.source(), command.Type_Name(command.type()));

         ftags::ProjectDb* projectDb = nullptr;

         if (command.projectname().empty())
         {
            if (!command.directoryname().empty())
            {
               std::filesystem::path inputPath{command.directoryname()};

               /*
                * traverse directory up to find a project root
                */
               while ((nullptr == projectDb) && (inputPath != inputPath.root_directory()))
               {
                  auto iter = projectsByPath.find(inputPath.string());
                  if (iter != projectsByPath.end())
                  {
                     projectDb = iter->second;
                  }
                  else
                  {
                     inputPath = inputPath.parent_path();
                  }
               }
            }
         }
         else
         {
            auto iter = projects.find(command.projectname());
            if (iter != projects.end())
            {
               projectDb = &iter->second;
            }
         }

         switch (command.type())
         {

         case ftags::Command_Type::Command_Type_QUERY:
            if (nullptr == projectDb)
            {
               reportUnknownProject(socket, command.projectname(), projects);
            }
            else
            {
               switch (command.querytype())
               {
               case ftags::Command_QueryType::Command_QueryType_IDENTIFY:
                  dispatchQueryIdentify(
                     socket, projectDb, command.filename(), command.linenumber(), command.columnnumber());
                  break;
               default:
                  dispatchFind(
                     socket, projectDb, command.querytype(), command.queryqualifier(), command.symbolname());
                  break;
               }
            }
            break;

         case ftags::Command_Type::Command_Type_DUMP_TRANSLATION_UNIT:
            if (nullptr == projectDb)
            {
               reportUnknownProject(socket, command.projectname(), projects);
            }
            else
            {
               dispatchDumpTranslationUnit(socket, projectDb, command.filename());
            }
            break;

         case ftags::Command_Type::Command_Type_UPDATE_TRANSLATION_UNIT:
            if (nullptr == projectDb)
            {
               spdlog::info(
                  fmt::format("Creating new project: {} in {}", command.projectname(), command.directoryname()));
               auto iter = projects.emplace(command.projectname(),
                                            ftags::ProjectDb(/* name = */ command.projectname(),
                                                             /* rootDirectory = */ command.directoryname()));
               projectDb = &iter.first->second;

               projectsByPath.emplace(command.directoryname(), projectDb);
            }
            dispatchUpdateTranslationUnit(socket, projectDb, command.filename());

#ifndef NDEBUG
            // self-check
            {
               for (const auto& [name, project] : projects)
               {
                  assert(name == project.getName());
               }

               for (const auto& [path, project] : projectsByPath)
               {
                  assert(path == project->getRoot());
               }
            }
#endif

            break;

         case ftags::Command_Type::Command_Type_PING:
            dispatchPing(socket);
            break;

         case ftags::Command_Type::Command_Type_QUERY_STATISTICS:
            if (nullptr == projectDb)
            {
               reportUnknownProject(socket, command.projectname(), projects);
            }
            else
            {
               dispatchQueryStatistics(socket, projectDb, command.symbolname());
            }
            break;

         case ftags::Command_Type::Command_Type_SAVE_DATABASE:
            dispatchSaveDatabase(socket, projectDb, command.projectname(), command.directoryname());
            break;

         case ftags::Command_Type::Command_Type_LOAD_DATABASE: {
            ftags::ProjectDb* newProjectDb =
               dispatchLoadDatabase(socket, command.projectname(), command.directoryname(), projects);
            projectsByPath.emplace(command.directoryname(), newProjectDb);
         }
         break;

         case ftags::Command_Type::Command_Type_ANALYZE_DATA:
            dispatchDataAnalysis(socket, projectDb, command.symbolname());
            break;

         case ftags::Command_Type::Command_Type_SHUT_DOWN:
            dispatchShutdown(socket);
            shuttingDown = true;
            break;

         default:
            dispatchUnknownCommand(socket);
            break;
         }
      }
   }
   catch (std::exception& ex)
   {
      spdlog::error("Caught exception: {}", ex.what());
   }

   spdlog::info("Shutting down");

   return 0;
}
