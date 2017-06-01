#ifndef SHRINKWRAP_GZBUF_HPP
#define SHRINKWRAP_GZBUF_HPP

#include <streambuf>
#include <array>
#include <vector>
#include <stdio.h>
#include <zlib.h>
#include <assert.h>
#include <iostream>
#include <limits>
#include <cstring>

namespace shrinkwrap
{
  class igzbuf : public std::streambuf
  {
  public:
    igzbuf(const std::string& file_path)
      :
      //zstrm_({0}),
      compressed_buffer_(default_block_size),
      decompressed_buffer_(default_block_size),
      decoded_position_(0),
      discard_amount_(0),
      fp_(fopen(file_path.c_str(), "rb")),
      put_back_size_(0),
      at_block_boundary_(false)
    {
      if (fp_)
      {
        zstrm_.zalloc = Z_NULL;
        zstrm_.zfree = Z_NULL;
        zstrm_.opaque = Z_NULL;
        zstrm_.avail_in = 0;
        zstrm_.next_in = Z_NULL;
        zlib_res_ = inflateInit2(&zstrm_, 15 + 16); // 16 for GZIP only.
        if (zlib_res_ != Z_OK)
        {
          // TODO: handle error.
        }
      }
      char* end = ((char*) decompressed_buffer_.data()) + decompressed_buffer_.size();
      setg(end, end, end);
    }

    igzbuf(igzbuf&& src)
      :
      std::streambuf(std::move(src))
    {
      this->move(std::move(src));
    }

    igzbuf& operator=(igzbuf&& src)
    {
      if (&src != this)
      {
        std::streambuf::operator=(std::move(src));
        this->destroy();
        this->move(std::move(src));
      }

      return *this;
    }

    ~igzbuf()
    {
      this->destroy();
    }

  private:
    //ixzbuf(const ixzbuf& src) = delete;
    //ixzbuf& operator=(const ixzbuf& src) = delete;

    void destroy()
    {
      if (fp_)
      {
        inflateEnd(&zstrm_);
        fclose(fp_);
      }
    }

    void move(igzbuf&& src)
    {
      zstrm_ = src.zstrm_;
      src.zstrm_ = {0};
      compressed_buffer_ = src.compressed_buffer_;
      decompressed_buffer_ = src.decompressed_buffer_;
      decoded_position_ = src.decoded_position_;
      discard_amount_ = src.discard_amount_;
      at_block_boundary_ = src.at_block_boundary_;
      fp_ = src.fp_;
      if (src.fp_)
        src.fp_ = nullptr;
      put_back_size_ = src.put_back_size_;
      zlib_res_ = src.zlib_res_;
    }

    std::streambuf::int_type underflow()
    {
      if (!fp_)
        return traits_type::eof();
      if (gptr() < egptr()) // buffer not exhausted
        return traits_type::to_int_type(*gptr());

      while (gptr() >= egptr() && zlib_res_ == LZMA_OK)
      {
        zstrm_.next_out = decompressed_buffer_.data();
        zstrm_.avail_out = decompressed_buffer_.size();

        if (zstrm_.avail_in == 0 && !feof(fp_))
        {
          replenish_compressed_buffer();
        }



        //assert(zstrm_.avail_in > 0);

        int r = inflate(&zstrm_, Z_NO_FLUSH);
        if (r == Z_STREAM_END)
        {
          // End of block.
          if (zstrm_.avail_in == 0 && !feof(fp_))
            replenish_compressed_buffer();

          if (zstrm_.avail_in > 0)
            r = inflateReset(&zstrm_);
        }

        zlib_res_ = r;

        char* start = ((char*) decompressed_buffer_.data());
        setg(start, start, start + (decompressed_buffer_.size() - zstrm_.avail_out));
        decoded_position_ += (egptr() - gptr());

        if (discard_amount_ > 0)
        {
          std::uint64_t advance_amount = discard_amount_;
          if ((egptr() - gptr()) < advance_amount)
            advance_amount = (egptr() - gptr());
          setg(start, gptr() + advance_amount, egptr());
          discard_amount_ -= advance_amount;
        }
      }

      if (zlib_res_ == Z_STREAM_END && gptr() >= egptr())
        return traits_type::eof();
      else if (zlib_res_ != Z_OK && zlib_res_ != Z_STREAM_END)
        return traits_type::eof();

      return traits_type::to_int_type(*gptr());
    }

