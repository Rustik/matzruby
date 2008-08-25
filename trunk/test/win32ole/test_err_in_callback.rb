#
# test Win32OLE avoids cfp consistency error when the exception raised
# in WIN32OLE_EVENT handler block. [ruby-dev:35450]
#

begin
  require 'win32ole'
rescue LoadError
end
require 'rbconfig'
if defined?(WIN32OLE)
  require 'mkmf'
  require 'test/unit'
  class TestErrInCallBack < Test::Unit::TestCase
    def setup
      @ruby = nil
      if File.exist?("./" + CONFIG["RUBY_INSTALL_NAME"] + CONFIG["EXEEXT"])
        sep = File::ALT_SEPARATOR || "/"
        @ruby = "." + sep + CONFIG["RUBY_INSTALL_NAME"]
        @iopt = $:.map {|e|
          " -I " + e
        }.join("")
        @script = File.dirname(__FILE__) + "/err_in_callback.rb"
        @param = create_temp_html
        @param = "file:///" + @param.gsub(/\\/, '/')
      end
    end

    def create_temp_html
      fso = WIN32OLE.new('Scripting.FileSystemObject')
      dummy_file = fso.GetTempName + ".html"
      cfolder = fso.getFolder(".")
      @str = "This is test HTML file for Win32OLE (#{Time.now})"
      f = cfolder.CreateTextFile(dummy_file)
      f.writeLine("<html><body><div id='str'>#{@str}</div></body></html>")
      f.close
      @f = dummy_file
      dummy_path = cfolder.path + "\\" + dummy_file
      dummy_path
    end

    def test_err_in_callback
      if @ruby
        cmd = "#{@ruby} -v #{@iopt} #{@script} #{@param} > test_err_in_callback.log 2>&1"
        system(cmd)
        str = ""
        open("test_err_in_callback.log") {|ifs|
          str = ifs.read
        }
        assert_match(/NameError/, str)
      end
    end

    def ie_quit
      sh = WIN32OLE.new('Shell.Application')
      sh.windows.each do |w|
        if w.ole_type.name == 'IWebBrowser2'
          20.times do |i|
            if w.locationURL != "" && w.document
              break
            end
            WIN32OLE_EVENT.message_loop
            sleep 1
          end
          e = w.document.getElementById("str")
          if e && e.innerHTML == @str
            w.quit
            WIN32OLE_EVENT.message_loop
            sleep 0.2
          end
        end
      end
    end

    def teardown
      WIN32OLE_EVENT.message_loop
      ie_quit
      File.unlink(@f)
      File.unlink("test_err_in_callback.log")
    end
  end
end
