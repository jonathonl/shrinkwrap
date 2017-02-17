# xzbuf
A streambuf for xz files.

# Usage
```c++
std::array<char, 1024> buf;
ixzbuf sbuf(fopen("file.xz"));
std::istream is(&sbuf);
is.seekg(-1024, std::ios::end);
is.read(buf.data(), buf.size());
```
