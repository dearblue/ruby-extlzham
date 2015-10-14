# encoding:utf-8 ;

# extlzham - ruby binding for LZHAM

This is ruby bindings for compression library
[LZHAM (https://github.com/richgel999/lzham\_codec)](https://github.com/richgel999/lzham_codec).

  * package name: extlzham
  * author: dearblue (mailto:dearblue@users.osdn.me)
  * version: 0.0.1.PROTOTYPE2
  * license: 2-clause BSD License (<LICENSE.md>)
  * report issue to: <https://osdn.jp/projects/rutsubo/ticket/>
  * dependency ruby: ruby-2.0+
  * dependency ruby gems: (none)
  * dependency libraries: (none)
  * bundled external libraries:
      * LZHAM-1.0-stable1 <https://github.com/richgel999/lzham_codec/releases/tag/v1_0_stable1>


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
