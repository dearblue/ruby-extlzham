# encoding:utf-8 ;

# extlzham - ruby binding for LZHAM

This is ruby bindings for compression library
[LZHAM (https://github.com/richgel999/lzham\_codec)](https://github.com/richgel999/lzham_codec).

*   PACKAGE NAME: extlzham
*   AUTHOR: dearblue <dearblue@users.sourceforge.jp>
*   VERSION: 0.0.1.PROTOTYPE2
*   LICENSING: 2-clause BSD License
*   REPORT ISSUE TO: <http://sourceforge.jp/projects/rutsubo/ticket/>
*   DEPENDENCY RUBY: ruby-2.0+
*   DEPENDENCY RUBY GEMS: (none)
*   DEPENDENCY LIBRARY: (none)
*   BUNDLED EXTERNAL LIBRARIES:
    *   LZHAM <https://github.com/richgel999/lzham_codec>


## HOW TO USE

### Simply process

``` ruby:ruby
# First, load library
require "extlzham"

# Prepair data
source = "sample data..." * 50

# Directly compression
encdata = LZHAM.encode(source)

# Directly decompression
decdata = LZHAM.decode(encdata)

# Comparison source and decoded data
p source == decdata
```

### Streaming process

``` ruby:ruby
# First, load library
require "extlzham"

# Prepair data
source = "sample data..." * 50
sourceio = StringIO.new(source)

# streaming compression
LZHAM.encode(dest = StringIO.new("")) do |encoder|
  while buf = sourceio.read(50) # Please increase the value if you want to actually use.
    encoder << buf
  end
  # block leave to encoder.close
end

# streaming decompression
dest.rewind
decdata = ""
LZHAM.decode(StringIO.new(decdata)) do |decoder|
  while buf = dest.read(50)
    decoder << buf
  end
end

# Comparison source and decoded data
p source == decdata
```

----

[a stub]