    void replenish_compressed_buffer()
    {
      zstrm_.next_in = compressed_buffer_.data();
      zstrm_.avail_in = fread(compressed_buffer_.data(), 1, compressed_buffer_.size(), fp_);
    }

//    std::streambuf::pos_type seekoff(std::streambuf::off_type off, std::ios_base::seekdir way, std::ios_base::openmode which)
//    {
//      std::uint64_t current_position = decoded_position_ - (egptr() - gptr());
//      current_position += discard_amount_; // TODO: overflow check.
//
//      pos_type pos{off_type(current_position)};
//
//      if (off == 0 && way == std::ios::cur)
//        return pos; // Supports tellg for streams that can't seek.
//
//      if (way == std::ios::cur)
//      {
//        pos = pos + off;
//      }
//      else if (way == std::ios::end)
//      {
//        if (!lzma_index_)
//        {
//          if (!init_index())
//            return pos_type(off_type(-1));
//        }
//
//        pos = pos_type(lzma_index_uncompressed_size(lzma_index_)) + off;
//      }
//      else
//      {
//        pos = off;
//      }
//
//      return seekpos(pos, which);
//    }
//
//    std::streambuf::pos_type seekpos(std::streambuf::pos_type pos, std::ios_base::openmode which)
//    {
//      if (fp_ == 0 || sync())
//        return pos_type(off_type(-1));
//
//      if (!lzma_index_) //stream_flags_.backward_size == LZMA_VLI_UNKNOWN)
//      {
//        if (!init_index())
//          return pos_type(off_type(-1));
//      }
//
//      if (lzma_index_iter_locate(&lzma_index_itr_, (std::uint64_t) off_type(pos))) // Returns true on failure.
//        return pos_type(off_type(-1));
//
//      long seek_amount = (lzma_index_itr_.block.compressed_file_offset > std::numeric_limits<long>::max() ? std::numeric_limits<long>::max() : static_cast<long>(lzma_index_itr_.block.compressed_file_offset));
//      if (fseek(fp_, seek_amount, SEEK_SET))
//        return pos_type(off_type(-1));
//
//      discard_amount_ = off_type(pos) - lzma_index_itr_.block.uncompressed_file_offset;
//      decoded_position_ = lzma_index_itr_.block.uncompressed_file_offset;
//
//      at_block_boundary_ = true;
//      lzma_block_decoder_.next_in = nullptr;
//      lzma_block_decoder_.avail_in = 0;
//      char* end = ((char*) decompressed_buffer_.data()) + decompressed_buffer_.size();
//      setg(end, end, end);
//
//      return pos;
//    }

  private:
    static const std::size_t default_block_size = 0xFFFF;
    z_stream zstrm_;
    std::vector<std::uint8_t> compressed_buffer_;
    std::vector<std::uint8_t> decompressed_buffer_;
    std::uint64_t decoded_position_;
    std::uint64_t discard_amount_;
    FILE* fp_;
    std::size_t put_back_size_;
    int zlib_res_;
    bool at_block_boundary_;
  };

  class obgzbuf : public std::streambuf
  {
  public:
    obgzbuf(const std::string& file_path)
      :
      zstrm_({0}),
      fp_(fopen(file_path.c_str(), "wb")),
      compressed_buffer_(default_block_size),
      decompressed_buffer_(default_block_size)
    {
      if (!fp_)
      {
        char* end = ((char*) decompressed_buffer_.data()) + decompressed_buffer_.size();
        setp(end, end);
      }
      else
      {
        zlib_res_ = deflateInit2(&zstrm_, Z_DEFAULT_COMPRESSION, Z_DEFLATED, (15 | 16), 8, Z_DEFAULT_STRATEGY); // |16 for GZIP
        if (zlib_res_ != Z_OK)
        {
          // TODO: handle error.
        }

       zstrm_.next_out = compressed_buffer_.data();
       zstrm_.avail_out = compressed_buffer_.size();

        char* end = ((char*) decompressed_buffer_.data()) + decompressed_buffer_.size();
        setp((char*) decompressed_buffer_.data(), end);
      }
    }

    obgzbuf(obgzbuf&& src)
      :
      std::streambuf(std::move(src))
    {
      this->move(std::move(src));
    }

    obgzbuf& operator=(obgzbuf&& src)
    {
      if (&src != this)
      {
        std::streambuf::operator=(std::move(src));
        this->close();
        this->move(std::move(src));
      }

      return *this;
    }

    ~obgzbuf()
    {
      this->close();
    }

  private:
    void move(obgzbuf&& src)
    {
      compressed_buffer_ = std::move(src.compressed_buffer_);
      decompressed_buffer_ = std::move(src.decompressed_buffer_);
      zstrm_ = src.zstrm_;
      fp_ = src.fp_;
      src.fp_ = nullptr;
      zlib_res_ = src.zlib_res_;
    }

