#pragma once
#include "tinyxml2/tinyxml2.h"

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <algorithm>
#include <sstream>
#include <type_traits>

#define XMLTREE_REGISTER_CONVERTER(f) namespace XmlTree { namespace Converters { template<>f }}											

#define XMLTREE_BEGIN_ENUM_CONVERTER(EnumType)                                              \
namespace XmlTree { namespace Enums                                                         \
{                                                                                           \
    template<>                                                                             \
    struct EnumString<EnumType> : public EnumStringBase<EnumString<EnumType>, EnumType>     \
    {                                                                                       \
        static void RegisterAll()                                                           \
        {

#define XMLTREE_MAP_ENUM(EnumVal, Str) Register(EnumVal, Str);

#define XMLTREE_END_ENUM_CONVERTER(EnumType)                    \
        }                                                       \
    };                                                          \
}}                                                              \
XMLTREE_REGISTER_CONVERTER(                                     \
    void Convert(const Element& e, EnumType& out)               \
    {                                                           \
        out = XMLTREE_ENUM_FROM_STRING(EnumType, e.Value());    \
    }                                                           \
);                                                              \
XMLTREE_REGISTER_CONVERTER(                                     \
    void Convert(const Attribute& e, EnumType& out)             \
    {                                                           \
        out = XMLTREE_ENUM_FROM_STRING(EnumType, e.Value());    \
    }                                                           \
);

#define XMLTREE_ENUM_TO_STRING(EnumType, EnumValue) XmlTree::Enums::EnumString<EnumType>::Str(EnumValue)
#define XMLTREE_ENUM_FROM_STRING(EnumType, EnumStr) XmlTree::Enums::EnumString<EnumType>::Val(EnumStr)


namespace XmlTree
{

    namespace detail
    {
        template<typename, typename T>
        struct has_convert {
            static_assert(
                std::integral_constant<T, false>::value,
                "Second template parameter needs to be of function type.");
        };

        template<typename C, typename Ret, typename... Args>
        struct has_convert<C, Ret(Args...)> {
        private:
            template<typename T>
            static constexpr auto check(T*) -> typename
                std::is_same<decltype(std::declval<T>().Convert(std::declval<Args>()...)), Ret>::type; 

            template<typename>
            static constexpr std::false_type check(...);

            typedef decltype(check<C>(0)) type;

        public:
            static constexpr bool value = type::value;
        };

        template<bool>
        struct convert_impl;

        template<>
        struct convert_impl<false>
        {
            template<typename TIn, typename TOut>
            static void convert(const TIn& e, TOut& out)
            {
                Converters::Convert(e, out);
            }
        };

        template<>
        struct convert_impl<true>
        {
            template<typename TIn, typename TOut>
            static void convert(const TIn& e, TOut& out)
            {
                out.Convert(e);
            }
        };

        template<typename TIn, typename TOut>
        void call_convert(const TIn& e, TOut& out)
        {
            convert_impl<detail::has_convert<TOut, void(const TIn&)>::value>::convert(e, out);
        }
    }

    template<typename T>
    class Optional
    {
    public:
        Optional()
            : _hasValue(false)
        {
        }

        Optional<T>& operator=(const T& value)
        {
            _hasValue = true;
            _value = value;

            return *this;
        }

        void Assign(const T& value)
        {
            _hasValue = true;
            _value = value;
        }

        T& Value()
        {
            return _value;
        }

        bool HasValue()
        {
            return _hasValue;
        }

        void HasValue(bool hasValue)
        {
            _hasValue = hasValue;
        }

        void Reset()
        {
            _hasValue = false;
        }

    private:
        bool _hasValue;
        T _value;
    };

    class Attribute
    {
    public:
        Attribute(const tinyxml2::XMLAttribute* attribute)
            : _attribute(attribute)
        {
        }

        std::string Name() const
        {
            return _attribute->Name() == nullptr ? "" : _attribute->Name();
        }

        std::string Value() const
        {
            return _attribute->Value() == nullptr ? "" : _attribute->Value();
        }

        template<typename T>
        void Convert(T& out)
        {
            detail::call_convert(*this, out);
        }

    private:
        const tinyxml2::XMLAttribute* _attribute;
    };

    class Element
    {
    public:
        Element(const tinyxml2::XMLElement* element)
            : _element(element)
        {
        }

        // name of element tag
        std::string Name() const
        {
            return _element->Name() == nullptr ? "" : _element->Name();
        }

        // value if value type element, otherwise empty string
        std::string Value() const
        {
            return _element->GetText() == nullptr ? "" : _element->GetText();
        }


        // convert element itself to type. if element is of value type this does conversion of the value.
        template<typename T>
        void Convert(T& out) const
        {
            detail::call_convert(*this, out);
        }

