#!ruby
#vim: set fileencoding:utf-8

require "mkmf"

dir = File.dirname(__FILE__).gsub(/[\[\{\?\*]/, "[\\0]")
filepattern = "{.,../contrib/lzham/{lzhamcomp,lzhamdecomp,lzhamlib}}/*.c{,pp}"
target = File.join(dir, filepattern)
files = Dir.glob(target).map { |n| File.basename n }
rejects = (RbConfig::CONFIG["arch"] =~ /mswin|mingw/) ? /_pthreads_/ : /_win32_/
files.reject! { |n| n =~ rejects }
$srcs = files

$VPATH.push "$(srcdir)/../contrib/lzham/lzhamcomp",
            "$(srcdir)/../contrib/lzham/lzhamdecomp",
            "$(srcdir)/../contrib/lzham/lzhamlib"

find_header "lzham.h", "$(srcdir)/../contrib/lzham/include"
find_header "lzham_comp.h", "$(srcdir)/../contrib/lzham/lzhamcomp"
find_header "lzham_decomp.h", "$(srcdir)/../contrib/lzham/lzhamdecomp"

if RbConfig::CONFIG["arch"] =~ /mingw/
  $CPPFLAGS << " -D__forceinline=__attribute__\\(\\(always_inline\\)\\)"
end

create_makefile("extlzham")
