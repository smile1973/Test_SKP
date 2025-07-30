#ifndef __JSON_H__
#define __JSON_H__

#include <map>
#include <string>
#include <vector>

namespace web {
namespace json {

enum ValueType
{
    kNumber,
    kString,
    kBoolean,
    kArray,
    kObject,
    kNull
};

class Node;

typedef std::string JsonString;
typedef std::vector<Node*> JsonArray;
typedef std::multimap<JsonString, Node*> JsonObject;

class Parser
{
public:
    //bool ParseFromBig5(const std::string& string, Node** node);
    bool Parse(const std::string& string, Node** node);
    bool Parse(const char* start, const char* end, Node** node);

private:
    void ParseArray(JsonArray* json_array);
    void ParseObject(JsonObject* json_object);
    void ParseValue(Node** node);
    void ParseString(JsonString* json_string);
    void PassSpace();
    void FindSymbol();
    void FindStringSymbol();
    void PassSpaceAndCheckTermination();

    const char* start_;
    const char* end_;
    const char* ptr_;
};

class Node
{
public:
    Node(ValueType value_type);
    ~Node();

    ValueType GetVlaueType() const { return value_type_; }
    JsonString* GetString();
    bool GetBoolean() const;
    JsonArray* GetArray();
    JsonObject* GetObj();

private:
    friend Parser;

    Node(const Node&);
    Node& operator=(const Node&);

    ValueType value_type_;
    void* value_;
};

}  // namespace json
}  // namespace web

#endif