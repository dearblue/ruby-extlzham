#!ruby

require "extlzham"

# directly string encode / decode
p data = LZHAM.encode("abcdefghijklmnopqrstuvwxyz" * 10)
p LZHAM.decode(data)


# directly string encode / decode with dictionaly size
# (IMPORTANT! MUST BE SAME SIZE both encoding and decoding)
dictsize = 20 # log(2)'d dictionaly size. This case is 1 MiB (2 ** 20).
p data = LZHAM.encode("abcdefghijklmnopqrstuvwxyz" * 10, dictsize: dictsize)
p LZHAM.decode(data, dictsize: dictsize)


# streaming processing

# setup streaming data
ss = StringIO.new("abcdefghijklmnopqrstuvwxyz" * 10) # source stream
es = StringIO.new # encoded stream
ds = StringIO.new # destination stream

# streaming encode with block
ss.rewind
es.rewind
LZHAM.encode(es) { |lzham| lzham << ss.read }

# streaming encode without block
ss.rewind
es.rewind
lzham = LZHAM.encode(es)
lzham << ss.read
lzham.finish # or lzham.close

# streaming decode with block
ss.rewind
es.rewind
ds.rewind
LZHAM.decode(ds) { |lzham| lzham << es.read }

# streaming decode without block
ss.rewind
es.rewind
ds.rewind
lzham = LZHAM.decode(ds)
lzham << es.read
lzham.finish # or lzham.close