        // convert named child element to type
        template<typename T>
        void Convert(const std::string& name, T& out) const
        {
            auto elem = _element->FirstChildElement(name.c_str());
            if (elem == nullptr)
            {
                throw std::runtime_error("Required element '" + name + "' not found.");
            }

            detail::call_convert(Element(elem), out);
        }

        // convert optional named child element to type
        template<typename T>
        bool ConvertOptional(const std::string& name, T& out, const T& defaultVal) const
        {
            auto elem = _element->FirstChildElement(name.c_str());
            if (elem == nullptr)
            {
                out = defaultVal;
                return false;
            }
            
            detail::call_convert(Element(elem), out);
            return true;
        }

        // convert optional named child element to type
        template<typename T>
        bool ConvertOptional(const std::string& name, Optional<T>& out) const
        {
            auto elem = _element->FirstChildElement(name.c_str());
            if (elem == nullptr)
            {
                out.Reset();
                return false;
            }

            out.HasValue(true);
            detail::call_convert(Element(elem), out.Value());
            return true;
        }

        // convert named attribute to type
        template<typename T>
        void ConvertAttribute(const std::string& name, T& out) const
        {
            auto attrib = _element->FindAttribute(name.c_str());
            if (attrib == nullptr)
            {
                throw std::runtime_error("Required attribute '" + name + "' not found.");
            }

            Attribute(attrib).Convert(out);
        }

        // convert optional named attribute to type
        template<typename T>
        bool ConvertAttributeOptional(const std::string& name, T& out, const T& defaultVal) const
        {
            auto attrib = _element->FindAttribute(name.c_str());
            if (attrib == nullptr)
            {
                out = defaultVal;
                return false;
            }
            
            Attribute(attrib).Convert(out);
            return true;
        }

        // convert optional named attribute to type
        template<typename T>
        bool ConvertAttributeOptional(const std::string& name, Optional<T>& out) const
        {
            auto attrib = _element->FindAttribute(name.c_str());
            if (attrib == nullptr)
            {
                out.Reset();
                return false;
            }

            out.HasValue(true);
            Attribute(attrib).Convert(out.Value());
            return true;
        }

        // convert named list of elements to vector of type
        template<typename T>
        void ConvertList(const std::string& listName, const std::string& elemName, std::vector<T>& out) const
        {
            if (_element->FirstChildElement(listName.c_str()) == nullptr)
            {
                throw std::runtime_error("Required list '" + listName + "' not found.");
            }

            ConvertListOptional(listName, elemName, out);
        }

        // convert optional named list of elements to vector of type
        template<typename T>
        bool ConvertListOptional(const std::string& listName, const std::string& elemName, std::vector<T>& out) const
        {
            auto list = _element->FirstChildElement(listName.c_str());
            if (list == nullptr)
            {
                return false;
            }

            for (auto e = list->FirstChildElement(elemName.c_str()); e != nullptr; e = e->NextSiblingElement(elemName.c_str()))
            {
                T i;
                detail::call_convert(Element(e), i);
                out.push_back(i);
            }

            return true;
        }

        // convert named repeated element to vector of type
        template<typename T>
        void ConvertRepeated(const std::string& name, std::vector<T>& out) const
        {
            for (auto e = _element->FirstChildElement(name.c_str()); e != nullptr; e = e->NextSiblingElement(name.c_str()))
            {
                T i;
                detail::call_convert(Element(e), i);
                out.push_back(i);
            }
        }

        // loop over all child elements and do custom processing
        void ForEachElement(std::function<void(Element& e)> func) const
        {
            for (auto e = _element->FirstChildElement(); e != nullptr; e = e->NextSiblingElement())
            {
                func(Element(e));
            }
        }

        // loop over all attributes on element and do custom processing
        void ForEachAttribute(std::function<void(Attribute& a)> func) const
        {
            for (auto a = _element->FirstAttribute(); a != nullptr; a->Next())
            {
                func(Attribute(a));
            }
        }

    private:
        const tinyxml2::XMLElement* _element;
    };


    namespace Enums
    {
        template<typename TDerived, typename TEnum>
        struct EnumStringBase
        {
        public:
            static const std::string& Str(TEnum e)
            {
                auto& map = GetMap();

                auto itr = std::find_if(map.begin(), map.end(), 
                [&](const std::pair<TEnum, std::string>& pair)
                {
                    return pair.first == e;
                });

                if (itr != map.end())
                {
                    return itr->second;
                }

                throw std::runtime_error("'" + std::to_string(static_cast<int>(e)) + "' is not a valid value for enum and cannot be convert to string.");
            }

            static TEnum Val(const std::string& str)
            {
                auto& map = GetMap();

                auto itr = std::find_if(map.begin(), map.end(),
                    [&](const std::pair<TEnum, std::string>& pair)
                {
                    return pair.second == str;
                });

                if (itr != map.end())
                {
                    return itr->first;
                }

                throw std::runtime_error("'" + str + "' cannot be converted to a valid enum value.");
            }
            
