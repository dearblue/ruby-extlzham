#vim: set fileencoding:utf-8

require_relative "lib/extlzham/version"

GEMSTUB = Gem::Specification.new do |s|
  s.name = "extlzham"
  s.version = LZHAM::VERSION
  s.summary = "ruby bindings for lzham"
  s.description = <<EOS
ruby bindings for lzham <https://github.com/richgel999/lzham_codec>.
EOS
  s.homepage = "https://osdn.jp/projects/rutsubo/"
  s.license = "2-clause BSD License"
  s.author = "dearblue"
  s.email = "dearblue@users.osdn.me"

  s.required_ruby_version = ">= 2.0"
  s.add_development_dependency "rspec", "~> 2.14"
  s.add_development_dependency "rake", "~> 10.0"
end

EXTRA.concat(FileList["contrib/lzham/{LICENSE,README.md,include/lzham.h,{lzhamcomp,lzhamdecomp,lzhamlib}/*.{h,hh,hxx,hpp,c,cc,cxx,cpp,C}}"])
