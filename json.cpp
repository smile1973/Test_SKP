#include "json.h"
#include "Utility.h"

#include <stdexcept>

namespace web {
namespace json {

const char UNEXPECTED_SYMBOL[] = "Unexpected symbol";
const char SYMBOLS_NOT_IN_PAIRS[] = "Symbols not in pairs";
const char UNEXPECTED_TERMINATION[] = "Unexpected termination";

//bool Parser::ParseFromBig5(const std::string& string, Node** node)
//{
//    std::string utf8_string = CUtility::ConvertBig5ToUtf8(string.c_str());
//    return Parse(utf8_string.c_str(), utf8_string.c_str() + utf8_string.length(), node);
//}

bool Parser::Parse(const std::string& string, Node** node)
{
    return Parse(string.c_str(), string.c_str() + string.size(), node);
}

bool Parser::Parse(const char* start, const char* end, Node** node)
{
    ptr_ = start_ = start;
    end_ = end;
    Node* temp_node = NULL;

    try
    {
        PassSpaceAndCheckTermination();
        switch (*ptr_)
        {
        case '[':
            temp_node = new Node(kArray);
            ParseArray(temp_node->GetArray());
            break;
        case '{':
            temp_node = new Node(kObject);
            ParseObject(temp_node->GetObj());
            break;
        default:
            throw std::logic_error(UNEXPECTED_SYMBOL);
        }
        PassSpace();
        if (ptr_ != end_)
            throw std::logic_error(UNEXPECTED_SYMBOL);
    }
    catch (std::exception)
    {
        if (temp_node)
            delete temp_node;
        return false;
    }
    *node = temp_node;
    return true;
}

void Parser::ParseArray(JsonArray* json_array)
{
    enum Status
    {
        kNull,
        kValue,
        kComma, //,
    } status = kNull;

    ++ptr_;
    PassSpace();
    while (ptr_ < end_)
    {
        switch (*ptr_)
        {
        case '[':
            if (status != kNull && status != kComma)
                throw std::logic_error(UNEXPECTED_SYMBOL);
            json_array->push_back(new Node(kArray));
            ParseArray(json_array->back()->GetArray());
            break;
        case ']':
            if (status != kNull && status != kValue)
                throw std::logic_error(UNEXPECTED_SYMBOL);
            ++ptr_;
            return;
        case '{':
            if (status != kNull && status != kComma)
                throw std::logic_error(UNEXPECTED_SYMBOL);
            json_array->push_back(new Node(kObject));
            ParseObject(json_array->back()->GetObj());
            break;
        case '}':
            throw std::logic_error(SYMBOLS_NOT_IN_PAIRS);
        case ':':
            throw std::logic_error(UNEXPECTED_SYMBOL);
        case ',':
            if (status != kValue)
                throw std::logic_error(UNEXPECTED_SYMBOL);
            ++ptr_;
            break;
        default:
            if (status != kNull && status != kComma)
                throw std::logic_error(UNEXPECTED_SYMBOL);
            json_array->push_back(NULL);
            ParseValue(&(json_array->back()));
        }
        PassSpace();
        status = static_cast<Status>(status == kComma ? 1 : status + 1);
    }
    throw std::logic_error(UNEXPECTED_TERMINATION);
}

void Parser::ParseObject(JsonObject* json_object)
{
    enum Status
    {
        kNull,
        kKey,
        kColon, //:
        kValue,
        kComma, //,
    } status = kNull;
    JsonString key;

    ++ptr_;
    PassSpace();
    while (ptr_ < end_)
    {
        switch (*ptr_)
        {
        case '[':
            if (status != kColon)
                throw std::logic_error(UNEXPECTED_SYMBOL);
            ParseArray(json_object->insert(std::make_pair(key, new Node(kArray)))->second->GetArray());
            break;
        case ']':
            throw std::logic_error(SYMBOLS_NOT_IN_PAIRS);
        case '{':
            if (status != kColon)
                throw std::logic_error(UNEXPECTED_SYMBOL);
            ParseObject(json_object->insert(std::make_pair(key, new Node(kObject)))->second->GetObj());
            break;
        case '}':
            if (status != kNull && status != kValue)
                throw std::logic_error(UNEXPECTED_SYMBOL);
            ++ptr_;
            return;
        case ':':
            if (status != kKey)
                throw std::logic_error(UNEXPECTED_SYMBOL);
            ++ptr_;
            break;
        case ',':
            if (status != kValue)
                throw std::logic_error(UNEXPECTED_SYMBOL);
            ++ptr_;
            break;
        default:
            if (status == kNull || status == kComma)
            {
                if (*ptr_ != '\"')
                    throw std::logic_error(UNEXPECTED_SYMBOL);
                ParseString(&key);
                break;
            }
            if (status != kColon)
                throw std::logic_error(UNEXPECTED_SYMBOL);
            ParseValue(&(json_object->insert(std::make_pair(key, reinterpret_cast<Node*>(NULL)))->second));
        }
        PassSpace();
        status = static_cast<Status>(status == kComma ? 1 : status + 1);
    }
    throw std::logic_error(UNEXPECTED_TERMINATION);
}

void Parser::ParseValue(Node** node)
{
    if (*ptr_ == '\"')
    {
        *node = new Node(kString);
        ParseString((*node)->GetString());
        return;
    }

    const char* content_start = ptr_;
    FindSymbol();
    JsonString value(content_start, ptr_ - content_start);
    size_t pos = value.find_last_not_of(' ');
    if (pos == JsonString::npos)
        throw std::logic_error(UNEXPECTED_SYMBOL);
    value.resize(pos + 1);

    if (value.compare("true") == 0)
    {
        *node = new Node(kBoolean);
        (*node)->value_ = reinterpret_cast<void*>(true); // Default is false.
    }
    else if (value.compare("false") == 0)
    {
        *node = new Node(kBoolean);
    }
    else if (value.compare("null") == 0)
    {
        *node = new Node(kNull);
    }
    else
    {
        *node = new Node(kNumber);
        (*node)->GetString()->assign(value); // I don't want parse number.
    }
}

void Parser::ParseString(JsonString* json_string)
{
    ++ptr_;
    const char* content_start = ptr_;
    FindStringSymbol();
    if (ptr_ == end_)
        throw std::logic_error(UNEXPECTED_TERMINATION);
    json_string->assign(content_start, ptr_++ - content_start);
}

inline void Parser::PassSpace()
{
    while (ptr_ < end_)
    {
        switch (*ptr_)
        {
        case ' ':case '\r': case '\n':
        case '\t': case '\f': case '\v':
            ++ptr_;
            break;
        default:
            return;
        }
    }
}

inline void Parser::FindSymbol()
{
    while (ptr_ < end_)
    {
        switch (*ptr_)
        {
        case '[': case ']':
        case '{': case '}':
        case ':': case ',':
            return;
        default:
            ++ptr_;
        }
    }
}

inline void Parser::FindStringSymbol()
{
    bool escaped = false;
    while (ptr_ < end_)
    {
        switch (*ptr_)
        {
        case '\"':
            if (!escaped)
                return;
        default:
            escaped = (*ptr_ == '\\');
            ++ptr_;
        }
    }
}

inline void Parser::PassSpaceAndCheckTermination()
{
    PassSpace();
    if (ptr_ == end_)
        throw std::logic_error(UNEXPECTED_TERMINATION);
}

Node::Node(ValueType value_type) : value_type_(value_type), value_(NULL)
{
    switch (value_type_)
    {
    case kNumber:
    case kString:
        value_ = new JsonString;
        break;
    case kArray:
        value_ = new JsonArray;
        break;
    case kObject:
        value_ = new JsonObject;
        break;
    }
}

Node::~Node()
{
    switch (value_type_)
    {
    case kNumber:
    case kString:
        delete reinterpret_cast<JsonString*>(value_);
        break;
    case kArray:
    {
        JsonArray* json_array = reinterpret_cast<JsonArray*>(value_);
        for (size_t i = 0; i < json_array->size(); ++i)
            delete (*json_array)[i];
        delete json_array;
        break;
    }
    case kObject:
    {
        JsonObject* json_object = reinterpret_cast<JsonObject*>(value_);
        JsonObject::iterator iter = json_object->begin();
        for (; iter != json_object->end(); ++iter)
            delete iter->second;
        delete json_object;
        break;
    }
    }
}

JsonString* Node::GetString()
{
    if (value_type_ != kNumber && value_type_ != kString)
        throw std::exception();
    return reinterpret_cast<JsonString*>(value_);
}

bool Node::GetBoolean() const
{
    if (value_type_ != kBoolean)
        throw std::exception();
    return value_ != NULL;
}

JsonArray* Node::GetArray()
{
    if (value_type_ != kArray)
        throw std::exception();
    return reinterpret_cast<JsonArray*>(value_);
}

JsonObject* Node::GetObj()
{
    if (value_type_ != kObject)
        throw std::exception();
    return reinterpret_cast<JsonObject*>(value_);
}

}  // namespace json
}  // namespace web
