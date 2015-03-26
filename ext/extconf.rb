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

find_header "lzham.h", "$(srcdir)/../contrib/lzham/include" or abort 1
find_header "lzham_comp.h", "$(srcdir)/../contrib/lzham/lzhamcomp" or abort 1
find_header "lzham_decomp.h", "$(srcdir)/../contrib/lzham/lzhamdecomp" or abort 1

if RbConfig::CONFIG["arch"] =~ /mingw/
  $CPPFLAGS << " -D__forceinline=__attribute__\\(\\(always_inline\\)\\)"
end

try_link "void main(void){}", " -Wl,-Bsymbolic " and $LDFLAGS << " -Wl,-Bsymbolic "

create_makefile("extlzham")
