#ifndef LIBVC_XZ_STREAMBUF_HPP_HPP
#define LIBVC_XZ_STREAMBUF_HPP_HPP

#include <streambuf>
#include <array>
#include <vector>
#include <stdio.h>
#include <lzma.h>

class ixzbuf : public std::streambuf
{
public:
  ixzbuf(FILE* fp) :
    decoded_position_(0),
    discard_amount_(0),
    fp_(fp),
    put_back_size_(0),
    lzma_index_(nullptr)
  {
//      char* end = ((char*)decompressed_buffer_.data()) + decompressed_buffer_.size();
//      setg(end, end, end);

    lzma_strm_ = LZMA_STREAM_INIT;
    fread(stream_header_.data(), stream_header_.size(), 1, fp); // TODO: handle error.
    reset_lzma_decoder();
  }

  ~ixzbuf()
  {
    lzma_end(&lzma_strm_);
    if (lzma_index_)
      lzma_index_end(lzma_index_, nullptr);
  }
private:
  void reset_lzma_decoder()
  {
    lzma_res_ = lzma_stream_decoder(&lzma_strm_, UINT64_MAX, LZMA_IGNORE_CHECK); //LZMA_CONCATENATED);

    //----------------------------------------------------------------//
    // Parse stream header.
    lzma_strm_.next_in = stream_header_.data();
    lzma_strm_.avail_in = 12;
    lzma_strm_.next_out = nullptr;
    lzma_strm_.avail_out = 0;
    lzma_res_ = lzma_code(&lzma_strm_, LZMA_RUN);
    //----------------------------------------------------------------//

    if (lzma_res_ != LZMA_OK)
    {
      // TODO: Handle error.
    }
    else
    {
      lzma_strm_.next_in = NULL;
      lzma_strm_.avail_in = 0;
      lzma_strm_.next_out = decompressed_buffer_.data();
      lzma_strm_.avail_out = decompressed_buffer_.size();
    }

    char* end = ((char*)decompressed_buffer_.data()) + decompressed_buffer_.size();
    setg(end, end, end);
  }

  std::streambuf::int_type underflow()
  {
    if (gptr() < egptr()) // buffer not exhausted
      return traits_type::to_int_type(*gptr());

    while (gptr() >= egptr() && lzma_res_ == LZMA_OK)
    {
      lzma_strm_.next_out = decompressed_buffer_.data();
      lzma_strm_.avail_out = decompressed_buffer_.size();

      while (lzma_res_ == LZMA_OK && lzma_strm_.avail_out > 0)
      {
        if (lzma_strm_.avail_in == 0 && !feof(fp_))
        {
          lzma_strm_.next_in = compressed_buffer_.data();
          lzma_strm_.avail_in = fread(compressed_buffer_.data(), 1, compressed_buffer_.size(), fp_);
        }

        lzma_res_ = lzma_code(&lzma_strm_, LZMA_RUN);
      }

      char* start = ((char*)decompressed_buffer_.data());
      setg(start, start, start + (decompressed_buffer_.size() - lzma_strm_.avail_out));

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

    if (lzma_res_ == LZMA_DATA_ERROR)
      lzma_res_ = LZMA_STREAM_END;

    if (lzma_res_ == LZMA_STREAM_END && gptr() >= egptr())
      return traits_type::eof();
    else if (lzma_res_ != LZMA_OK && lzma_res_ != LZMA_STREAM_END)
      return traits_type::eof();

    return traits_type::to_int_type(*gptr());
  }

  std::streambuf::pos_type seekoff(std::streambuf::off_type off, std::ios_base::seekdir way, std::ios_base::openmode which)
  {
    std::uint64_t current_position = decoded_position_ - (egptr() - gptr());
    current_position += discard_amount_; // TODO: overflow check.

    pos_type pos{off_type(current_position)};

    if (off == 0 && way == std::ios::cur)
      return pos; // Supports tellg for streams that can't seek.

    if (way == std::ios::cur)
    {
      pos = pos + off;
    }
    else if (way == std::ios::end)
    {
      if (!lzma_index_)
      {
        if (!init_index())
          return pos_type(off_type(-1));
      }

      pos = pos_type(lzma_index_uncompressed_size(lzma_index_)) + off;
    }
    else
    {
      pos = off;
    }

    return seekpos(pos, which);
  }

  std::streambuf::pos_type seekpos(std::streambuf::pos_type pos, std::ios_base::openmode which)
  {
    if (fp_ == 0 || sync())
      return pos_type(off_type(-1));

    if (!lzma_index_) //stream_flags_.backward_size == LZMA_VLI_UNKNOWN)
    {
      if (!init_index())
        return pos_type(off_type(-1));
    }

    if (lzma_index_iter_locate(&lzma_index_itr_, (std::uint64_t)off_type(pos))) // Returns true on failure.
      return pos_type(off_type(-1));

    long seek_amount = (lzma_index_itr_.block.compressed_file_offset > std::numeric_limits<long>::max() ? std::numeric_limits<long>::max() : static_cast<long>(lzma_index_itr_.block.compressed_file_offset));
    if (fseek(fp_, seek_amount, SEEK_SET))
      return pos_type(off_type(-1));

    discard_amount_ = off_type(pos) - lzma_index_itr_.block.uncompressed_file_offset;

    reset_lzma_decoder();

    return pos;
  }

  bool init_index()
  {
    std::array<std::uint8_t, 12> stream_footer;
    if (fseek(fp_, -12, SEEK_END) || !fread(stream_footer.data(), 12, 1, fp_))
      return false;

    if (lzma_stream_footer_decode(&stream_flags_, stream_footer.data()) != LZMA_OK)
      return false;

    /*lzma_index_ = lzma_index_init(NULL);
    if (!lzma_index_)
      return pos_type(off_type(-1));*/

    std::vector<std::uint8_t> index_raw(stream_flags_.backward_size);
    if (fseek(fp_, -(stream_flags_.backward_size + 12), SEEK_END) || !fread(index_raw.data(), index_raw.size(), 1, fp_))
      return false;

    std::uint64_t memlimit = UINT64_MAX;
    size_t in_pos = 0;
    auto res = lzma_index_buffer_decode(&lzma_index_, &memlimit, nullptr, index_raw.data(), &in_pos, index_raw.size());
    if (res != LZMA_OK)
      return false;

    lzma_index_iter_init(&lzma_index_itr_, lzma_index_);

    return true;
  }
private:
  lzma_stream_flags stream_flags_;
  lzma_stream lzma_strm_;
  lzma_index_iter lzma_index_itr_;
  std::array<std::uint8_t, 12> stream_header_;
  std::array<std::uint8_t, BUFSIZ> compressed_buffer_;
  std::array<std::uint8_t, BUFSIZ> decompressed_buffer_;
  std::uint64_t decoded_position_;
  std::uint64_t discard_amount_;
  FILE* fp_;
  std::size_t put_back_size_;
  lzma_index* lzma_index_;
  lzma_ret lzma_res_;
};

#endif //LIBVC_XZ_STREAMBUF_HPP_HPP