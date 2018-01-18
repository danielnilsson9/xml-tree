/* 
 * XmlTree.h
 *
 * Copyright (C) 2017 Daniel Nilsson
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#pragma once
#include <tinyxml2.h>

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <algorithm>
#include <sstream>
#include <type_traits>

#define XMLTREE_REGISTER_CONVERTER(f) namespace XmlTree { namespace Converters { template<> inline f }}

#define XMLTREE_BEGIN_ENUM_CONVERTER(EnumType)                                              \
namespace XmlTree { namespace Enums                                                         \
{                                                                                           \
    template<>                                                                              \
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
    void Convert(Element& e, EnumType& out)                     \
    {                                                           \
        out = XMLTREE_ENUM_FROM_STRING(EnumType, e.Value());    \
    }                                                           \
);                                                              \
XMLTREE_REGISTER_CONVERTER(                                     \
    void Convert(Attribute& e, EnumType& out)                   \
    {                                                           \
        out = XMLTREE_ENUM_FROM_STRING(EnumType, e.Value());    \
    }                                                           \
);

#define XMLTREE_ENUM_TO_STRING(EnumType, EnumValue) XmlTree::Enums::EnumString<EnumType>::Str(EnumValue)
#define XMLTREE_ENUM_FROM_STRING(EnumType, EnumStr) XmlTree::Enums::EnumString<EnumType>::Val(EnumStr)


namespace XmlTree
{
    
    // forward declaration
    class Element;
    class Attribute;
    namespace Converters
    {
        template<typename T> void Convert(Element& a, T& out);
        template<typename T> void Convert(Attribute& a, T& out);
    }

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
                std::is_convertible<decltype(std::declval<T>().Convert(std::declval<Args>()...)), Ret>::type; 

            template<typename>
            static constexpr std::false_type check(...);

            typedef decltype(check<C>(0)) type;

        public:
            static constexpr bool value = type::value;
        };

        template<bool> struct convert_impl;

        template<> struct convert_impl<false>
        {
            template<typename TIn, typename TOut>
            static void convert(TIn& e, TOut& out)
            {
                Converters::Convert(e, out);
            }
        };

        template<> struct convert_impl<true>
        {
            template<typename TIn, typename TOut>
            static void convert(TIn& e, TOut& out)
            {
                out.Convert(e);
            }
        };
        
        template<typename TIn, typename TOut>
        void call_convert(TIn& e, TOut& out)
        {
            convert_impl<detail::has_convert<TOut, void(TIn&)>::value>::convert(e, out);
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

        // name of attribute
        std::string Name() const
        {
            return _attribute->Name() == nullptr ? "" : _attribute->Name();
        }

        // value of attribute
        std::string Value() const
        {
            return _attribute->Value() == nullptr ? "" : _attribute->Value();
        }

        template<typename T>
        void Convert(T& out)
        {
            detail::call_convert(const_cast<Attribute&>(*this), out);
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

        // true if element has named attribute, false otherwise
        bool HasAttribute(const std::string& name) const
        {
            return _element->FindAttribute(name.c_str()) != nullptr;
        }

        // get named attribute, throws exception if does not exist.
        Attribute Attribute(const std::string& name) const
        {
            auto attrib = _element->FindAttribute(name.c_str());
            if (attrib == nullptr)
            {
                throw std::runtime_error("Element '" + Name() + "' does not have an attribute named '" + name + "'.");
            }

            return XmlTree::Attribute(attrib);
        }

        // true if element has named child element, false otherwise
        bool HasChild(const std::string& name) const
        {
            return _element->FirstChildElement(name.c_str()) != nullptr;
        }

        // get named child element, throws exception if does not exist.
        Element Child(const std::string& name) const
        {
            auto elem = _element->FirstChildElement(name.c_str());
            if (elem == nullptr)
            {
                throw std::runtime_error("Element '" + Name() + "' does not have a child named '" + name + "'.");
            }

            return Element(elem);
        }


        // convert element itself to type, if element is of value type this will perform conversion of the value.
        template<typename T>
        void Convert(T& out) const
        {
            detail::call_convert(const_cast<Element&>(*this), out);
        }

        // convert named child element to type
        template<typename T>
        void Convert(const std::string& name, T& out) const
        {
            if (!HasChild(name))
            {
                throw std::runtime_error("Required element '" + name + "' not found.");
            }

            Child(name).Convert(out);
        }

        // convert optional named child element to type
        template<typename T>
        bool ConvertOptional(const std::string& name, T& out, const T& defaultVal) const
        {
            if (!HasChild(name))
            {
                out = defaultVal;
                return false;
            }
            
            Child(name).Convert(out);
            return true;
        }

        // convert optional named child element to type
        template<typename T>
        bool ConvertOptional(const std::string& name, Optional<T>& out) const
        {
            if (!HasChild(name))
            {
                out.Reset();
                return false;
            }

            out.HasValue(true);
            Child(name).Convert(out.Value());
            return true;
        }

        // convert named attribute to type
        template<typename T>
        void ConvertAttribute(const std::string& name, T& out) const
        {
            if (!HasAttribute(name))
            {
                throw std::runtime_error("Required attribute '" + name + "' not found.");
            }

            Attribute(name).Convert(out);
        }

        // convert optional named attribute to type
        template<typename T>
        bool ConvertAttributeOptional(const std::string& name, T& out, const T& defaultVal) const
        {
            if (!HasAttribute(name))
            {
                out = defaultVal;
                return false;
            }
            
            Attribute(name).Convert(out);
            return true;
        }

        // convert optional named attribute to type
        template<typename T>
        bool ConvertAttributeOptional(const std::string& name, Optional<T>& out) const
        {
            if (!HasAttribute(name))
            {
                out.Reset();
                return false;
            }

            out.HasValue(true);
            Attribute(name).Convert(out.Value());
            return true;
        }

        // convert named list of elements to vector of type
        template<typename T>
        void ConvertList(const std::string& listName, const std::string& elemName, std::vector<T>& out) const
        {
            if (!HasChild(listName))
            {
                throw std::runtime_error("Required list '" + listName + "' not found.");
            }

            ConvertListOptional(listName, elemName, out);
        }

        // convert optional named list of elements to vector of type
        template<typename T>
        bool ConvertListOptional(const std::string& listName, const std::string& elemName, std::vector<T>& out) const
        {
            if (!HasChild(listName))
            {
                return false;
            }

            auto list = _element->FirstChildElement(listName.c_str());
            for (auto e = list->FirstChildElement(elemName.c_str()); e != nullptr; e = e->NextSiblingElement(elemName.c_str()))
            {
                T i;
                Element(e).Convert(i);
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
                Element(e).Convert(i);
                out.push_back(i);
            }
        }

        // loop over all child elements and do custom processing
        void ForEachElement(std::function<void(Element& e)> func) const
        {
            for (auto e = _element->FirstChildElement(); e != nullptr; e = e->NextSiblingElement())
            {
                Element tmp(e);
                func(tmp);
            }
        }

        // loop over all attributes on element and do custom processing
        void ForEachAttribute(std::function<void(XmlTree::Attribute& a)> func) const
        {
            for (auto a = _element->FirstAttribute(); a != nullptr; a->Next())
            {
                XmlTree::Attribute tmp(a);
                func(tmp);
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
                auto& map = Map();

                auto itr = std::find_if(map.begin(), map.end(), 
                    [&](const std::pair<TEnum, std::string>& pair)
                    {
                        return pair.first == e;
                    });

                if (itr != map.end())
                {
                    return itr->second;
                }

                throw std::runtime_error("'" + std::to_string(static_cast<int>(e)) + "' is not a valid value for enum and cannot be converted to string.");
            }

            static TEnum Val(const std::string& str)
            {
                auto& map = Map();

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

            static EnumMap& Map()
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
                auto& map = Map();
                map.push_back(std::pair<TEnum, std::string>(e, str ));
            }
        };

        template<typename TEnum>
        struct EnumString : public EnumStringBase<EnumString<TEnum>, TEnum>
        {
            static void RegisterAll() { /* static_assert(false, "Enum type not registered with XmlTree."); */ }
        };
    }

    namespace Converters
    {
        // elements
        
        template<>
        inline void Convert<std::string>(Element& e, std::string& out)
        {
            out = e.Value();
        }

        template<>
        inline void Convert<bool>(Element& e, bool& out)
        {
            out = false;
            std::istringstream(e.Value()) >> std::boolalpha >> out;
        }

        template<>
        inline void Convert<float>(Element& a, float& out)
        {
            out = std::stof(a.Value());
        }

        template<>
        inline void Convert<double>(Element& a, double& out)
        {
            out = std::stod(a.Value());
        }

        template<>
        inline void Convert<uint16_t>(Element& e, uint16_t& out)
        {
            out = static_cast<uint16_t>(std::stoul(e.Value()));
        }

        template<>
        inline void Convert<int16_t>(Element& e, int16_t& out)
        {
            out = static_cast<int16_t>(std::stol(e.Value()));
        }

        template<>
        inline void Convert<uint32_t>(Element& a, uint32_t& out)
        {
            out = std::stoul(a.Value());
        }

        template<>
        inline void Convert<int32_t>(Element& a, int32_t& out)
        {
            out = std::stol(a.Value());
        }

        template<>
        inline void Convert<uint64_t>(Element& a, uint64_t& out)
        {
            out = std::stoull(a.Value());
        }

        template<>
        inline void Convert<int64_t>(Element& a, int64_t& out)
        {
            out = std::stoll(a.Value());
        }


        // attributes

        template<>
        inline void Convert<std::string>(Attribute& a, std::string& out)
        {
            out = a.Value();
        }

        template<>
        inline void Convert<bool>(Attribute& a, bool& out)
        {
            out = false;
            std::istringstream(a.Value()) >> std::boolalpha >> out;
        }

        template<>
        inline void Convert<float>(Attribute& a, float& out)
        {
            out = std::stof(a.Value());
        }

        template<>
        inline void Convert<double>(Attribute& a, double& out)
        {
            out = std::stod(a.Value());
        }

        template<>
        inline void Convert<uint16_t>(Attribute& a, uint16_t& out)
        {
            out = static_cast<uint16_t>(std::stoul(a.Value()));
        }

        template<>
        inline void Convert<int16_t>(Attribute& a, int16_t& out)
        {
            out = static_cast<int16_t>(std::stol(a.Value()));
        }

        template<>
        inline void Convert<uint32_t>(Attribute& a, uint32_t& out)
        {
            out = std::stoul(a.Value());
        }

        template<>
        inline void Convert<int32_t>(Attribute& a, int32_t& out)
        {
            out = std::stol(a.Value());
        }

        template<>
        inline void Convert<uint64_t>(Attribute& a, uint64_t& out)
        {
            out = std::stoull(a.Value());
        }

        template<>
        inline void Convert<int64_t>(Attribute& a, int64_t& out)
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

        Element(root).Convert(res);
        return res;
    }

    template<typename T>
    T Parse(const std::string& xml, const std::string& rootElement)
    {
        T res;

        tinyxml2::XMLDocument doc;
        if (tinyxml2::XML_SUCCESS != doc.Parse(xml.c_str()))
        {
            throw std::runtime_error(doc.ErrorStr());
        }

        auto root = doc.FirstChildElement(rootElement.c_str());
        if (root == nullptr)
        {
            throw std::runtime_error("Root element '" + rootElement + "' not found.");
        }

        Element(root).Convert(res);
        return res;
    }

}
