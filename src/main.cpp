#include <iostream>

#include "xrapp.h"

int main(int argc, char* argv[])
{
    std::cout << "jtaibo SandBox" << std::endl;
    std::cout << "  called " << argv[0] << " with " << argc - 1 << " parameters" << std::endl;

    XRApp *the_app = new XRApp();

    the_app->mainLoop();

    return(0);
}
