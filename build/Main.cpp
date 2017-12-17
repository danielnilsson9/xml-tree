#include "Example1.h"
#include "Example2.h"
#include "Example3.h"
#include "Example4.h"
#include <iostream>

enum class Example { Example1, Example2, Example3, Example4 };

// Set example to run:
Example run = Example::Example4;

int main()
{
    switch (run)
    {
    case Example::Example1:
        Example1().Run();
        break;
    case Example::Example2:
        Example2().Run();
        break;
    case Example::Example3:
        Example3().Run();
        break;
    case Example::Example4:
        Example4().Run();
        break;
    }

    std::cout << std::endl << std::endl;
    std::cout << "Press ENTER to exit." << std::endl;
    std::cin.get();
}