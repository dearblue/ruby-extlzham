#!ruby

require "test-unit"
require "openssl"
require "extlzham"

SMALLSIZE = 4000
BIGSIZE = 12000000

SAMPLES = [
  "",
  "\0" * SMALLSIZE,
  "\0" * BIGSIZE,
  "\xaa".b * SMALLSIZE,
  "\xaa".b * BIGSIZE,
  OpenSSL::Random.random_bytes(SMALLSIZE),
  OpenSSL::Random.random_bytes(BIGSIZE),
]

SAMPLES << File.read("/usr/ports/INDEX-10", mode: "rb") rescue nil # if on FreeBSD
SAMPLES << File.read("/boot/kernel/kernel", mode: "rb") rescue nil # if on FreeBSD

class TestExtlzham < Test::Unit::TestCase
  SAMPLES.size.times do |i|
    class_eval <<-EOS, __FILE__, __LINE__ + 1
      def test_encode1_decode_#{i}
        assert { LZHAM.decode(LZHAM.encode(SAMPLES[#{i}])) == SAMPLES[#{i}] }
      end

      def test_encode2_decode_#{i}
        assert { LZHAM.decode(LZHAM::Encoder.encode(SAMPLES[#{i}])) == SAMPLES[#{i}] }
      end
    EOS
  end
end
