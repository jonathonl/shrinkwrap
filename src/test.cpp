
#include "xzbuf.hpp"

#include <iostream>

int main(int argc, char* argv[])
{

  ixzbuf sbuf("xzbuf.hpp.xz");
  std::istream is(&sbuf);

  std::array<char, 64> tmp;
  is.seekg(-2200, std::ios::end);
  while (is)
  {
    is.read(tmp.data(), tmp.size());
    std::cout << std::string(tmp.data(), is.gcount());
  }

  std::cout.flush();

  return 0;
}
