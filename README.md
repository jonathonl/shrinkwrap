# Shrink Wrap
A std::streambuf wrapper for compression formats.

## ixzbuf with std::istream
```c++
std::array<char, 1024> buf;
ixzbuf sbuf("file.xz");
std::istream is(&sbuf);
is.seekg(-1024, std::ios::end);
while (is)
{
  is.read(buf.data(), buf.size());
  std::cout.write(buf.data(), is.gcount());
}
```
## ixzbuf with std::istreambuf_iterator
```c++
ixzbuf sbuf("file.xz");
for (std::istreambuf_iterator<char> it(&sbuf); it != std::istreambuf_iterator<char>{}; ++it)
  std::cout.put(*it);
```

## ixzstream 
```c++
std::array<char, 1024> buf;
ixzstream is("file.xz");
while (is)
{
  is.read(buf.data(), buf.size());
  std::cout.write(buf.data(), is.gcount());
}
```

## oxzstream 
```c++
std::array<char, 1024 * 1024> buf;
oxzstream os("file.xz");
while (std::cin)
{
  std::cin.read(buf.data(), buf.size());
  os.write(buf.data(), buf.gcount());
  os.flush(); // flush() creates block boundary.
}
```

## Caveats
* Does not support files with concatenated xz streams.
