
#include "xzbuf.hpp"
#include <fstream>

#include <iostream>

int main(int argc, char* argv[])
{

  {
    std::ifstream ifs("README.md");
    oxzbuf obuf("test.xz");
    std::ostream os(&obuf);
    std::array<char, 512> buf;
    while (ifs)
    {
      ifs.read(buf.data(), buf.size());
      os.write(buf.data(), ifs.gcount());
      os.flush();
    }
  }



  ixzbuf sbuf("test.xz");
  std::istream is(&sbuf);

  std::array<char, 64> tmp;
  is.seekg(-7, std::ios::end);
  auto p = is.tellg();
  is.read(tmp.data(), 7);
  is.seekg(p, std::ios::beg);
  while (is)
  {
    is.read(tmp.data(), tmp.size());
    std::cout << std::string(tmp.data(), is.gcount());
  }

  std::cout.flush();

  return 0;
}
