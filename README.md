# xzbuf
A streambuf for xz files.

## ixzbuf with std::istream
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
## ixzbuf with std::istreambuf_iterator
```c++
ixzbuf sbuf("file.xz");
for (std::istreambuf_iterator<char> it(&sbuf); it != std::istreambuf_iterator<char>{}; ++it)
{
  std::cout.put(*it);
}
```

## ixzstream 
```c++
std::array<char, 1024> buf;
ixzstream is("file.xz");
while (is.read(buf.data(), buf.size()))
  std::cout.write(buf.data(), is.gcount());
```

## Caveats
* Does not support files with concatenated streams.
