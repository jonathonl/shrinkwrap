
#include "xzbuf.hpp"
#include <fstream>

#include <iostream>
#include <iomanip>
#include <sys/stat.h>
#include <random>

bool file_exists(const std::string& file_path) {
  struct stat st;
  return (stat(file_path.c_str(), &st) == 0);
}

bool generate_seek_test_file(const std::string& file_path)
{
  oxzstream ofs(file_path);
  for (unsigned i = 0; i < (2048 / 4) && ofs.good(); ++i)
  {
    if (((i * 4) % 512) == 0)
      ofs.flush();
    ofs << std::setfill('0') << std::setw(3) << i << " " ;
  }
  return ofs.good();
}

bool run_seek_test(const std::string& file_path)
{
  ixzstream ifs(file_path);
  std::vector<int> pos_sequence;
  pos_sequence.reserve(128);
  std::mt19937 rg(std::uint32_t(std::chrono::system_clock::now().time_since_epoch().count()));
  for (unsigned i = 0; i < 128 && ifs.good(); ++i)
  {
    int val = 2048 / 4;
    int pos = rg() % val;
    pos_sequence.push_back(pos);
    ifs.seekg(pos * 4, std::ios::beg);
    ifs >> val;
    if (val != pos)
    {
      std::cerr << "Seek failure sequence:" << std::endl;
      for (auto it = pos_sequence.begin(); it != pos_sequence.end(); ++it)
      {
        if (it != pos_sequence.begin())
          std::cerr << ",";
        std::cerr << *it;
      }
      std::cerr << std::endl;
      return false;
    }
  }
  return ifs.good();
}

int main(int argc, char* argv[])
{
  std::size_t error_count = 0;

  {
    std::string seek_test_file = "seek_test_file.txt.xz";
    if (!file_exists(seek_test_file) && !generate_seek_test_file(seek_test_file))
    {
      std::cerr << "FAILED to generate seek_test_file.txt" << std::endl;
      ++error_count;
    }
    else
    {
      if (!run_seek_test(seek_test_file))
      {
        std::cerr << "FAILED seek test." << std::endl;
        ++error_count;
      }
    }
  }

  if (error_count)
    std::cerr << error_count << " errors!" << std::endl;
  else
    std::cout << "Success." << std::endl;

  return 0;
}
