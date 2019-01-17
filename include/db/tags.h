#ifndef DB_TAGS_H_INCLUDED
#define DB_TAGS_H_INCLUDED

#include <string>
#include <vector>

namespace ftags
{

class Cursor
{
public:
   const char* getSymbolNamespace() const;
   const char* getSymbolName() const;
   const char* getFileName() const;

   int getStartLine() const;
   int getEndLine() const;
   int getStartColumn() const;
   int getEndColumn() const;

   bool isDeclaration() const;
   bool isDefinition() const;
   bool isReference() const;
};

class Tags
{
public:
   /*
    * Construction
    */

   Tags();

   void addCursor(const Cursor& cursor);

   void eraseRecords(const std::string& fileName);

   /*
    * Queries
    */

   std::vector<Cursor*> findDeclaration(const std::string& symbolName) const;

   std::vector<Cursor*> findDefinition(const std::string& symbolName) const;

   std::vector<Cursor*> findWhereUsed(Cursor* cursor) const;

   std::vector<Cursor*> findOverloadDefinitions(Cursor* cursor) const;

   Cursor* getDefinition(Cursor* cursor) const;
   Cursor* getDeclaration(Cursor* cursor) const;

   Cursor* followLocation(const char* fileName, int startLine, int startColumn, int endLine, int endColumn) const;

   std::vector<Cursor*> getBaseClasses(Cursor* cursor) const;
   std::vector<Cursor*> getDerivedClasses(Cursor* cursor) const;

   /*
    * Management
    */

   void mergeFrom(const Tags& other);

   /*
    * Serialization
    */

   std::vector<uint8_t> serialize() const;

   static Tags deserialize(const std::vector<uint8_t>& buffer);
};

} // namespace ftags

#endif // DB_TAGS_H_INCLUDED
