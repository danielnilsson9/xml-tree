#include "XmlTree.h"
#include <iostream>

namespace Ex2Data
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
	};
}

/**
  Setup enum conversion between enum string and
  value and register converter with xml-tree.

  IMPORTANT: must be done in global namespace.
*/
XMLTREE_BEGIN_ENUM_CONVERTER(Ex2Data::Note::Priority)
  XMLTREE_MAP_ENUM(Ex2Data::Note::Priority::Low, "Low")
  XMLTREE_MAP_ENUM(Ex2Data::Note::Priority::Medium, "Medium")
  XMLTREE_MAP_ENUM(Ex2Data::Note::Priority::High, "High")
XMLTREE_END_ENUM_CONVERTER(Ex2Data::Note::Priority)

/**
  Register external conversion function with xml-tree.
  Avoids having the conversion function being a member
  of the target struct/class.

  IMPORTANT: must be done in global namespace.
*/
XMLTREE_REGISTER_CONVERTER(										\
void Convert(const XmlTree::Element& e, Ex2Data::Note& out)		\
{																\
	e.ConvertAttribute("id", out.id);							\
	e.Convert("from", out.from);								\
	e.Convert("to", out.to);									\
	e.Convert("priority", out.priority);						\
	e.Convert("heading", out.heading);							\
	e.ConvertOptional("body", out.body, std::string(""));		\
});


class Example2
{
public:
	void Run()
	{
		try
		{
			auto note = XmlTree::Read<Ex2Data::Note>("../example/data/note.xml", "note");

			std::cout << "Note" << std::endl;
			std::cout << "----------------------------------" << std::endl;
			std::cout << "id:       " << note.id << std::endl;
			std::cout << "from:     " << note.from << std::endl;
			std::cout << "to:       " << note.to << std::endl;
			std::cout << "priority: " << XMLTREE_ENUM_TO_STRING(Ex2Data::Note::Priority, note.priority) << std::endl;
			std::cout << "heading:  " << note.heading << std::endl;
			std::cout << "body:     " << note.body << std::endl;
		}
		catch (std::runtime_error& e)
		{
			std::cout << e.what() << std::endl;
		}
	}
};