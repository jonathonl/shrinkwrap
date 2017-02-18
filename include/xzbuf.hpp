#ifndef LIBVC_XZ_STREAMBUF_HPP_HPP
#define LIBVC_XZ_STREAMBUF_HPP_HPP

#include <streambuf>
#include <array>
#include <vector>
#include <stdio.h>
#include <lzma.h>
#include <assert.h>

class ixzbuf : public std::streambuf
{
public:
  ixzbuf(FILE* fp) :
    decoded_position_(0),
    discard_amount_(0),
    fp_(fp),
    put_back_size_(0),
    lzma_index_(nullptr),
    at_block_boundary_(true)
  {
//      char* end = ((char*)decompressed_buffer_.data()) + decompressed_buffer_.size();
//      setg(end, end, end);

    lzma_strm_ = LZMA_STREAM_INIT;
    fread(stream_header_.data(), stream_header_.size(), 1, fp); // TODO: handle error.
    reset_lzma_decoder();
  }

  ixzbuf(const std::string& file_path)
    : ixzbuf(fopen(file_path.c_str(), "rb"))
  {
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
    lzma_res_ = lzma_stream_header_decode(&stream_header_flags_, stream_header_.data());
    if (lzma_res_ != LZMA_OK)
    {
      // TODO: handle error.
    }

//    lzma_res_ = lzma_stream_decoder(&lzma_strm_, UINT64_MAX, LZMA_IGNORE_CHECK); //LZMA_CONCATENATED);

//    //----------------------------------------------------------------//
//    // Parse stream header.
//    lzma_strm_.next_in = stream_header_.data();
//    lzma_strm_.avail_in = 12;
//    lzma_strm_.next_out = nullptr;
//    lzma_strm_.avail_out = 0;
//    lzma_res_ = lzma_code(&lzma_strm_, LZMA_RUN);
//    //----------------------------------------------------------------//

//    if (lzma_res_ != LZMA_OK)
//    {
//      // TODO: Handle error.
//    }
//    else
//    {
//      lzma_strm_.next_in = NULL;
//      lzma_strm_.avail_in = 0;
//      lzma_strm_.next_out = decompressed_buffer_.data();
//      lzma_strm_.avail_out = decompressed_buffer_.size();
//    }

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

      if (at_block_boundary_)
      {
        lzma_strm_.total_out = 0;
        lzma_strm_.total_in = 0;
        bytes_read_of_current_block_ = 0;

        std::vector<std::uint8_t> block_header(LZMA_BLOCK_HEADER_SIZE_MAX);
        if (lzma_strm_.avail_in == 0 && !feof(fp_))
        {
          replenish_compressed_buffer();
        }
        // TODO: make sure avail_in is greater than 0;
        std::memcpy(block_header.data(), lzma_strm_.next_in, 1);
        ++(lzma_strm_.next_in);
        --(lzma_strm_.avail_in);

        lzma_block_.version = 0;
        lzma_block_.check = stream_header_flags_.check;
        lzma_block_.filters = lzma_block_filters_buf_;
        lzma_block_.header_size = lzma_block_header_size_decode (block_header[0]);

        if (lzma_block_.header_size == 0x00)
        {
          // Index indicator found
          lzma_res_ = LZMA_STREAM_END;
        }
        else
        {
          std::size_t bytes_already_copied = 0;
          if (lzma_strm_.avail_in < (lzma_block_.header_size - 1))
          {
            bytes_already_copied = lzma_strm_.avail_in;
            std::memcpy(&block_header[1], lzma_strm_.next_in, bytes_already_copied);
            lzma_strm_.avail_in -= bytes_already_copied;
            lzma_strm_.next_in += bytes_already_copied;
            assert(lzma_strm_.avail_in == 0);
            replenish_compressed_buffer();
          }

          // TODO: make sure avail_in is greater than (lzma_block_.header_size - 1) - bytes_already_copied.
          std::size_t bytes_left_to_copy = (lzma_block_.header_size - 1) - bytes_already_copied;
          std::memcpy(&block_header[1 + bytes_already_copied], lzma_strm_.next_in, bytes_left_to_copy);
          lzma_strm_.avail_in -= bytes_left_to_copy;
          lzma_strm_.next_in += bytes_left_to_copy;

          lzma_res_ = lzma_block_header_decode(&lzma_block_, nullptr, block_header.data());
          if (lzma_res_ != LZMA_OK )
          {
            // TODO: handle error.
          }
          else
          {
            lzma_res_ = lzma_block_decoder(&lzma_strm_, &lzma_block_);
            // TODO: handle error.
          }
        }
        at_block_boundary_ = false;
      }

      while (lzma_res_ == LZMA_OK && lzma_strm_.avail_out > 0 && lzma_block_.uncompressed_size > lzma_strm_.total_out)
      {
        if (lzma_strm_.avail_in == 0 && !feof(fp_))
        {
          replenish_compressed_buffer();
        }

        assert(lzma_strm_.avail_in > 0);

        lzma_ret r = lzma_code(&lzma_strm_, LZMA_RUN);
        if (r == LZMA_STREAM_END)
        {
          // End of block.
          at_block_boundary_ = true;
          r = LZMA_OK;
        }
        lzma_res_ = r;
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

//    if (lzma_res_ == LZMA_DATA_ERROR)
//      lzma_res_ = LZMA_STREAM_END;

    if (lzma_res_ == LZMA_STREAM_END && gptr() >= egptr())
      return traits_type::eof();
    else if (lzma_res_ != LZMA_OK && lzma_res_ != LZMA_STREAM_END)
      return traits_type::eof();

    return traits_type::to_int_type(*gptr());
  }

  void replenish_compressed_buffer()
  {
    lzma_strm_.next_in = compressed_buffer_.data();
    lzma_strm_.avail_in = fread(compressed_buffer_.data(), 1, compressed_buffer_.size(), fp_);
    bytes_read_of_current_block_ += lzma_strm_.avail_in;
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
    if (fseek(fp_, -12, SEEK_END) || !fread(stream_footer_.data(), 12, 1, fp_))
      return false;

    if (lzma_stream_footer_decode(&stream_footer_flags_, stream_footer_.data()) != LZMA_OK)
      return false;

    /*lzma_index_ = lzma_index_init(NULL);
    if (!lzma_index_)
      return pos_type(off_type(-1));*/

    std::vector<std::uint8_t> index_raw(stream_footer_flags_.backward_size);
    if (fseek(fp_, -(stream_footer_flags_.backward_size + 12), SEEK_END) || !fread(index_raw.data(), index_raw.size(), 1, fp_))
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
  lzma_stream_flags stream_header_flags_;
  lzma_stream_flags stream_footer_flags_;
  lzma_stream lzma_strm_;
  lzma_block lzma_block_;
  lzma_filter lzma_block_filters_buf_[LZMA_FILTERS_MAX + 1];
  lzma_index_iter lzma_index_itr_;
  std::array<std::uint8_t, LZMA_STREAM_HEADER_SIZE> stream_header_;
  std::array<std::uint8_t, LZMA_STREAM_HEADER_SIZE> stream_footer_;
  std::array<std::uint8_t, (BUFSIZ >= LZMA_BLOCK_HEADER_SIZE_MAX ? BUFSIZ : LZMA_BLOCK_HEADER_SIZE_MAX)> compressed_buffer_;
  std::array<std::uint8_t, (BUFSIZ >= LZMA_BLOCK_HEADER_SIZE_MAX ? BUFSIZ : LZMA_BLOCK_HEADER_SIZE_MAX)> decompressed_buffer_;
  std::size_t bytes_read_of_current_block_;
  std::uint64_t decoded_position_;
  std::uint64_t discard_amount_;
  FILE* fp_;
  std::size_t put_back_size_;
  lzma_index* lzma_index_;
  lzma_ret lzma_res_;
  bool at_block_boundary_;
};

#endif //LIBVC_XZ_STREAMBUF_HPP_HPP