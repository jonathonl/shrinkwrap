#ifndef SHRINKWRAP_ISTREAM_HPP
#define SHRINKWRAP_ISTREAM_HPP

#include "xz.hpp"
#include "gz.hpp"

#include <streambuf>
#include <memory>

namespace shrinkwrap
{
  namespace detail
  {
    template<typename T, typename ...Args>
    std::unique_ptr<T> make_unique(Args&& ...args)
    {
      return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
  }

  class istream : public std::istream
  {
  public:
    istream(const std::string& file_path)
      :
      std::istream(nullptr)
    {
      FILE* fp = fopen(file_path.c_str(), "rb");

      int first_byte = fgetc(fp);
      ungetc(first_byte, fp);

      switch (char(first_byte))
      {
        case '\x1F':
          sbuf_ = detail::make_unique<::shrinkwrap::bgz::ibuf>(fp);
          break;
        case char('\xFD'):
          sbuf_ = detail::make_unique<::shrinkwrap::xz::ibuf>(fp);
          break;
        case '\x28':
          throw std::runtime_error("zstd files not yet supported.");
        default:
          throw std::runtime_error("raw files not yet supported.");

      }

      this->rdbuf(sbuf_.get());
    }

    istream(istream&& src)
      :
      std::istream(src.sbuf_.get()),
      sbuf_(std::move(src.sbuf_))
    {
    }

    istream& operator=(istream&& src)
    {
      if (&src != this)
      {
        std::istream::operator=(std::move(src));
        sbuf_ = std::move(src.sbuf_);
      }
      return *this;
    }

  private:
    std::unique_ptr<std::streambuf> sbuf_;
  };
}

#endif //SHRINKWRAP_ISTREAM_HPP