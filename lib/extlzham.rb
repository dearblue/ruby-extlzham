#vim: set fileencoding:utf-8

ver = RbConfig::CONFIG["ruby_version"].slice(/\d+\.\d+/)
soname = File.basename(__FILE__, ".rb") << ".so"
lib = File.join(File.dirname(__FILE__), ver, soname)
if File.file?(lib)
  require_relative File.join(ver, soname)
else
  require_relative soname
end

require "stringio"
require_relative "extlzham/version"

module LZHAM
  #
  # call-seq:
  #   encode(string, opts = {}) -> encoded string
  #   encode(out_stream, opts = {}) -> lzham encoder object
  #   encode(out_stream, opts = {}) { |encoder| ... } -> out_stream
  #
  # [RETURN (encoded string)]
  #   Return LZHAM'd binary string.
  #
  # [RETURN (lzham encoder object)]
  #   Return LZHAM encoder. When finished process, must call <tt>.finish</tt> method (as same as Zlib::Deflate#finish).
  #
  # [RETURN (out_stream)]
  #   Return out_stream parameter.
  #
  # [string]
  #   Input data as binary.
  #
  # [out_stream]
  #   Writable I/O liked object for LZHAM'd binary data.
  #
  #   This object is called <tt><<</tt> method.
  #
  #   This object is not closed after finished process.
  #
  # [opts (dictsize)]
  #   Set in lzham_compress_params.m_dict_size_log2. Default is <tt>LZHAM::MIN_DICT_SIZE_LOG2</tt>.
  #
  # [opts (level)]
  #   Set in lzham_compress_params.m_level. Default is <tt>LZHAM::COMP_LEVEL_DEFAULT</tt>.
  #
  # [opts (table_update_rate)]
  #   Set in lzham_compress_params.m_table_update_rate. Default is <tt>0</tt>.
  #
  # [opts (threads)]
  #   Set in lzham_compress_params.m_max_helper_threads. Default is <tt>-1</tt>.
  #
  # [opts (flags)]
  #   Set in lzham_compress_params.m_compress_flags. Default is <tt>0</tt>.
  #
  # [opts (table_max_update_interval)]
  #   Set in lzham_compress_params.m_table_max_update_interval. Default is <tt>0</tt>.
  #
  # [opts (table_update_interval_slow_rate)]
  #   Set in lzham_compress_params.m_table_update_interval_slow_rate. Default is <tt>0</tt>.
  #
  # [opts (seed)]
  #   Set in lzham_compress_params.m_pSeed_bytes and lzham_compress_params.m_num_seed_bytes.
  #
  #   But this parameter is ignored currentry (not implemented).
  #
  # ==== example (when given string)
  #
  #   require "extlzham"
  #   lzhamd = LZHAM.encode("This is not LZHAM'd string data." * 50)
  #   p lzhamd.class # => String
  #   p lzhamd # => ...binary data...
  #
  def self.encode(obj, *args)
    if obj.kind_of?(String)
      ex = LZHAM::Encoder.new(s = "".force_encoding(Encoding::BINARY), *args)
      ex << obj
      ex.finish
      s
    else
      enc = Encoder.new(obj, *args)
      return enc unless block_given?
      begin
        yield(enc)
        obj
      ensure
        enc.finish
      end
    end
  end

  #
  # call-seq:
  #   decode(string, opts = {}) -> decoded string
  #   decode(out_stream, opts = {}) -> lzham decoder object
  #   decode(out_stream, opts = {}) { |decoder| ... } -> out_stream
  #
  # [opts (dictsize)]
  #   Set in lzham_decompress_params.m_dict_size_log2. Default is <tt>LZHAM::MIN_DICT_SIZE_LOG2</tt>.
  #
  # [opts (table_update_rate)]
  #   Set in lzham_decompress_params.m_table_update_rate. Default is <tt>0</tt>.
  #
  # [opts (flags)]
  #   Set in lzham_decompress_params.m_compress_flags. Default is <tt>0</tt>.
  #
  # [opts (table_max_update_interval)]
  #   Set in lzham_decompress_params.m_table_max_update_interval. Default is <tt>0</tt>.
  #
  # [opts (table_update_interval_slow_rate)]
  #   Set in lzham_decompress_params.m_table_update_interval_slow_rate. Default is <tt>0</tt>.
  #
  # [opts (seed)]
  #   Set in lzham_decompress_params.m_pSeed_bytes and lzham_decompress_params.m_num_seed_bytes.
  #
  #   But this parameter is ignored currentry (not implemented).
  #
  def self.decode(obj, *args)
    if obj.kind_of?(String)
      dx = LZHAM::Decoder.new(s = "".force_encoding(Encoding::BINARY), *args)
      dx << obj
      dx.finish
      s
    else
      dec = Decoder.new(obj, *args)
      return dec unless block_given?
      begin
        yield(dec)
        obj
      ensure
        dec.finish
      end
    end
  end

  class << self
    alias compress encode
    alias decompress decode
    alias uncompress decode
  end

  Compressor = Encoder
  Decompressor = Decoder
  Uncompressor = Decoder

  class Encoder
    alias encode update
    alias compress update
    alias close finish
  end

  class Decoder
    alias decode update
    alias decompress update
    alias uncompress update
    alias close finish
  end
end
