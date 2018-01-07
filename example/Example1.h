#include "XmlTree.h"
#include <iostream>

namespace Ex1Data
{
    struct Note
    {
        uint32_t id;
        std::string from;
        std::string to;
        std::string priority;
        std::string heading;
        std::string body;

        void Convert(XmlTree::Element& e)
        {
            e.ConvertAttribute("id", id);
            e.Convert("from", from);
            e.Convert("to", to);
            e.Convert("priority", priority);
            e.Convert("heading", heading);
            e.ConvertOptional("body", body, std::string(""));
        }
    };
}

class Example1
{
public:
    void Run()
    {
        try
        {
            auto note = XmlTree::Read<Ex1Data::Note>("../example/data/note.xml", "note");

            std::cout << "Note" << std::endl;
            std::cout << "----------------------------------" << std::endl;
            std::cout << "id:       " << note.id << std::endl;
            std::cout << "from:     " << note.from << std::endl;
            std::cout << "to:       " << note.to << std::endl;
            std::cout << "priority: " << note.priority << std::endl;
            std::cout << "heading:  " << note.heading << std::endl;
            std::cout << "body:     " << note.body << std::endl;
        }
        catch (std::runtime_error& e)
        {
            std::cout << e.what() << std::endl;
        }
    }
};
