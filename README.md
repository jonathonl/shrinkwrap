# xzbuf
A streambuf for xz files.

## Usage
```c++
std::array<char, 1024> buf;
ixzbuf sbuf("file.xz");
std::istream is(&sbuf);
is.seekg(-1024, std::ios::end);
while (is.read(buf.data(), buf.size()))
{
  std::cout.write(buf.data(), is.gcount());
}
```

## Caveats
* Does not support files with concatenated streams.
