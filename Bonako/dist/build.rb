#!/usr/local/bin/ruby -Ku
#
# Releaseバージョンを作成します。
#

require 'fileutils'
require 'make_release'

$HtmlBasePath = "E:/Dropbox/NicoNico/homepage/garnet-alice.net/programs/bonako"

#
# ビルドしたファイルを配布用ディレクトリにコピーします。
#
def setup_dist(appdata)
  outdir = appdata.outdir_path
  bonako_dir = "Bonako_#{appdata.version_}"
  dfpn_dir = "Bonako.DFPN_#{appdata.version_}"

  Dir::mkdir(File.join(outdir, "#{bonako_dir}"))
  Dir::mkdir(File.join(outdir, "#{bonako_dir}/dll"))
  Dir::mkdir(File.join(outdir, "#{bonako_dir}/log"))

  Dir::mkdir(File.join(outdir, "#{dfpn_dir}"))
  Dir::mkdir(File.join(outdir, "#{dfpn_dir}/dll"))
  Dir::mkdir(File.join(outdir, "#{dfpn_dir}/log"))
  
  Dir.glob(File.join(outdir, "*")).each do |name|
    if /\.(pdb|xml)$/i =~ name or
       /nunit\.framework\./ =~ name
      deleteall(name)
    elsif /(\.dll)|(bg|de|es|fr|it|ja|lt|nl|pt|ru|zh)([-]\w+)?$/i =~ name or
          (File::ftype(name) == "directory" and
           /(bg|de|es|fr|it|ja|lt|nl|pt|ru|zh)([-]\w+)?$/i =~ name)
      FileUtils.cp(name, File.join(outdir, "#{bonako_dir}/dll", File.basename(name)))
      FileUtils.mv(name, File.join(outdir, "#{dfpn_dir}/dll", File.basename(name)))
    elsif /bonaster\.exe/ =~ name or /NLog\.config/ =~ name
      FileUtils.cp(name, File.join(outdir, "#{bonako_dir}", File.basename(name)))
      FileUtils.mv(name, File.join(outdir, "#{dfpn_dir}", File.basename(name)))
    elsif /ShogiData/ =~ name or /Bonako\.exe/ =~ name
      FileUtils.mv(name, File.join(outdir, "#{bonako_dir}", File.basename(name)))
    elsif /Bonako\.DFPN\.exe/ =~ name
      FileUtils.mv(name, File.join(outdir, "#{dfpn_dir}", File.basename(name)))
    end
  end
  
  path = File.join(outdir, "#{bonako_dir}/log/dummy")
  system("echo dummy > \"#{path}\"")
  
  path = File.join(outdir, "#{dfpn_dir}/log/dummy")
  system("echo dummy > \"#{path}\"")
  
  FileUtils.cp("readme.html", File.join(outdir, "#{bonako_dir}"))
#  FileUtils.cp("readme.html", File.join(outdir, "Bonako.DFPN"))
end

#
# VoteClient.htmlを更新します。
#
def make_recent(appdata)
  input_path = File.join("bonako_tmpl.html")
  output_path = File.join("bonako.html")

  appdata.convert_template(input_path, output_path)
end

#
# 配布ファイルを作成します。
#
def make_dist(appdata)
  # アセンブリバージョンが入ったディレクトリに
  # 作成ファイルを出力します。
  solution_path = File.join(File.dirname(appdata.dist_path), "Bonako.sln")
  appdata.build(solution_path, "CLR_V4")
  setup_dist(appdata)
  
  # zipに圧縮します。
  #appdata.outdir_path = File.join(appdata.outdir_path, "Bonako")
  appdata.make_zip()
  
  # versioninfo.xmlなどを更新します。
  appdata.make_versioninfo()
  appdata.make_release_note()
  make_recent(appdata)
end

#
# 必要なファイルをコピーします。
#
def copy_dist(appdata)
  FileUtils.copy(appdata.zip_path, File.join($HtmlBasePath, "download"))
  FileUtils.copy(appdata.versioninfo_path, File.join($HtmlBasePath, "update"))
  FileUtils.copy(appdata.releasenote_path, File.join($HtmlBasePath, "update"))
  FileUtils.copy(File.join(appdata.dist_path, "bonako.html"), File.join($HtmlBasePath, "download"))
end

# このスクリプトのパスは $basepath/dist/xxx.rb となっています。
dist_path = File.dirname(File.expand_path($0))

assemblyinfo_path = File.join(dist_path, "../Bonako/Properties/AssemblyInfo.cs")
history_path = File.join(dist_path, "history.yaml")
appdata = AppData.new("bonako", dist_path, assemblyinfo_path, history_path)

ARGV.each do |arg|
  case arg
  when "make": make_dist(appdata)
  when "copy": copy_dist(appdata)
  end
end