    void close()
    {
      if (fp_)
      {
        sync();
        int res = deflateEnd(&zstrm_);
        if (zlib_res_ == Z_OK)
          zlib_res_ = res;
        fclose(fp_);
        fp_ = nullptr;
      }
    }

    int overflow(int c)
    {
      if (!fp_)
        return traits_type::eof();

      if ((epptr() - pptr()) > 0)
      {
        assert(!"Put buffer not empty, this should never happen");
        this->sputc(static_cast<char>(0xFF & c));
      }
      else
      {
        zstrm_.next_in = decompressed_buffer_.data();
        zstrm_.avail_in = decompressed_buffer_.size();
        while (zlib_res_ == Z_OK && zstrm_.avail_in > 0)
        {
          zlib_res_ = deflate(&zstrm_, Z_FINISH);
          if (zstrm_.avail_out == 0 || zlib_res_ == Z_STREAM_END)
          {
            if (!fwrite(compressed_buffer_.data(), compressed_buffer_.size() - zstrm_.avail_out, 1, fp_))
            {
              // TODO: handle error.
              return traits_type::eof();
            }
            zstrm_.next_out = compressed_buffer_.data();
            zstrm_.avail_out = compressed_buffer_.size();
          }
        }

        if (zlib_res_ == LZMA_STREAM_END)
          zlib_res_ = deflateReset(&zstrm_);

        assert(zstrm_.avail_in == 0);
        decompressed_buffer_[0] = reinterpret_cast<unsigned char&>(c);
        setp((char*) decompressed_buffer_.data() + 1, (char*) decompressed_buffer_.data() + decompressed_buffer_.size());
      }

      return (zlib_res_ == LZMA_OK ? traits_type::to_int_type(c) : traits_type::eof());
    }

    int sync()
    {
      if (!fp_)
        return -1;

      zstrm_.next_in = decompressed_buffer_.data();
      zstrm_.avail_in = decompressed_buffer_.size() - (epptr() - pptr());
      if (zstrm_.avail_in)
      {
        while (zlib_res_ == Z_OK)
        {
          zlib_res_ = deflate(&zstrm_, Z_FINISH);
          if (zstrm_.avail_out == 0 || (zlib_res_ == Z_STREAM_END && compressed_buffer_.size() != zstrm_.avail_out))
          {
            if (!fwrite(compressed_buffer_.data(), compressed_buffer_.size() - zstrm_.avail_out, 1, fp_))
            {
              // TODO: handle error.
              return -1;
            }
            zstrm_.next_out = compressed_buffer_.data();
            zstrm_.avail_out = compressed_buffer_.size();
          }
        }

        if (zlib_res_ == LZMA_STREAM_END)
          zlib_res_ = deflateReset(&zstrm_);

        if (zlib_res_ != LZMA_OK)
          return -1;

        assert(zstrm_.avail_in == 0);
        setp((char*) decompressed_buffer_.data(), (char*) decompressed_buffer_.data() + decompressed_buffer_.size());
      }

      return 0;
    }

  private:
    static const std::size_t default_block_size = 0xFFFF;
    std::vector<std::uint8_t> compressed_buffer_;
    std::vector<std::uint8_t> decompressed_buffer_;
    z_stream zstrm_;
    FILE* fp_;
    int zlib_res_;
  };

  class igzstream : public std::istream
  {
  public:
    igzstream(const std::string& file_path)
      :
      std::istream(&sbuf_),
      sbuf_(file_path)
    {
    }

    igzstream(igzstream&& src)
      :
      std::istream(&sbuf_),
      sbuf_(std::move(src.sbuf_))
    {
    }

    igzstream& operator=(igzstream&& src)
    {
      if (&src != this)
      {
        std::istream::operator=(std::move(src));
        sbuf_ = std::move(src.sbuf_);
      }
      return *this;
    }

  private:
    igzbuf sbuf_;
  };

  class obgzstream : public std::ostream
  {
  public:
    obgzstream(const std::string& file_path)
      :
      std::ostream(&sbuf_),
      sbuf_(file_path)
    {
    }

    obgzstream(obgzstream&& src)
      :
      std::ostream(&sbuf_),
      sbuf_(std::move(src.sbuf_))
    {
    }

    obgzstream& operator=(obgzstream&& src)
    {
      if (&src != this)
      {
        std::ostream::operator=(std::move(src));
        sbuf_ = std::move(src.sbuf_);
      }
      return *this;
    }

  private:
    obgzbuf sbuf_;
  };
}

#endif //SHRINKWRAP_GZBUF_HPP