        protected:
            using EnumMap = std::vector<std::pair<TEnum, std::string>>;

            static EnumMap& GetMap()
            {
                static EnumMap map;
                static bool first = true;

                if (first)
                {
                    first = false;
                    TDerived::RegisterAll();
                }

                return map;
            }

            static void Register(TEnum e, const std::string str)
            {
                auto& map = GetMap();
                map.push_back(std::pair<TEnum, std::string>(e, str ));
            }
        };

        template<typename TEnum>
        struct EnumString : public EnumStringBase<EnumString<TEnum>, TEnum>
        {
            static void RegisterAll() { assert(false); }
        };
    }

    namespace Converters
    {
        // elements

        template<typename T>
        void Convert(const Element& e, T& out)
        {
            static_assert(false, "Missing element converter for type.");
        }

        template<>
        void Convert<std::string>(const Element& e, std::string& out)
        {
            out = e.Value();
        }

        template<>
        void Convert<bool>(const Element& e, bool& out)
        {
            out = false;
            std::istringstream(e.Value()) >> std::boolalpha >> out;
        }

        template<>
        void Convert<float>(const Element& a, float& out)
        {
            out = std::stof(a.Value());
        }

        template<>
        void Convert<double>(const Element& a, double& out)
        {
            out = std::stod(a.Value());
        }

        template<>
        void Convert<uint16_t>(const Element& e, uint16_t& out)
        {
            out = static_cast<uint16_t>(std::stoul(e.Value()));
        }

        template<>
        void Convert<int16_t>(const Element& e, int16_t& out)
        {
            out = static_cast<int16_t>(std::stol(e.Value()));
        }

        template<>
        void Convert<uint32_t>(const Element& a, uint32_t& out)
        {
            out = std::stoul(a.Value());
        }

        template<>
        void Convert<int32_t>(const Element& a, int32_t& out)
        {
            out = std::stol(a.Value());
        }

        template<>
        void Convert<uint64_t>(const Element& a, uint64_t& out)
        {
            out = std::stoull(a.Value());
        }

        template<>
        void Convert<int64_t>(const Element& a, int64_t& out)
        {
            out = std::stoll(a.Value());
        }


        // attributes

        template<typename T>
        void Convert(const Attribute& a, T& out)
        {
            static_assert(false, "Missing attribute converter for type.");
        }

        template<>
        void Convert<std::string>(const Attribute& a, std::string& out)
        {
            out = a.Value();
        }

        template<>
        void Convert<bool>(const Attribute& a, bool& out)
        {
            out = false;
            std::istringstream(a.Value()) >> std::boolalpha >> out;
        }

        template<>
        void Convert<float>(const Attribute& a, float& out)
        {
            out = std::stof(a.Value());
        }

        template<>
        void Convert<double>(const Attribute& a, double& out)
        {
            out = std::stod(a.Value());
        }

        template<>
        void Convert<uint16_t>(const Attribute& a, uint16_t& out)
        {
            out = static_cast<uint16_t>(std::stoul(a.Value()));
        }

        template<>
        void Convert<int16_t>(const Attribute& a, int16_t& out)
        {
            out = static_cast<int16_t>(std::stol(a.Value()));
        }

        template<>
        void Convert<uint32_t>(const Attribute& a, uint32_t& out)
        {
            out = std::stoul(a.Value());
        }

        template<>
        void Convert<int32_t>(const Attribute& a, int32_t& out)
        {
            out = std::stol(a.Value());
        }

        template<>
        void Convert<uint64_t>(const Attribute& a, uint64_t& out)
        {
            out = std::stoull(a.Value());
        }

        template<>
        void Convert<int64_t>(const Attribute& a, int64_t& out)
        {
            out = std::stoll(a.Value());
        }
    }


    template<typename T>
    T Read(const std::string& filePath, const std::string& rootElement)
    {
        T res;

        tinyxml2::XMLDocument doc;
        if (tinyxml2::XML_SUCCESS != doc.LoadFile(filePath.c_str()))
        {
            throw std::runtime_error(doc.ErrorStr());
        }

        auto root = doc.FirstChildElement(rootElement.c_str());
        if (root == nullptr)
        {
            throw std::runtime_error("Root element '" + rootElement + "' not found.");
        }

        detail::call_convert(Element(root), res);
        return res;
    }

    template<typename T>
    T Parse(const std::string& xml, const std::string& rootElement)
    {
        T res;

        tinyxml2::XMLDocument doc;
        if (tinyxml2::XML_SUCCESS != doc.Parse(xml))
        {
            throw std::runtime_error(doc.ErrorStr());
        }

        auto root = doc.FirstChildElement(rootElementName.c_str());
        if (root == nullptr)
        {
            throw std::runtime_error("Root element '" + rootElement + "' not found.");
        }

        detail::call_convert(Element(root), res);
        return res;
    }

}