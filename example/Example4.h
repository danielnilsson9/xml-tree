#include "XmlTree.h"
#include <iostream>

namespace Ex4Data
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

    struct Info
    {
        int page;
        int lastPage;

        void Convert(XmlTree::Element& e)
        {
            e.ConvertAttribute("page", page);
            e.ConvertAttribute("of", lastPage);
        }
    };

    struct Page
    {
        Info info;
        std::vector<Note> notes;

        //int page;
        //int lastPage;

        void Convert(XmlTree::Element& e)
        {
            // Example: collapse structure
            // 
            // If we wanted to have the "page" and "lastPage" directly 
            // in this struct instead we could convert them by
            // accessing the "info" child element from here.

            //e.Child("info").ConvertAttribute("page", page);
            //e.Child("info").ConvertAttribute("of", lastPage);

            e.Convert("info", info);
            e.ConvertList("notes", "note", notes);
        }
    };
}

/**
  Setup enum conversion between enum string and
  value and register converter with xml-tree.

  IMPORTANT: must be done in global namespace.
*/
XMLTREE_BEGIN_ENUM_CONVERTER(Ex4Data::Note::Priority)
  XMLTREE_MAP_ENUM(Ex4Data::Note::Priority::Low, "Low")
  XMLTREE_MAP_ENUM(Ex4Data::Note::Priority::Medium, "Medium")
  XMLTREE_MAP_ENUM(Ex4Data::Note::Priority::High, "High")
XMLTREE_END_ENUM_CONVERTER(Ex4Data::Note::Priority)


class Example4
{
public:
    void Run()
    {
        try
        {
            auto page = XmlTree::Read<Ex4Data::Page>("../example/data/page_notes.xml", "page");

            std::cout << "Page " << page.info.page << " of " << page.info.lastPage << std::endl << std::endl;
            for (auto& note : page.notes)
            {
                std::cout << "Note" << std::endl;
                std::cout << "----------------------------------" << std::endl;
                std::cout << "id:       " << note.id << std::endl;
                std::cout << "from:     " << note.from << std::endl;
                std::cout << "to:       " << note.to << std::endl;
                std::cout << "priority: " << XMLTREE_ENUM_TO_STRING(Ex4Data::Note::Priority, note.priority) << std::endl;
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