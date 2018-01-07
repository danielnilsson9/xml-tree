#include "XmlTree.h"
#include <iostream>

namespace Ex3Data
{
    struct Note
    {
        enum class Priority { Low, Medium, High };

        uint32_t id;
        std::string from;
        std::string to;
        Priority priority;
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

    struct Notes
    {
        std::vector<Note> notes;

        void Convert(XmlTree::Element& e)
        {
            e.ConvertRepeated("note", notes);
        }
    };
}

/**
  Setup enum conversion between enum string and
  value and register converter with xml-tree.

  IMPORTANT: must be done in global namespace.
*/
XMLTREE_BEGIN_ENUM_CONVERTER(Ex3Data::Note::Priority)
  XMLTREE_MAP_ENUM(Ex3Data::Note::Priority::Low, "Low")
  XMLTREE_MAP_ENUM(Ex3Data::Note::Priority::Medium, "Medium")
  XMLTREE_MAP_ENUM(Ex3Data::Note::Priority::High, "High")
XMLTREE_END_ENUM_CONVERTER(Ex3Data::Note::Priority)


class Example3
{
public:
    void Run()
    {
        try
        {
            auto notes = XmlTree::Read<Ex3Data::Notes>("../example/data/notes.xml", "notes");

            for (auto& note : notes.notes)
            {
                std::cout << "Note" << std::endl;
                std::cout << "----------------------------------" << std::endl;
                std::cout << "id:       " << note.id << std::endl;
                std::cout << "from:     " << note.from << std::endl;
                std::cout << "to:       " << note.to << std::endl;
                std::cout << "priority: " << XMLTREE_ENUM_TO_STRING(Ex3Data::Note::Priority, note.priority) << std::endl;
                std::cout << "heading:  " << note.heading << std::endl;
                std::cout << "body:     " << note.body << std::endl;
                std::cout << std::endl;
            }
        }
        catch (std::runtime_error& e)
        {
            std::cout << e.what() << std::endl;
        }
    }
};